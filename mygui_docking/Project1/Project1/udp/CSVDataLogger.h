#ifndef CSV_DATA_LOGGER_H
#define CSV_DATA_LOGGER_H

#include "FiberGratingAnalyzer.h"
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

class CSVDataLogger {
public:
    CSVDataLogger();
    ~CSVDataLogger();

    bool startLogging(const std::string& filename = "");
    void stopLogging();
    bool isLogging() const;
    void logSensorData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors);
    std::string getCurrentFilename() const;
    size_t getLoggedCount() const;

private:
    void ensureDirectoryExists(const std::string& path);
    std::string generateFilename() const;
    void writeHeader();
    void writeData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors,
        const std::chrono::system_clock::time_point& timestamp);

private:
    std::ofstream file_;
    std::string filename_;
    std::mutex fileMutex_;
    bool isLogging_;
    size_t loggedCount_;
    std::chrono::steady_clock::time_point startTime_;
};

#endif // CSV_DATA_LOGGER_H