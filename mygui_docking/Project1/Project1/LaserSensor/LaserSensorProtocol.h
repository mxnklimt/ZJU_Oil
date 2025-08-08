// LaserSensorProtocol.h


#pragma once
#include "RS485/RS485Manager.h"
#include "data/Data.h"
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
#include <iostream>

class LaserSensorProtocol {
public:
    // 构造函数，需要传入一个已初始化的RS485Manager对象
    LaserSensorProtocol(RS485Manager& rs485)
        : rs485Manager(rs485) {
    }

    // 获取某个地址的距离（单位：毫米）
    uint16_t getDistance(uint8_t addr) {
        sendDistanceCommand(addr);
        auto response = receiveDistanceResponse();
        return parseDistance(response, addr);
    }

    // 开始轮询采集
    void startContinuousCollection(const std::vector<uint8_t>& deviceAddresses) {
        if (isCollecting) return;
        isCollecting = true;
        //collectedDataMap.clear();

        collectionThread = std::thread([this, deviceAddresses]() {
            while (isCollecting) {
                auto now = std::chrono::system_clock::now();
                for (auto addr : deviceAddresses) {
                    try {
                        uint16_t dist = getDistance(addr);
                        std::lock_guard<std::mutex> lock(LasergetMutex);
                        collectedLasorMap[addr].emplace_back(now, dist);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "地址 0x" << std::hex << int(addr) << " 采集失败: " << e.what() << "\n";
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 每台设备间隔
                }
            }
            });
    }

    // 停止采集并返回所有设备数据
    std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>>>
        stopContinuousCollection()
    {
        if (!isCollecting) return {};
        isCollecting = false;
        if (collectionThread.joinable()) collectionThread.join();

        /*std::lock_guard<std::mutex> lock(LasergetMutex);
        return collectedDataMap;*/
    }
    // 在类里加：
    std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>>>
        getLatestData() {
        std::lock_guard<std::mutex> lock(LasergetMutex);
        return collectedLasorMap; // 返回采集线程里最新的数据
    }


private:
    RS485Manager& rs485Manager;
    std::atomic<bool> isCollecting{ false };
    std::thread collectionThread;
   
    //std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>>> collectedDataMap;

    // CRC16 (MODBUS)
    uint16_t calculateCRC(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; ++j) {
                if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
                else crc >>= 1;
            }
        }
        return crc;
    }

    // 发送测距命令
    void sendDistanceCommand(uint8_t addr) {
        std::vector<uint8_t> command = {
            addr, 0x04, 0x00, 0x00, 0x00, 0x02
        };
        uint16_t crc = calculateCRC(command.data(), command.size());
        command.push_back(static_cast<uint8_t>(crc & 0xFF));
        command.push_back(static_cast<uint8_t>(crc >> 8));

        rs485Manager.send(command);
    }

    // 接收距离响应
    std::vector<uint8_t> receiveDistanceResponse() {
        return rs485Manager.receive(9, 1000); // 9字节，1秒超时
    }

    // 解析响应
    uint16_t parseDistance(const std::vector<uint8_t>& response, uint8_t addr) {
        if (response.size() < 9) throw std::runtime_error("无效的响应长度");
        if (response[0] != addr) throw std::runtime_error("响应地址不匹配");
        if (response[1] != 0x04) throw std::runtime_error("无效的功能码");
        if (response[2] != 0x04) throw std::runtime_error("无效的字节数");

        uint16_t receivedCRC = (response[8] << 8) | response[7];
        uint16_t calculatedCRC = calculateCRC(response.data(), response.size() - 2);
        if (receivedCRC != calculatedCRC) throw std::runtime_error("CRC校验失败");

        uint32_t distVal = (response[3] << 24) | (response[4] << 16) | (response[5] << 8) | response[6];
        return static_cast<uint16_t>(distVal);
    }
    
};


//class LaserSensorProtocol {
//public:
//    void setDeviceAddress(uint8_t newAddr) {
//        currentAddress = newAddr;
//    }
//    // 构造函数，需要传入一个已初始化的RS485Manager对象
//    LaserSensorProtocol(RS485Manager& rs485, uint8_t deviceAddress = 0x01)
//        : rs485Manager(rs485), deviceAddress(deviceAddress) {
//    }
//
//    // 获取距离测量值（单位：毫米）
//    uint16_t getDistance() {
//        // 发送测距命令
//        sendDistanceCommand();
//
//        // 接收响应数据
//        std::vector<uint8_t> response = receiveDistanceResponse();
//
//        // 解析距离值
//        return parseDistance(response);
//    }
//
//    // 开始连续采集数据
//    void startContinuousCollection() {
//        if (!isCollecting) {
//            isCollecting = true;
//            collectedData.clear();
//            collectionThread = std::thread([this]() {
//                while (isCollecting) {
//                    try {
//                        auto distance = getDistance();
//                        auto now = std::chrono::system_clock::now();
//
//                        std::lock_guard<std::mutex> lock(dataMutex);
//                        collectedData.emplace_back(now, distance);
//                    }
//                    catch (const std::exception& e) {
//                        std::cerr << "采集数据时出错: " << e.what() << std::endl;
//                    }
//                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ms采样间隔
//                }
//                });
//        }
//    }
//
//    // 停止连续采集并返回采集的数据
//    std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>> stopContinuousCollection() {
//        if (isCollecting) {
//            isCollecting = false;
//            if (collectionThread.joinable()) {
//                collectionThread.join();
//            }
//
//            std::lock_guard<std::mutex> lock(dataMutex);
//            return collectedData;
//        }
//        return {};
//    }
//
//private:
//    uint8_t currentAddress;
//    RS485Manager& rs485Manager;
//    uint8_t deviceAddress;
//    std::atomic<bool> isCollecting{ false };
//    std::thread collectionThread;
//    std::mutex dataMutex;
//    std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>> collectedData;
//
//    // 计算CRC16校验码 (MODBUS)
//    uint16_t calculateCRC(const uint8_t* data, size_t length) {
//        uint16_t crc = 0xFFFF;
//        for (size_t i = 0; i < length; ++i) {
//            crc ^= data[i];
//            for (uint8_t j = 0; j < 8; ++j) {
//                if (crc & 0x0001) {
//                    crc = (crc >> 1) ^ 0xA001;
//                }
//                else {
//                    crc >>= 1;
//                }
//            }
//        }
//        return crc;
//    }
//
//    // 发送测距命令 (根据文档中的MODBUS协议)
//    void sendDistanceCommand() {
//        // 命令格式: 地址码(1) + 功能码(1) + 寄存器地址(2) + 寄存器数量(2) + CRC(2)
//        std::vector<uint8_t> command(6);
//        command[0] = deviceAddress;  // 地址码
//        command[1] = 0x04;           // 功能码(读取输入寄存器)
//        command[2] = 0x00;           // 寄存器地址高字节(距离寄存器地址0x0000)
//        command[3] = 0x00;           // 寄存器地址低字节
//        command[4] = 0x00;           // 寄存器数量高字节(读取2个寄存器)
//        command[5] = 0x02;           // 寄存器数量低字节
//
//        // 计算CRC
//        uint16_t crc = calculateCRC(command.data(), command.size() - 2);
//        command.push_back(static_cast<uint8_t>(crc & 0xFF));  // CRC低字节
//        command.push_back(static_cast<uint8_t>(crc >> 8));    // CRC高字节
//
//        try {
//            rs485Manager.send(command);
//        }
//        catch (const std::runtime_error& e) {
//            throw std::runtime_error("发送测距命令失败: " + std::string(e.what()));
//        }
//    }
//
//    // 接收距离响应
//    std::vector<uint8_t> receiveDistanceResponse() {
//        try {
//            // 响应格式: 地址码(1) + 功能码(1) + 字节数(1) + 数据(4) + CRC(2)
//            return rs485Manager.receive(9, 1000); // 等待1秒超时
//        }
//        catch (const std::runtime_error& e) {
//            throw std::runtime_error("接收距离响应失败: " + std::string(e.what()));
//        }
//    }
//
//    // 解析距离值
//    uint16_t parseDistance(const std::vector<uint8_t>& response) {
//        // 检查响应长度
//        if (response.size() < 9) {
//            throw std::runtime_error("无效的响应长度");
//        }
//
//        // 检查地址码
//        if (response[0] != deviceAddress) {
//            throw std::runtime_error("响应地址不匹配");
//        }
//
//        // 检查功能码
//        if (response[1] != 0x04) {
//            throw std::runtime_error("无效的功能码");
//        }
//
//        // 检查字节数
//        if (response[2] != 0x04) {
//            throw std::runtime_error("无效的字节数");
//        }
//
//        // 验证CRC
//        uint16_t receivedCRC = (response[response.size() - 1] << 8) | response[response.size() - 2];
//        uint16_t calculatedCRC = calculateCRC(response.data(), response.size() - 2);
//        if (receivedCRC != calculatedCRC) {
//            throw std::runtime_error("CRC校验失败");
//        }
//
//        // 组合距离值（大端格式）
//        uint32_t distance = (response[3] << 24) | (response[4] << 16) | (response[5] << 8) | response[6];
//
//        // 根据文档示例，距离值需要转换
//        // 示例中: 00 01 19 36 (十六进制) = 1795 (十进制) => 1175mm
//        // 这里假设直接返回原始值，实际可能需要根据文档中的转换公式处理
//        return static_cast<uint16_t>(distance);
//    }
//};
//class LaserSensorProtocol {
//public:
//    // 构造函数，需要传入一个已初始化的RS485Manager对象
//    LaserSensorProtocol(RS485Manager& rs485) : rs485Manager(rs485) {}
//
//    // 获取距离测量值（单位：毫米）
//    uint16_t getDistance() {
//        // 发送测距命令
//        sendDistanceCommand();
//
//        // 接收响应数据
//        std::vector<uint8_t> response = receiveDistanceResponse();
//
//        // 解析距离值
//        return parseDistance(response);
//    }
//
//    // 开始连续采集数据
//    void startContinuousCollection() {
//        if (!isCollecting) {
//            isCollecting = true;
//            collectedData.clear();
//            collectionThread = std::thread([this]() {
//                while (isCollecting) {
//                    try {
//                        auto distance = getDistance();
//                        auto now = std::chrono::system_clock::now();
//
//                        std::lock_guard<std::mutex> lock(dataMutex);
//                        collectedData.emplace_back(now, distance);
//                    }
//                    catch (const std::exception& e) {
//                        std::cerr << "采集数据时出错: " << e.what() << std::endl;
//                    }
//                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ms采样间隔
//                }
//                });
//        }
//    }
//
//    // 停止连续采集并返回采集的数据
//    std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>> stopContinuousCollection() {
//        if (isCollecting) {
//            isCollecting = false;
//            if (collectionThread.joinable()) {
//                collectionThread.join();
//            }
//
//            std::lock_guard<std::mutex> lock(dataMutex);
//            return collectedData;
//        }
//        return {};
//    }
//
//    // 保存采集的数据到Excel文件
//    void saveDataToExcel(const std::string& filePath) {
//        std::lock_guard<std::mutex> lock(dataMutex);
//
//        try {
//            OpenXLSX::XLDocument doc;
//            doc.create(filePath, false);
//            doc.open(filePath);
//            auto wks = doc.workbook().worksheet("Sheet1");
//
//            // 写入表头
//            wks.cell(1, 1).value() = "Time";
//            wks.cell(1, 2).value() = "Distance (mm)";
//
//            // 写入数据
//            for (size_t row = 0; row < collectedData.size(); ++row) {
//                const auto& [timestamp, distance] = collectedData[row];
//                auto time_t = std::chrono::system_clock::to_time_t(timestamp);
//                std::tm tm;
//                localtime_s(&tm, &time_t);
//                std::ostringstream oss;
//                oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
//                wks.cell(row + 2, 1).value() = oss.str();
//                wks.cell(row + 2, 2).value() = distance;
//            }
//
//            doc.save();
//            doc.close();
//        }
//        catch (const std::exception& e) {
//            throw std::runtime_error("保存数据到Excel失败: " + std::string(e.what()));
//        }
//    }
//
//private:
//    RS485Manager& rs485Manager;
//    std::atomic<bool> isCollecting{ false };
//    std::thread collectionThread;
//    std::mutex dataMutex;
//    std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>> collectedData;
//
//    // 发送测距命令
//    void sendDistanceCommand() {
//        // 假设协议格式为：帧头(0xAA) + 命令码(0x01) + 校验和
//        std::vector<uint8_t> command = {
//            //0xAA,   // 帧头
//            0x01,   // 测距命令
//            0xAB    // 校验和(示例值，实际应根据协议计算)
//        };
//
//        try {
//            rs485Manager.send(command);
//        }
//        catch (const std::runtime_error& e) {
//            throw std::runtime_error("发送测距命令失败: " + std::string(e.what()));
//        }
//    }
//
//    // 接收距离响应
//    std::vector<uint8_t> receiveDistanceResponse() {
//        try {
//            // 假设响应格式为6字节：帧头 + 数据高字节 + 数据低字节 + 状态 + 校验和
//            return rs485Manager.receive(6, 1000); // 等待1秒超时
//        }
//        catch (const std::runtime_error& e) {
//            throw std::runtime_error("接收距离响应失败: " + std::string(e.what()));
//        }
//    }
//
//    // 解析距离值
//    uint16_t parseDistance(const std::vector<uint8_t>& response) {
//        // 检查响应长度
//        if (response.size() < 6) {
//            throw std::runtime_error("无效的响应长度");
//        }
//
//        // 检查帧头
//        if (response[0] != 0xAA) {
//            throw std::runtime_error("无效的响应帧头");
//        }
//
//        // 计算校验和（简单示例）
//        uint8_t checksum = response[0];
//        for (size_t i = 1; i < response.size() - 1; ++i) {
//            checksum ^= response[i];
//        }
//
//        if (checksum != response.back()) {
//            throw std::runtime_error("校验和错误");
//        }
//
//        // 组合距离值（假设大端格式）
//        uint16_t distance = (response[1] << 8) | response[2];
//
//        // 检查状态字节（假设第4字节为状态）
//        if (response[3] != 0x00) {
//            throw std::runtime_error("传感器返回错误状态: " + std::to_string(response[3]));
//        }
//
//        return distance;
//    }
//};