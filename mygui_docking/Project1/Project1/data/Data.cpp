//Data.cpp

#include"data/Data.h"  


//cpp中定义全局变量

//adxl355数据
const size_t MAX_POINTS = 500;  
RS485Manager serialManager;
ADXL355Parser parser;
std::mutex ADXL355Mutex;
std::atomic<bool> collectingADXL355 = false;
std::thread adxl355Thread;       //线程
ADXL355Data adxl355Data;         //数据结构


//JY61P数据
char s_cDataUpdate = 0;
int iComPort = 7;
int iBaud = 9600;
int iAddress = 0x0D;
std::atomic<bool> collectingJY61P = false;
std::thread JY61PThread;       //线程
std::mutex JY61PMutex;
std::deque<JY61PData> JY61PData_dataQue; // 存储数据的队列
JY61PData jy61pData;