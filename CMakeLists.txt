cmake_minimum_required(VERSION 3.16)
project(ProducerConsumer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -lrt")

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCES
    src/shared.cpp
    src/buffer.cpp
    src/logger.cpp
    src/stats.cpp
    src/producer.cpp
    src/consumer.cpp
    src/main.cpp
    src/mainwindow.cpp
)

set(HEADERS
    src/shared.h
    src/buffer.h
    src/logger.h
    src/stats.h
    src/producer.h
    src/consumer.h
    src/mainwindow.h
)

if(QT_VERSION_MAJOR EQUAL 6)
    add_executable(producer_consumer ${SOURCES} ${HEADERS})
    target_link_libraries(producer_consumer PRIVATE Qt6::Widgets Threads::Threads)
else()
    add_executable(producer_consumer ${SOURCES} ${HEADERS})
    target_link_libraries(producer_consumer PRIVATE Qt5::Widgets Threads::Threads)
endif()

target_link_libraries(producer_consumer PRIVATE rt)

set_target_properties(producer_consumer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS producer_consumer DESTINATION bin)