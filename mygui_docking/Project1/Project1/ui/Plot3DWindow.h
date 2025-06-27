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
	static void ExcelGetData();
	static std::vector<std::string> listAvailableSerialPorts();
	std::vector<float> dValues, eValues;
	std::vector<std::tm> times;
};