// FILE: src/main.cpp
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <pthread.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "shared.h"
#include "buffer.h"
#include "logger.h"
#include "stats.h"
#include "producer.h"
#include "consumer.h"

#include <QApplication>
#include "mainwindow.h"

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

bool runTest(const char* testName, Config cfg) {
    std::cout << "\n========== Running Test: " << testName << " ==========\n";

    g_config = cfg;
    g_stopSignal = false;
    g_producersDone = false;

    initBuffer(cfg.bufferSize);

    sem_init(&g_empty, 0, cfg.bufferSize);
    sem_init(&g_fullHigh, 0, 0);
    sem_init(&g_fullNormal, 0, 0);
    sem_init(&g_mutex, 0, 1);

    g_totalProduced = 0;
    g_totalConsumed = 0;
    g_totalHighProduced = 0;
    g_totalNormalProduced = 0;
    g_totalHighConsumed = 0;
    g_totalNormalConsumed = 0;
    g_currentBufferOccupancy = 0;
    g_nextItemId = 0;

    getStats()->reset();
    getStats()->setStartTime(getCurrentTime());
    getLogger()->clear();

    int numProducers = cfg.numProducers;
    int numConsumers = cfg.numConsumers;

    pthread_t* producerThreads = new pthread_t[numProducers];
    pthread_t* consumerThreads = new pthread_t[numConsumers];

    for (int i = 0; i < numProducers; i++) {
        ProducerArgs* args = new ProducerArgs();
        args->threadId = i + 1;
        args->itemsToProduce = cfg.itemsPerProducer;
        pthread_create(&producerThreads[i], nullptr, producerThread, args);
    }

    for (int i = 0; i < numConsumers; i++) {
        ConsumerArgs* args = new ConsumerArgs();
        args->threadId = i + 1;
        pthread_create(&consumerThreads[i], nullptr, consumerThread, args);
    }

    for (int i = 0; i < numProducers; i++) {
        pthread_join(producerThreads[i], nullptr);
    }

    g_producersDone = true;

    for (int i = 0; i < numConsumers; i++) {
        pthread_join(consumerThreads[i], nullptr);
    }

    getStats()->setEndTime(getCurrentTime());

    int totalProduced = g_totalProduced.load();
    int totalConsumed = g_totalConsumed.load();
    int expectedTotal = cfg.numProducers * cfg.itemsPerProducer;

    bool passed = (totalProduced == expectedTotal) && (totalProduced == totalConsumed);

    std::cout << "Expected: " << expectedTotal << ", Produced: " << totalProduced
              << ", Consumed: " << totalConsumed << "\n";
    std::cout << "Test " << (passed ? "PASSED" : "FAILED") << "\n";

    sem_destroy(&g_empty);
    sem_destroy(&g_fullHigh);
    sem_destroy(&g_fullNormal);
    sem_destroy(&g_mutex);

    destroyBuffer();

    delete[] producerThreads;
    delete[] consumerThreads;

    return passed;
}

void runAllTests() {
    std::vector<TestResult> results;

    Config test1;
    test1.bufferSize = 5;
    test1.numProducers = 1;
    test1.numConsumers = 1;
    test1.itemsPerProducer = 10;
    results.push_back({"Basic Correctness", runTest("Basic Correctness", test1), ""});

    Config test2;
    test2.bufferSize = 10;
    test2.numProducers = 4;
    test2.numConsumers = 4;
    test2.itemsPerProducer = 50;
    results.push_back({"Stress Test", runTest("Stress Test", test2), ""});

    Config test3;
    test3.bufferSize = 5;
    test3.numProducers = 2;
    test3.numConsumers = 1;
    test3.itemsPerProducer = 20;
    test3.produceDelayMs = 10;
    test3.consumeDelayMs = 300;
    results.push_back({"Buffer Full (Producers Faster)", runTest("Buffer Full", test3), ""});

    Config test4;
    test4.bufferSize = 5;
    test4.numProducers = 1;
    test4.numConsumers = 2;
    test4.itemsPerProducer = 20;
    test4.produceDelayMs = 300;
    test4.consumeDelayMs = 10;
    results.push_back({"Buffer Empty (Consumers Faster)", runTest("Buffer Empty", test4), ""});

    Config test5;
    test5.bufferSize = 10;
    test5.numProducers = 2;
    test5.numConsumers = 2;
    test5.itemsPerProducer = 30;
    test5.highPriorityPct = 50;
    results.push_back({"Priority Test", runTest("Priority Test", test5), ""});

    std::cout << "\n========== TEST SUMMARY ==========\n";
    for (const auto& tr : results) {
        std::cout << tr.name << ": " << (tr.passed ? "PASS" : "FAIL") << "\n";
    }

    std::ofstream file("test_results.txt", std::ios::out | std::ios::trunc);
    file << "========== TEST RESULTS ==========\n\n";
    for (const auto& tr : results) {
        file << tr.name << ": " << (tr.passed ? "PASS" : "FAIL") << "\n";
    }
    file.close();
}

int main(int argc, char* argv[]) {
    Config config;

    bool runTests = false;
    bool guiMode = true;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            runTests = true;
            guiMode = false;
        } else if (strcmp(argv[i], "--nogui") == 0) {
            guiMode = false;
        } else {
            int val = atoi(argv[i]);
            if (i + 1 < argc && strcmp(argv[i], "bufferSize") == 0) {
                config.bufferSize = val;
            } else if (i > 0 && isdigit(argv[i][0])) {
                switch (i) {
                    case 1: config.bufferSize = val; break;
                    case 2: config.numProducers = val; break;
                    case 3: config.numConsumers = val; break;
                    case 4: config.itemsPerProducer = val; break;
                }
            }
        }
    }

    if (runTests) {
        runAllTests();
        return 0;
    }

    if (!guiMode) {
        config.bufferSize = 10;
        config.numProducers = 2;
        config.numConsumers = 2;
        config.itemsPerProducer = 20;
        config.highPriorityPct = 30;
        config.produceDelayMs = 100;
        config.consumeDelayMs = 150;

        for (int i = 1; i < argc && i <= 4; i++) {
            int val = atoi(argv[i]);
            if (i == 1) config.bufferSize = val;
            else if (i == 2) config.numProducers = val;
            else if (i == 3) config.numConsumers = val;
            else if (i == 4) config.itemsPerProducer = val;
        }

        g_config = config;
        g_stopSignal = false;
        g_producersDone = false;

        initBuffer(config.bufferSize);

        sem_init(&g_empty, 0, config.bufferSize);
        sem_init(&g_fullHigh, 0, 0);
        sem_init(&g_fullNormal, 0, 0);
        sem_init(&g_mutex, 0, 1);

        g_totalProduced = 0;
        g_totalConsumed = 0;
        g_totalHighProduced = 0;
        g_totalNormalProduced = 0;
        g_totalHighConsumed = 0;
        g_totalNormalConsumed = 0;
        g_currentBufferOccupancy = 0;
        g_nextItemId = 0;

        getStats()->setStartTime(getCurrentTime());

        pthread_t* producerThreads = new pthread_t[config.numProducers];
        pthread_t* consumerThreads = new pthread_t[config.numConsumers];

        for (int i = 0; i < config.numProducers; i++) {
            ProducerArgs* args = new ProducerArgs();
            args->threadId = i + 1;
            args->itemsToProduce = config.itemsPerProducer;
            pthread_create(&producerThreads[i], nullptr, producerThread, args);
        }

        for (int i = 0; i < config.numConsumers; i++) {
            ConsumerArgs* args = new ConsumerArgs();
            args->threadId = i + 1;
            pthread_create(&consumerThreads[i], nullptr, consumerThread, args);
        }

        for (int i = 0; i < config.numProducers; i++) {
            pthread_join(producerThreads[i], nullptr);
        }

        g_producersDone = true;

        for (int i = 0; i < config.numConsumers; i++) {
            pthread_join(consumerThreads[i], nullptr);
        }

        getStats()->setEndTime(getCurrentTime());

        Stats* stats = getStats();

        std::cout << "\n========== SIMULATION RESULTS ==========\n\n";
        std::cout << "Total Produced: " << stats->getTotalProduced() << "\n";
        std::cout << "Total Consumed: " << stats->getTotalConsumed() << "\n";
        std::cout << "High Produced: " << stats->getTotalHighProduced() << "\n";
        std::cout << "High Consumed: " << stats->getTotalHighConsumed() << "\n";
        std::cout << "Normal Produced: " << stats->getTotalNormalProduced() << "\n";
        std::cout << "Normal Consumed: " << stats->getTotalNormalConsumed() << "\n";
        std::cout << "Producer Throughput: " << std::fixed << std::setprecision(2)
                  << stats->getProducerThroughput() << " items/s\n";
        std::cout << "Consumer Throughput: " << stats->getConsumerThroughput() << " items/s\n";
        std::cout << "Avg Producer Wait: " << stats->getAvgProducerWaitTime() << " ms\n";
        std::cout << "Avg Consumer Wait: " << stats->getAvgConsumerWaitTime() << " ms\n";
        std::cout << "Avg HIGH Priority Wait: " << stats->getAvgHighPriorityWaitTime() << " ms\n";
        std::cout << "Avg NORMAL Priority Wait: " << stats->getAvgNormalPriorityWaitTime() << " ms\n";

        std::ofstream file("results.txt", std::ios::out | std::ios::trunc);
        file << "========== SIMULATION RESULTS ==========\n\n";
        file << "Configuration:\n";
        file << "  Buffer Size: " << config.bufferSize << "\n";
        file << "  Producers: " << config.numProducers << "\n";
        file << "  Consumers: " << config.numConsumers << "\n";
        file << "  Items per Producer: " << config.itemsPerProducer << "\n\n";
        file << "Statistics:\n";
        file << "  Total Produced: " << stats->getTotalProduced() << "\n";
        file << "  Total Consumed: " << stats->getTotalConsumed() << "\n";
        file << "  High Produced: " << stats->getTotalHighProduced() << "\n";
        file << "  High Consumed: " << stats->getTotalHighConsumed() << "\n";
        file << "  Normal Produced: " << stats->getTotalNormalProduced() << "\n";
        file << "  Normal Consumed: " << stats->getTotalNormalConsumed() << "\n\n";
        file << "Performance:\n";
        file << "  Producer Throughput: " << std::fixed << std::setprecision(2)
             << stats->getProducerThroughput() << " items/s\n";
        file << "  Consumer Throughput: " << stats->getConsumerThroughput() << " items/s\n";
        file << "  Avg Producer Wait: " << stats->getAvgProducerWaitTime() << " ms\n";
        file << "  Avg Consumer Wait: " << stats->getAvgConsumerWaitTime() << " ms\n";
        file << "  Avg HIGH Priority Wait: " << stats->getAvgHighPriorityWaitTime() << " ms\n";
        file << "  Avg NORMAL Priority Wait: " << stats->getAvgNormalPriorityWaitTime() << " ms\n";
        file.close();

        sem_destroy(&g_empty);
        sem_destroy(&g_fullHigh);
        sem_destroy(&g_fullNormal);
        sem_destroy(&g_mutex);

        destroyBuffer();

        delete[] producerThreads;
        delete[] consumerThreads;

        return 0;
    }

    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Producer-Consumer Dashboard");

    MainWindow window;
    window.setConfig(config);
    window.show();

    return app.exec();
}