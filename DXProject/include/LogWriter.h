#pragma once

#include <iostream>
#include <fstream>
#include <mutex>
#include <sstream>

class LogWriter {
private:
    std::ofstream logFile;
    std::mutex mtx;

    LogWriter() {
        logFile.open("DXProject.log", std::ios::trunc);
        if (!logFile.is_open()) {
            std::cerr << "Error opening log file!" << std::endl;
        }
    }

    LogWriter(const LogWriter&) = delete;
    LogWriter& operator=(const LogWriter&) = delete;

public:
    static LogWriter& getInstance() {
        static LogWriter instance;
        return instance;
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::lock_guard<std::mutex> lock(mtx);
        if (logFile.is_open()) {
            std::ostringstream oss;
            (oss << ... << args);
            logFile << oss.str() << std::endl;
        }
    }

    ~LogWriter() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    //// Helper stream that gathers operator<< input and writes on destruction
    //class Stream {
    //    std::ostringstream oss;
    //    LogWriter& writer;
    //public:
    //    explicit Stream(LogWriter& w) : writer(w) {}
    //    Stream(const Stream&) = delete;
    //    Stream& operator=(const Stream&) = delete;

    //    template<typename T>
    //    Stream& operator<<(T&& value) {
    //        oss << std::forward<T>(value);
    //        return *this;
    //    }

    //    // flush accumulated message on destruction
    //    ~Stream() {
    //        writer.log(oss.str());
    //    }
    //};

    //// Factory for creating a temporary Stream bound to the singleton
    //static Stream makeStream() {
    //    return Stream(getInstance());
    //}
};

// Macro allowing calls like: log("Mesh " << meshNum++);
//#define log(...) LogWriter::makeStream() << __VA_ARGS__

#define LOG(...) LogWriter::getInstance().log(__VA_ARGS__)