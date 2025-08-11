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
    uint32_t getDistance(uint8_t addr) {
        sendDistanceCommand(addr);
        auto response = receiveDistanceResponse();
        return parseDistance(response, addr);
    }

    // 开始轮询采集
     void startContinuousCollection(const std::vector<uint8_t>& deviceAddresses) {
    if (isCollecting) return;
    isCollecting = true;

    const size_t MAX_SIZE = 1000;  // 最大缓存条数，根据需求调整

    collectionThread = std::thread([this, deviceAddresses, MAX_SIZE]() {
        while (isCollecting) {
            auto now = std::chrono::system_clock::now();
            for (auto addr : deviceAddresses) {
                try {
                    uint32_t dist = getDistance(addr);
                    {
                        std::lock_guard<std::mutex> lock(LasergetMutex);
                        auto& vec = collectedLasorMap[addr];
                        if (vec.size() >= MAX_SIZE) {
                            // 删除最旧数据，保持最大长度
                            vec.erase(vec.begin());
                        }
                        vec.emplace_back(now, dist);
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "地址 0x" << std::hex << int(addr) << " 采集失败: " << e.what() << "\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 每台设备间隔
            }
        }
        });
}

    //void startContinuousCollection(const std::vector<uint8_t>& deviceAddresses) {
    //    if (isCollecting) return;
    //    isCollecting = true;
    //    //collectedDataMap.clear();

    //    collectionThread = std::thread([this, deviceAddresses]() {
    //        while (isCollecting) {
    //            auto now = std::chrono::system_clock::now();
    //            for (auto addr : deviceAddresses) {
    //                try {
    //                    uint32_t dist = getDistance(addr);
    //                    std::lock_guard<std::mutex> lock(LasergetMutex);
    //                    collectedLasorMap[addr].emplace_back(now, dist);
    //                }
    //                catch (const std::exception& e) {
    //                    std::cerr << "地址 0x" << std::hex << int(addr) << " 采集失败: " << e.what() << "\n";
    //                }
    //                std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 每台设备间隔
    //            }
    //        }
    //        });
    //}
    // 开始轮询采集
    void startContinuousCollection2(const std::vector<uint8_t>& deviceAddresses) {
        if (isCollecting2) return;
        isCollecting2 = true;
        //collectedDataMap.clear();

        collectionThread = std::thread([this, deviceAddresses]() {
            while (isCollecting2) {
                auto now = std::chrono::system_clock::now();
                for (auto addr : deviceAddresses) {
                    try {
                        uint32_t dist = getDistance(addr);
                        std::lock_guard<std::mutex> lock(LasergetMutex2);
                        collectedLasorMap2[addr].emplace_back(now, dist);
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
    std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint32_t>>>
        getLatestData() {
        std::lock_guard<std::mutex> lock(LasergetMutex);
        return collectedLasorMap; // 返回采集线程里最新的数据
    }


private:
    RS485Manager& rs485Manager;
    std::atomic<bool> isCollecting{ false };
	std::atomic<bool> isCollecting2{ false };   
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
        //return rs485Manager.receive(9); // 9字节，1秒超时
        return rs485Manager.receiveLaser(400);

    }

    // 解析响应
    uint32_t parseDistance(const std::vector<uint8_t>& response, uint8_t addr) {
        if (response.size() < 9) throw std::runtime_error("无效的响应长度");
        
            if (response[0] != addr) {
                std::cerr << "响应地址不匹配，期望: " << (int)addr << "，实际: " << (int)response[0] << std::endl;
                throw std::runtime_error("响应地址不匹配");
            }
        
        if (response[1] != 0x04) throw std::runtime_error("无效的功能码");
        if (response[2] != 0x04) throw std::runtime_error("无效的字节数");

        uint16_t receivedCRC = (response[8] << 8) | response[7];
        uint16_t calculatedCRC = calculateCRC(response.data(), response.size() - 2);
        if (receivedCRC != calculatedCRC) throw std::runtime_error("CRC校验失败");

        uint32_t distVal = (response[3] << 24) | (response[4] << 16) | (response[5] << 8) | response[6];
        return static_cast<uint32_t>(distVal);
    }
    
};


