#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>

class ADXL355Parser {
public:
    struct AccelerationData {
        float x;  // X轴加速度 (g)
        float y;  // Y轴加速度 (g)
        float z;  // Z轴加速度 (g)
    };

    ADXL355Parser(uint8_t deviceId = 0x00) : deviceId_(deviceId) {}

    void setDeviceId(uint8_t id) { deviceId_ = id; }
    uint8_t getDeviceId() const { return deviceId_; }

    // 生成读取加速度指令 (修正CRC字节顺序)
    std::vector<uint8_t> generateReadAccelerationCommand() {
        std::vector<uint8_t> command = {
            deviceId_,      // Modbus地址
            0x03,           // 功能码(读)
            0x00, 0xF7,     // 寄存器地址 (AXH)
            0x00, 0x06,     // 读取长度 (6个寄存器)
            0x00, 0x00      // CRC占位
        };

        uint16_t crc = calculateCRC(command.data(), command.size() - 2);
        // 低字节在前，高字节在后
        command[6] = crc & 0xFF;
        command[7] = (crc >> 8) & 0xFF;

        return command;
    }

    // 解析加速度数据 (修正CRC检查)
    AccelerationData parseAccelerationResponse(const std::vector<uint8_t>& response) {
        if (response.size() != 17) {
            throw std::runtime_error("响应长度应为17字节");
        }

        // 验证设备ID和功能码
        if (response[0] != deviceId_ || response[1] != 0x03) {
            throw std::runtime_error("设备ID或功能码不匹配");
        }

        // 验证数据长度
        if (response[2] != 0x0C) {
            throw std::runtime_error("数据长度应为12字节(0x0C)");
        }

        // 解析加速度数据
        AccelerationData data;
        data.x = convertToAcceleration(response[3], response[4], response[5], response[6]);
        data.y = convertToAcceleration(response[7], response[8], response[9], response[10]);
        data.z = convertToAcceleration(response[11], response[12], response[13], response[14]);

        // 验证CRC (注意响应中的CRC是低字节在前)
        uint16_t receivedCrc = (response[16] << 8) | response[15];
        uint16_t calculatedCrc = calculateCRC(response.data(), response.size() - 2);

        if (receivedCrc != calculatedCrc) {
            throw std::runtime_error("CRC校验失败");
        }

        return data;
    }

private:
    uint8_t deviceId_;

    // 加速度值转换
    float convertToAcceleration(uint8_t hh, uint8_t hl, uint8_t lh, uint8_t ll) {
        int32_t value = (static_cast<int32_t>(hh) << 24) |
            (static_cast<int32_t>(hl) << 16) |
            (static_cast<int32_t>(lh) << 8) |
            static_cast<int32_t>(ll);
        return value * 8.0f / 524288.0f;
    }

    // CRC计算 (Modbus RTU)
    uint16_t calculateCRC(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF;

        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];

            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) {
                    crc >>= 1;
                    crc ^= 0xA001;
                }
                else {
                    crc >>= 1;
                }
            }
        }

        return crc;
    }
};