#ifndef LOG_PROCESSOR_H
#define LOG_PROCESSOR_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sqlite3.h>
#include <iostream>
#include <fstream>
#include <condition_variable>
#include <queue>

class LogProcessor {
public:
    LogProcessor(const std::string& dbFile);
    ~LogProcessor();
    void processLogFile(const std::string& logFile);

private:
    void processLine(const std::string& line);
    void insertIntoDatabase(const std::string& processedData);
    void insertBatchIntoDatabase(const std::vector<std::string>& processedDataBatch);
    void seedDatabase(); // Function to insert seed data

    void worker(); // Thread worker function
    void startWorkers();

    sqlite3* db;
    std::mutex dbMutex; // Mutex for thread-safe database access
    std::vector<std::thread> workers; // Thread pool
    std::queue<std::string> lineQueue; // Queue for lines to be processed
    std::condition_variable cv; // Condition variable for signaling
    const int MAX_THREADS = 4; // Maximum number of threads
    const int BATCH_SIZE = 10; // Adjust as needed
    bool stopWorkers = false; // Flag to stop worker threads
};

#endif // LOG_PROCESSOR_H
