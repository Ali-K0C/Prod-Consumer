// FILE: src/stats.h
#ifndef STATS_H
#define STATS_H

#include <atomic>
#include <vector>
#include <pthread.h>
#include <string>

struct ThreadStats {
    int threadId;
    std::string type;
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<long long> totalWaitTimeNs{0};
    std::atomic<int> waitCount{0};
};

class Stats {
public:
    Stats();

    void recordProduce(int threadId, double waitTime);
    void recordConsume(int threadId, double waitTime, int priority);

    int getTotalProduced();
    int getTotalConsumed();
    int getTotalHighProduced();
    int getTotalNormalProduced();
    int getTotalHighConsumed();
    int getTotalNormalConsumed();
    int getCurrentBufferOccupancy();

    double getProducerThroughput();
    double getConsumerThroughput();
    double getAvgProducerWaitTime();
    double getAvgConsumerWaitTime();
    double getAvgHighPriorityWaitTime();
    double getAvgNormalPriorityWaitTime();

    void addThreadStats(int threadId, const std::string& type);
    std::vector<ThreadStats*> getAllThreadStats();

    void setStartTime(double time);
    void setEndTime(double time);

    void reset();

private:
    std::atomic<int> totalProduced;
    std::atomic<int> totalConsumed;
    std::atomic<int> totalHighProduced;
    std::atomic<int> totalNormalProduced;
    std::atomic<int> totalHighConsumed;
    std::atomic<int> totalNormalConsumed;

    std::atomic<long long> totalProducerWaitTimeNs;
    std::atomic<int> producerWaitCount;
    std::atomic<long long> totalConsumerWaitTimeNs;
    std::atomic<int> consumerWaitCount;
    std::atomic<long long> totalHighPriorityWaitTimeNs;
    std::atomic<int> highPriorityWaitCount;
    std::atomic<long long> totalNormalPriorityWaitTimeNs;
    std::atomic<int> normalPriorityWaitCount;

    std::vector<ThreadStats*> threadStatsList;
    pthread_mutex_t threadStatsMutex;

    double startTime;
    double endTime;
};

Stats* getStats();

#endif
