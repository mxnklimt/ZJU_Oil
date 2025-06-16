#pragma once
#include<vector>
class Application
{
public:
	static void ShowWindow();
	static void CylinderPlots();
	static void ShowWindow2();
	std::vector<float> dValues, eValues;
	std::vector<std::tm> times;
};