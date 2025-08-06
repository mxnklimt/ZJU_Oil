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