//EmaFilter.h
#pragma once
#include <unordered_map>
#include <cstdint>
#include <mutex>
// ADXL355 三轴加速度 EMA 状态
struct EmaStateXYZ {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool initialized = false;
};
struct EmaState {
    double horizontal = 0.0;
    double vertical = 0.0;
    bool initialized = false;
};
struct EmaState9Axis {
    double ax = 0.0, ay = 0.0, az = 0.0;
    double wx = 0.0, wy = 0.0, wz = 0.0;
    double roll = 0.0, pitch = 0.0, yaw = 0.0;
    bool initialized = false;
};
class EmaFilterManager {
public:
    //void update(uint8_t addr, double currentHorizontal, double currentVertical, double alpha);
    void update(uint8_t addr, double currentHorizontal, double currentVertical, double alpha);

    double getHorizontal(uint8_t addr) const;
    double getVertical(uint8_t addr) const;
    bool has(uint8_t addr) const;                 // 是否已经初始化
    void set(uint8_t addr, double h, double v);   // 设置初始 EMA 值

    // 新增 ADXL355 三轴 EMA 接口
    void updateXYZ(uint8_t addr, double currentX, double currentY, double currentZ, double alpha);
    double getX(uint8_t addr) const;
    double getY(uint8_t addr) const;
    double getZ(uint8_t addr) const;
	void setXYZ(uint8_t addr, double x, double y, double z);
	bool hasXYZ(uint8_t addr) const;              // 是否已经初始化三轴 EMA

    void update9Axis(uint8_t addr, const double a[3], const double w[3], const double angle[3], double alpha);
    void get9Axis(uint8_t addr, double a[3], double w[3], double angle[3]) const;
    bool has9Axis(uint8_t addr) const;
    void set9Axis(uint8_t addr, const double a[3], const double w[3], const double angle[3]);
    void clearAll();
private:
    std::unordered_map<uint8_t, EmaState> emaStates_;
    std::unordered_map<uint8_t, EmaStateXYZ> adxl355EmaStates_;
    std::unordered_map<uint8_t, EmaState9Axis> jy61pEmaStates_;
    std::mutex mtx;
    std::unordered_map<uint8_t, std::tuple<double, double, double>> lastEmaMapXYZ;
    
};

// ====== 全局变量声明（别在头文件中定义！！！）======
extern EmaFilterManager emaFilterManager;

extern const double alpha;
extern std::unordered_map<uint8_t, std::pair<double, double>> lastFilteredMap;
