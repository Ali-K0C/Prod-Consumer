// FILE: src/logger.cpp
#include "logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>

static Logger* g_logger = nullptr;

Logger::Logger(int max) : maxEntries(max) {
    pthread_mutex_init(&mutex, nullptr);
}

Logger::~Logger() {
    pthread_mutex_destroy(&mutex);
}

void Logger::log(double timestamp, const std::string& threadType, int threadId,
                 const std::string& action, int itemId, int priority,
                 const std::string& details) {
    LogEntry entry;
    entry.timestamp = timestamp;
    entry.threadType = threadType;
    entry.threadId = threadId;
    entry.action = action;
    entry.itemId = itemId;
    entry.priority = priority;
    entry.details = details;

    pthread_mutex_lock(&mutex);
    entries.push_back(entry);
    if (entries.size() > maxEntries) {
        entries.erase(entries.begin());
    }
    pthread_mutex_unlock(&mutex);

    std::ostringstream oss;
    oss << "[" << std::fixed << std::setprecision(3) << timestamp << "s] ["
        << threadType << " " << threadId << "] " << action << " item#" << itemId;

    if (priority == 1) {
        oss << " HIGH";
    } else if (priority == 0) {
        oss << " NORMAL";
    }

    if (!details.empty()) {
        oss << " " << details;
    }

    std::cout << oss.str() << std::endl;
}

std::vector<LogEntry> Logger::getRecentEntries(int count) {
    pthread_mutex_lock(&mutex);
    int start = entries.size() > count ? entries.size() - count : 0;
    std::vector<LogEntry> result(entries.begin() + start, entries.end());
    pthread_mutex_unlock(&mutex);
    return result;
}

std::string Logger::getAllLogsAsString() {
    pthread_mutex_lock(&mutex);
    std::ostringstream oss;
    for (const auto& e : entries) {
        oss << "[" << std::fixed << std::setprecision(3) << e.timestamp << "s] ["
            << e.threadType << " " << e.threadId << "] " << e.action
            << " item#" << e.itemId;
        if (e.priority == 1) oss << " HIGH";
        else if (e.priority == 0) oss << " NORMAL";
        if (!e.details.empty()) oss << " " << e.details;
        oss << "\n";
    }
    pthread_mutex_unlock(&mutex);
    return oss.str();
}

void Logger::clear() {
    pthread_mutex_lock(&mutex);
    entries.clear();
    pthread_mutex_unlock(&mutex);
}

Logger* getLogger() {
    if (!g_logger) {
        g_logger = new Logger();
    }
    return g_logger;
}