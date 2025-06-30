#pragma once
#include<vector>
#include<string>
class Application
{
public:
	static void ShowWindow();
	static void CylinderPlots();
	static void ShowWindow2();
	static void ShowADXL355();
	static void ShowJY61P();
	static void ExcelGetData();
	static void JY61PInit(const std::string& portName);
	static std::vector<std::string> listAvailableSerialPorts();
	std::vector<float> dValues, eValues;
	std::vector<std::tm> times;
};