#include<vector>
#include<string>
#include<chrono>
#include<map>
#include<unordered_map>
#include <filesystem>

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

//void SaveDualAxisToXLSX(const std::vector<std::pair<std::chrono::system_clock::time_point,
//    std::map<uint8_t, DualAxisSensorParser::AngleData>>>& data);
void SaveDualAxisToXLSX(
    const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, DualAxisSensorParser::AngleData>>>& collectedData,
    const std::vector<uint8_t>& deviceAddresses,
    const std::string& baseFileName,
    std::mutex& collectedDataMutex
);
void StopAndSaveDualAxisData(
    std::atomic<bool>& isCollectingData,
    std::thread& dataCollectionThread,
    const std::vector<uint8_t>& deviceAddresses,
    const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, DualAxisSensorParser::AngleData>>>& collectedData,
    std::mutex& collectedDataMutex,
    const std::string& baseFileName = "DualAxis_data"
);
void SaveLaserToXLSX(
    const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::unordered_map<uint8_t, std::vector<std::pair<std::chrono::system_clock::time_point, uint32_t>>>>>& data);
void MoveLatestFiles(const std::filesystem::path& basePath, size_t count);
std::string generateTimestampFolderName();
void SaveAMTToXLSX(
    const std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, AMTData>>>& data);
void SaveBSQJNToXLSX(
    const std::vector<std::pair<
    std::chrono::system_clock::time_point,
    std::map<uint8_t, std::vector<float>>
    >>&data);