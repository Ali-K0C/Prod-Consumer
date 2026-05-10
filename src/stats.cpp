// FILE: src/stats.cpp
#include "stats.h"
#include "shared.h"
#include <string>
#include <cmath>

static Stats* g_stats = nullptr;

Stats::Stats() : startTime(0), endTime(0) {
    pthread_mutex_init(&threadStatsMutex, nullptr);
}

void Stats::recordProduce(int threadId, double waitTime) {
    totalProduced.fetch_add(1);
    totalProducerWaitTimeNs.fetch_add(static_cast<long long>(waitTime * 1e9));
    producerWaitCount.fetch_add(1);

    pthread_mutex_lock(&threadStatsMutex);
    for (auto* ts : threadStatsList) {
        if (ts->threadId == threadId) {
            ts->produced.fetch_add(1);
            ts->totalWaitTimeNs.fetch_add(static_cast<long long>(waitTime * 1e9));
            ts->waitCount.fetch_add(1);
            break;
        }
    }
    pthread_mutex_unlock(&threadStatsMutex);
}

void Stats::recordConsume(int threadId, double waitTime, int priority) {
    totalConsumed.fetch_add(1);
    totalConsumerWaitTimeNs.fetch_add(static_cast<long long>(waitTime * 1e9));
    consumerWaitCount.fetch_add(1);

    if (priority == 1) {
        totalHighConsumed.fetch_add(1);
        totalHighPriorityWaitTimeNs.fetch_add(static_cast<long long>(waitTime * 1e9));
        highPriorityWaitCount.fetch_add(1);
    } else {
        totalNormalConsumed.fetch_add(1);
        totalNormalPriorityWaitTimeNs.fetch_add(static_cast<long long>(waitTime * 1e9));
        normalPriorityWaitCount.fetch_add(1);
    }

    pthread_mutex_lock(&threadStatsMutex);
    for (auto* ts : threadStatsList) {
        if (ts->threadId == threadId) {
            ts->consumed.fetch_add(1);
            break;
        }
    }
    pthread_mutex_unlock(&threadStatsMutex);
}

int Stats::getTotalProduced() { return totalProduced.load(); }
int Stats::getTotalConsumed() { return totalConsumed.load(); }
int Stats::getTotalHighProduced() { return totalHighProduced.load(); }
int Stats::getTotalNormalProduced() { return totalNormalProduced.load(); }
int Stats::getTotalHighConsumed() { return totalHighConsumed.load(); }
int Stats::getTotalNormalConsumed() { return totalNormalConsumed.load(); }

int Stats::getCurrentBufferOccupancy() {
    return g_currentBufferOccupancy.load();
}

double Stats::getProducerThroughput() {
    double elapsed = endTime > 0 ? endTime - startTime : getCurrentTime() - startTime;
    if (elapsed <= 0) return 0;
    return totalProduced.load() / elapsed;
}

double Stats::getConsumerThroughput() {
    double elapsed = endTime > 0 ? endTime - startTime : getCurrentTime() - startTime;
    if (elapsed <= 0) return 0;
    return totalConsumed.load() / elapsed;
}

double Stats::getAvgProducerWaitTime() {
    int count = producerWaitCount.load();
    if (count == 0) return 0;
    return (totalProducerWaitTimeNs.load() / 1e9 / count) * 1000.0;
}

double Stats::getAvgConsumerWaitTime() {
    int count = consumerWaitCount.load();
    if (count == 0) return 0;
    return (totalConsumerWaitTimeNs.load() / 1e9 / count) * 1000.0;
}

double Stats::getAvgHighPriorityWaitTime() {
    int count = highPriorityWaitCount.load();
    if (count == 0) return 0;
    return (totalHighPriorityWaitTimeNs.load() / 1e9 / count) * 1000.0;
}

double Stats::getAvgNormalPriorityWaitTime() {
    int count = normalPriorityWaitCount.load();
    if (count == 0) return 0;
    return (totalNormalPriorityWaitTimeNs.load() / 1e9 / count) * 1000.0;
}

void Stats::addThreadStats(int threadId, const std::string& type) {
    pthread_mutex_lock(&threadStatsMutex);
    ThreadStats* ts = new ThreadStats();
    ts->threadId = threadId;
    ts->type = type;
    threadStatsList.push_back(ts);
    pthread_mutex_unlock(&threadStatsMutex);
}

std::vector<ThreadStats*> Stats::getAllThreadStats() {
    std::vector<ThreadStats*> result;
    pthread_mutex_lock(&threadStatsMutex);
    result = threadStatsList;
    pthread_mutex_unlock(&threadStatsMutex);
    return result;
}

void Stats::setStartTime(double time) { startTime = time; }
void Stats::setEndTime(double time) { endTime = time; }

void Stats::reset() {
    totalProduced = 0;
    totalConsumed = 0;
    totalHighProduced = 0;
    totalNormalProduced = 0;
    totalHighConsumed = 0;
    totalNormalConsumed = 0;
    totalProducerWaitTimeNs = 0;
    producerWaitCount = 0;
    totalConsumerWaitTimeNs = 0;
    consumerWaitCount = 0;
    totalHighPriorityWaitTimeNs = 0;
    highPriorityWaitCount = 0;
    totalNormalPriorityWaitTimeNs = 0;
    normalPriorityWaitCount = 0;
    startTime = 0;
    endTime = 0;
}

Stats* getStats() {
    if (!g_stats) {
        g_stats = new Stats();
    }
    return g_stats;
}
