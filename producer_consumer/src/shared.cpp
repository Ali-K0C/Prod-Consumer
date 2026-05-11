// FILE: src/shared.cpp
#include "shared.h"
#include <pthread.h>
#include <ctime>
#include <cmath>

Config g_config;

sem_t g_empty;
sem_t g_fullHigh;
sem_t g_fullNormal;
sem_t g_mutex;

std::atomic<int> g_totalProduced(0);
std::atomic<int> g_totalConsumed(0);
std::atomic<int> g_totalHighProduced(0);
std::atomic<int> g_totalNormalProduced(0);
std::atomic<int> g_totalHighConsumed(0);
std::atomic<int> g_totalNormalConsumed(0);

std::atomic<int> g_currentBufferOccupancy(0);

std::atomic<bool> g_producersDone(false);
std::atomic<bool> g_stopSignal(false);

std::atomic<int> g_nextItemId(0);

pthread_mutex_t g_statsMutex = PTHREAD_MUTEX_INITIALIZER;

double getCurrentTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
