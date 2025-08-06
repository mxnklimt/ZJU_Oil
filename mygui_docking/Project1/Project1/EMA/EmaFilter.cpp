//EmaFilter.cpp
#include "EmaFilter.h"
// ===== 全局变量定义区域（只定义一次）======
EmaFilterManager emaFilterManager;
const double alpha = 0.2;
// 存储上一次采集的 filtered 值（用于 EMA）
std::unordered_map<uint8_t, std::pair<double, double>> lastFilteredMap;

void EmaFilterManager::update(uint8_t addr, double currentHorizontal, double currentVertical, double alpha) {
    auto& state = emaStates_[addr];
    if (!state.initialized) {
        state.horizontal = currentHorizontal;
        state.vertical = currentVertical;
        state.initialized = true;
    }
    else {
        state.horizontal = alpha * currentHorizontal + (1.0 - alpha) * state.horizontal;
        state.vertical = alpha * currentVertical + (1.0 - alpha) * state.vertical;
    }
}

double EmaFilterManager::getHorizontal(uint8_t addr) const {
    auto it = emaStates_.find(addr);
    return it != emaStates_.end() ? it->second.horizontal : 0.0;
}

double EmaFilterManager::getVertical(uint8_t addr) const {
    auto it = emaStates_.find(addr);
    return it != emaStates_.end() ? it->second.vertical : 0.0;
}

bool EmaFilterManager::has(uint8_t addr) const {
    auto it = emaStates_.find(addr);
    return it != emaStates_.end() && it->second.initialized;
}

void EmaFilterManager::set(uint8_t addr, double h, double v) {
    emaStates_[addr] = EmaState{ h, v, true };
}
void EmaFilterManager::updateXYZ(uint8_t addr, double currentX, double currentY, double currentZ, double alpha) {
    auto& state = adxl355EmaStates_[addr];
    if (!state.initialized) {
        state.x = currentX;
        state.y = currentY;
        state.z = currentZ;
        state.initialized = true;
    }
    else {
        state.x = alpha * currentX + (1.0 - alpha) * state.x;
        state.y = alpha * currentY + (1.0 - alpha) * state.y;
        state.z = alpha * currentZ + (1.0 - alpha) * state.z;
    }
}

double EmaFilterManager::getX(uint8_t addr) const {
    auto it = adxl355EmaStates_.find(addr);
    return it != adxl355EmaStates_.end() ? it->second.x : 0.0;
}

double EmaFilterManager::getY(uint8_t addr) const {
    auto it = adxl355EmaStates_.find(addr);
    return it != adxl355EmaStates_.end() ? it->second.y : 0.0;
}

double EmaFilterManager::getZ(uint8_t addr) const {
    auto it = adxl355EmaStates_.find(addr);
    return it != adxl355EmaStates_.end() ? it->second.z : 0.0;
}
void EmaFilterManager::setXYZ(uint8_t addr, double x, double y, double z) {
    adxl355EmaStates_[addr] = EmaStateXYZ{ x, y, z, true };
}
bool EmaFilterManager::hasXYZ(uint8_t addr) const {
    auto it = adxl355EmaStates_.find(addr);
    return it != adxl355EmaStates_.end() && it->second.initialized;
}
void EmaFilterManager::update9Axis(uint8_t addr, const float a[3], const float w[3], const float angle[3], double alpha) {
    auto& state = jy61pEmaStates_[addr];
    if (!state.initialized) {
        state.ax = a[0]; state.ay = a[1]; state.az = a[2];
        state.wx = w[0]; state.wy = w[1]; state.wz = w[2];
        state.roll = angle[0]; state.pitch = angle[1]; state.yaw = angle[2];
        state.initialized = true;
    }
    else if (state.ax == a[0] && state.ay == a[1] && state.az == a[2])
    {
        state.ax = a[0]; state.ay = a[1]; state.az = a[2];
        state.wx = w[0]; state.wy = w[1]; state.wz = w[2];
        state.roll = angle[0]; state.pitch = angle[1]; state.yaw = angle[2];
    }
    else {
        state.ax = alpha * a[0] + (1.0 - alpha) * state.ax;
        state.ay = alpha * a[1] + (1.0 - alpha) * state.ay;
        state.az = alpha * a[2] + (1.0 - alpha) * state.az;

        state.wx = alpha * w[0] + (1.0 - alpha) * state.wx;
        state.wy = alpha * w[1] + (1.0 - alpha) * state.wy;
        state.wz = alpha * w[2] + (1.0 - alpha) * state.wz;

        state.roll = alpha * angle[0] + (1.0 - alpha) * state.roll;
        state.pitch = alpha * angle[1] + (1.0 - alpha) * state.pitch;
        state.yaw = alpha * angle[2] + (1.0 - alpha) * state.yaw;
    }
}

//void EmaFilterManager::get9Axis(uint8_t addr, float a[3], float w[3], float angle[3]) const {
//    auto it = jy61pEmaStates_.find(addr);
//    if (it != jy61pEmaStates_.end()) {
//        const auto& state = it->second;
//        a[0] = state.ax; a[1] = state.ay; a[2] = state.az;
//        w[0] = state.wx; w[1] = state.wy; w[2] = state.wz;
//        angle[0] = state.roll; angle[1] = state.pitch; angle[2] = state.yaw;
//    }
//    else {
//        a[0] = a[1] = a[2] = 0;
//        w[0] = w[1] = w[2] = 0;
//        angle[0] = angle[1] = angle[2] = 0;
//    }
//}
void EmaFilterManager::get9Axis(uint8_t addr, float a[3], float w[3], float angle[3]) const {
    auto it = jy61pEmaStates_.find(addr);
    if (it == jy61pEmaStates_.end() || !it->second.initialized) {
        // 未初始化，返回0（或你也可以返回NaN用于调试）
        a[0] = a[1] = a[2] = 0.0f;
        w[0] = w[1] = w[2] = 0.0f;
        angle[0] = angle[1] = angle[2] = 0.0f;
        return;
    }

    const auto& state = it->second;
    a[0] = state.ax; a[1] = state.ay; a[2] = state.az;
    w[0] = state.wx; w[1] = state.wy; w[2] = state.wz;
    angle[0] = state.roll; angle[1] = state.pitch; angle[2] = state.yaw;
}

bool EmaFilterManager::has9Axis(uint8_t addr) const {
    auto it = jy61pEmaStates_.find(addr);
    return it != jy61pEmaStates_.end() && it->second.initialized;
}

void EmaFilterManager::set9Axis(uint8_t addr, const float a[3], const float w[3], const float angle[3]) {
    jy61pEmaStates_[addr] = {
        a[0], a[1], a[2],
        w[0], w[1], w[2],
        angle[0], angle[1], angle[2],
        true
    };
}
void EmaFilterManager::clearAll() {
    emaStates_.clear();
    adxl355EmaStates_.clear();
    jy61pEmaStates_.clear();
}