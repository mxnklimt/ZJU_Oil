// 
// Parser.h
#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring>

class BSQJNParser {
public:
    explicit BSQJNParser(uint8_t deviceAddr = 0x01) : addr(deviceAddr) {}

    void setAddress(uint8_t newAddr) { addr = newAddr; }
    uint8_t getAddress() const { return addr; }

    // ====== 指令生成 ======

    // 读取单通道浮点值 (地址: 200 + (ch-1)*2)
    std::vector<uint8_t> makeReadFloatCmd(int channel) const {
        uint16_t startReg = 200 + (channel - 1) * 2;
        return makeReadCmd(startReg, 2);
    }

    // 读取所有 4 通道浮点值
    std::vector<uint8_t> makeReadAllFloatCmd() const {
        return makeReadCmd(200, 8);
    }

    // 读取单通道长整型值 (地址: 500 + (ch-1)*2)
    std::vector<uint8_t> makeReadLongCmd(int channel) const {
        uint16_t startReg = 500 + (channel - 1) * 2;
        return makeReadCmd(startReg, 2);
    }

    // 读取所有 4 通道长整型值
    std::vector<uint8_t> makeReadAllLongCmd() const {
        return makeReadCmd(500, 8);
    }

    // 零点校准（功能码 0x05），ch=1~4 单通道，ch=0 所有通道
    std::vector<uint8_t> makeZeroCalibrationCmd(int channel) const {
        uint16_t regAddr = (channel == 0) ? 0x0004 : (channel - 1);
        return makeWriteSingleCoilCmd(regAddr, 0xFF00);
    }

    // 清零操作（功能码 0x05），ch=1~4 单通道，ch=0 所有通道
    std::vector<uint8_t> makeClearCmd(int channel) const {
        uint16_t regAddr = (channel == 0) ? 0x0068 : (0x0064 + (channel - 1));
        return makeWriteSingleCoilCmd(regAddr, 0xFF00);
    }

    // 满度校准（功能码 0x10），传入系数（0.00010 ~ 9.99999）
    std::vector<uint8_t> makeFullScaleCalCmd(int channel, double coeff) const {
        if (coeff < 0.00010 || coeff > 9.99999) {
            throw std::runtime_error("校准系数超出范围");
        }
        uint32_t val = static_cast<uint32_t>(coeff * 100000 + 0.5);
        uint16_t startReg = 800 + (channel - 1);
        return makeWriteLongCmd(startReg, val);
    }

    // 小数点位置设置（功能码 0x10），pos=0~4
    std::vector<uint8_t> makeDecimalPosCmd(uint8_t pos) const {
        if (pos > 4) throw std::runtime_error("小数点位置无效");
        return makeWriteLongCmd(900, pos);
    }

    // ====== 数据解析 ======

    //// 解析浮点数（Modbus 返回 4 字节 IEEE754，按设备字节序）
    //static float parseFloat(const std::vector<uint8_t>& data, size_t index) {
    //    if (data.size() < index + 4) throw std::runtime_error("浮点数据不足");
    //    float val;
    //    uint8_t bytes[4] = { data[index], data[index + 1], data[index + 2], data[index + 3] };
    //    std::memcpy(&val, bytes, sizeof(float));
    //    return val;
    

    //static float parseFloat(const std::vector<uint8_t>& data, size_t index) {
    //    if (data.size() < index + 4) throw std::runtime_error("浮点数据不足");

    //    // 强制大端序处理（文档4.2.2节要求）
    //    union {
    //        uint32_t i;
    //        float f;
    //    } converter;

    //    converter.i = (static_cast<uint32_t>(data[index]) << 24) |
    //        (static_cast<uint32_t>(data[index + 1]) << 16) |
    //        (static_cast<uint32_t>(data[index + 2]) << 8) |
    //        static_cast<uint32_t>(data[index + 3]);

    //    // 数据有效性检查（文档4.2节输入范围）
    //    if (abs(converter.f) > 1e6) {  // 假设最大量程1,000,000
    //        throw std::runtime_error("超出量程的浮点数值");
    //    }
    //    return converter.f;
    //}
    static float parseFloat(const std::vector<uint8_t>& data, size_t index) {
        if (data.size() < index + 4) throw std::runtime_error("浮点数据不足");

        // 强制大端序处理（文档4.2.2节要求）
        uint32_t raw = (static_cast<uint32_t>(data[index]) << 24) |
            (static_cast<uint32_t>(data[index + 1]) << 16) |
            (static_cast<uint32_t>(data[index + 2]) << 8) |
            static_cast<uint32_t>(data[index + 3]);

        // 兼容性转换（避免联合体未定义行为）
        float value;
        memcpy(&value, &raw, sizeof(float));

        // 量程检查（文档4.2节）
        if (fabs(value) > 1e6f) throw std::runtime_error("超出量程");
        return value;
    }
    // 解析有符号长整型
    static int32_t parseLong(const std::vector<uint8_t>& data, size_t index) {
        if (data.size() < index + 4) throw std::runtime_error("整型数据不足");
        int32_t val = (static_cast<int32_t>(data[index]) << 24) |
            (static_cast<int32_t>(data[index + 1]) << 16) |
            (static_cast<int32_t>(data[index + 2]) << 8) |
            (static_cast<int32_t>(data[index + 3]));
        return val;
    }

    // CRC16
    static uint16_t calculateCRC(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
                else crc >>= 1;
            }
        }
        return crc;
    }

private:
    uint8_t addr;

    // 通用读命令
    std::vector<uint8_t> makeReadCmd(uint16_t startReg, uint16_t regCount) const {
        std::vector<uint8_t> cmd = {
            addr,
            0x03,
            static_cast<uint8_t>(startReg >> 8),
            static_cast<uint8_t>(startReg & 0xFF),
            static_cast<uint8_t>(regCount >> 8),
            static_cast<uint8_t>(regCount & 0xFF)
        };
        appendCRC(cmd);
        return cmd;
    }

    // 写单个线圈（0x05）
    std::vector<uint8_t> makeWriteSingleCoilCmd(uint16_t regAddr, uint16_t value) const {
        std::vector<uint8_t> cmd = {
            addr,
            0x05,
            static_cast<uint8_t>(regAddr >> 8),
            static_cast<uint8_t>(regAddr & 0xFF),
            static_cast<uint8_t>(value >> 8),
            static_cast<uint8_t>(value & 0xFF)
        };
        appendCRC(cmd);
        return cmd;
    }

    // 写 4 字节长整型（功能码 0x10，寄存器数=2）
    std::vector<uint8_t> makeWriteLongCmd(uint16_t startReg, uint32_t value) const {
        std::vector<uint8_t> cmd = {
            addr,
            0x10,
            static_cast<uint8_t>(startReg >> 8),
            static_cast<uint8_t>(startReg & 0xFF),
            0x00, 0x02, // 寄存器数量
            0x04,       // 字节数
            static_cast<uint8_t>((value >> 24) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF)
        };
        appendCRC(cmd);
        return cmd;
    }

    // 附加 CRC
    static void appendCRC(std::vector<uint8_t>& cmd) {
        uint16_t crc = calculateCRC(cmd.data(), cmd.size());
        cmd.push_back(static_cast<uint8_t>(crc & 0xFF)); // 低字节
        cmd.push_back(static_cast<uint8_t>(crc >> 8));   // 高字节
    }

};
