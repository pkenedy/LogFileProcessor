#include "LogProcessor.h"
#include <sstream>

// Constructor to open the SQLite database and start worker threads
LogProcessor::LogProcessor(const std::string& dbFile) {
    if (sqlite3_open(dbFile.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    } else {
        std::cout << "Opened database successfully." << std::endl;

        const char* createTableSQL = "CREATE TABLE IF NOT EXISTS logs (id INTEGER PRIMARY KEY AUTOINCREMENT, data TEXT UNIQUE);";
        char* errorMessage = nullptr;
        if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
            std::cerr << "SQL error during table creation: " << errorMessage << std::endl;
            sqlite3_free(errorMessage);
        } else {
            std::cout << "Table created successfully (or already exists)." << std::endl;
            seedDatabase();
        }

        startWorkers();
    }
}

// Destructor to close the database and clean up threads
LogProcessor::~LogProcessor() {
    {
        std::lock_guard<std::mutex> lock(dbMutex);
        stopWorkers = true;
    }
    cv.notify_all(); // Wake up all waiting threads
    for (auto& worker : workers) {
        worker.join();
    }
    if (db) {
        sqlite3_close(db);
    }
}

// Seed data insertion into the database
void LogProcessor::seedDatabase() {
    std::lock_guard<std::mutex> lock(dbMutex);

    const char* seedSQL = "INSERT OR IGNORE INTO logs (data) VALUES ('2024-10-22 10:23:34 - User login attempt'),"
                          " ('2024-10-22 10:24:45 - User logout'),"
                          " ('2024-10-22 10:25:56 - Server restarted');";

    char* errorMessage = nullptr;
    if (sqlite3_exec(db, seedSQL, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        std::cerr << "SQL error during seed data insertion: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
    } else {
        std::cout << "Seed data inserted successfully." << std::endl;
    }
}

// Process each line and add it to the queue
void LogProcessor::processLine(const std::string& line) {
    std::unique_lock<std::mutex> lock(dbMutex);
    lineQueue.push(line);
    cv.notify_one();
}

// Batch insert into the database with duplicates check
void LogProcessor::insertBatchIntoDatabase(const std::vector<std::string>& processedDataBatch) {
    std::lock_guard<std::mutex> lock(dbMutex);

    for (const auto& processedData : processedDataBatch) {
        std::string checkSQL = "SELECT COUNT(*) FROM logs WHERE data = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, checkSQL.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, processedData.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0) {
                std::cout << "Duplicate entry found for: " << processedData << std::endl;
                sqlite3_finalize(stmt);
                continue; // Skip insertion for duplicates
            }
        }
        sqlite3_finalize(stmt);

        std::string insertSQL = "INSERT INTO logs (data) VALUES (?);";
        if (sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, processedData.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "SQL error during insert: " << sqlite3_errmsg(db) << std::endl;
            } else {
                std::cout << "Inserted: " << processedData << std::endl;
            }
        }
        sqlite3_finalize(stmt);
    }
}

// Thread worker function to process log entries from the queue
void LogProcessor::worker() {
    std::vector<std::string> processedDataBatch;

    while (true) {
        std::string line;
        {
            std::unique_lock<std::mutex> lock(dbMutex);
            cv.wait(lock, [this] { return stopWorkers || !lineQueue.empty(); });
            if (stopWorkers && lineQueue.empty()) {
                if (!processedDataBatch.empty()) {
                    insertBatchIntoDatabase(processedDataBatch); // Insert any remaining data
                }
                return;
            }
            line = lineQueue.front();
            lineQueue.pop();
        }

        // Log the processing of the line by thread ID
        std::ostringstream oss;
        oss << "Processing line in thread ID: " << std::this_thread::get_id() << std::endl;
        std::cout << oss.str();

        processedDataBatch.push_back(line);

        // Insert batch if we reached the defined size
        if (processedDataBatch.size() >= BATCH_SIZE) {
            insertBatchIntoDatabase(processedDataBatch);
            processedDataBatch.clear(); // Clear the batch after insertion
        }
    }
}

// Start worker threads for concurrent log processing
void LogProcessor::startWorkers() {
    for (int i = 0; i < MAX_THREADS; ++i) {
        workers.emplace_back(&LogProcessor::worker, this);
    }
}

// Read and process the log file
void LogProcessor::processLogFile(const std::string& logFile) {
    std::ifstream file(logFile);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << logFile << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        processLine(line);
    }
    file.close();

    // Signal workers to stop after processing is done
    {
        std::lock_guard<std::mutex> lock(dbMutex);
        stopWorkers = true;
    }
    cv.notify_all(); // Wake up all waiting threads

    // Wait for all workers to finish processing
    for (auto& worker : workers) {
        worker.join();
    }

    std::cout << "All log entries processed successfully and inserted into the database." << std::endl;
}
