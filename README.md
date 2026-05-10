# Producer-Consumer Using POSIX Semaphores

A multi-threaded Producer-Consumer system implementation for Linux using POSIX semaphores, with priority-based consumption and a Qt-based GUI dashboard.

## Features

- **Dual Priority Queues**: Separate buffers for HIGH and NORMAL priority items
- **Priority-Based Consumption**: HIGH priority items are always consumed first
- **POSIX Semaphores**: Strict synchronization using `sem_t` (empty, fullHigh, fullNormal, mutex)
- **Real-Time Statistics**: Throughput, wait times, per-thread metrics
- **Qt GUI Dashboard**: Live buffer visualization, statistics panel, and log viewer
- **Configurable Parameters**: Buffer size, thread counts, delays, priority ratio
- **Comprehensive Testing**: 5 test cases validating correctness

## Requirements

- Linux (Ubuntu/Debian)
- GCC 11+ with C++17 support
- CMake 3.16+
- Qt5 or Qt6 (optional - for GUI)
- POSIX threads (pthread)

### Install Dependencies (Ubuntu/Debian)

```bash
# For console-only build (no GUI)
sudo apt install build-essential cmake

# For GUI build
sudo apt install qtbase5-dev qt5-qmake
# OR
sudo apt install qt6-base-dev qt6-qmake
```

## Build

```bash
# Clone and navigate to project
cd producer_consumer

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make
```

## Usage

### GUI Mode (default)

```bash
./bin/producer_consumer
```

### Console Mode (no GUI)

```bash
./bin/producer_consumer --nogui [bufferSize] [numProducers] [numConsumers] [itemsPerProducer]

# Examples:
./bin/producer_consumer --nogui      # Use defaults (10, 2, 2, 20)
./bin/producer_consumer --nogui 5 1 1 10  # Buffer=5, 1P, 1C, 10 items
```

### Run Tests

```bash
./bin/producer_consumer --test
```

## Configuration Options

| Parameter        | Default | Description                         |
| ---------------- | ------- | ----------------------------------- |
| bufferSize       | 10      | Capacity of the bounded buffer      |
| numProducers     | 2       | Number of producer threads          |
| numConsumers     | 2       | Number of consumer threads          |
| itemsPerProducer | 20      | Items each producer creates         |
| highPriorityPct  | 30      | % of items that are HIGH priority   |
| produceDelayMs   | 100     | Max delay between productions (ms)  |
| consumeDelayMs   | 150     | Max delay between consumptions (ms) |

## Project Structure

```
producer_consumer/
├── CMakeLists.txt
├── src/
│   ├── main.cpp           - Entry point, CLI, test runner
│   ├── shared.h/cpp       - Global config, semaphores, globals
│   ├── buffer.h/cpp       - Dual circular queue implementation
│   ├── producer.h/cpp     - Producer thread logic
│   ├── consumer.h/cpp     - Consumer thread logic
│   ├── logger.h/cpp       - Thread-safe logging
│   ├── stats.h/cpp        - Statistics collection
│   └── mainwindow.h/cpp   - Qt GUI dashboard
├── results.txt            - Generated results (after run)
└── PROJECT_REPORT.md      - Detailed project report
```

## How It Works

### Synchronization

The system uses exactly 4 POSIX semaphores:

1. **empty** (counting, initial=BUFFER_SIZE) - counts available slots
2. **fullHigh** (counting, initial=0) - counts HIGH priority items
3. **fullNormal** (counting, initial=0) - counts NORMAL items
4. **mutex** (binary, initial=1) - mutual exclusion for critical section

### Producer Flow

```
sem_wait(&empty)       → wait for empty slot
sem_wait(&mutex)       → enter critical section
push item to queue
sem_post(&mutex)       → exit critical section
sem_post(&full*)       → signal available item
```

### Consumer Flow

```
sem_trywait(&fullHigh) → check HIGH priority (non-blocking)
if success:
    pop from highQueue
else:
    sem_wait(&fullNormal) → wait for NORMAL item
    pop from normalQueue
sem_post(&empty)       → signal empty slot
```

## Sample Output

```
[25894.267s] [PRODUCER 1] PRODUCED item#0 NORMAL -> buffer: 1/50
[25894.268s] [CONSUMER 1] CONSUMED item#0 NORMAL -> buffer: 0/50
[25894.270s] [PRODUCER 2] PRODUCED item#1 HIGH -> buffer: 1/50
[25894.271s] [CONSUMER 2] CONSUMED item#1 HIGH -> buffer: 0/50
...
```

## Test Cases

1. **Basic Correctness**: 1P, 1C, Buffer=5, 10 items → validates produced == consumed
2. **Stress Test**: 4P, 4C, Buffer=10, 50 items/producer → validates no deadlock
3. **Buffer Full**: produceDelay=10ms, consumeDelay=300ms → producers block
4. **Buffer Empty**: produceDelay=300ms, consumeDelay=10ms → consumers block
5. **Priority Test**: 50% HIGH priority → HIGH items consumed first

## License

MIT License - feel free to use for educational purposes.
