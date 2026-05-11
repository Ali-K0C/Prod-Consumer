// FILE: src/shared.h
#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>
#include <atomic>
#include <cstdint>

struct Item {
    int id;
    int producerID;
    int priority; // 0 = NORMAL, 1 = HIGH
    double produceTime;
};

struct Config {
    int bufferSize = 10;
    int numProducers = 2;
    int numConsumers = 2;
    int itemsPerProducer = 20;
    int highPriorityPct = 30;
    int produceDelayMs = 100;
    int consumeDelayMs = 150;
};

extern Config g_config;

extern sem_t g_empty;
extern sem_t g_fullHigh;
extern sem_t g_fullNormal;
extern sem_t g_mutex;

extern std::atomic<int> g_totalProduced;
extern std::atomic<int> g_totalConsumed;
extern std::atomic<int> g_totalHighProduced;
extern std::atomic<int> g_totalNormalProduced;
extern std::atomic<int> g_totalHighConsumed;
extern std::atomic<int> g_totalNormalConsumed;

extern std::atomic<int> g_currentBufferOccupancy;

extern std::atomic<bool> g_producersDone;
extern std::atomic<bool> g_stopSignal;

extern std::atomic<int> g_nextItemId;

extern pthread_mutex_t g_statsMutex;

double getCurrentTime();

#endif