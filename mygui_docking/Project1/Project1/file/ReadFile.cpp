#include <iostream>
#include <vector>
#include <string>
#include "ReadFile.h"
#include "OpenXLSX/OpenXLSX.hpp"
#include <regex>
#include <ctime>
#include <iomanip>
#include <sstream>
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


