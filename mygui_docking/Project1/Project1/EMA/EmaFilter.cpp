#include "EmaFilter.h"
// ===== 全局变量定义区域（只定义一次）======
EmaFilterManager emaFilterManager;
const double alpha = 0.2;

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
