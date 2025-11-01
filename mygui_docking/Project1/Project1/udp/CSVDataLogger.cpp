#include "CSVDataLogger.h"
#include <iostream>
#include <chrono>

CSVDataLogger::CSVDataLogger() 
    : isLogging_(false), loggedCount_(0) {
}

CSVDataLogger::~CSVDataLogger() {
    stopLogging();
}

bool CSVDataLogger::startLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (isLogging_) {
        std::cout << "数据记录已在运行中" << std::endl;
        return false;
    }

    // 生成文件名
    filename_ = filename.empty() ? generateFilename() : filename;
    
    // 确保目录存在
    ensureDirectoryExists("data");
    
    // 打开文件
    file_.open(filename_, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        std::cerr << "无法创建文件: " << filename_ << std::endl;
        return false;
    }

    isLogging_ = true;
    loggedCount_ = 0;
    startTime_ = std::chrono::steady_clock::now();
    
    // 写入CSV文件头
    writeHeader();
    
    std::cout << "开始记录数据到: " << filename_ << std::endl;
    return true;
}

void CSVDataLogger::stopLogging() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (isLogging_) {
        if (file_.is_open()) {
            file_.close();
        }
        isLogging_ = false;
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime_);
        
        std::cout << "数据记录已停止" << std::endl;
        std::cout << "记录文件: " << filename_ << std::endl;
        std::cout << "记录时长: " << duration.count() << " 秒" << std::endl;
        std::cout << "记录数据量: " << loggedCount_ << " 条" << std::endl;
    }
}

bool CSVDataLogger::isLogging() const {
    return isLogging_;
}

void CSVDataLogger::logSensorData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors) {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (!isLogging_ || !file_.is_open()) {
        return;
    }

    auto timestamp = std::chrono::system_clock::now();
    writeData(sensors, timestamp);
    loggedCount_ += sensors.size();
}

std::string CSVDataLogger::getCurrentFilename() const {
    return filename_;
}

size_t CSVDataLogger::getLoggedCount() const {
    return loggedCount_;
}

void CSVDataLogger::ensureDirectoryExists(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "创建目录失败: " << e.what() << std::endl;
    }
}

std::string CSVDataLogger::generateFilename() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    std::ostringstream oss;
    oss << "data/fbg_data_";
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    oss << ".csv";
    
    return oss.str();
}

void CSVDataLogger::writeHeader() {
    if (!file_.is_open()) return;
    
    file_ << "Timestamp,Channel,Sequence,Wavelength(nm),PhysicalValue,HasPhysicalValue,Status" << std::endl;
    file_.flush();
}

void CSVDataLogger::writeData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors, 
                             const std::chrono::system_clock::time_point& timestamp) {
    if (!file_.is_open()) return;
    
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    for (const auto& sensor : sensors) {
        // 时间戳
        file_ << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << ",";
        
        // 通道和序列号
        file_ << sensor.channel << ",";
        file_ << static_cast<int>(sensor.sequence) << ",";
        
        // 波长值
        file_ << std::fixed << std::setprecision(3) << sensor.wavelength << ",";
        
        // 物理量值
        if (sensor.hasPhysicalValue) {
            file_ << std::setprecision(4) << sensor.physicalValue;
        } else {
            file_ << "N/A";
        }
        file_ << ",";
        
        // 是否有物理量
        file_ << (sensor.hasPhysicalValue ? "true" : "false") << ",";
        
        // 状态（基于波长范围判断）
        std::string status = "正常";
        if (sensor.wavelength < 1520.0 || sensor.wavelength > 1620.0) {
            status = "异常";
        }
        file_ << status;
        
        file_ << std::endl;
    }
    
    file_.flush();
}