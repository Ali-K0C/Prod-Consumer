// FILE: src/consumer.cpp
#include "consumer.h"
#include "shared.h"
#include "buffer.h"
#include "logger.h"
#include "stats.h"
#include <cstdlib>
#include <unistd.h>
#include <sstream>

void* consumerThread(void* arg) {
    ConsumerArgs* args = (ConsumerArgs*)arg;
    int threadId = args->threadId;

    getStats()->addThreadStats(threadId, "CONSUMER");

    while (true) {
        Item item;
        int priority = -1;
        double waitStart = getCurrentTime();
        double waitTime = 0;

        if (sem_trywait(&g_fullHigh) == 0) {
            priority = 1;
            waitTime = getCurrentTime() - waitStart;

            sem_wait(&g_mutex);

            Buffer* buf = getBuffer();
            if (buf->getCountHigh() > 0) {
                buf->pop(item, priority);
                int bufOccupancy = g_currentBufferOccupancy.fetch_sub(1);

                sem_post(&g_mutex);
                sem_post(&g_empty);

                double consumeTime = getCurrentTime();
                std::ostringstream details;
                details << "-> buffer: " << (bufOccupancy - 1) << "/" << g_config.bufferSize;
                getLogger()->log(consumeTime, "CONSUMER", threadId,
                                 "CONSUMED", item.id, priority, details.str());

                getStats()->recordConsume(threadId, waitTime, priority);
            } else {
                sem_post(&g_fullHigh);
                sem_post(&g_mutex);
            }

        } else {
            if (sem_trywait(&g_fullNormal) == 0) {
                priority = 0;
                waitTime = getCurrentTime() - waitStart;

                sem_wait(&g_mutex);

                Buffer* buf = getBuffer();
                if (buf->getCountNormal() > 0) {
                    buf->pop(item, priority);
                    int bufOccupancy = g_currentBufferOccupancy.fetch_sub(1);

                    sem_post(&g_mutex);
                    sem_post(&g_empty);

                    double consumeTime = getCurrentTime();
                    std::ostringstream details;
                    details << "-> buffer: " << (bufOccupancy - 1) << "/" << g_config.bufferSize;
                    getLogger()->log(consumeTime, "CONSUMER", threadId,
                                     "CONSUMED", item.id, priority, details.str());

                    getStats()->recordConsume(threadId, waitTime, priority);
                } else {
                    sem_post(&g_fullNormal);
                    sem_post(&g_mutex);
                }
            } else {
                if (g_producersDone.load() && g_currentBufferOccupancy.load() == 0) {
                    break;
                }
                usleep(1000);
                continue;
            }
        }

        if (g_config.consumeDelayMs > 0) {
            int delay = rand() % (g_config.consumeDelayMs + 1);
            usleep(delay * 1000);
        }

        if (g_producersDone.load() && g_currentBufferOccupancy.load() == 0) {
            break;
        }
    }

    return nullptr;
}