#pragma once
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iomanip>
#include"FiberGratingAnalyzer.h"

class FibreGratingManager {
private:
    std::atomic<bool> running_{ false };
    std::atomic<bool> dataReceiving_{ false };
    std::thread workerThread_;
    std::mutex dataMutex_;
    std::condition_variable cv_;

    // 统计变量
    std::atomic<int> totalPackets_{ 0 };
    std::atomic<int> totalSensors_{ 0 };
    std::chrono::steady_clock::time_point startTime_;

public:
    FibreGratingManager() = default;

    ~FibreGratingManager() {
        stop();
    }

    // 启动光纤光栅分析器（非阻塞）
    bool start() {
        if (dataReceiving_.load()) {
            std::cout << "光纤光栅分析器已在运行中..." << std::endl;
            return false;
        }

        // 重置统计信息
        totalPackets_ = 0;
        totalSensors_ = 0;
        startTime_ = std::chrono::steady_clock::now();
        running_ = true;
        dataReceiving_ = true;

        // 在独立线程中启动数据接收
        workerThread_ = std::thread([this]() {
            this->runAnalyzer();
            });

        std::cout << "光纤光栅分析器启动成功" << std::endl;
        return true;
    }

    // 停止光纤光栅分析器
    void stop() {
        if (!dataReceiving_.load()) return;

        running_ = false;
        dataReceiving_ = false;

        // 通知等待的线程
        cv_.notify_all();

        if (workerThread_.joinable()) {
            workerThread_.join();
        }

        displayFinalStatistics();
    }

    // 检查是否正在运行
    bool isRunning() const {
        return dataReceiving_.load();
    }

    // 获取运行统计信息
    struct Statistics {
        int totalPackets;
        int totalSensors;
        long long runTimeSeconds;
        double packetsPerSecond;
        double sensorsPerSecond;
    };

    Statistics getStatistics() const {
        auto currentTime = std::chrono::steady_clock::now();
        auto runTime = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - startTime_).count();

        Statistics stats{};
        stats.totalPackets = totalPackets_.load();
        stats.totalSensors = totalSensors_.load();
        stats.runTimeSeconds = runTime;
        stats.packetsPerSecond = (runTime > 0) ?
            static_cast<double>(stats.totalPackets) / runTime : 0.0;
        stats.sensorsPerSecond = (runTime > 0) ?
            static_cast<double>(stats.totalSensors) / runTime : 0.0;

        return stats;
    }

private:
    void runAnalyzer() {
        const int LISTEN_PORT = 8071;
        const std::string LOCAL_ADDRESS = "0.0.0.0";

        std::cout << "=== 光纤光栅解调仪数据接收程序 ===" << std::endl;
        std::cout << "监听地址: " << LOCAL_ADDRESS << std::endl;
        std::cout << "监听端口: " << LISTEN_PORT << std::endl;
        std::cout << "开始时间: " << __DATE__ << " " << __TIME__ << std::endl;
        std::cout << "==================================" << std::endl << std::endl;

        FiberGratingAnalyzer analyzer;

        // 设置端口
        if (!analyzer.setPort(LISTEN_PORT)) {
            std::cerr << "错误: 无法在端口 " << LISTEN_PORT << " 上启动监听" << std::endl;
            dataReceiving_ = false;
            return;
        }

        // 数据回调函数
        auto dataCallback = [this](const std::vector<FiberGratingAnalyzer::SensorData>& sensors) {
            this->processSensorData(sensors);
            };

        // 开始接收数据[1,2](@ref)
        if (!analyzer.startReceiving(dataCallback)) {
            std::cerr << "错误: 无法启动数据接收线程" << std::endl;
            dataReceiving_ = false;
            return;
        }

        std::cout << "数据接收已启动，开始监听..." << std::endl;

        // 接收循环[2,4](@ref)
        try {
            while (running_.load()) {
                std::unique_lock<std::mutex> lock(dataMutex_);
                cv_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                    return !running_.load();
                    });

                // 心跳检测
                static int heartbeat = 0;
                if (++heartbeat % 10 == 0 && totalPackets_.load() == 0) {
                    std::cout << "等待数据输入中..." << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "数据接收异常: " << e.what() << std::endl;
        }

        // 停止接收[1](@ref)
        analyzer.stopReceiving();
        dataReceiving_ = false;
    }

    void processSensorData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors) {
        totalPackets_++;
        totalSensors_ += sensors.size();

        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - startTime_).count();

        // 实时显示统计信息[2](@ref)
        std::cout << "\033[2J\033[1;1H";
        std::cout << "=== 实时数据监控 ===" << std::endl;
        std::cout << "运行时间: " << elapsed << " 秒" << std::endl;
        std::cout << "数据包数: " << totalPackets_.load() << std::endl;
        std::cout << "传感器数据总数: " << totalSensors_.load() << std::endl;
        std::cout << "平均频率: " << (elapsed > 0 ? totalPackets_.load() / elapsed : 0) << " Hz" << std::endl;
        std::cout << "==========================" << std::endl;

        if (!sensors.empty()) {
            std::cout << "最新数据包详情 (" << sensors.size() << " 个传感器):" << std::endl;
            std::cout << std::setw(6) << "通道"
                << std::setw(8) << "序列号"
                << std::setw(12) << "波长(nm)"
                << std::setw(15) << "物理量"
                << std::setw(10) << "状态" << std::endl;
            std::cout << std::string(55, '-') << std::endl;

            for (const auto& sensor : sensors) {
                std::cout << std::setw(6) << sensor.channel
                    << std::setw(8) << sensor.sequence
                    << std::setw(12) << std::fixed << std::setprecision(3) << sensor.wavelength;

                if (sensor.hasPhysicalValue) {
                    std::cout << std::setw(15) << std::setprecision(4) << sensor.physicalValue;
                }
                else {
                    std::cout << std::setw(15) << "N/A";
                }

                if (sensor.wavelength < 1520.0 || sensor.wavelength > 1620.0) {
                    std::cout << std::setw(10) << "异常";
                }
                else {
                    std::cout << std::setw(10) << "正常";
                }
                std::cout << std::endl;
            }
        }

        std::cout << std::endl << "正在接收数据... 使用 stop() 方法停止";
        std::cout.flush();
    }

    void displayFinalStatistics() {
        auto endTime = std::chrono::steady_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(
            endTime - startTime_).count();

        std::cout << "\n=== 接收统计 ===" << std::endl;
        std::cout << "总运行时间: " << totalDuration << " 秒" << std::endl;
        std::cout << "总数据包数: " << totalPackets_.load() << std::endl;
        std::cout << "总传感器数据: " << totalSensors_.load() << std::endl;
        if (totalDuration > 0) {
            std::cout << "平均数据率: " << (totalPackets_.load() / totalDuration) << " 包/秒" << std::endl;
            std::cout << "平均传感器数据率: " << (totalSensors_.load() / totalDuration) << " 数据/秒" << std::endl;
        }
        std::cout << "光纤光栅分析器已停止" << std::endl;
    }
};