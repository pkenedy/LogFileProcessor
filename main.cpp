#include "LogProcessor.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Simulate command-line arguments for testing purposes
    std::string dbFile = "logs.db";
    std::string logFile = "C:/Users/paul/Documents/LogFileProcessor/my_log.log"; // Update to your log file path

    LogProcessor logProcessor(dbFile);
    logProcessor.processLogFile(logFile);
    
    return 0;
}
