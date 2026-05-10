// FILE: src/mainwindow.cpp
#include "mainwindow.h"
#include "shared.h"
#include "buffer.h"
#include "logger.h"
#include "stats.h"
#include "producer.h"
#include "consumer.h"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QSplitter>
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <iomanip>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isRunning(false), simulationComplete(false),
      producerThreads(nullptr), consumerThreads(nullptr), numProducers(0), numConsumers(0) {

    setWindowTitle("Producer-Consumer Dashboard");
    resize(1000, 800);

    setupUI();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateStats);
    connect(this, &MainWindow::simulationFinished, this, &MainWindow::onSimulationFinished);
    updateTimer->start(200);
}

MainWindow::~MainWindow() {
    if (producerThreads) delete[] producerThreads;
    if (consumerThreads) delete[] consumerThreads;
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QGroupBox* configGroup = new QGroupBox("Configuration", this);
    QGridLayout* configLayout = new QGridLayout(configGroup);

    configLayout->addWidget(new QLabel("Buffer Size:"), 0, 0);
    spinBufferSize = new QSpinBox(this);
    spinBufferSize->setRange(1, 100);
    spinBufferSize->setValue(10);
    configLayout->addWidget(spinBufferSize, 0, 1);

    configLayout->addWidget(new QLabel("# Producers:"), 0, 2);
    spinNumProducers = new QSpinBox(this);
    spinNumProducers->setRange(1, 10);
    spinNumProducers->setValue(2);
    configLayout->addWidget(spinNumProducers, 0, 3);

    configLayout->addWidget(new QLabel("# Consumers:"), 0, 4);
    spinNumConsumers = new QSpinBox(this);
    spinNumConsumers->setRange(1, 10);
    spinNumConsumers->setValue(2);
    configLayout->addWidget(spinNumConsumers, 0, 5);

    configLayout->addWidget(new QLabel("Items/Producer:"), 1, 0);
    spinItemsPerProducer = new QSpinBox(this);
    spinItemsPerProducer->setRange(1, 1000);
    spinItemsPerProducer->setValue(20);
    configLayout->addWidget(spinItemsPerProducer, 1, 1);

    configLayout->addWidget(new QLabel("High Priority %:"), 1, 2);
    spinHighPriorityPct = new QSpinBox(this);
    spinHighPriorityPct->setRange(0, 100);
    spinHighPriorityPct->setValue(30);
    configLayout->addWidget(spinHighPriorityPct, 1, 3);

    configLayout->addWidget(new QLabel("Produce Delay (ms):"), 1, 4);
    spinProduceDelay = new QSpinBox(this);
    spinProduceDelay->setRange(0, 10000);
    spinProduceDelay->setValue(100);
    configLayout->addWidget(spinProduceDelay, 1, 5);

    configLayout->addWidget(new QLabel("Consume Delay (ms):"), 2, 0);
    spinConsumeDelay = new QSpinBox(this);
    spinConsumeDelay->setRange(0, 10000);
    spinConsumeDelay->setValue(150);
    configLayout->addWidget(spinConsumeDelay, 2, 1);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    btnStart = new QPushButton("Start", this);
    btnStop = new QPushButton("Stop", this);
    lblStatus = new QLabel("Idle", this);
    lblStatus->setStyleSheet("font-weight: bold; color: gray;");

    connect(btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    buttonLayout->addWidget(btnStart);
    buttonLayout->addWidget(btnStop);
    buttonLayout->addWidget(lblStatus);
    buttonLayout->addStretch();

    configLayout->addLayout(buttonLayout, 3, 0, 1, 6);

    mainLayout->addWidget(configGroup);

    QGroupBox* bufferGroup = new QGroupBox("Buffer Visualizer", this);
    QHBoxLayout* bufferLayout = new QHBoxLayout(bufferGroup);

    QWidget* visualizerContainer = new QWidget(this);
    QHBoxLayout* visualizerLayout = new QHBoxLayout(visualizerContainer);
    visualizerLayout->setSpacing(2);

    bufferVisualizer = visualizerContainer;
    bufferCells.clear();

    for (int i = 0; i < 10; i++) {
        QLabel* cell = new QLabel(this);
        cell->setFixedSize(40, 40);
        cell->setStyleSheet("background-color: #808080; border: 1px solid #555;");
        bufferCells.push_back(cell);
        visualizerLayout->addWidget(cell);
    }

    bufferLayout->addWidget(visualizerContainer);
    lblBufferOccupancy = new QLabel("Buffer: 0/10", this);
    bufferLayout->addWidget(lblBufferOccupancy);

    mainLayout->addWidget(bufferGroup);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    QGroupBox* statsGroup = new QGroupBox("Live Statistics", this);
    QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);

    tblStats = new QTableWidget(12, 2, this);
    tblStats->setHorizontalHeaderLabels({"Metric", "Value"});
    tblStats->horizontalHeader()->setStretchLastSection(true);
    tblStats->verticalHeader()->setVisible(false);
    tblStats->setColumnWidth(0, 200);
    statsLayout->addWidget(tblStats);

    splitter->addWidget(statsGroup);

    QGroupBox* threadStatsGroup = new QGroupBox("Per-Thread Stats", this);
    QVBoxLayout* threadStatsLayout = new QVBoxLayout(threadStatsGroup);

    tblThreadStats = new QTableWidget(0, 3, this);
    tblThreadStats->setHorizontalHeaderLabels({"Thread ID", "Type", "Count"});
    tblThreadStats->horizontalHeader()->setStretchLastSection(true);
    threadStatsLayout->addWidget(tblThreadStats);

    splitter->addWidget(threadStatsGroup);

    mainLayout->addWidget(splitter);

    QGroupBox* logGroup = new QGroupBox("Log Viewer", this);
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);

    txtLog = new QTextEdit(this);
    txtLog->setReadOnly(true);
    txtLog->setMaximumHeight(150);
    logLayout->addWidget(txtLog);

    mainLayout->addWidget(logGroup);

    QMenuBar* menuBar = this->menuBar();
    QMenu* testMenu = new QMenu("Tests", this);
    QAction* runTestsAction = new QAction("Run Test Cases", this);
    connect(runTestsAction, &QAction::triggered, this, &MainWindow::runTestCases);
    testMenu->addAction(runTestsAction);
    menuBar->addMenu(testMenu);
}

void MainWindow::setConfig(const struct Config& cfg) {
    config = cfg;
    spinBufferSize->setValue(cfg.bufferSize);
    spinNumProducers->setValue(cfg.numProducers);
    spinNumConsumers->setValue(cfg.numConsumers);
    spinItemsPerProducer->setValue(cfg.itemsPerProducer);
    spinHighPriorityPct->setValue(cfg.highPriorityPct);
    spinProduceDelay->setValue(cfg.produceDelayMs);
    spinConsumeDelay->setValue(cfg.consumeDelayMs);
}

void MainWindow::onStartClicked() {
    if (isRunning) return;

    config.bufferSize = spinBufferSize->value();
    config.numProducers = spinNumProducers->value();
    config.numConsumers = spinNumConsumers->value();
    config.itemsPerProducer = spinItemsPerProducer->value();
    config.highPriorityPct = spinHighPriorityPct->value();
    config.produceDelayMs = spinProduceDelay->value();
    config.consumeDelayMs = spinConsumeDelay->value();

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
    g_producersDone = false;

    getStats()->reset();
    getStats()->setStartTime(getCurrentTime());
    getLogger()->clear();

    numProducers = config.numProducers;
    numConsumers = config.numConsumers;
    producerThreads = new pthread_t[numProducers];
    consumerThreads = new pthread_t[numConsumers];

    for (int i = 0; i < numProducers; i++) {
        ProducerArgs* args = new ProducerArgs();
        args->threadId = i + 1;
        args->itemsToProduce = config.itemsPerProducer;
        pthread_create(&producerThreads[i], nullptr, producerThread, args);
    }

    for (int i = 0; i < numConsumers; i++) {
        ConsumerArgs* args = new ConsumerArgs();
        args->threadId = i + 1;
        pthread_create(&consumerThreads[i], nullptr, consumerThread, args);
    }

    isRunning = true;
    simulationComplete = false;
    lblStatus->setText("Running");
    lblStatus->setStyleSheet("font-weight: bold; color: green;");

    btnStart->setEnabled(false);
    btnStop->setEnabled(true);

    while (bufferCells.size() < config.bufferSize) {
        QLabel* cell = new QLabel(this);
        cell->setFixedSize(40, 40);
        cell->setStyleSheet("background-color: #808080; border: 1px solid #555;");
        bufferCells.push_back(cell);
        static_cast<QHBoxLayout*>(bufferVisualizer->layout())->addWidget(cell);
    }

    for (int i = bufferCells.size() - 1; i >= config.bufferSize; i--) {
        QLabel* cell = bufferCells[i]; bufferCells.erase(bufferCells.begin() + i);
        cell->hide();
    }
}

void MainWindow::onStopClicked() {
    if (!isRunning) return;

    g_stopSignal = true;
    g_producersDone = true;

    isRunning = false;
    lblStatus->setText("Stopping...");
    lblStatus->setStyleSheet("font-weight: bold; color: orange;");

    QTimer::singleShot(50, this, SLOT(checkStopDone()));
}

void MainWindow::checkStopDone() {
    bool allDone = true;
    for (int i = 0; i < numConsumers; i++) {
        void* ret;
        if (pthread_tryjoin_np(consumerThreads[i], &ret) != 0) {
            allDone = false;
            break;
        }
    }
    if (allDone) {
        for (int i = 0; i < numProducers; i++) {
            void* ret;
            if (pthread_tryjoin_np(producerThreads[i], &ret) != 0) {
                allDone = false;
                break;
            }
        }
    }

    if (allDone) {
        getStats()->setEndTime(getCurrentTime());
        simulationComplete = true;
        lblStatus->setText("Finished");
        lblStatus->setStyleSheet("font-weight: bold; color: blue;");
        saveResultsToFile();
        emit simulationFinished();
    } else {
        QTimer::singleShot(50, this, SLOT(checkStopDone()));
    }
}

void MainWindow::updateStats() {
    Stats* stats = getStats();
    Buffer* buf = getBuffer();

    if (!buf) return;

    int totalProduced = stats->getTotalProduced();
    int totalConsumed = stats->getTotalConsumed();
    int totalHighProduced = stats->getTotalHighProduced();
    int totalNormalProduced = stats->getTotalNormalProduced();
    int totalHighConsumed = stats->getTotalHighConsumed();
    int totalNormalConsumed = stats->getTotalNormalConsumed();

    double producerThroughput = stats->getProducerThroughput();
    double consumerThroughput = stats->getConsumerThroughput();
    double avgProducerWait = stats->getAvgProducerWaitTime();
    double avgConsumerWait = stats->getAvgConsumerWaitTime();
    double avgHighWait = stats->getAvgHighPriorityWaitTime();
    double avgNormalWait = stats->getAvgNormalPriorityWaitTime();

    QStringList metrics;
    metrics << "Total Produced" << QString::number(totalProduced);
    metrics << "Total Consumed" << QString::number(totalConsumed);
    metrics << "High Produced" << QString::number(totalHighProduced);
    metrics << "High Consumed" << QString::number(totalHighConsumed);
    metrics << "Normal Produced" << QString::number(totalNormalProduced);
    metrics << "Normal Consumed" << QString::number(totalNormalConsumed);
    metrics << "Producer Throughput" << QString::number(producerThroughput, 'f', 2) + " items/s";
    metrics << "Consumer Throughput" << QString::number(consumerThroughput, 'f', 2) + " items/s";
    metrics << "Avg Producer Wait" << QString::number(avgProducerWait, 'f', 2) + " ms";
    metrics << "Avg Consumer Wait" << QString::number(avgConsumerWait, 'f', 2) + " ms";
    metrics << "Avg HIGH Priority Wait" << QString::number(avgHighWait, 'f', 2) + " ms";
    metrics << "Avg NORMAL Priority Wait" << QString::number(avgNormalWait, 'f', 2) + " ms";

    for (int i = 0; i < 12; i++) {
        tblStats->setItem(i, 0, new QTableWidgetItem(metrics[i * 2]));
        tblStats->setItem(i, 1, new QTableWidgetItem(metrics[i * 2 + 1]));
    }

    int bufOccupancy = buf->getCountHigh() + buf->getCountNormal();
    lblBufferOccupancy->setText(QString("Buffer: %1/%2").arg(bufOccupancy).arg(config.bufferSize));

    for (int i = 0; i < bufferCells.size(); i++) {
        if (i < buf->getCountHigh()) {
            bufferCells[i]->setStyleSheet("background-color: #e74c3c; border: 1px solid #555;");
        } else if (i < buf->getCountHigh() + buf->getCountNormal()) {
            bufferCells[i]->setStyleSheet("background-color: #3498db; border: 1px solid #555;");
        } else {
            bufferCells[i]->setStyleSheet("background-color: #808080; border: 1px solid #555;");
        }
    }

    auto threadStats = stats->getAllThreadStats();
    tblThreadStats->setRowCount(threadStats.size());
    for (size_t i = 0; i < threadStats.size(); i++) {
        tblThreadStats->setItem(i, 0, new QTableWidgetItem(QString::number(threadStats[i]->threadId)));
        tblThreadStats->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(threadStats[i]->type)));
        int count = threadStats[i]->type == "PRODUCER" ? threadStats[i]->produced.load() : threadStats[i]->consumed.load();
        tblThreadStats->setItem(i, 2, new QTableWidgetItem(QString::number(count)));
    }

    auto recentLogs = getLogger()->getRecentEntries(100);
    txtLog->clear();
    for (const auto& entry : recentLogs) {
        QString line = QString("[%1s] [%2 %3] %4 item#%5")
                          .arg(entry.timestamp, 0, 'f', 3)
                          .arg(QString::fromStdString(entry.threadType))
                          .arg(entry.threadId)
                          .arg(QString::fromStdString(entry.action))
                          .arg(entry.itemId);

        if (entry.priority == 1) line += " HIGH";
        else if (entry.priority == 0) line += " NORMAL";

        if (!entry.details.empty()) line += " " + QString::fromStdString(entry.details);

        QTextCharFormat format;
        if (entry.threadType == "PRODUCER") {
            format.setForeground(QColor("#1abc9c"));
        } else {
            format.setForeground(QColor("#e67e22"));
        }
        if (entry.action.find("WAITING") != std::string::npos) {
            format.setForeground(QColor("#e74c3c"));
        }

        txtLog->append(line);
    }

    QTextCursor cursor = txtLog->textCursor();
    cursor.movePosition(QTextCursor::End);
    txtLog->setTextCursor(cursor);

    if (isRunning && g_producersDone.load() && g_currentBufferOccupancy.load() == 0) {
        onStopClicked();
    }
}

void MainWindow::onSimulationFinished() {
    btnStart->setEnabled(true);
    btnStop->setEnabled(false);
}

void MainWindow::saveResultsToFile() {
    std::ofstream file("results.txt", std::ios::out | std::ios::trunc);

    Stats* stats = getStats();

    file << "========== SIMULATION RESULTS ==========\n\n";
    file << "Configuration:\n";
    file << "  Buffer Size: " << config.bufferSize << "\n";
    file << "  Producers: " << config.numProducers << "\n";
    file << "  Consumers: " << config.numConsumers << "\n";
    file << "  Items per Producer: " << config.itemsPerProducer << "\n";
    file << "  High Priority %: " << config.highPriorityPct << "\n";
    file << "  Produce Delay: " << config.produceDelayMs << " ms\n";
    file << "  Consume Delay: " << config.consumeDelayMs << " ms\n\n";

    file << "Statistics:\n";
    file << "  Total Produced: " << stats->getTotalProduced() << "\n";
    file << "  Total Consumed: " << stats->getTotalConsumed() << "\n";
    file << "  High Priority Produced: " << stats->getTotalHighProduced() << "\n";
    file << "  High Priority Consumed: " << stats->getTotalHighConsumed() << "\n";
    file << "  Normal Priority Produced: " << stats->getTotalNormalProduced() << "\n";
    file << "  Normal Priority Consumed: " << stats->getTotalNormalConsumed() << "\n\n";

    file << "Performance Metrics:\n";
    file << "  Producer Throughput: " << std::fixed << std::setprecision(2)
         << stats->getProducerThroughput() << " items/s\n";
    file << "  Consumer Throughput: " << stats->getConsumerThroughput() << " items/s\n";
    file << "  Avg Producer Wait Time: " << stats->getAvgProducerWaitTime() << " ms\n";
    file << "  Avg Consumer Wait Time: " << stats->getAvgConsumerWaitTime() << " ms\n";
    file << "  Avg HIGH Priority Wait: " << stats->getAvgHighPriorityWaitTime() << " ms\n";
    file << "  Avg NORMAL Priority Wait: " << stats->getAvgNormalPriorityWaitTime() << " ms\n\n";

    file << "Logs:\n";
    file << getLogger()->getAllLogsAsString();

    file.close();

    QMessageBox::information(this, "Results Saved",
                             "Results have been saved to results.txt");
}

void MainWindow::runTestCases() {
    QMessageBox::information(this, "Tests", "Running test cases in console mode...\nUse --test flag with console version.");
}