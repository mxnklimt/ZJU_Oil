#pragma once
//Data.h
#include <vector>
#include <mutex>
#include <deque>
#include <thread>
#include <atomic>
#include"ADXL355/ADXL355Parser.h"
#include"RS485/RS485Manager.h"



//extern声明全局变量
//ADXL355
extern const size_t MAX_POINTS;
extern std::mutex ADXL355Mutex;
extern std::atomic<bool> collectingADXL355;
extern RS485Manager serialManager;
extern ADXL355Parser parser;
extern std::thread adxl355Thread;       //线程
extern class ADXL355Data adxl355Data;         //数据结构


//JY61P
extern char s_cDataUpdate;
extern int iComPort;
extern int iBaud;
extern int iAddress;

class ADXL355Data
{
public:
	std::deque<ADXL355Parser::AccelerationData> dataQue ;//使用别的类里的结构体要加上作用域
};




