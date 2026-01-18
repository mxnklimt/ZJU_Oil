#pragma once
#include<vector>
#include<string>
#include"udp/FibreGratingManager.h"
class Application
{
private:
    
public:
    // 添加光纤光栅管理器作为成员变量
    inline static FibreGratingManager fibreGratingManager_;
	static void ShowWindow();
	static void CylinderPlots();
	static void ShowDualAxisSensor();
	static void ShowADXL355();
	static void ShowJY61P();
	static void JY61PInit(const std::string& portName);
	static std::vector<std::string> listAvailableSerialPorts();
	static void ShowSynchronizedCapture();
    static void ShowSynchronizedCapture_new();
	static void ShowExcel();
	static void ShowLaserSensor();
	static void ShowLaserSensor2();
    static void ShowLaserSensor3();
	static void ShowAMT();
	static void ShowBSQJN();
	static void ShowFibreGratingAnalyzerUI();
	std::vector<float> dValues, eValues;
	std::vector<std::tm> times;

    // 修正后的静态方法
    static void ShowFibreGratingAnalyzer() {
        fibreGratingManager_.start();
    }

    static void StopFibreGratingAnalyzer() {
        fibreGratingManager_.stop();
    }

    static bool IsFibreGratingAnalyzerRunning() {
        return fibreGratingManager_.isRunning();
    }

    // 静态显示统计信息方法
    static void DisplayFibreGratingStats() {
        if (fibreGratingManager_.isRunning()) {
            auto stats = fibreGratingManager_.getStatistics();
            std::cout << "光纤光栅分析器运行中 - "
                << "数据包: " << stats.totalPackets
                << ", 传感器: " << stats.totalSensors
                << ", 运行时间: " << stats.runTimeSeconds << "秒" << std::endl;
        }
        else {
            std::cout << "光纤光栅分析器未运行" << std::endl;
        }
    }
};
// 显示串口选择器
void ShowSerialPortSelector(const std::vector<std::string>& ports, int& selectedIndex, const char* label = u8"串口");
// 显示波特率选择器
void ShowBaudRateSelector(const char* const* baudRates, int baudRateCount, int& selectedIndex, const char* label = u8"波特率");
