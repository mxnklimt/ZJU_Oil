#pragma once
//Data.h
#include <vector>
#include <mutex>
#include <deque>
#include <thread>
#include <atomic>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include"ADXL355/ADXL355Parser.h"
#include"RS485/RS485Manager.h"
#include"DualAxisSensor/DualAxisSensorParser.h"
#include"BSQJN/BSQJNParser.h"
#include"FiberGratingAnalyzer.h"

#define rightdevice // #define 
// device
extern bool devicechoice;
//串口管理
extern RS485Manager serialManager_Axis; // 用于Axis的串口管理
extern RS485Manager serialManager_JY61; // 用于JY61P的串口管理
extern RS485Manager serialManager_laser; // 用于激光传感器的串口管理
extern std::mutex dataMutex;
//extern声明全局变量
//ADXL355
extern const size_t MAX_POINTS;
extern std::mutex ADXL355Mutex;
extern std::atomic<bool> collectingADXL355;
extern RS485Manager serialManager;
extern RS485Manager serialManager_AMT;
extern ADXL355Parser parser;
extern std::thread adxl355Thread;       //线程
extern class ADXL355Data adxl355Data;         //数据结构
extern std::thread adxl355PollingThread;
extern bool adxlxlsxing; // 是否正在保存数据到XLSX文件
class ADXL355Data
{
public:
	std::deque<ADXL355Parser::AccelerationData> dataQue;//使用别的类里的结构体要加上作用域
};
extern std::vector<uint8_t> adxl355DeviceAddresses;
extern std::map<uint8_t, std::thread> adxl355Threads;
extern std::map<uint8_t, class ADXL355Data> adxl355DataMap;
extern std::map<uint8_t, class ADXL355Parser> adxl355Parsers;
extern std::mutex RS485SendRecvMutex;


extern std::unordered_set<uint8_t> adxl355NeedInitEMASet;  // 保存开始时初始化的地址
extern std::mutex adxl355InitMutex;
//JY61P
class JY61PData
{
public:
	struct angle
	{
		double a[3] = { 0.0f, 0.0f, 0.0f }; // 加速度
		double w[3] = { 0.0f, 0.0f, 0.0f }; // 角速度
		double Angle[3] = { 0.0f, 0.0f, 0.0f }; // 姿态角
		double EMA_a[3] = { 0.0f, 0.0f, 0.0f }; // EMA 加速度
		double EMA_w[3] = { 0.0f, 0.0f, 0.0f }; // EMA 角速度
		double EMA_Angle[3] = { 0.0f, 0.0f, 0.0f }; // EMA 姿态角
		double temperature = 0.0f;//温度
	};

	std::deque<angle> dataQue;//使用别的类里的结构体要加上作用域
};
extern char s_cDataUpdate;
extern int iComPort;
extern int iBaud;
extern int iAddress;
extern std::atomic<bool> collectingJY61P;
extern std::thread JY61PThread;
extern std::mutex JY61PMutex;
extern class JY61PData jy61pData;         //数据结构
extern std::mutex jy61pDataMutex;
extern std::unordered_map<uint8_t, JY61PData> jy61pDataMap;
extern std::vector<uint8_t> jy61pDeviceAddresses;
extern bool jy61xlsxing; // 是否正在保存数据到XLSX文件

//CHANGE
extern RS485Manager jy61pSerial;

struct Vec3 {
	float x, y, z;
	Vec3() = default;
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

//DualAxis数据
extern std::vector<uint8_t> dualAxisDeviceAddresses;
extern RS485Manager serialDualAxis;
extern std::map<uint8_t, DualAxisSensorParser::AngleData> dualAxisDataMap;
extern std::map<uint8_t, std::unique_ptr<DualAxisSensorParser>> dualAxisParsers;
extern struct save;
extern std::map<uint8_t, save>dualAxis_Save;
extern bool dualAxisxlsxing;

extern std::map<uint8_t, DualAxisSensorParser::AngleData> angleDataMap;

extern std::map<uint8_t, DualAxisSensorParser::AngleData> dualAxisSnapshotBuffer;

//lasor
extern std::vector<uint8_t> laserDeviceAddresses;
extern std::mutex LasergetMutex;
extern std::mutex LasergetMutex2;
extern bool lasorxlsxing; // 是否正在保存数据到XLSX文件
extern std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint32_t>>> collectedLasorMap;
extern std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint32_t>>> collectedLasorMap2;
extern class RS485Manager serialManager2;

//采样周期
extern float timeInterval; // 采样周期，单位为毫秒

//AMT
extern std::vector<uint8_t> amtAddresses; 
// 数据缓存
struct AMTData {
	double position = 0.0;
	double temperature = 0.0;
};
extern std::mutex amtDataMutex;
extern std::unordered_map<uint8_t, std::deque<AMTData>> amtDataMap;
//BSQJN
// 设备地址列表
extern std::vector<uint8_t> bsqjnDeviceAddresses;

// 解析器
extern std::unordered_map<uint8_t, BSQJNParser> bsqjnParsers;

// 数据结构
struct BSQJNData {
	//std::deque<float> dataQue; // 拉力值队列
	std::deque<std::vector<float>> dataQue;
};

// 地址 -> 数据
extern std::unordered_map<uint8_t, BSQJNData> bsqjnDataMap;

// 数据锁
extern std::mutex BSQJNMutex;

// 采集控制
extern std::atomic<bool> collectingBSQJN;
extern std::thread bsqjnPollingThread;

// 保存标志（如果你需要保存 xlsx，可用这个）
extern bool bsqjnSavingFlag;
extern std::mutex RS485BSQMutex;

//fiber
// 定义全局变量（在源文件中）
extern std::vector<FiberGratingAnalyzer::SensorData> g_sensorDataCollection;
extern std::mutex g_sensorDataMutex;