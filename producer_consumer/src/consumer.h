// FILE: src/consumer.h
#ifndef CONSUMER_H
#define CONSUMER_H

#include <pthread.h>

struct ConsumerArgs {
    int threadId;
};

void* consumerThread(void* arg);

#endif