// FILE: src/buffer.h
#ifndef BUFFER_H
#define BUFFER_H

#include "shared.h"

class Buffer {
public:
    Buffer(int capacity);
    ~Buffer();

    bool push(const Item& item);
    bool pop(Item& item, int priority);
    int getCountHigh() const;
    int getCountNormal() const;
    int getCapacity() const;
    Item* getHighQueue();
    Item* getNormalQueue();

private:
    int capacity;
    Item* highQueue;
    Item* normalQueue;
    int highIn;
    int highOut;
    int normalIn;
    int normalOut;
    int countHigh;
    int countNormal;
};

void initBuffer(int capacity);
void destroyBuffer();
Buffer* getBuffer();

#endif