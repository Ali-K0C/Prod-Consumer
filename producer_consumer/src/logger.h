// FILE: src/logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <pthread.h>

struct LogEntry {
    double timestamp;
    std::string threadType;
    int threadId;
    std::string action;
    int itemId;
    int priority;
    std::string details;
};

class Logger {
public:
    Logger(int maxEntries = 200);
    ~Logger();

    void log(double timestamp, const std::string& threadType, int threadId,
             const std::string& action, int itemId, int priority,
             const std::string& details = "");

    std::vector<LogEntry> getRecentEntries(int count = 100);
    std::string getAllLogsAsString();
    void clear();

private:
    std::vector<LogEntry> entries;
    int maxEntries;
    pthread_mutex_t mutex;
};

Logger* getLogger();

#endif