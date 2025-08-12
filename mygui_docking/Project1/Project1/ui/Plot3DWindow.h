#pragma once
#include<vector>
#include<string>
class Application
{
public:
	static void ShowWindow();
	static void CylinderPlots();
	static void ShowDualAxisSensor();
	static void ShowADXL355();
	static void ShowJY61P();
	static void JY61PInit(const std::string& portName);
	static std::vector<std::string> listAvailableSerialPorts();
	static void ShowSynchronizedCapture();
	static void ShowExcel();
	static void ShowLaserSensor();
	static void ShowLaserSensor2();
	static void ShowAMT();
	std::vector<float> dValues, eValues;
	std::vector<std::tm> times;
};
// 显示串口选择器
void ShowSerialPortSelector(const std::vector<std::string>& ports, int& selectedIndex, const char* label = u8"串口");
// 显示波特率选择器
void ShowBaudRateSelector(const char* const* baudRates, int baudRateCount, int& selectedIndex, const char* label = u8"波特率");
