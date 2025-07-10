#include<vector>
#include<string>
#include<chrono>
#include<map>
#include<unordered_map>
#include"data/Data.h"
class ReadFile
{
public:
    static void readColumnsDandEFloat(const std::string& filePath,
        const std::string& sheetName,
        std::vector<float>& columnD,
        std::vector<float>& columnE);
    static void readColumnCTimeOnly(
        const std::string& filePath,
        const std::string& sheetName,
        std::vector<std::tm>& timeList
    );
};
std::string generateUniqueFileName(const std::string& baseName, const std::string& extension = ".xlsx");
void SaveJY61PToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::unordered_map<uint8_t, JY61PData::angle>>>& data);
void SaveADXL355ToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, ADXL355Parser::AccelerationData>>>& data);
void SaveDualAxisToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, DualAxisSensorParser::AngleData>>>& data);