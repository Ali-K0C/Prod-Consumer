// FILE: src/producer.cpp
#include "producer.h"
#include "shared.h"
#include "buffer.h"
#include "logger.h"
#include "stats.h"
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sstream>

void* producerThread(void* arg) {
    ProducerArgs* args = (ProducerArgs*)arg;
    int threadId = args->threadId;
    int itemsToProduce = args->itemsToProduce;

    getStats()->addThreadStats(threadId, "PRODUCER");

    for (int i = 0; i < itemsToProduce; i++) {
        if (g_producersDone.load()) break;

        double startWait = getCurrentTime();

        sem_wait(&g_empty);

        double waitTime = getCurrentTime() - startWait;

        sem_wait(&g_mutex);

        Item item;
        item.id = g_nextItemId.fetch_add(1);
        item.producerID = threadId;
        item.produceTime = getCurrentTime();

        int randVal = rand() % 100;
        if (randVal < g_config.highPriorityPct) {
            item.priority = 1;
            g_totalHighProduced.fetch_add(1);
        } else {
            item.priority = 0;
            g_totalNormalProduced.fetch_add(1);
        }

        Buffer* buf = getBuffer();
        buf->push(item);

        g_currentBufferOccupancy.fetch_add(1);
        g_totalProduced.fetch_add(1);

        int bufOccupancy = g_currentBufferOccupancy.load();

        sem_post(&g_mutex);

        if (item.priority == 1) {
            sem_post(&g_fullHigh);
        } else {
            sem_post(&g_fullNormal);
        }

        std::ostringstream details;
        details << "-> buffer: " << bufOccupancy << "/" << g_config.bufferSize;
        getLogger()->log(item.produceTime, "PRODUCER", threadId,
                         "PRODUCED", item.id, item.priority, details.str());

        getStats()->recordProduce(threadId, waitTime);

        if (i < itemsToProduce - 1) {
            int delay = rand() % (g_config.produceDelayMs + 1);
            usleep(delay * 1000);
        }
    }

    return nullptr;
}