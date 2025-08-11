//RS485Manager.h
#pragma once
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <stdexcept>

class RS485Manager {
public:
    RS485Manager() : hSerial(INVALID_HANDLE_VALUE) {}

    ~RS485Manager() {
        close();
    }

    // 打开串口
    void open(const std::string& portName, DWORD baudRate = 9600) {
        close(); // 确保之前打开的端口已关闭

        // 转换端口名格式
        std::wstring widePortName(portName.begin(), portName.end());
        widePortName = L"\\\\.\\" + widePortName; // 支持COM10以上端口

        hSerial = CreateFile(
            widePortName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hSerial == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("无法打开串口: " + portName);
        }

        // 配置串口参数
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            close();
            throw std::runtime_error("无法获取串口参数");
        }

        dcbSerialParams.BaudRate = baudRate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            close();
            throw std::runtime_error("无法设置串口参数");
        }

        // 配置超时
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(hSerial, &timeouts)) {
            close();
            throw std::runtime_error("无法设置串口超时");
        }

        // 清空缓冲区
        PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    }

    // 关闭串口
    void close() {
        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
        }
    }

    // 检查串口是否打开
    bool isOpen() const {
        return hSerial != INVALID_HANDLE_VALUE;
    }

    // 发送数据
    void send(const std::vector<uint8_t>& data) {
        if (!isOpen()) {
            throw std::runtime_error("串口未打开");
        }

        DWORD bytesWritten;
        if (!WriteFile(hSerial, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL)) {
            throw std::runtime_error("发送数据失败");
        }

        if (bytesWritten != data.size()) {
            throw std::runtime_error("发送数据不完整");
        }
    }

    // 计算Modbus CRC16，data为指针，length为字节数
    uint16_t calculateCRC(const uint8_t* data, size_t length) {
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
    // rs485Manager类里新增这个成员函数
    std::vector<uint8_t> receiveLaser(DWORD timeoutMs = 1000) {
        std::vector<uint8_t> buffer;
        auto startTime = std::chrono::steady_clock::now();

        while (true) {
            // 每次读1字节，避免跨包混乱
            std::vector<uint8_t> tmp;
            try {
                tmp = this->receive(1, 100);  // 调用已有的receive读1字节，100ms超时
            }
            catch (const std::exception& e) {
                // 读超时或失败，判断总超时
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
                if (elapsed >= timeoutMs) {
                    throw std::runtime_error("激光传感器接收超时");
                }
                else {
                    continue;  // 没超时，继续读
                }
            }

            buffer.push_back(tmp[0]);

            // 等待缓冲区至少3字节判断包头
            while (buffer.size() >= 3) {
                uint8_t addr = buffer[0];
                uint8_t func = buffer[1];
                uint8_t byteCount = buffer[2];

                // 这里的地址范围根据你的设备调整，比如1~0x0B
                if ((addr < 1 || addr > 0x0B) || func != 0x04 || byteCount != 0x04) {
                    // 包头不合法，丢弃第一个字节继续找
                    buffer.erase(buffer.begin());
                    if (buffer.size() < 3) break;
                    continue;
                }

                // 包头合法，检查是否收到完整9字节
                if (buffer.size() < 9) break;

                // 校验CRC
                uint16_t recvCRC = (buffer[8] << 8) | buffer[7];
                uint16_t calcCRC = calculateCRC(buffer.data(), 7);
                if (recvCRC == calcCRC) {
                    // 找到完整包，返回前9字节
                    std::vector<uint8_t> packet(buffer.begin(), buffer.begin() + 9);
                    buffer.erase(buffer.begin(), buffer.begin() + 9);
                    return packet;
                }
                else {
                    // CRC错误，丢弃第一个字节继续找包头
                    buffer.erase(buffer.begin());
                    if (buffer.size() < 3) break;
                }
            }

            // 判断是否超时
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            if (elapsed >= timeoutMs) {
                throw std::runtime_error("激光传感器接收超时");
            }
        }
    }

    // 接收数据
    std::vector<uint8_t> receive(size_t expectedSize, DWORD timeoutMs = 1000) {
        if (!isOpen()) {
            throw std::runtime_error("串口未打开");
        }

        std::vector<uint8_t> buffer(expectedSize);
        DWORD bytesRead = 0;
        DWORD totalBytesRead = 0;

        // 设置接收超时
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutConstant = timeoutMs;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        SetCommTimeouts(hSerial, &timeouts);

        while (totalBytesRead < expectedSize) {
            if (!ReadFile(hSerial, buffer.data() + totalBytesRead,
                static_cast<DWORD>(expectedSize - totalBytesRead), &bytesRead, NULL)) {
                throw std::runtime_error("接收数据失败");
            }

            if (bytesRead == 0) {
                throw std::runtime_error("接收超时");
            }

            totalBytesRead += bytesRead;
        }

        return buffer;
    }

    // 接收ADXL355加速度数据
    std::vector<uint8_t> receiveADXL355Response() {
        // ADXL355响应格式: 17字节
        return receive(17);
    }

private:
    HANDLE hSerial;
};
