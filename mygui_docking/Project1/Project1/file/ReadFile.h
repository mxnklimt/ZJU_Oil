#include<vector>
#include<string>
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