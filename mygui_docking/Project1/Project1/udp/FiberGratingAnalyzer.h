// FiberGratingAnalyzer.h
#ifndef FIBER_GRATING_ANALYZER_H
#define FIBER_GRATING_ANALYZER_H

#include <string>
#include <functional>
#include <vector>
#include <cstdint>

// 前向声明
namespace boost::asio {
    class io_context;
   // class ip::udp;
}
namespace boost::asio::ip
{
    class udp;
}
class FiberGratingAnalyzer {
public:
    // 传感器数据结构
    struct SensorData {
        uint16_t channel;      // 通道号
        uint8_t sequence;       // 序列号
        double wavelength;      // 波长值 (nm)
        float physicalValue;    // 物理量值 (如果有)
        bool hasPhysicalValue; // 是否有物理量值
    };

    // 回调函数类型定义
    using DataCallback = std::function<void(const std::vector<SensorData>&)>;

    // 构造函数和析构函数
    FiberGratingAnalyzer();
    ~FiberGratingAnalyzer();

    // 设置接收数据的端口
    bool setPort(int port);

    // 开始接收数据
    bool startReceiving(const DataCallback& callback);

    // 停止接收数据
    void stopReceiving();

    // 检查是否正在接收数据
    bool isReceiving() const;

private:
    // 实现细节的PIMPL模式
    class Impl;
    Impl* pimpl;
};

#endif // FIBER_GRATING_ANALYZER_H