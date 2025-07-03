////sheetdata.h
//#pragma once
//#include <vector>
//#include <ctime>
//
//extern std::vector<float> dValues_save;
//extern std::vector<float> eValues_save;
//extern std::vector<std::tm> times_save;
// sheetdata.h
#pragma once
#include <vector>
#include <ctime>
#include <atomic>
#include <mutex>
#include <future>

extern std::vector<float> dValues_save;
extern std::vector<float> eValues_save;
extern std::vector<std::tm> times_save;
extern std::atomic<bool> dataLoadingDone; // 标志加载是否完成
// 控制和同步
extern std::atomic<bool> readfile;  // 是否已加载完成
extern std::mutex sheetDataMutex;
extern std::future<void> excelLoadFuture;
void loadExcelDataAsync();
void checkLoadingStatus();