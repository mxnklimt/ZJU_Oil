#include <iostream>
#include <vector>
#include <string>

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
//void SaveDualAxisToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
//    std::map<uint8_t, DualAxisSensorParser::AngleData>>>& data) {
//    std::string saveFilePath = generateUniqueFileName("DualAxis_sync");
//    OpenXLSX::XLDocument doc;
//    doc.create(saveFilePath, false);
//    doc.open(saveFilePath);
//    auto wks = doc.workbook().worksheet("Sheet1");
//
//    wks.cell(1, 1).value() = "Time";
//    int col = 2;
//    for (uint8_t addr : dualAxisDeviceAddresses) {
//        std::stringstream ss;
//        ss << "Device 0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)addr;
//        wks.cell(1, col++) = ss.str() + " Filtered Horizontal";
//        wks.cell(1, col++) = ss.str() + " Filtered Vertical";
//        wks.cell(1, col++) = ss.str() + " Raw Horizontal";
//        wks.cell(1, col++) = ss.str() + " Raw Vertical";
//    }
//
//    for (size_t row = 0; row < data.size(); ++row) {
//        auto time_t = std::chrono::system_clock::to_time_t(data[row].first);
//        std::tm tm; localtime_s(&tm, &time_t);
//        std::ostringstream oss; oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
//        wks.cell(row + 2, 1).value() = oss.str();
//
//        int col = 2;
//        for (uint8_t addr : dualAxisDeviceAddresses) {
//            if (data[row].second.count(addr)) {
//                auto& val = data[row].second.at(addr);
//                wks.cell(row + 2, col++) = val.filtered_horizontal;
//                wks.cell(row + 2, col++) = val.filtered_vertical;
//                wks.cell(row + 2, col++) = val.raw_horizontal;
//                wks.cell(row + 2, col++) = val.raw_vertical;
//            }
//            else col += 4;
//        }
//    }
//    doc.save(); doc.close();
//}
void SaveADXL355ToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, ADXL355Parser::AccelerationData>>>& data) {
    std::string saveFilePath = generateUniqueFileName("ADXL355_sync");
    OpenXLSX::XLDocument doc;
    doc.create(saveFilePath, false);
    doc.open(saveFilePath);
    auto wks = doc.workbook().worksheet("Sheet1");

    wks.cell(1, 1).value() = "Time";
    int col = 2;
    for (uint8_t addr : adxl355DeviceAddresses) {
        std::stringstream ss;
        ss << "Device 0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)addr;
        wks.cell(1, col++) = ss.str() + " Accel X";
        wks.cell(1, col++) = ss.str() + " Accel Y";
        wks.cell(1, col++) = ss.str() + " Accel Z";
    }

    for (size_t row = 0; row < data.size(); ++row) {
        auto time_t = std::chrono::system_clock::to_time_t(data[row].first);
        std::tm tm; localtime_s(&tm, &time_t);
        std::ostringstream oss; oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        wks.cell(row + 2, 1).value() = oss.str();

        int col = 2;
        for (uint8_t addr : adxl355DeviceAddresses) {
            if (data[row].second.count(addr)) {
                auto& val = data[row].second.at(addr);
                wks.cell(row + 2, col++) = val.x;
                wks.cell(row + 2, col++) = val.y;
                wks.cell(row + 2, col++) = val.z;
            }
            else col += 3;
        }
    }
    doc.save(); doc.close();
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
            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            std::tm tm;
            localtime_s(&tm, &time_t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            std::string timeStr = oss.str();

            for (uint8_t addr : deviceAddresses) {
                if (dataMap.count(addr)) {
                    const auto& angles = dataMap.at(addr);

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

        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"数据已保存到: %s", saveFilePath.c_str());
    }
    catch (const std::exception& e) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"保存失败: %s", e.what());
    }
}

void SaveJY61PToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::unordered_map<uint8_t, JY61PData::angle>>>& data) {
    std::string saveFilePath = generateUniqueFileName("JY61P_sync");
    OpenXLSX::XLDocument doc;
    doc.create(saveFilePath, false);
    doc.open(saveFilePath);
    auto wks = doc.workbook().worksheet("Sheet1");

    wks.cell(1, 1).value() = "Time";
    int col = 2;
    for (uint8_t addr : jy61pDeviceAddresses) {
        std::stringstream ss;
        ss << "Device 0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)addr;
        wks.cell(1, col++) = ss.str() + " Accel X";
        wks.cell(1, col++) = ss.str() + " Accel Y";
        wks.cell(1, col++) = ss.str() + " Accel Z";
        wks.cell(1, col++) = ss.str() + " Gyro X";
        wks.cell(1, col++) = ss.str() + " Gyro Y";
        wks.cell(1, col++) = ss.str() + " Gyro Z";
        wks.cell(1, col++) = ss.str() + " Angle X";
        wks.cell(1, col++) = ss.str() + " Angle Y";
        wks.cell(1, col++) = ss.str() + " Angle Z";
    }

    for (size_t row = 0; row < data.size(); ++row) {
        auto time_t = std::chrono::system_clock::to_time_t(data[row].first);
        std::tm tm; localtime_s(&tm, &time_t);
        std::ostringstream oss; oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        wks.cell(row + 2, 1).value() = oss.str();

        int col = 2;
        for (uint8_t addr : jy61pDeviceAddresses) {
            if (data[row].second.count(addr)) {
                auto& angle = data[row].second.at(addr);
                wks.cell(row + 2, col++) = angle.a[0];
                wks.cell(row + 2, col++) = angle.a[1];
                wks.cell(row + 2, col++) = angle.a[2];
                wks.cell(row + 2, col++) = angle.w[0];
                wks.cell(row + 2, col++) = angle.w[1];
                wks.cell(row + 2, col++) = angle.w[2];
                wks.cell(row + 2, col++) = angle.Angle[0];
                wks.cell(row + 2, col++) = angle.Angle[1];
                wks.cell(row + 2, col++) = angle.Angle[2];
            }
            else col += 9;
        }
    }
    doc.save(); doc.close();
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


