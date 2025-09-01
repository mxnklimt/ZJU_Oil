
// JY61Parser.h 
#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "RS485/RS485Manager.h"
#include "data/Data.h"  // 记得包含你定义 JY61PData 的头文件

class JY61Parser {
public:
    bool parseRegisterResponse(const std::vector<uint8_t>& resp, float& value) {
        if (resp.size() < 5 || resp[0] != 0x55) return false;

        uint8_t regAddr = resp[2];
        int16_t raw = (int16_t)((resp[4] << 8) | resp[3]); // 高低字节拼接
        // 按寄存器定义缩放
        if (regAddr >= 0x34 && regAddr <= 0x36) { // Acc
            value = raw / 32768.0f * 16.0f; // g
        }
        else if (regAddr >= 0x37 && regAddr <= 0x39) { // Gyro
            value = raw / 32768.0f * 2000.0f; // dps
        }
        else if (regAddr >= 0x3d && regAddr <= 0x3f) { // Angle
            value = raw / 32768.0f * 180.0f; // deg
        }
        else if (regAddr == 0x40) { // Temp
            value = raw / 100.0f;
        }
        else {
            return false;
        }
        return true;
    }
    JY61Parser(uint8_t deviceAddr = 0x50) : addr(deviceAddr) {}

    // -------------------- 协议帧解析 --------------------
    bool parseFrame(const std::vector<uint8_t>& frame, JY61PData::angle& outData) {
        if (frame.size() != 11) return false;
        if (frame[0] != 0x55) return false; // 帧头固定0x55

        uint8_t type = frame[1];
        uint8_t sum = 0;
        for (int i = 0; i < 10; i++) sum += frame[i];
        if ((sum & 0xFF) != frame[10]) return false; // 校验和错误

        switch (type) {
        case 0x51: { // 加速度 + 温度
            int16_t axRaw = (int16_t)((frame[3] << 8) | frame[2]);
            int16_t ayRaw = (int16_t)((frame[5] << 8) | frame[4]);
            int16_t azRaw = (int16_t)((frame[7] << 8) | frame[6]);
            int16_t tempRaw = (int16_t)((frame[9] << 8) | frame[8]);

            outData.a[0] = axRaw / 32768.0 * 16.0; // g 单位
            outData.a[1] = ayRaw / 32768.0 * 16.0;
            outData.a[2] = azRaw / 32768.0 * 16.0;
            outData.temperature = tempRaw / 100.0; // ℃
            break;
        }
        case 0x52: { // 角速度
            int16_t wxRaw = (int16_t)((frame[3] << 8) | frame[2]);
            int16_t wyRaw = (int16_t)((frame[5] << 8) | frame[4]);
            int16_t wzRaw = (int16_t)((frame[7] << 8) | frame[6]);

            outData.w[0] = wxRaw / 32768.0 * 2000.0;
            outData.w[1] = wyRaw / 32768.0 * 2000.0;
            outData.w[2] = wzRaw / 32768.0 * 2000.0;
            break;
        }
        case 0x53: { // 角度
            int16_t rollRaw = (int16_t)((frame[3] << 8) | frame[2]);
            int16_t pitchRaw = (int16_t)((frame[5] << 8) | frame[4]);
            int16_t yawRaw = (int16_t)((frame[7] << 8) | frame[6]);

            outData.Angle[0] = rollRaw / 32768.0 * 180.0;
            outData.Angle[1] = pitchRaw / 32768.0 * 180.0;
            outData.Angle[2] = yawRaw / 32768.0 * 180.0;
            break;
        }
        default:
            return false;
        }
        return true;
    }


    // -------------------- 下发指令 --------------------
    std::vector<uint8_t> buildReadRegisterCmd(uint8_t regAddr) {
        // 协议：FF AA 27 REG_ADDR 00
        std::vector<uint8_t> cmd = { 0xFF, 0xAA, 0x27, regAddr, 0x00 };
        //return cmd;
        return { 0x03,0x03,0x00,0x34,0x00,0x0f,0x45,0xE2 };
    }

    std::vector<uint8_t> readRegister(RS485Manager& jy61manager, uint8_t regAddr) {
        auto cmd = buildReadRegisterCmd(regAddr);
        jy61manager.send(cmd);
        return jy61manager.receive(11);
    }

    // -------------------- 常用寄存器地址 --------------------
    enum Register : uint8_t {
        REG_AX = 0x34, REG_AY = 0x35, REG_AZ = 0x36,
        REG_WX = 0x37, REG_WY = 0x38, REG_WZ = 0x39,
        REG_ROLL = 0x3D, REG_PITCH = 0x3E, REG_YAW = 0x3F,
        REG_TEMP = 0x40
    };

private:
    uint8_t addr;
};