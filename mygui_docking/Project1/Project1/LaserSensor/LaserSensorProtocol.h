// LaserSensorProtocol.h
#pragma once
#include "RS485/RS485Manager.h"
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <mutex>
#include <deque>
#include <unordered_map>
#include <chrono>
#include <sstream>
#include <iomanip>

class LaserSensorProtocol {
public:
    // 构造函数，需要传入一个已初始化的RS485Manager对象
    LaserSensorProtocol(RS485Manager& rs485) : rs485Manager(rs485) {}

    // 获取距离测量值（单位：毫米）
    uint16_t getDistance() {
        // 发送测距命令
        sendDistanceCommand();

        // 接收响应数据
        std::vector<uint8_t> response = receiveDistanceResponse();

        // 解析距离值
        return parseDistance(response);
    }

    // 开始连续采集数据
    void startContinuousCollection() {
        if (!isCollecting) {
            isCollecting = true;
            collectedData.clear();
            collectionThread = std::thread([this]() {
                while (isCollecting) {
                    try {
                        auto distance = getDistance();
                        auto now = std::chrono::system_clock::now();

                        std::lock_guard<std::mutex> lock(dataMutex);
                        collectedData.emplace_back(now, distance);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "采集数据时出错: " << e.what() << std::endl;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ms采样间隔
                }
                });
        }
    }

    // 停止连续采集并返回采集的数据
    std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>> stopContinuousCollection() {
        if (isCollecting) {
            isCollecting = false;
            if (collectionThread.joinable()) {
                collectionThread.join();
            }

            std::lock_guard<std::mutex> lock(dataMutex);
            return collectedData;
        }
        return {};
    }

    // 保存采集的数据到Excel文件
    void saveDataToExcel(const std::string& filePath) {
        std::lock_guard<std::mutex> lock(dataMutex);

        try {
            OpenXLSX::XLDocument doc;
            doc.create(filePath, false);
            doc.open(filePath);
            auto wks = doc.workbook().worksheet("Sheet1");

            // 写入表头
            wks.cell(1, 1).value() = "Time";
            wks.cell(1, 2).value() = "Distance (mm)";

            // 写入数据
            for (size_t row = 0; row < collectedData.size(); ++row) {
                const auto& [timestamp, distance] = collectedData[row];
                auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                std::tm tm;
                localtime_s(&tm, &time_t);
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
                wks.cell(row + 2, 1).value() = oss.str();
                wks.cell(row + 2, 2).value() = distance;
            }

            doc.save();
            doc.close();
        }
        catch (const std::exception& e) {
            throw std::runtime_error("保存数据到Excel失败: " + std::string(e.what()));
        }
    }

private:
    RS485Manager& rs485Manager;
    std::atomic<bool> isCollecting{ false };
    std::thread collectionThread;
    std::mutex dataMutex;
    std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>> collectedData;

    // 发送测距命令
    void sendDistanceCommand() {
        // 假设协议格式为：帧头(0xAA) + 命令码(0x01) + 校验和
        std::vector<uint8_t> command = {
            0xAA,   // 帧头
            0x01,   // 测距命令
            0xAB    // 校验和(示例值，实际应根据协议计算)
        };

        try {
            rs485Manager.send(command);
        }
        catch (const std::runtime_error& e) {
            throw std::runtime_error("发送测距命令失败: " + std::string(e.what()));
        }
    }

    // 接收距离响应
    std::vector<uint8_t> receiveDistanceResponse() {
        try {
            // 假设响应格式为6字节：帧头 + 数据高字节 + 数据低字节 + 状态 + 校验和
            return rs485Manager.receive(6, 1000); // 等待1秒超时
        }
        catch (const std::runtime_error& e) {
            throw std::runtime_error("接收距离响应失败: " + std::string(e.what()));
        }
    }

    // 解析距离值
    uint16_t parseDistance(const std::vector<uint8_t>& response) {
        // 检查响应长度
        if (response.size() < 6) {
            throw std::runtime_error("无效的响应长度");
        }

        // 检查帧头
        if (response[0] != 0xAA) {
            throw std::runtime_error("无效的响应帧头");
        }

        // 计算校验和（简单示例）
        uint8_t checksum = response[0];
        for (size_t i = 1; i < response.size() - 1; ++i) {
            checksum ^= response[i];
        }

        if (checksum != response.back()) {
            throw std::runtime_error("校验和错误");
        }

        // 组合距离值（假设大端格式）
        uint16_t distance = (response[1] << 8) | response[2];

        // 检查状态字节（假设第4字节为状态）
        if (response[3] != 0x00) {
            throw std::runtime_error("传感器返回错误状态: " + std::to_string(response[3]));
        }

        return distance;
    }
};