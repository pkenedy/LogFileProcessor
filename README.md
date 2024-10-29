# Log File Processor

This application is designed to read log files, process each log entry, and store them in a SQLite database. The application utilizes multithreading to efficiently handle large log files by processing multiple entries simultaneously. It also ensures that duplicate log entries are not stored in the database. The application is built in C++ and uses the SQLite3 library for database management.

## Features

- **SQLite Integration**: Automatically creates a database and a `logs` table to store processed log entries.
- **Multithreading**: Uses worker threads to handle log processing concurrently, improving performance for large log files.
- **Duplicate Entry Prevention**: Each log entry is checked to ensure it is not already in the database before insertion.
- **Batch Processing**: Log entries are processed and inserted into the database in batches to improve efficiency.

## Implementation Approach

### 1. **Database Management**

The application connects to a SQLite database specified by the user. If the database file does not exist, it will be created. The `logs` table is created with the following structure:

- **id**: An auto-incrementing primary key.
- **data**: The log entry, stored as a text string. Each entry is unique to prevent duplicates.

Before processing logs, seed data can be inserted into the database. This ensures that the database has some initial log entries for testing or validation purposes.

### 2. **Log File Processing**

The application reads a log file line by line using the `processLogFile()` function. Each line represents a log entry and is added to a queue (`lineQueue`). Worker threads, started by the `startWorkers()` function, pick up these lines from the queue and insert them into the SQLite database after processing.

### 3. **Multithreading and Thread Safety**

The application uses multiple worker threads to handle log entries concurrently. The number of worker threads is determined by a constant (`MAX_THREADS`), which is set to 4 by default.

- **Mutex Locking**: To ensure thread safety when accessing shared resources like the SQLite database or the queue, mutexes (`dbMutex`) are used.
- **Condition Variables**: A condition variable (`cv`) is employed to signal worker threads when new data is available in the queue, preventing busy-waiting.

### 4. **Batch Insertion**

To optimize the insertion process, log entries are accumulated into batches. When the batch size reaches a predefined limit (`BATCH_SIZE`), or when all log entries have been processed, the batch is inserted into the database in one operation. This reduces the overhead associated with inserting one entry at a time.

### 5. **Duplicate Prevention**

Before inserting a log entry, the application checks whether the entry already exists in the database using a `SELECT` query. If the entry is found, the application skips the insertion for that log line to prevent duplicates.

### 6. **Shutdown the worker threads**

The application ensures that all worker threads are properly joined and stopped when processing completes or when the application is terminated. The `stopWorkers` flag is used to signal threads to stop processing, and `join()` is called to wait for all threads to finish their tasks before shutting down.

## Usage

1. **Clone the Repository**:
    ```bash
    git clone https://github.com/paul/log-processing-app.git
    cd log-processing-app
    ```

2. **Build the Application**:
    Ensure that you have the SQLite library installed, and then compile the application:
    ```bash
     g++ -o LogFileProcessor main.cpp LogProcessor.cpp -lsqlite3 -lpthread


3. **Run the Application**:
    ```bash
    ./LogFileProcessor 
    

4. **Database Verification

After running the program, you can verify the entries in the database by opening it using SQLite:

bash

sqlite3 logs.db

Then execute the following commands:

sql

.tables               -- To list the tables
SELECT * FROM logs;  -- To see the entries in the logs table

## Dependencies

- SQLite3
- C++11 or later
- POSIX-compliant threading library (`pthread`)

### Windows Users

For Windows users, it is recommended to use **MSYS2** for building and running the application. Follow these steps:

1. Install MSYS2 and SQLite3.
2. Use the `g++` compiler provided by MSYS2 to compile the application.






