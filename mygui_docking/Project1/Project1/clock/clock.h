#pragma once
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
// 方法2：获取格式化的本地时间字符串（包含毫秒），格式如：2024-06-12 15:30:45.821
std::string getFormattedTimeWithMs() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    // 线程安全的本地时间转换
    std::tm local_time;
#ifdef _WIN32
    localtime_s(&local_time, &time_t_now);
#else
    localtime_r(&time_t_now, &local_time);
#endif

    // 计算毫秒部分
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    // 格式化输出
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << millis;

    return oss.str();
}
void ensureMinInterval(std::chrono::steady_clock::time_point& startTime) {
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime);
    const int minIntervalMs = 300;

    if (elapsed.count() < minIntervalMs) {
        auto remainingTime = std::chrono::milliseconds(minIntervalMs) - elapsed;
        std::this_thread::sleep_for(remainingTime);
    }

    // 更新起始时间为当前时间，为下一次间隔判断做准备
    startTime = std::chrono::steady_clock::now();
}
void ensureMinInterval_jy61(std::chrono::steady_clock::time_point& startTime) {
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime);
    const int minIntervalMs = 2000;

    if (elapsed.count() < minIntervalMs) {
        auto remainingTime = std::chrono::milliseconds(minIntervalMs) - elapsed;
        std::this_thread::sleep_for(remainingTime);
    }

    // 更新起始时间为当前时间，为下一次间隔判断做准备
    startTime = std::chrono::steady_clock::now();
}