//Data.cpp

#include"data/Data.h"  


//cpp中定义全局变量
const size_t MAX_POINTS = 500;  
RS485Manager serialManager;
ADXL355Parser parser;
std::mutex ADXL355Mutex;
std::atomic<bool> collectingADXL355 = false;
std::thread adxl355Thread;       //线程
ADXL355Data adxl355Data;         //数据结构