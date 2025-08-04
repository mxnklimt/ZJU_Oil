#include"CollectData.h"
#include"data/Data.h"

void initializedualAxisParsers() {
    //遍历设备地址列表
    for (uint8_t addr : dualAxisDeviceAddresses) {
        //std::make_unique智能指针构造函数
        //为每个设备地址创建一个DualAxisSensorParser实例，并且初始化地址和串口对象
        dualAxisParsers[addr] = std::make_unique<DualAxisSensorParser>(serialDualAxis, addr);
        // 设置采样率
        dualAxisParsers[addr]->setSamplingRate(DualAxisSensorParser::SamplingRate::ADS_1_Hz);
    }
}

