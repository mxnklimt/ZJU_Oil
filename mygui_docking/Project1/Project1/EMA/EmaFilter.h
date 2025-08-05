#pragma once
#include <unordered_map>
#include <cstdint>

struct EmaState {
    double horizontal = 0.0;
    double vertical = 0.0;
    bool initialized = false;
};

class EmaFilterManager {
public:
    void update(uint8_t addr, double currentHorizontal, double currentVertical, double alpha);

    double getHorizontal(uint8_t addr) const;
    double getVertical(uint8_t addr) const;

private:
    std::unordered_map<uint8_t, EmaState> emaStates_;
};

// ====== 全局变量声明（别在头文件中定义！！！）======
extern EmaFilterManager emaFilterManager;
extern const double alpha;
#pragma once
