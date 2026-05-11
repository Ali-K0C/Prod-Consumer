// FILE: src/buffer.cpp
#include "buffer.h"
#include <cstring>
#include <algorithm>

static Buffer* g_buffer = nullptr;

Buffer::Buffer(int cap) : capacity(cap) {
    highQueue = new Item[capacity];
    normalQueue = new Item[capacity];
    highIn = highOut = 0;
    normalIn = normalOut = 0;
    countHigh = countNormal = 0;
}

Buffer::~Buffer() {
    delete[] highQueue;
    delete[] normalQueue;
}

bool Buffer::push(const Item& item) {
    if (item.priority == 1) {
        if (countHigh >= capacity) return false;
        highQueue[highIn] = item;
        highIn = (highIn + 1) % capacity;
        countHigh++;
    } else {
        if (countNormal >= capacity) return false;
        normalQueue[normalIn] = item;
        normalIn = (normalIn + 1) % capacity;
        countNormal++;
    }
    return true;
}

bool Buffer::pop(Item& item, int priority) {
    if (priority == 1) {
        if (countHigh <= 0) return false;
        item = highQueue[highOut];
        highOut = (highOut + 1) % capacity;
        countHigh--;
    } else {
        if (countNormal <= 0) return false;
        item = normalQueue[normalOut];
        normalOut = (normalOut + 1) % capacity;
        countNormal--;
    }
    return true;
}

int Buffer::getCountHigh() const { return countHigh; }
int Buffer::getCountNormal() const { return countNormal; }
int Buffer::getCapacity() const { return capacity; }
Item* Buffer::getHighQueue() { return highQueue; }
Item* Buffer::getNormalQueue() { return normalQueue; }

void initBuffer(int capacity) {
    if (g_buffer) delete g_buffer;
    g_buffer = new Buffer(capacity);
}

void destroyBuffer() {
    if (g_buffer) {
        delete g_buffer;
        g_buffer = nullptr;
    }
}

Buffer* getBuffer() {
    return g_buffer;
}
