// FiberGratingAnalyzer.cpp
#include "FiberGratingAnalyzer.h"
#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <sstream>
// PIMPL实现类
class FiberGratingAnalyzer::Impl {
public:
    Impl() : io_context(), socket(io_context), receiving(false) {}

    ~Impl() {
        stop();
    }

    bool setPort(int port) {
        try {
            boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), port);
            socket.open(endpoint.protocol());
            socket.bind(endpoint);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "端口设置失败: " << e.what() << std::endl;
            return false;
        }
    }

    bool startReceiving(const DataCallback& callback) {
        if (receiving) {
            return false;
        }

        receiving = true;
        receiveThread = std::thread([this, callback]() {
            receiveData(callback);
            });

        return true;
    }

    void stop() {
        receiving = false;
        if (receiveThread.joinable()) {
            receiveThread.join();
        }

        if (socket.is_open()) {
            socket.close();
        }
    }

    bool isReceiving() const {
        return receiving;
    }

private:
    void receiveData(const DataCallback& callback) {
        std::vector<uint8_t> buffer(2048); // 足够大的缓冲区

        while (receiving) {
            try {
                boost::asio::ip::udp::endpoint remote_endpoint;
                boost::system::error_code error;

                size_t length = socket.receive_from(
                    boost::asio::buffer(buffer), remote_endpoint, 0, error);

                if (error && error != boost::asio::error::message_size) {
                    if (receiving) { // 只有在仍然接收时才报告错误
                        std::cerr << "接收错误: " << error.message() << std::endl;
                    }
                    continue;
                }

                // 解析接收到的数据
                auto sensorData = parseData(buffer.data(), length);
                if (!sensorData.empty() && callback) {
                    callback(sensorData);
                }
            }
            catch (const std::exception& e) {
                if (receiving) { // 只有在仍然接收时才报告错误
                    std::cerr << "接收异常: " << e.what() << std::endl;
                }
            }
        }
    }

    //std::vector<SensorData> parseData(const uint8_t* data, size_t length) {
    //    std::vector<SensorData> result;

    //    // 检查最小长度和数据头
    //    if (length < 5) return result; // 至少需要5字节(4字节头+1字节通道数)

    //    // 检查数据头 "FBGV" 或 "FBGW"
    //    if (data[0] == 0x46 && data[1] == 0x42 && data[2] == 0x47 &&
    //        (data[3] == 0x56 || data[3] == 0x57)) {

    //        bool hasPhysicalValue = (data[3] == 0x56); // 'V'表示有物理量
    //        uint8_t channelCount = data[4]; // 通道数

    //        size_t pos = 5; // 当前位置

    //        for (uint8_t ch = 0; ch < channelCount && pos < length; ch++) {
    //            if (pos >= length) break;

    //            uint8_t sensorCount = data[pos++]; // 当前通道的传感器数量

    //            for (uint8_t s = 0; s < sensorCount && pos < length; s++) {
    //                SensorData sensor;
    //                sensor.channel = ch + 1; // 通道从1开始
    //                sensor.sequence = s + 1; // 序列从1开始
    //                sensor.hasPhysicalValue = hasPhysicalValue;

    //                // 读取波长 (2字节，大端)
    //                if (pos + 1 >= length) break;
    //                uint16_t wavelengthRaw = (data[pos] << 8) | data[pos + 1];
    //                pos += 2;
    //                sensor.wavelength = wavelengthRaw / 1000.0 + 1520.0; // 转换为nm

    //                // 如果有物理量，读取物理量 (4字节，大端)
    //                if (hasPhysicalValue) {
    //                    if (pos + 3 >= length) break;

    //                    // 将4字节大端数据转换为float
    //                    uint32_t physicalRaw = (data[pos] << 24) |
    //                        (data[pos + 1] << 16) |
    //                        (data[pos + 2] << 8) |
    //                        data[pos + 3];
    //                    pos += 4;

    //                    // 重新解释字节为float
    //                    sensor.physicalValue = *reinterpret_cast<float*>(&physicalRaw);
    //                }

    //                result.push_back(sensor);
    //            }
    //        }
    //    }

    //    return result;
    //}

    //std::vector<SensorData> parseData(const uint8_t* data, size_t length) {
    //    std::vector<SensorData> result;

    //    // 检查最小长度和数据头
    //    if (length < 5) return result; // 至少需要5字节(4字节头+1字节通道数)

    //    // 检查数据头 "FBGV" 或 "FBGW"
    //    if (data[0] == 0x46 && data[1] == 0x42 && data[2] == 0x47 &&
    //        (data[3] == 0x56 || data[3] == 0x57)) {

    //        bool hasPhysicalValue = (data[3] == 0x56); // 'V'表示有物理量
    //        uint8_t channelCount = data[4]; // 通道数

    //        size_t pos = 5; // 当前位置

    //        for (uint8_t ch = 0; ch < channelCount && pos < length; ch++) {
    //            if (pos >= length) break;

    //            uint8_t sensorCount = data[pos++]; // 当前通道的传感器数量

    //            for (uint8_t s = 0; s < sensorCount && pos < length; s++) {
    //                SensorData sensor;
    //                sensor.channel = ch + 1; // 通道从1开始
    //                sensor.sequence = s + 1; // 序列从1开始
    //                sensor.hasPhysicalValue = hasPhysicalValue;

    //                // 读取波长 (2字节，大端)
    //                if (pos + 1 >= length) break;
    //                uint16_t wavelengthRaw = (static_cast<uint32_t>(data[pos]) << 8) |
    //                    (static_cast<uint32_t>(data[pos + 1])); // 添加类型转换避免潜在问题
    //                pos += 2;
    //                sensor.wavelength = wavelengthRaw / 1000.0 + 1520.0; // 转换为nm

    //                // 如果有物理量，读取物理量 (4字节，大端)
    //                if (hasPhysicalValue) {
    //                    if (pos + 3 >= length) break;

    //                    // 1. 将4字节大端数据组合成uint32_t
    //                    uint32_t physicalRaw = (static_cast<uint32_t>(data[pos]) << 24) |
    //                        (static_cast<uint32_t>(data[pos + 1]) << 16) |
    //                        (static_cast<uint32_t>(data[pos + 2]) << 8) |
    //                        (static_cast<uint32_t>(data[pos + 3]));
    //                    pos += 4;

    //                    // 2. 安全的方式：使用 memcpy 进行类型转换 [1,6,7](@ref)
    //                    // 或者使用 C++20 的 std::bit_cast (如果编译器支持)
    //                    std::memcpy(&sensor.physicalValue, &physicalRaw, sizeof(float));

    //                    // 注意：如果数据流中的浮点数字节序与主机不同，此处可能需要额外的字节序转换
    //                    // 例如，如果数据是大端序，而主机是小端序，则需要交换字节序
    //                    // sensor.physicalValue = swapFloatEndian(sensor.physicalValue); // 需要自定义此函数
    //                }

    //                result.push_back(sensor);
    //            }
    //        }
    //    }
    //    return result;

    //}
//std::vector<SensorData> parseData(const uint8_t* data, size_t length) {
//    std::vector<SensorData> result;
//
//    // 检查最小长度和数据头
//    if (length < 5) return result; // 至少需要5字节(4字节头+1字节通道数)
//
//    // 检查数据头 "FBGV" 或 "FBGW"
//    if (data[0] == 0x46 && data[1] == 0x42 && data[2] == 0x47 &&
//        (data[3] == 0x56 || data[3] == 0x57)) {
//
//        bool hasPhysicalValue = (data[3] == 0x56); // 'V'表示有物理量
//        uint8_t channelCount = data[4]; // 通道数
//
//        size_t pos = 5; // 当前位置
//        std::cout << "=== 开始解析数据包 ===" << std::endl;
//        std::cout << "数据头: FBG" << (char)data[3] << std::endl;
//        std::cout << "通道数: " << (int)channelCount << std::endl;
//        std::cout << "数据总长度: " << length << " 字节" << std::endl;
//
//        for (uint8_t ch = 0; ch < channelCount && pos < length; ch++) {
//            if (pos >= length) break;
//
//            uint8_t sensorCount = data[pos++]; // 当前通道的传感器数量
//            std::cout << "通道 " << (ch + 1) << " 传感器数量: " << (int)sensorCount << std::endl;
//
//            for (uint8_t s = 0; s < sensorCount && pos < length; s++) {
//                SensorData sensor;
//                sensor.channel = ch + 1; // 通道从1开始
//                sensor.sequence = s + 1; // 序列从1开始
//                sensor.hasPhysicalValue = hasPhysicalValue;
//
//                // 读取波长 (2字节，大端)
//                if (pos + 1 >= length) break;
//
//                // 保存原始字节用于调试输出
//                uint8_t byte1 = data[pos];
//                uint8_t byte2 = data[pos + 1];
//
//                uint16_t wavelengthRaw = (static_cast<uint32_t>(byte1) << 8) |
//                    (static_cast<uint32_t>(byte2));
//                pos += 2;
//                sensor.wavelength = wavelengthRaw / 1000.0 + 1520.0; // 转换为nm
//
//                // 打印波长原始数据详细信息[1,3](@ref)
//                std::cout << "通道 " << std::setw(2) << (int)sensor.channel
//                    << " 传感器 " << std::setw(2) << (int)sensor.sequence
//                    << " - 原始字节: 0x" << std::hex << std::uppercase
//                    << std::setw(2) << std::setfill('0') << (int)byte1
//                    << " " << std::setw(2) << std::setfill('0') << (int)byte2
//                    << std::dec << std::setfill(' ')  // 恢复十进制输出[1](@ref)
//                    << " | 原始值: " << std::setw(5) << wavelengthRaw
//                    << " | 计算波长: " << std::fixed << std::setprecision(3)
//                    << sensor.wavelength << " nm" << std::endl;
//
//                // 如果有物理量，读取物理量 (4字节，大端)
//                if (hasPhysicalValue) {
//                    if (pos + 3 >= length) break;
//
//                    // 1. 将4字节大端数据组合成uint32_t
//                    uint32_t physicalRaw = (static_cast<uint32_t>(data[pos]) << 24) |
//                        (static_cast<uint32_t>(data[pos + 1]) << 16) |
//                        (static_cast<uint32_t>(data[pos + 2]) << 8) |
//                        (static_cast<uint32_t>(data[pos + 3]));
//                    pos += 4;
//
//                    // 2. 安全的方式：使用 memcpy 进行类型转换
//                    std::memcpy(&sensor.physicalValue, &physicalRaw, sizeof(float));
//
//                    // 打印物理量原始数据[3](@ref)
//                    std::cout << "               物理量原始字节: 0x" << std::hex << std::uppercase
//                        << std::setw(2) << std::setfill('0') << (int)data[pos - 4] << " "
//                        << std::setw(2) << std::setfill('0') << (int)data[pos - 3] << " "
//                        << std::setw(2) << std::setfill('0') << (int)data[pos - 2] << " "
//                        << std::setw(2) << std::setfill('0') << (int)data[pos - 1]
//                        << std::dec << std::setfill(' ')
//                        << " | 物理量值: " << std::setprecision(4)
//                        << sensor.physicalValue << std::endl;
//                }
//
//                result.push_back(sensor);
//            }
//        }
//
//        std::cout << "=== 数据包解析完成，共解析 " << result.size() << " 个传感器 ===" << std::endl;
//    }
//    else {
//        std::cout << "!!! 无效的数据头 !!!" << std::endl;
//    }
//
//    return result;
//}
std::vector<SensorData> parseData(const uint8_t* data, size_t length) {
    std::vector<SensorData> result;

    // 检查最小长度和数据头
    if (length < 5) return result; // 至少需要5字节(4字节头+1字节通道数)

    // 检查数据头 "FBGV" 或 "FBGW"
    if (data[0] == 0x46 && data[1] == 0x42 && data[2] == 0x47 &&
        (data[3] == 0x56 || data[3] == 0x57)) {

        bool hasPhysicalValue = (data[3] == 0x56); // 'V'表示有物理量
        uint8_t channelCount = data[4]; // 通道数

        size_t pos = 5; // 当前位置
        std::cout << "=== 开始解析数据包 ===" << std::endl;
        std::cout << "数据头: FBG" << (char)data[3] << std::endl;
        std::cout << "通道数: " << (int)channelCount << std::endl;
        std::cout << "数据总长度: " << length << " 字节" << std::endl;
        std::cout << "字节序: 小端模式 (低位在前，高位在后)" << std::endl;

        for (uint8_t ch = 0; ch < channelCount && pos < length; ch++) {
            if (pos >= length) break;

            uint8_t sensorCount = data[pos++]; // 当前通道的传感器数量
            std::cout << "通道 " << (ch + 1) << " 传感器数量: " << (int)sensorCount << std::endl;

            for (uint8_t s = 0; s < sensorCount && pos < length; s++) {
                SensorData sensor;
                sensor.channel = ch + 1; // 通道从1开始
                sensor.sequence = s + 1; // 序列从1开始
                sensor.hasPhysicalValue = hasPhysicalValue;

                // 读取波长 (2字节，小端字节序 - 修正后)
                if (pos + 1 >= length) break;

                // 保存原始字节用于调试输出
                uint8_t lowByte = data[pos];     // 低位字节（第一个字节）
                uint8_t highByte = data[pos + 1]; // 高位字节（第二个字节）

                // 修正：按照小端字节序解析，低位在前，高位在后
                uint16_t wavelengthRaw = (static_cast<uint32_t>(highByte) << 8) |
                    (static_cast<uint32_t>(lowByte));
                pos += 2;
                sensor.wavelength = wavelengthRaw / 1000.0 + 1520.0; // 转换为nm

                // 打印波长原始数据详细信息
                std::cout << "通道 " << std::setw(2) << (int)sensor.channel
                    << " 传感器 " << std::setw(2) << (int)sensor.sequence
                    << " - 原始字节: 0x" << std::hex << std::uppercase
                    << std::setw(2) << std::setfill('0') << (int)lowByte
                    << " " << std::setw(2) << std::setfill('0') << (int)highByte
                    << std::dec << std::setfill(' ')  // 恢复十进制输出
                    << " | 字节序: 低(" << std::setw(3) << (int)lowByte
                    << ") 高(" << std::setw(3) << (int)highByte << ")"
                    << " | 原始值: " << std::setw(5) << wavelengthRaw
                    << " | 计算波长: " << std::fixed << std::setprecision(3)
                    << sensor.wavelength << " nm" << std::endl;

                // 如果有物理量，读取物理量 (4字节，需要确认字节序)
                if (hasPhysicalValue) {
                    if (pos + 3 >= length) break;

                    // 注意：这里需要确认物理量数据的字节序是否与波长一致
                    // 当前假设也是小端字节序
                    uint32_t physicalRaw = (static_cast<uint32_t>(data[pos + 3]) << 24) |
                        (static_cast<uint32_t>(data[pos + 2]) << 16) |
                        (static_cast<uint32_t>(data[pos + 1]) << 8) |
                        (static_cast<uint32_t>(data[pos]));
                    pos += 4;

                    // 使用 memcpy 进行类型转换
                    std::memcpy(&sensor.physicalValue, &physicalRaw, sizeof(float));

                    // 打印物理量原始数据
                    std::cout << "               物理量原始字节: 0x" << std::hex << std::uppercase
                        << std::setw(2) << std::setfill('0') << (int)data[pos - 4] << " "
                        << std::setw(2) << std::setfill('0') << (int)data[pos - 3] << " "
                        << std::setw(2) << std::setfill('0') << (int)data[pos - 2] << " "
                        << std::setw(2) << std::setfill('0') << (int)data[pos - 1]
                        << std::dec << std::setfill(' ')
                        << " | 物理量值: " << std::setprecision(4)
                        << sensor.physicalValue << std::endl;
                }

                result.push_back(sensor);
            }
        }

        std::cout << "=== 数据包解析完成，共解析 " << result.size() << " 个传感器 ===" << std::endl;
    }
    else {
        std::cout << "!!! 无效的数据头 !!!" << std::endl;
    }

    return result;
}

    boost::asio::io_context io_context;
    boost::asio::ip::udp::socket socket;
    std::thread receiveThread;
    std::atomic<bool> receiving;
};

// FiberGratingAnalyzer 类实现
FiberGratingAnalyzer::FiberGratingAnalyzer() : pimpl(new Impl()) {}

FiberGratingAnalyzer::~FiberGratingAnalyzer() {
    delete pimpl;
}

bool FiberGratingAnalyzer::setPort(int port) {
    return pimpl->setPort(port);
}

bool FiberGratingAnalyzer::startReceiving(const DataCallback& callback) {
    return pimpl->startReceiving(callback);
}

void FiberGratingAnalyzer::stopReceiving() {
    pimpl->stop();
}

bool FiberGratingAnalyzer::isReceiving() const {
    return pimpl->isReceiving();
}