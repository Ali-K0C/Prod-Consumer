// FILE: src/producer.h
#ifndef PRODUCER_H
#define PRODUCER_H

#include <pthread.h>

struct ProducerArgs {
    int threadId;
    int itemsToProduce;
};

void* producerThread(void* arg);

#endif