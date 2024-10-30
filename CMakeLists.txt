cmake_minimum_required(VERSION 3.10)
project(LogFileProcessor)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)

# Add executable target
add_executable(LogFileProcessor main.cpp LogProcessor.cpp)

# Manually specify the paths for SQLite3 include and library files
set(SQLITE3_INCLUDE_DIR "C:/msys64/mingw64/include")
set(SQLITE3_LIBRARY "C:/msys64/mingw64/lib/libsqlite3.a")

# Link SQLite3 directly without using find_package
target_include_directories(LogFileProcessor PRIVATE ${SQLITE3_INCLUDE_DIR})
target_link_libraries(LogFileProcessor PRIVATE ${SQLITE3_LIBRARY})

# Link threading libraries
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(LogFileProcessor PRIVATE Threads::Threads)
