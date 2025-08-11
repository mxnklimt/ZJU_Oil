// AMTParser.h
#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cmath>

class AMTParser {
public:
    explicit AMTParser(uint8_t deviceAddr = 0x01) : addr(deviceAddr) {}

    void setAddress(uint8_t newAddr) { addr = newAddr; }
    uint8_t getAddress() const { return addr; }

    // ====== 指令生成 ======

    // 读取位置1（寄存器0x0004，长度2）
    std::vector<uint8_t> makeReadPosition1Cmd() const {
        return makeReadCmd(0x0004, 0x0002);
    }

    // 同时读取位置1 + 温度1（寄存器0x0004，长度3）
    std::vector<uint8_t> makeReadPos1Temp1Cmd() const {
        return makeReadCmd(0x0004, 0x0003);
    }

    // 读取温度1（寄存器0x0006，长度1）
    std::vector<uint8_t> makeReadTemp1Cmd() const {
        return makeReadCmd(0x0006, 0x0001);
    }

    // 读取位置2（寄存器0x0002，长度2）
    std::vector<uint8_t> makeReadPosition2Cmd() const {
        return makeReadCmd(0x0002, 0x0002);
    }

    // ====== 数据解析 ======

    // 解析位置（单位mm），输入 Modbus 数据帧（完整）
    double parsePosition(const std::vector<uint8_t>& packet, size_t intIndex, size_t fracIndex) const {
        if (packet.size() < std::max(intIndex, fracIndex) + 2) {
            throw std::runtime_error("位置数据包长度不足");
        }
        uint16_t intPart = (packet[intIndex] << 8) | packet[intIndex + 1];
        uint16_t fracPart = (packet[fracIndex] << 8) | packet[fracIndex + 1];
        return intPart + static_cast<double>(fracPart) / 65535.0;
    }

    // 解析温度（摄氏度），输入 Modbus 数据帧（完整）
    double parseTemperature(const std::vector<uint8_t>& packet, size_t tempIndex) const {
        if (packet.size() < tempIndex + 2) {
            throw std::runtime_error("温度数据包长度不足");
        }
        uint16_t raw = (packet[tempIndex] << 8) | packet[tempIndex + 1];
        int signBits = (raw >> 11) & 0x1F; // 高5位符号
        int valueBits = raw & 0x07FF;      // 低11位数值

        if (signBits == 0x00) { // 正温度
            return valueBits / 16.0;
        }
        else if (signBits == 0x1F) { // 负温度
            int twosComplement = (~valueBits & 0x07FF) + 1;
            return -(twosComplement / 16.0);
        }
        else {
            throw std::runtime_error("温度符号位异常");
        }
    }

    // CRC16
    static uint16_t calculateCRC(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc = (crc >> 1) ^ 0xA001;
                }
                else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

private:
    uint8_t addr;

    // 通用读取指令
    std::vector<uint8_t> makeReadCmd(uint16_t startReg, uint16_t regCount) const {
        std::vector<uint8_t> cmd(6);
        cmd[0] = addr;
        cmd[1] = 0x03; // 功能码：读保持寄存器
        cmd[2] = static_cast<uint8_t>(startReg >> 8);
        cmd[3] = static_cast<uint8_t>(startReg & 0xFF);
        cmd[4] = static_cast<uint8_t>(regCount >> 8);
        cmd[5] = static_cast<uint8_t>(regCount & 0xFF);

        uint16_t crc = calculateCRC(cmd.data(), cmd.size());
        cmd.push_back(static_cast<uint8_t>(crc & 0xFF)); // CRC低字节
        cmd.push_back(static_cast<uint8_t>(crc >> 8));   // CRC高字节
        return cmd;
    }
};
