// LaserSensorProtocol.h
#pragma once
#include "RS485/RS485Manager.h"
#include <vector>
#include <cstdint>
#include <stdexcept>

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

private:
    RS485Manager& rs485Manager;

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