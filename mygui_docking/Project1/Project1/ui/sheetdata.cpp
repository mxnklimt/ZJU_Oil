////sheetdata.cpp

//#include <ctime>
//#include "ui/sheetdata.h"
//std::vector<float> dValues_save, eValues_save;
//std::vector<std::tm> times_save;

// sheetdata.cpp
#include "sheetdata.h"
#include "file/ReadFile.h" // 你自己的读取Excel的类
#include <utility>    // for std::move
#include "Imgui/imgui.h"

std::vector<float> dValues_save;
std::vector<float> eValues_save;
std::vector<std::tm> times_save;

std::atomic<bool> readfile = false;
std::mutex sheetDataMutex;
std::future<void> excelLoadFuture;
std::atomic<bool> dataLoadingDone{ false }; // 标志加载是否完成
// 异步加载Excel数据
void loadExcelDataAsync() {
    excelLoadFuture = std::async(std::launch::async, [] {
        std::vector<float> dValues, eValues;
        std::vector<std::tm> times;

        // 实际读取
        ReadFile::readColumnsDandEFloat("data.xlsx", "Sheet1", dValues, eValues);
        ReadFile::readColumnCTimeOnly("data.xlsx", "Sheet1", times);

        // 写入共享数据区（加锁）
        {
            std::lock_guard<std::mutex> lock(sheetDataMutex);
            dValues_save = std::move(dValues);
            eValues_save = std::move(eValues);
            times_save = std::move(times);
        }

        dataLoadingDone = true;
        });
}
// 主线程检测函数（每帧调用）
void checkLoadingStatus() {
    if (excelLoadFuture.valid()) {
        auto status = excelLoadFuture.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            excelLoadFuture.get(); // 确保异常被抛出，任务结束
        }
    }
}