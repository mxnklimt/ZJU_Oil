#ifndef DUALAXISSENSORPARSER_H
#define DUALAXISSENSORPARSER_H

#include "RS485/RS485Manager.h"
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cmath>
#include <string>

class DualAxisSensorParser {
public:
    // 传感器采样率枚举
    enum class SamplingRate {
        ADS_1_Hz = 0x4000,
        ADS_10_Hz = 0x0666,
        ADS_20_Hz = 0x0333,
        ADS_50_Hz = 0x0147,
        ADS_100_Hz = 0x00A3,
        ADS_200_Hz = 0x0051,
        ADS_333_Hz = 0x0031,
        ADS_500_Hz = 0x0020
    };

    // 校准命令枚举
    enum class CalibrationCommand {
        CLEAR_CALIBRATION = 0x63,
        START_CALIBRATION = 0x30,
        CALIBRATE_HORIZONTAL = 0x66,
        CALIBRATE_VERTICAL = 0x70
    };

    // 角度数据
    struct AngleData {
        double filtered_horizontal;  // 滤波后的水平轴角度
        double filtered_vertical;    // 滤波后的垂直轴角度
        double raw_horizontal;       // 原始水平轴角度
        double raw_vertical;         // 原始垂直轴角度
    };

    // 构造函数
    explicit DualAxisSensorParser(RS485Manager& rs485, uint8_t deviceAddress = 0x0C)
        : rs485_(rs485), deviceAddress_(deviceAddress) {
    }

    // 设置设备地址
    void setDeviceAddress(uint8_t address) {
        if (address > 0xFF) {
            throw std::invalid_argument("Invalid device address");
        }
        deviceAddress_ = address;
    }

    // 读取角度数据
    AngleData readAngles() {
        // 构建请求报文: 地址 功能码 起始地址高位 起始地址低位 寄存器数量高位 寄存器数量低位
        std::vector<uint8_t> request = {
            deviceAddress_,
            0x03,       // 功能码: 读取保持寄存器
            0x0A,       // 起始地址高位
            0x02,       // 起始地址低位
            0x00,       // 寄存器数量高位
            0x04,       // 寄存器数量低位 (读取4个寄存器)
            0x00, 0x00  // CRC占位符，后面会计算
        };

        // 计算CRC并替换占位符
        uint16_t crc = calculateCRC(request.data(), request.size() - 2);
        request[request.size() - 2] = crc & 0xFF;
        request[request.size() - 1] = (crc >> 8) & 0xFF;

        // 发送请求并接收响应
        rs485_.send(request);
        std::vector<uint8_t> response = rs485_.receive(13); // 13字节响应

        // 验证响应
        validateResponse(response, 0x03, 8);

        // 解析角度数据
        AngleData data;
        data.filtered_horizontal = parseAngle(response[3], response[4]);
        data.filtered_vertical = parseAngle(response[5], response[6]);
        data.raw_horizontal = parseAngle(response[7], response[8]);
        data.raw_vertical = parseAngle(response[9], response[10]);

        return data;
    }

    // 读取采样率
    SamplingRate readSamplingRate() {
        std::vector<uint8_t> request = {
            deviceAddress_,
            0x03,       // 功能码: 读取保持寄存器
            0x1A,       // 起始地址高位
            0x02,       // 起始地址低位
            0x00,       // 寄存器数量高位
            0x01,       // 寄存器数量低位 (读取1个寄存器)
            0x00, 0x00  // CRC占位符
        };

        // 计算CRC
        uint16_t crc = calculateCRC(request.data(), request.size() - 2);
        request[request.size() - 2] = crc & 0xFF;
        request[request.size() - 1] = (crc >> 8) & 0xFF;

        // 发送请求并接收响应
        rs485_.send(request);
        std::vector<uint8_t> response = rs485_.receive(7); // 7字节响应

        // 验证响应
        validateResponse(response, 0x03, 2);

        // 解析采样率
        uint16_t rate = (response[3] << 8) | response[4];
        return static_cast<SamplingRate>(rate);
    }

    // 设置采样率
    void setSamplingRate(SamplingRate rate) {
        uint16_t rateValue = static_cast<uint16_t>(rate);

        std::vector<uint8_t> request = {
            deviceAddress_,
            0x06,       // 功能码: 写单个寄存器
            0x0A,       // 起始地址高位
            0x05,       // 起始地址低位
            static_cast<uint8_t>((rateValue >> 8) & 0xFF),  // 数据高位
            static_cast<uint8_t>(rateValue & 0xFF),         // 数据低位
            0x00, 0x00  // CRC占位符
        };

        // 计算CRC
        uint16_t crc = calculateCRC(request.data(), request.size() - 2);
        request[request.size() - 2] = crc & 0xFF;
        request[request.size() - 1] = (crc >> 8) & 0xFF;

        // 发送请求并接收响应
        rs485_.send(request);
        std::vector<uint8_t> response = rs485_.receive(8); // 8字节响应

        // 验证响应 (应该回显请求)
        if (response.size() != 8 ||
            response[0] != request[0] ||
            response[1] != request[1] ||
            response[2] != request[2] ||
            response[3] != request[3] ||
            response[4] != request[4] ||
            response[5] != request[5]) {
            throw std::runtime_error("Failed to set sampling rate");
        }
    }

    // 执行校准步骤
    void performCalibration(CalibrationCommand command) {
        std::vector<uint8_t> request = {
            deviceAddress_,
            0x06,       // 功能码: 写单个寄存器
            0x2A,       // 起始地址高位
            0x02,       // 起始地址低位
            0x00,       // 数据高位
            static_cast<uint8_t>(command),  // 数据低位
            0x00, 0x00  // CRC占位符
        };

        // 计算CRC
        uint16_t crc = calculateCRC(request.data(), request.size() - 2);
        request[request.size() - 2] = crc & 0xFF;
        request[request.size() - 1] = (crc >> 8) & 0xFF;

        // 发送请求并接收响应
        rs485_.send(request);
        std::vector<uint8_t> response = rs485_.receive(8); // 8字节响应

        // 验证响应 (应该回显请求)
        if (response.size() != 8 ||
            response[0] != request[0] ||
            response[1] != request[1] ||
            response[2] != request[2] ||
            response[3] != request[3] ||
            response[4] != request[4] ||
            response[5] != request[5]) {
            throw std::runtime_error("Calibration command failed");
        }
    }

    // 更改串口配置
    void changeSerialConfig(uint8_t baudRateCode, uint8_t parity, uint8_t newAddress) {
        std::vector<uint8_t> request = {
            deviceAddress_,
            0x06,       // 功能码: 写单个寄存器
            0xBD,       // 起始地址高位
            parity,       // 起始地址低位 (校验位设置) 默认0x00无校验
            baudRateCode,  // 波特率代码
            newAddress,    // 新地址
            0x00, 0x00  // CRC占位符
        };

        // 计算CRC
        uint16_t crc = calculateCRC(request.data(), request.size() - 2);
        request[request.size() - 2] = crc & 0xFF;
        request[request.size() - 1] = (crc >> 8) & 0xFF;

        // 发送请求并接收响应
        rs485_.send(request);
        std::vector<uint8_t> response = rs485_.receive(8); // 8字节响应

        // 验证响应 (应该回显请求)
        if (response.size() != 8 ||
            response[0] != request[0] ||
            response[1] != request[1] ||
            response[2] != request[2] ||
            response[3] != request[3] ||
            response[4] != request[4] ||
            response[5] != request[5]) {
            throw std::runtime_error("Failed to change serial configuration");
        }
    }

    // 读取设备信息
    std::string readDeviceInfo() {
        std::vector<uint8_t> request = {
            deviceAddress_,
            0x03,       // 功能码: 读取保持寄存器
            0xBB,       // 起始地址高位
            0x00,       // 起始地址低位
            0x00,       // 寄存器数量高位
            0x08,       // 寄存器数量长度低位 
            0x00, 0x00  // CRC占位符
        };

        // 计算CRC
        uint16_t crc = calculateCRC(request.data(), request.size() - 2);
        request[request.size() - 2] = crc & 0xFF;
        request[request.size() - 1] = (crc >> 8) & 0xFF;

        // 发送请求并接收响应
        rs485_.send(request);
        //std::vector<uint8_t> response = rs485_.receive(19); // 19字节响应 (1+1+1+16)
        std::vector<uint8_t> response = rs485_.receive(21); // 19字节响应 (1+1+1+16)
        // 验证响应
        validateResponse(response, 0x03, 16);

        // 解析设备信息 (ASCII字符串)
        std::string info;
        for (size_t i = 3; i < 19; ++i) {
            if (response[i] != 0) {  // 跳过空字符
                info += static_cast<char>(response[i]);
            }
        }

        return info;
    }
    // 验证Modbus响应
    void validateResponse(const std::vector<uint8_t>& response, uint8_t expectedFunctionCode, uint8_t expectedByteCount) {
        if (response.size() < 5) {
            throw std::runtime_error("Invalid response length");
        }

        // 检查地址和功能码
        if (response[0] != deviceAddress_) {
            throw std::runtime_error("Device address mismatch in response");
        }

        // 检查错误响应
        if (response[1] & 0x80) {
            std::string errorMsg = "Modbus error: ";
            switch (response[2]) {
            case 0x01: errorMsg += "Illegal function"; break;
            case 0x02: errorMsg += "Illegal data address"; break;
            case 0x03: errorMsg += "Illegal data value"; break;
            case 0x04: errorMsg += "Slave device failure"; break;
            default: errorMsg += "Unknown error code"; break;
            }
            throw std::runtime_error(errorMsg);
        }

        // 检查功能码
        if (response[1] != expectedFunctionCode) {
            throw std::runtime_error("Unexpected function code in response");
        }

        // 检查字节计数
        if (expectedFunctionCode == 0x03 && response[2] != expectedByteCount) {
            throw std::runtime_error("Unexpected byte count in response");
        }

        // 验证CRC
        uint16_t receivedCRC = (response[response.size() - 1] << 8) | response[response.size() - 2];
        //uint16_t receivedCRC = response[response.size() - 2] | (response[response.size() - 1] << 8);

        uint16_t calculatedCRC = calculateCRC(response.data(), response.size() - 2);
        if (receivedCRC != calculatedCRC) {
            throw std::runtime_error("CRC check failed in response");
        }
    }

    // CRC16计算 (Modbus)
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
private:
    RS485Manager& rs485_;
    uint8_t deviceAddress_;

    // 解析角度值 (两个字节转换为有符号整数，然后除以100)
    double parseAngle(uint8_t highByte, uint8_t lowByte) {
        int16_t value = static_cast<int16_t>((highByte << 8) | lowByte);
        return static_cast<double>(value) / 100.0;
    }

 
};



#endif // DUALAXISSENSORPARSER_H