#include "LogProcessor.h"

// Constructor to open the SQLite database
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
            seedDatabase(); // Call to insert seed data
        }

        startWorkers(); // Start worker threads
    }
}

// Destructor to close the database and clean up threads
LogProcessor::~LogProcessor() {
    {
        std::lock_guard<std::mutex> lock(dbMutex);
        stopWorkers = true; // Signal workers to stop
    }
    cv.notify_all(); // Wake up all waiting threads
    for (auto& worker : workers) {
        worker.join(); // Wait for all threads to finish
    }
    if (db) {
        sqlite3_close(db);
    }
}

// Function to insert seed data
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

// Function to process each line of the log file
void LogProcessor::processLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(dbMutex);
    lineQueue.push(line); // Push line to queue
    cv.notify_one(); // Notify one worker thread
}

// Thread worker function
void LogProcessor::worker() {
    while (true) {
        std::string line;
        {
            std::unique_lock<std::mutex> lock(dbMutex);
            cv.wait(lock, [this] { return stopWorkers || !lineQueue.empty(); });
            if (stopWorkers && lineQueue.empty()) {
                return; // Exit if stopping and no more lines
            }
            line = lineQueue.front();
            lineQueue.pop();
        }
        insertIntoDatabase(line);
    }
}

// Start worker threads
void LogProcessor::startWorkers() {
    for (int i = 0; i < MAX_THREADS; ++i) {
        workers.emplace_back(&LogProcessor::worker, this);
    }
}

// Thread-safe function to insert data into the database
void LogProcessor::insertIntoDatabase(const std::string& processedData) {
    std::lock_guard<std::mutex> lock(dbMutex);

    // Check if the entry already exists
    std::string checkSQL = "SELECT COUNT(*) FROM logs WHERE data = ?;";
    sqlite3_stmt* stmt;

    // Prepare the statement for checking duplicates
    if (sqlite3_prepare_v2(db, checkSQL.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, processedData.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count > 0) {
                std::cout << "Duplicate entry found for: " << processedData << std::endl;
                sqlite3_finalize(stmt);
                return; // Don't insert duplicate
            }
        }
    }
    sqlite3_finalize(stmt);

    // Proceed to insert if it's not a duplicate
    std::string sql = "INSERT INTO logs (data) VALUES (?);";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, processedData.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "SQL error during insert: " << sqlite3_errmsg(db) << std::endl;
        } else {
            std::cout << "Inserted: " << processedData << std::endl;
        }
    }
    sqlite3_finalize(stmt);
}

// Function to process the log file
void LogProcessor::processLogFile(const std::string& logFile) {
    std::ifstream file(logFile);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << logFile << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Process the line
        processLine(line);
    }

    file.close();

    // Wait for all workers to finish processing
    for (auto& worker : workers) {
        worker.join();
    }

    // Final summary message
    std::cout << "All log entries processed successfully and inserted into the database." << std::endl;
}
