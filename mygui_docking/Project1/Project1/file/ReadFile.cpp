#include <iostream>
#include <vector>
#include <string>
#include <OpenXLSX.hpp>
#include <iostream>
#include <unordered_map>
#include <cmath>
#include <regex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <string>   
#include "Imgui/imgui.h"
#include "ReadFile.h"
#include "OpenXLSX/OpenXLSX.hpp"
#include "data/Data.h"
#include "EMA/EmaFilter.h"

void StopAndSaveDualAxisData(
    std::atomic<bool>& isCollectingData,
    std::thread& dataCollectionThread,
    const std::vector<uint8_t>& deviceAddresses,
    const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, DualAxisSensorParser::AngleData>>>& collectedData,
    std::mutex& collectedDataMutex,
    const std::string& baseFileName
) {
    isCollectingData = false;
    if (dataCollectionThread.joinable()) {
        dataCollectionThread.join();
    }

    try {
        std::string saveFilePath = generateUniqueFileName(baseFileName);
        OpenXLSX::XLDocument doc;

        doc.create(saveFilePath, false);
        doc.open(saveFilePath);
        auto wks = doc.workbook().worksheet("Sheet1");

        // 表头
        wks.cell(1, 1).value() = "Time";
        wks.cell(1, 2).value() = "Device Addr";
        wks.cell(1, 3).value() = "Filtered Horizontal";
        wks.cell(1, 4).value() = "Filtered Vertical";
        wks.cell(1, 5).value() = "Raw Horizontal";
        wks.cell(1, 6).value() = "Raw Vertical";
		wks.cell(1, 7).value() = "EMA Horizontal";
		wks.cell(1, 8).value() = "EMA Vertical";

        // 数据
        std::lock_guard<std::mutex> lock(collectedDataMutex);
        int row = 2; // 从第2行开始写数据
        for (const auto& [timestamp, dataMap] : collectedData) {
            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            std::tm tm;
            localtime_s(&tm, &time_t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            std::string timeStr = oss.str();

            for (uint8_t addr : deviceAddresses) {
                if (dataMap.find(addr) != dataMap.end()) {
                    const auto& angles = dataMap.at(addr);

                    // 时间列
                    wks.cell(row, 1).value() = timeStr;
                    // 设备地址列
                    std::stringstream addrSS;
                    addrSS << "0x" << std::uppercase << std::hex
                        << std::setw(2) << std::setfill('0') << static_cast<int>(addr);
                    wks.cell(row, 2).value() = addrSS.str();
                    // 数据列
                    wks.cell(row, 3).value() = angles.filtered_horizontal;
                    wks.cell(row, 4).value() = angles.filtered_vertical;
                    wks.cell(row, 5).value() = angles.raw_horizontal;
                    wks.cell(row, 6).value() = angles.raw_vertical;
					wks.cell(row, 7).value() = angles.EMA_horizontal;
					wks.cell(row, 8).value() = angles.EMA_vertical;
                    row++;
                }
            }
        }

        doc.save();
        doc.close();

        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"数据已保存到: %s", saveFilePath.c_str());
    }
    catch (const std::exception& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"保存失败: %s", e.what());
    }
}
void VerifyAndFixADXL355EMA(const std::string& filePath, double alpha) {
    try {
        OpenXLSX::XLDocument doc;
        doc.open(filePath);
        auto wks = doc.workbook().worksheet("Sheet1");

        std::unordered_map<std::string, std::tuple<double, double, double>> lastEmaMap;
        int row = 2;
        int fixCount = 0;

        while (true) {
            auto timeCell = wks.cell(row, 1);
            if (timeCell.value().type() == OpenXLSX::XLValueType::Empty) break;

            std::string addrStr = wks.cell(row, 2).value().get<std::string>();

            double rawX = wks.cell(row, 3).value().get<double>();
            double rawY = wks.cell(row, 4).value().get<double>();
            double rawZ = wks.cell(row, 5).value().get<double>();

            double savedEmaX = wks.cell(row, 6).value().get<double>();
            double savedEmaY = wks.cell(row, 7).value().get<double>();
            double savedEmaZ = wks.cell(row, 8).value().get<double>();

            // 获取上一次 EMA，如果没有就用当前 raw 初始化
            double prevEmaX = rawX, prevEmaY = rawY, prevEmaZ = rawZ;

            if (lastEmaMap.count(addrStr)) {
                std::tie(prevEmaX, prevEmaY, prevEmaZ) = lastEmaMap[addrStr];
            }

            // 计算新的 EMA
            double calcEmaX = alpha * rawX + (1 - alpha) * prevEmaX;
            double calcEmaY = alpha * rawY + (1 - alpha) * prevEmaY;
            double calcEmaZ = alpha * rawZ + (1 - alpha) * prevEmaZ;

            // 浮点误差阈值
            const double epsilon = 1e-6;

            bool needFix =
                std::abs(savedEmaX - calcEmaX) > epsilon ||
                std::abs(savedEmaY - calcEmaY) > epsilon ||
                std::abs(savedEmaZ - calcEmaZ) > epsilon;

            if (needFix) {
                /*std::cout << "[修正] 行 " << row << "，设备 " << addrStr
                    << "\n -> EMA_X 原: " << savedEmaX << " 计算: " << calcEmaX
                    << "\n -> EMA_Y 原: " << savedEmaY << " 计算: " << calcEmaY
                    << "\n -> EMA_Z 原: " << savedEmaZ << " 计算: " << calcEmaZ << "\n";*/

                wks.cell(row, 6).value() = calcEmaX;
                wks.cell(row, 7).value() = calcEmaY;
                wks.cell(row, 8).value() = calcEmaZ;

                fixCount++;
            }

            // 更新缓存
            lastEmaMap[addrStr] = { calcEmaX, calcEmaY, calcEmaZ };

            row++;
        }

        doc.save();
        doc.close();

        if (fixCount == 0) {
            //std::cout << "所有 ADXL355 EMA 数据均正确，无需修正。\n";
        }
        else {
            //std::cout << "共修正了 " << fixCount << " 行 ADXL355 EMA 数据。\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "ADXL355 EMA 验证修正失败：" << e.what() << "\n";
    }
}


void SaveADXL355ToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, ADXL355Parser::AccelerationData>>>& data) {

    std::string saveFilePath = generateUniqueFileName("ADXL355_sync");
    OpenXLSX::XLDocument doc;
    doc.create(saveFilePath, false);
    doc.open(saveFilePath);
    auto wks = doc.workbook().worksheet("Sheet1");

    // ===== 写表头 =====
    wks.cell(1, 1).value() = "Time";
    wks.cell(1, 2).value() = "Device Addr";
    wks.cell(1, 3).value() = "Accel X";
    wks.cell(1, 4).value() = "Accel Y";
    wks.cell(1, 5).value() = "Accel Z";
    wks.cell(1, 6).value() = "EMA Accel X";
    wks.cell(1, 7).value() = "EMA Accel Y";
    wks.cell(1, 8).value() = "EMA Accel Z";

    // ===== 写入数据，每个时间戳-设备单独一行 =====
    int row = 2;
    for (const auto& [timestamp, snapshot] : data) {
        // 格式化时间戳
        
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm;
        localtime_s(&tm, &time_t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        std::string timeStr = oss.str();

        for (const auto& [addr, val] : snapshot) {
            // 如果三个 EMA 都是 0，就跳过这一行，不写入
            if (val.EMA_x == 0.0 && val.EMA_y == 0.0 && val.EMA_z == 0.0) {
                continue; // 跳过
            }
            // 写时间
            wks.cell(row, 1).value() = timeStr;

            // 写设备地址
            std::stringstream ss;
            ss << "0x" << std::uppercase << std::hex
                << std::setw(2) << std::setfill('0') << (int)addr;
            wks.cell(row, 2).value() = ss.str();

            // 写加速度数据
            wks.cell(row, 3).value() = val.x;
            wks.cell(row, 4).value() = val.y;
            wks.cell(row, 5).value() = val.z;
            wks.cell(row, 6).value() = val.EMA_x;
            wks.cell(row, 7).value() = val.EMA_y;
            wks.cell(row, 8).value() = val.EMA_z;

            row++; // 下一行
        }
    }

    doc.save();
    doc.close();
    VerifyAndFixADXL355EMA(saveFilePath,alpha); // 放在 save() 和 close() 后面

}


void VerifyAndFixJY61PEMA(const std::string& filePath, double alpha) {
    try {
        OpenXLSX::XLDocument doc;
        doc.open(filePath);
        auto wks = doc.workbook().worksheet("Sheet1");

        // 存储上一次的 EMA 数据，key 是设备地址字符串
        struct EMAData {
            double EMA_a[3]{ 0,0,0 };
            double EMA_w[3]{ 0,0,0 };
            double EMA_Angle[3]{ 0,0,0 };
        };
        std::unordered_map<std::string, EMAData> lastEmaMap;

        int row = 2;
        int fixCount = 0;
        const double epsilon = 1e-6;

        while (true) {
            auto timeCell = wks.cell(row, 1);
            if (timeCell.value().type() == OpenXLSX::XLValueType::Empty) break;

            std::string addrStr = wks.cell(row, 2).value().get<std::string>();

            // 读取原始数据（加速度、角速度、角度）
            double a[3] = {
                wks.cell(row, 3).value().get<double>(),
                wks.cell(row, 4).value().get<double>(),
                wks.cell(row, 5).value().get<double>()
            };
            double w[3] = {
                wks.cell(row, 6).value().get<double>(),
                wks.cell(row, 7).value().get<double>(),
                wks.cell(row, 8).value().get<double>()
            };
            double Angle[3] = {
                wks.cell(row, 9).value().get<double>(),
                wks.cell(row, 10).value().get<double>(),
                wks.cell(row, 11).value().get<double>()
            };

            // 读取保存的 EMA 数据
            double savedEMA_a[3] = {
                wks.cell(row, 12).value().get<double>(),
                wks.cell(row, 13).value().get<double>(),
                wks.cell(row, 14).value().get<double>()
            };
            double savedEMA_w[3] = {
                wks.cell(row, 15).value().get<double>(),
                wks.cell(row, 16).value().get<double>(),
                wks.cell(row, 17).value().get<double>()
            };
            double savedEMA_Angle[3] = {
                wks.cell(row, 18).value().get<double>(),
                wks.cell(row, 19).value().get<double>(),
                wks.cell(row, 20).value().get<double>()
            };

            // 取上一次 EMA，没有则初始化为当前 raw
            EMAData lastEma = {};
            if (lastEmaMap.count(addrStr)) {
                lastEma = lastEmaMap[addrStr];
            }
            else {
                for (int i = 0; i < 3; ++i) {
                    lastEma.EMA_a[i] = a[i];
                    lastEma.EMA_w[i] = w[i];
                    lastEma.EMA_Angle[i] = Angle[i];
                }
            }

            // 计算新的 EMA
            EMAData calcEma;
            for (int i = 0; i < 3; ++i) {
                calcEma.EMA_a[i] = alpha * a[i] + (1 - alpha) * lastEma.EMA_a[i];
                calcEma.EMA_w[i] = alpha * w[i] + (1 - alpha) * lastEma.EMA_w[i];
                calcEma.EMA_Angle[i] = alpha * Angle[i] + (1 - alpha) * lastEma.EMA_Angle[i];
            }

            // 检查是否需要修正
            bool needFix = false;
            for (int i = 0; i < 3; ++i) {
                if (std::abs(savedEMA_a[i] - calcEma.EMA_a[i]) > epsilon ||
                    std::abs(savedEMA_w[i] - calcEma.EMA_w[i]) > epsilon ||
                    std::abs(savedEMA_Angle[i] - calcEma.EMA_Angle[i]) > epsilon) {
                    needFix = true;
                    break;
                }
            }

            if (needFix) {
                fixCount++;
                // 输出修正日志
                //std::cout << "[修正] 行 " << row << "，设备 " << addrStr << "\n";
                /*for (int i = 0; i < 3; ++i) {
                    std::cout << " EMA_a[" << i << "] 原: " << savedEMA_a[i] << " 计算: " << calcEma.EMA_a[i] << "\n";
                    std::cout << " EMA_w[" << i << "] 原: " << savedEMA_w[i] << " 计算: " << calcEma.EMA_w[i] << "\n";
                    std::cout << " EMA_Angle[" << i << "] 原: " << savedEMA_Angle[i] << " 计算: " << calcEma.EMA_Angle[i] << "\n";
                }*/

                // 写回修正后的 EMA 值
                for (int i = 0; i < 3; ++i) {
                    wks.cell(row, 12 + i).value() = calcEma.EMA_a[i];
                    wks.cell(row, 15 + i).value() = calcEma.EMA_w[i];
                    wks.cell(row, 18 + i).value() = calcEma.EMA_Angle[i];
                }
            }

            // 更新缓存
            lastEmaMap[addrStr] = calcEma;

            row++;
        }

        doc.save();
        doc.close();

        if (fixCount == 0) {
            //std::cout << "所有 JY61P EMA 数据均正确，无需修正。\n";
        }
        else {
            //std::cout << "共修正了 " << fixCount << " 行 JY61P EMA 数据。\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "JY61P EMA 验证修正失败：" << e.what() << "\n";
    }
}

void VerifyAndFixEMA(const std::string& filePath, double alpha) {
    try {
        OpenXLSX::XLDocument doc;
        doc.open(filePath);
        auto wks = doc.workbook().worksheet("Sheet1");

        std::unordered_map<std::string, std::pair<double, double>> lastEmaMap;
        int row = 2;
        int fixCount = 0;

        while (true) {
            auto timeCell = wks.cell(row, 1);
            if (timeCell.value().type() == OpenXLSX::XLValueType::Empty) break;

            std::string addrStr = wks.cell(row, 2).value().get<std::string>();

            double rawH = wks.cell(row, 5).value().get<double>();
            double rawV = wks.cell(row, 6).value().get<double>();
            double savedEmaH = wks.cell(row, 7).value().get<double>();
            double savedEmaV = wks.cell(row, 8).value().get<double>();

            // 获取上一个 EMA，如果没有则用当前 raw 初始化
            double prevEmaH = rawH;
            double prevEmaV = rawV;

            if (lastEmaMap.count(addrStr)) {
                prevEmaH = lastEmaMap[addrStr].first;
                prevEmaV = lastEmaMap[addrStr].second;
            }

            // 计算理论 EMA
            double calcEmaH = alpha * rawH + (1 - alpha) * prevEmaH;
            double calcEmaV = alpha * rawV + (1 - alpha) * prevEmaV;

            // 误差阈值，考虑浮点误差
            const double epsilon = 1e-6;

            bool needFix = (std::abs(savedEmaH - calcEmaH) > epsilon) || (std::abs(savedEmaV - calcEmaV) > epsilon);

            if (needFix) {
                /*std::cout << "[修正] 行 " << row << "，设备 " << addrStr
                    << "，EMA_H 原值: " << savedEmaH << " 计算值: " << calcEmaH
                    << "，EMA_V 原值: " << savedEmaV << " 计算值: " << calcEmaV << "\n";*/

                wks.cell(row, 7).value() = calcEmaH;
                wks.cell(row, 8).value() = calcEmaV;

                fixCount++;
            }

            // 更新 EMA 缓存
            lastEmaMap[addrStr] = { calcEmaH, calcEmaV };

            row++;
        }

        doc.save();
        doc.close();

        if (fixCount == 0) {
            std::cout << "所有 EMA 数据均符合计算结果，无需修正。\n";
        }
        else {
            std::cout << "共修正了 " << fixCount << " 行 EMA 数据。\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "EMA 验证修正失败：" << e.what() << "\n";
    }
}


void SaveDualAxisToXLSX(
    const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, DualAxisSensorParser::AngleData>>>& collectedData,
    const std::vector<uint8_t>& deviceAddresses,
    const std::string& baseFileName,
    std::mutex& collectedDataMutex
) {
    try {
        std::string saveFilePath = generateUniqueFileName(baseFileName);
        OpenXLSX::XLDocument doc;

        doc.create(saveFilePath, false);
        doc.open(saveFilePath);
        auto wks = doc.workbook().worksheet("Sheet1");

        // 表头
        wks.cell(1, 1).value() = "Time";
        wks.cell(1, 2).value() = "Device Addr";
        wks.cell(1, 3).value() = "Filtered Horizontal";
        wks.cell(1, 4).value() = "Filtered Vertical";
        wks.cell(1, 5).value() = "Raw Horizontal";
        wks.cell(1, 6).value() = "Raw Vertical";
        wks.cell(1, 7).value() = "EMA Horizontal";
        wks.cell(1, 8).value() = "EMA Vertical";

        // 数据写入
        std::lock_guard<std::mutex> lock(collectedDataMutex);
        int row = 2; // 从第2行开始
        for (const auto& [timestamp, dataMap] : collectedData) {

            //------------------------------------------------------------

            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            std::tm tm;
            localtime_s(&tm, &time_t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            std::string timeStr = oss.str();

            for (uint8_t addr : deviceAddresses) {
                if (dataMap.count(addr)) {
                    const auto& angles = dataMap.at(addr);

                    // 只要 EMA 全为 0，就跳过该设备的一行数据
                    if (angles.EMA_horizontal == 0.0 && angles.EMA_vertical == 0.0) {
                        continue;
                    }
                    wks.cell(row, 1).value() = timeStr;

                    std::stringstream addrSS;
                    addrSS << "0x" << std::uppercase << std::hex
                        << std::setw(2) << std::setfill('0') << static_cast<int>(addr);
                    wks.cell(row, 2).value() = addrSS.str();

                    wks.cell(row, 3).value() = angles.filtered_horizontal;
                    wks.cell(row, 4).value() = angles.filtered_vertical;
                    wks.cell(row, 5).value() = angles.raw_horizontal;
                    wks.cell(row, 6).value() = angles.raw_vertical;
                    wks.cell(row, 7).value() = angles.EMA_horizontal;
                    wks.cell(row, 8).value() = angles.EMA_vertical;

                    row++;
                }
            }
        }

        doc.save();
        doc.close();
        VerifyAndFixEMA(saveFilePath,alpha);
        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"数据已保存到: %s", saveFilePath.c_str());
    }
    catch (const std::exception& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"保存失败: %s", e.what());
    }
    
}
void SaveLaserToXLSX(
    const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>>>>>& data)
{
    std::string saveFilePath = generateUniqueFileName("Laser_sync");
    OpenXLSX::XLDocument doc;
    doc.create(saveFilePath, false);
    doc.open(saveFilePath);
    auto wks = doc.workbook().worksheet("Sheet1");

    // 写表头
    wks.cell(1, 1).value() = "Snapshot Time";        // 快照采集时间
    wks.cell(1, 2).value() = "Device Addr";          // 设备地址
    wks.cell(1, 3).value() = "Data Value";           // 数据值

    int row = 2;
    for (const auto& [snapshotTime, deviceMap] : data) {
        // 格式化快照采集时间
        auto snap_t = std::chrono::system_clock::to_time_t(snapshotTime);
        std::tm snap_tm;
        localtime_s(&snap_tm, &snap_t);
        std::ostringstream snap_oss;
        snap_oss << std::put_time(&snap_tm, "%Y-%m-%d %H:%M:%S");
        std::string snapTimeStr = snap_oss.str();

        for (const auto& [addr, dataVec] : deviceMap) {
            // 设备地址格式化
            std::ostringstream addr_ss;
            addr_ss << "0x" << std::uppercase << std::hex
                << std::setw(2) << std::setfill('0') << (int)addr;
            std::string addrStr = addr_ss.str();

            for (const auto& [/*dataTime*/_, value] : dataVec) {
                // 写入数据行
                wks.cell(row, 1).value() = snapTimeStr;
                wks.cell(row, 2).value() = addrStr;
                wks.cell(row, 3).value() = value;

                row++;
            }
        }
    }

    doc.save();
    doc.close();
}

//void SaveLaserToXLSX(
//    const std::vector<std::pair<std::chrono::system_clock::time_point,
//    std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>>>>>& data)
//{
//    // 这里写具体保存代码
//}
void SaveJY61PToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::unordered_map<uint8_t, JY61PData::angle>>>& data) {

    try {
        std::string saveFilePath = generateUniqueFileName("JY61P_sync");
        OpenXLSX::XLDocument doc;
        doc.create(saveFilePath, false);
        doc.open(saveFilePath);
        auto wks = doc.workbook().worksheet("Sheet1");

        // 写表头
        wks.cell(1, 1).value() = "Time";
        wks.cell(1, 2).value() = "Device Addr";

        // 下面依次写所有数据字段
        wks.cell(1, 3).value() = "Accel X";
        wks.cell(1, 4).value() = "Accel Y";
        wks.cell(1, 5).value() = "Accel Z";
        wks.cell(1, 6).value() = "Gyro X";
        wks.cell(1, 7).value() = "Gyro Y";
        wks.cell(1, 8).value() = "Gyro Z";
        wks.cell(1, 9).value() = "Angle X";
        wks.cell(1, 10).value() = "Angle Y";
        wks.cell(1, 11).value() = "Angle Z";
        wks.cell(1, 12).value() = "EMA Accel X";
        wks.cell(1, 13).value() = "EMA Accel Y";
        wks.cell(1, 14).value() = "EMA Accel Z";
        wks.cell(1, 15).value() = "EMA Gyro X";
        wks.cell(1, 16).value() = "EMA Gyro Y";
        wks.cell(1, 17).value() = "EMA Gyro Z";
        wks.cell(1, 18).value() = "EMA Angle X";
        wks.cell(1, 19).value() = "EMA Angle Y";
        wks.cell(1, 20).value() = "EMA Angle Z";

        int row = 2;
        for (const auto& [timestamp, snapshot] : data) {


            // 格式化时间字符串
            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &time_t);
#else
            localtime_r(&time_t, &tm);
#endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            std::string timeStr = oss.str();

            // 每个设备单独写一行
            for (uint8_t addr : jy61pDeviceAddresses) {
                
                if (snapshot.find(addr) != snapshot.end()) {
                    const auto& angle = snapshot.at(addr);
                    // 只要 EMA 全为 0，就跳过该设备的一行数据
                    if (angle.EMA_a[1] == 0.0 && angle.EMA_a[2] == 0.0) {
                        continue;
                    }
                    wks.cell(row, 1).value() = timeStr;
                    std::stringstream ss;
                    ss << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)addr;
                    wks.cell(row, 2).value() = ss.str();

                    wks.cell(row, 3).value() = angle.a[0];
                    wks.cell(row, 4).value() = angle.a[1];
                    wks.cell(row, 5).value() = angle.a[2];
                    wks.cell(row, 6).value() = angle.w[0];
                    wks.cell(row, 7).value() = angle.w[1];
                    wks.cell(row, 8).value() = angle.w[2];
                    wks.cell(row, 9).value() = angle.Angle[0];
                    wks.cell(row, 10).value() = angle.Angle[1];
                    wks.cell(row, 11).value() = angle.Angle[2];

                    wks.cell(row, 12).value() = angle.EMA_a[0];
                    wks.cell(row, 13).value() = angle.EMA_a[1];
                    wks.cell(row, 14).value() = angle.EMA_a[2];
                    wks.cell(row, 15).value() = angle.EMA_w[0];
                    wks.cell(row, 16).value() = angle.EMA_w[1];
                    wks.cell(row, 17).value() = angle.EMA_w[2];
                    wks.cell(row, 18).value() = angle.EMA_Angle[0];
                    wks.cell(row, 19).value() = angle.EMA_Angle[1];
                    wks.cell(row, 20).value() = angle.EMA_Angle[2];

                    row++;
                }
            }
        }

        doc.save();
        doc.close();
        VerifyAndFixJY61PEMA(saveFilePath, alpha);
        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"数据已保存到: %s", saveFilePath.c_str());
		
    }
    catch (const std::exception& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"保存失败: %s", e.what());
    }
}


std::string generateUniqueFileName(const std::string& baseName, const std::string& extension) {
    std::string filename = baseName + extension;
    int counter = 1;
    while (std::filesystem::exists(filename)) {
        filename = baseName + "_" + std::to_string(counter++) + extension;
    }
    return filename;
}
 void ReadFile::readColumnsDandEFloat(
    const std::string& filePath,
    const std::string& sheetName,
    std::vector<float>& columnD,
    std::vector<float>& columnE
) {
    try {
        OpenXLSX::XLDocument doc;
        doc.open(filePath);

        auto wks = doc.workbook().worksheet(sheetName);
        auto range = wks.rows();

        uint64_t rowNumber = 1;
        for (const auto& row : range) {
            // 跳过第一行（表头）
            if (rowNumber == 1) {
                ++rowNumber;
                continue;
            }

            auto cellD = wks.cell(OpenXLSX::XLCellReference(rowNumber, 4));  // D列
            auto cellE = wks.cell(OpenXLSX::XLCellReference(rowNumber, 5));  // E列

            float valueD = 0.0f, valueE = 0.0f;

            if (cellD.value().type() == OpenXLSX::XLValueType::Float ||
                cellD.value().type() == OpenXLSX::XLValueType::Integer)
                valueD = cellD.value().get<float>();

            if (cellE.value().type() == OpenXLSX::XLValueType::Float ||
                cellE.value().type() == OpenXLSX::XLValueType::Integer)
                valueE = cellE.value().get<float>();

            columnD.push_back(valueD);
            columnE.push_back(valueE);

            ++rowNumber;
        }

        doc.close();
    }
    catch (const std::exception& e) {
        std::cerr << "读取失败: " << e.what() << std::endl;
    }
}

 void ReadFile::readColumnCTimeOnly(
     const std::string& filePath,
     const std::string& sheetName,
     std::vector<std::tm>& timeList
 ) {
     try {
         OpenXLSX::XLDocument doc;
         doc.open(filePath);
         auto wks = doc.workbook().worksheet(sheetName);
         auto rows = wks.rows();

         uint64_t rowNumber = 1;
         for (const auto& row : rows) {
             if (rowNumber == 1) {  // 跳过第一行
                 ++rowNumber;
                 continue;
             }

             auto cell = wks.cell(OpenXLSX::XLCellReference(rowNumber, 3)); // C列

             std::string text;
             if (cell.value().type() == OpenXLSX::XLValueType::String)
                 text = cell.value().get<std::string>();
             else
                 continue;  // fallback

             // 提取时间部分（21:43:00）：
             std::smatch match;
             std::regex timeRegex(R"((\d{2}):(\d{2}):(\d{2}))");
             if (std::regex_search(text, match, timeRegex)) {
                 std::tm t = {};
                 t.tm_hour = std::stoi(match[1].str());
                 t.tm_min = std::stoi(match[2].str());
                 t.tm_sec = std::stoi(match[3].str());

                 timeList.push_back(t);
             }

             ++rowNumber;
         }

         doc.close();
     }
     catch (const std::exception& e) {
         std::cerr << "读取失败: " << e.what() << std::endl;
     }
 }


