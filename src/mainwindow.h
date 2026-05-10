// FILE: src/mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <vector>
#include <pthread.h>
#include <shared.h>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setConfig(const struct Config& cfg);
    void startSimulation();
    void stopSimulation();

signals:
    void simulationFinished();

public slots:
    void onStartClicked();
    void onStopClicked();
    void updateStats();
    void onSimulationFinished();
    void checkStopDone();

private:
    void setupUI();
    void createMenuBar();
    void runTestCases();
    void saveResultsToFile();

    struct Config config;

    QSpinBox* spinBufferSize;
    QSpinBox* spinNumProducers;
    QSpinBox* spinNumConsumers;
    QSpinBox* spinItemsPerProducer;
    QSpinBox* spinHighPriorityPct;
    QSpinBox* spinProduceDelay;
    QSpinBox* spinConsumeDelay;

    QPushButton* btnStart;
    QPushButton* btnStop;
    QLabel* lblStatus;

    QWidget* bufferVisualizer;
    std::vector<QLabel*> bufferCells;
    QLabel* lblBufferOccupancy;

    QTextEdit* txtLog;
    QTableWidget* tblStats;
    QTableWidget* tblThreadStats;

    QTimer* updateTimer;

    pthread_t* producerThreads;
    pthread_t* consumerThreads;
    int numProducers;
    int numConsumers;

    bool isRunning;
    bool simulationComplete;
};

#endif
