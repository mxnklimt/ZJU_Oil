//Data.cpp
#include<map>
#include"data/Data.h"  
#include<unordered_map>
#include"DualAxisSensor/DualAxisSensorParser.h"
#include<unordered_set>
//cpp中定义全局变量

std::mutex dataMutex;
//adxl355数据
const size_t MAX_POINTS = 500;  
RS485Manager serialManager;
ADXL355Parser parser;
std::mutex ADXL355Mutex;
std::atomic<bool> collectingADXL355 = false;
std::thread adxl355Thread;       //线程
ADXL355Data adxl355Data;         //数据结构
std::vector<uint8_t> adxl355DeviceAddresses = {0x01,0x02,0x03,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B};
//如果列表里有用不了的设备，线程时间轴会对不上
std::map<uint8_t, std::thread> adxl355Threads;
std::map<uint8_t, ADXL355Data> adxl355DataMap;
std::map<uint8_t, ADXL355Parser> adxl355Parsers;
std::mutex RS485SendRecvMutex;
std::thread adxl355PollingThread;

std::unordered_set<uint8_t> adxl355NeedInitEMASet;  // 保存开始时初始化的地址
std::mutex adxl355InitMutex;


bool adxlxlsxing = false; // 是否正在保存数据到XLSX文件
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
std::vector<uint8_t> jy61pDeviceAddresses = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A };
std::mutex jy61pDataMutex;
std::unordered_map<uint8_t, JY61PData> jy61pDataMap;

bool jy61xlsxing = false; // 是否正在保存数据到XLSX文件
//DualAxis数据
RS485Manager serialDualAxis;
//std::vector<uint8_t> dualAxisDeviceAddresses = { 0x03, 0x0C };
std::vector<uint8_t> dualAxisDeviceAddresses = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A };
std::map<uint8_t, DualAxisSensorParser::AngleData> dualAxisDataMap;
std::mutex dualAxisMutex;
//map中的uint8是0x01, 0x02, 0x03等设备地址
//std::unique_ptr是指向这个类的智能指针，unique_ptr 表示独占所有权：一个对象只能被一个指针拥有，不能复制，只能移动
std::map<uint8_t, std::unique_ptr<DualAxisSensorParser>> dualAxisParsers;
bool dualAxisxlsxing = false; // 是否正在保存数据到XLSX文件
