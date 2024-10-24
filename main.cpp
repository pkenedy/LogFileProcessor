#include "LogProcessor.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <database_file> <log_file>" << std::endl;
        return 1;
    }
    
    LogProcessor logProcessor(argv[1]); // Database file from command line
    logProcessor.processLogFile(argv[2]); // Log file from command line
    return 0;
}
