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
