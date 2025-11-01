////FiberGratingAnalyzerUI.cpp
//#include "udp/FiberGratingAnalyzerUI.h"
//#include <iomanip>
//#include <sstream>
//#include <algorithm>
//
//FiberGratingAnalyzerUI::FiberGratingAnalyzerUI()
//    : analyzer_(nullptr), isReceiving_(false) {
//}
//
//FiberGratingAnalyzerUI::~FiberGratingAnalyzerUI() {
//    StopReceiving();
//}
//
//void FiberGratingAnalyzerUI::SetAnalyzer(FiberGratingAnalyzer* analyzer) {
//    analyzer_ = analyzer;
//}
//
//void FiberGratingAnalyzerUI::ShowUI() {
//    if (!analyzer_) return;
//
//    ImGui::Begin(u8"光纤光栅分析器");
//
//    RenderControlPanel();
//    ImGui::Separator();
//
//    RenderDataDisplay();
//    ImGui::Separator();
//    RenderStatistics();
//    ImGui::Separator();
//    RenderCharts();
//
//    ImGui::End();
//}
//
//void FiberGratingAnalyzerUI::StartReceiving() {
//    if (isReceiving_ || !analyzer_) return;
//
//    // 设置端口 [1,6](@ref)
//    if (!analyzer_->setPort(8071)) {
//        ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"端口设置失败!");
//        return;
//    }
//
//    isReceiving_ = true;
//    startTime_ = std::chrono::steady_clock::now();
//    totalPackets_ = 0;
//    totalSensors_ = 0;
//
//    // 清空历史数据
//    {
//        std::lock_guard<std::mutex> lock(dataMutex_);
//        currentData_.clear();
//        wavelengthHistory_.clear();
//        physicalValueHistory_.clear();
//    }
//
//    // 启动数据接收线程
//    dataThread_ = std::thread([this]() {
//        auto callback = [this](const std::vector<FiberGratingAnalyzer::SensorData>& sensors) {
//            this->UpdateData(sensors);
//            };
//
//        analyzer_->startReceiving(callback);
//        });
//
//    ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"开始接收数据...");
//}
//
//void FiberGratingAnalyzerUI::StopReceiving() {
//    if (!isReceiving_ || !analyzer_) return;
//
//    isReceiving_ = false;
//    analyzer_->stopReceiving();
//
//    if (dataThread_.joinable()) {
//        dataThread_.join();
//    }
//}
//
//void FiberGratingAnalyzerUI::UpdateData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors) {
//    std::lock_guard<std::mutex> lock(dataMutex_);
//
//    totalPackets_++;
//    totalSensors_ += sensors.size();
//    currentData_ = sensors;
//
//
//    // 记录数据到CSV文件
//    if (isLoggingData_) {
//        dataLogger_.logSensorData(sensors);
//    }
//    // 更新历史数据用于图表显示
//    auto currentTime = std::chrono::steady_clock::now();
//    auto timePoint = std::chrono::duration_cast<std::chrono::milliseconds>(
//        currentTime - startTime_).count() / 1000.0f;
//
//    for (const auto& sensor : sensors) {
//        int sensorId = sensor.channel * 100 + sensor.sequence;
//
//        // 更新波长历史
//        wavelengthHistory_[sensorId].push_back(sensor.wavelength);
//        if (wavelengthHistory_[sensorId].size() > maxHistoryPoints_) {
//            wavelengthHistory_[sensorId].erase(wavelengthHistory_[sensorId].begin());
//        }
//
//        // 更新物理量历史（如果有）
//        if (sensor.hasPhysicalValue) {
//            physicalValueHistory_[sensorId].push_back(sensor.physicalValue);
//            if (physicalValueHistory_[sensorId].size() > maxHistoryPoints_) {
//                physicalValueHistory_[sensorId].erase(physicalValueHistory_[sensorId].begin());
//            }
//        }
//    }
//}
//
//void FiberGratingAnalyzerUI::RenderControlPanel() {
//    ImGui::Text(u8"控制面板");
//
//    if (!isReceiving_) {
//        if (ImGui::Button(u8"开始接收")) {
//            StartReceiving();
//        }
//    }
//    else {
//        if (ImGui::Button(u8"停止接收")) {
//            StopReceiving();
//        }
//        ImGui::SameLine();
//        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"运行中...");
//    }
//
//    ImGui::SameLine();
//    ImGui::Checkbox(u8"显示物理量", &showPhysicalValues_);
//    ImGui::SameLine();
//    ImGui::Checkbox(u8"显示图表", &showWavelengthCharts_);
//    ImGui::SameLine();
//    ImGui::Checkbox(u8"自动缩放", &autoScaleCharts_);
//
//    // 温度校准设置 [6](@ref)
//    if (ImGui::TreeNode(u8"温度校准设置")) {
//        ImGui::InputFloat(u8"温度系数 (nm/°C)", &tempCalibration_.coefficient);
//        ImGui::InputFloat(u8"参考波长 (nm)", &tempCalibration_.referenceWavelength);
//        ImGui::InputFloat(u8"参考温度 (°C)", &tempCalibration_.referenceTemperature);
//        ImGui::TreePop();
//    }
//}
//
//void FiberGratingAnalyzerUI::RenderDataDisplay() {
//    ImGui::Text(u8"实时数据");
//
//    std::lock_guard<std::mutex> lock(dataMutex_);
//
//    if (currentData_.empty()) {
//        ImGui::Text(u8"暂无数据");
//        return;
//    }
//
//    // 创建表格显示数据
//    if (ImGui::BeginTable(u8"SensorData", showPhysicalValues_ ? 5 : 4,
//        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
//        // 表头
//        ImGui::TableSetupColumn(u8"通道");
//        ImGui::TableSetupColumn(u8"序列号");
//        ImGui::TableSetupColumn(u8"波长 (nm)");
//        if (showPhysicalValues_) {
//            ImGui::TableSetupColumn(u8"物理量");
//        }
//        ImGui::TableSetupColumn(u8"状态");
//        ImGui::TableHeadersRow();
//
//        for (const auto& sensor : currentData_) {
//            ImGui::TableNextRow();
//
//            // 通道列
//            ImGui::TableSetColumnIndex(0);
//            ImGui::Text("%d", sensor.channel);
//
//            // 序列号列
//            ImGui::TableSetColumnIndex(1);
//            ImGui::Text("%d", sensor.sequence);
//
//            // 波长列
//            ImGui::TableSetColumnIndex(2);
//            ImGui::Text("%.3f", sensor.wavelength);
//
//            // 物理量列
//            if (showPhysicalValues_) {
//                ImGui::TableSetColumnIndex(3);
//                if (sensor.hasPhysicalValue) {
//                    // 应用温度校准 [6](@ref)
//                    float calibratedValue = sensor.physicalValue;
//                    //ApplyTemperatureCalibration(const_cast<FiberGratingAnalyzer::SensorData&>(sensor));
//                    ImGui::Text("%.4f", calibratedValue);
//                }
//                else {
//                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "N/A");
//                }
//            }
//
//            // 状态列
//            int statusColumn = showPhysicalValues_ ? 4 : 3;
//            ImGui::TableSetColumnIndex(statusColumn);
//
//            // 根据波长值判断状态 [2](@ref)
//            if (sensor.wavelength < 1520.0 || sensor.wavelength > 1620.0) {
//                ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"异常");
//            }
//            else {
//                ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"正常");
//            }
//        }
//
//        ImGui::EndTable();
//    }
//}
//
//void FiberGratingAnalyzerUI::RenderStatistics() {
//    ImGui::Text(u8"统计信息");
//
//    auto currentTime = std::chrono::steady_clock::now();
//    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
//        currentTime - startTime_).count();
//
//    ImGui::Text(u8"运行时间: %lld 秒", elapsedSeconds);
//    ImGui::Text(u8"接收数据包: %d", totalPackets_);
//    ImGui::Text(u8"传感器数据总数: %d", totalSensors_);
//
//    if (elapsedSeconds > 0) {
//        float packetRate = static_cast<float>(totalPackets_) / elapsedSeconds;
//        float sensorRate = static_cast<float>(totalSensors_) / elapsedSeconds;
//        ImGui::Text(u8"数据率: %.2f 包/秒, %.2f 传感器/秒", packetRate, sensorRate);
//    }
//}
//
//void FiberGratingAnalyzerUI::RenderCharts() {
//    if (!showWavelengthCharts_) return;
//
//    std::lock_guard<std::mutex> lock(dataMutex_);
//
//    if (wavelengthHistory_.empty()) return;
//
//    ImGui::Text(u8"波长变化趋势");
//
//    for (const auto& [sensorId, wavelengths] : wavelengthHistory_) {
//        if (wavelengths.empty()) continue;
//
//        int channel = sensorId / 100;
//        int sequence = sensorId % 100;
//
//        std::string label = u8"通道 " + std::to_string(channel) +
//            u8" - 传感器 " + std::to_string(sequence);
//
//        if (ImGui::TreeNode(label.c_str())) {
//            // 计算图表范围
//            float minVal = *std::min_element(wavelengths.begin(), wavelengths.end());
//            float maxVal = *std::max_element(wavelengths.begin(), wavelengths.end());
//            float range = maxVal - minVal;
//
//            if (range < 0.1f) range = 0.1f; // 最小范围
//
//            ImVec2 graphSize(-1, 100);
//
//            // 波长图表
//            ImGui::Text(u8"波长 (nm): %.3f", wavelengths.back());
//            if (ImGui::BeginChild(("WaveChart_" + std::to_string(sensorId)).c_str(),
//                graphSize, false)) {
//                ImGui::PlotLines("", wavelengths.data(), wavelengths.size(),
//                    0, nullptr, minVal - range * 0.1f,
//                    maxVal + range * 0.1f, graphSize);
//            }
//            ImGui::EndChild();
//
//            // 物理量图表（如果有）
//            if (showPhysicalValues_ && physicalValueHistory_.count(sensorId)) {
//                const auto& physicalValues = physicalValueHistory_.at(sensorId);
//                if (!physicalValues.empty()) {
//                    ImGui::Text(u8"物理量: %.4f", physicalValues.back());
//                    if (ImGui::BeginChild(("PhysChart_" + std::to_string(sensorId)).c_str(),
//                        graphSize, false)) {
//                        float pMin = *std::min_element(physicalValues.begin(), physicalValues.end());
//                        float pMax = *std::max_element(physicalValues.begin(), physicalValues.end());
//                        float pRange = pMax - pMin;
//                        if (pRange < 0.1f) pRange = 0.1f;
//
//                        ImGui::PlotLines("", physicalValues.data(), physicalValues.size(),
//                            0, nullptr, pMin - pRange * 0.1f,
//                            pMax + pRange * 0.1f, graphSize);
//                    }
//                    ImGui::EndChild();
//                }
//            }
//
//            ImGui::TreePop();
//        }
//    }
//}
//
//void FiberGratingAnalyzerUI::ApplyTemperatureCalibration(FiberGratingAnalyzer::SensorData& sensor) {
//    if (sensor.hasPhysicalValue) {
//        // 基于布拉格光栅温度特性进行校准 [6](@ref)
//        float wavelengthShift = sensor.wavelength - tempCalibration_.referenceWavelength;
//        float temperatureChange = wavelengthShift / tempCalibration_.coefficient;
//        sensor.physicalValue = tempCalibration_.referenceTemperature + temperatureChange;
//    }
//}


#include "FiberGratingAnalyzerUI.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

FiberGratingAnalyzerUI::FiberGratingAnalyzerUI()
    : analyzer_(nullptr), isReceiving_(false), isLoggingData_(false) {
    // 初始化文件名输入框
    strcpy_s(logFilename, sizeof(logFilename), "");
}

FiberGratingAnalyzerUI::~FiberGratingAnalyzerUI() {
    StopReceiving();
    StopDataLogging();
}

void FiberGratingAnalyzerUI::SetAnalyzer(FiberGratingAnalyzer* analyzer) {
    analyzer_ = analyzer;
}

void FiberGratingAnalyzerUI::ShowUI() {
    if (!analyzer_) return;

    ImGui::Begin(u8"光纤光栅分析器");

    RenderControlPanel();
    ImGui::Separator();
    RenderDataLoggingPanel();  // 新增数据记录面板
    ImGui::Separator();
    RenderDataDisplay();
    ImGui::Separator();
    RenderStatistics();
    ImGui::Separator();
    RenderCharts();

    ImGui::End();
}

void FiberGratingAnalyzerUI::StartReceiving() {
    if (isReceiving_ || !analyzer_) return;

    if (!analyzer_->setPort(8071)) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"端口设置失败!");
        return;
    }

    isReceiving_ = true;
    startTime_ = std::chrono::steady_clock::now();
    totalPackets_ = 0;
    totalSensors_ = 0;

    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        currentData_.clear();
        wavelengthHistory_.clear();
        physicalValueHistory_.clear();
    }

    dataThread_ = std::thread([this]() {
        auto callback = [this](const std::vector<FiberGratingAnalyzer::SensorData>& sensors) {
            this->UpdateData(sensors);
            };

        analyzer_->startReceiving(callback);
        });

    ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"开始接收数据...");
}

void FiberGratingAnalyzerUI::StopReceiving() {
    if (!isReceiving_ || !analyzer_) return;

    isReceiving_ = false;
    analyzer_->stopReceiving();

    if (dataThread_.joinable()) {
        dataThread_.join();
    }
}

void FiberGratingAnalyzerUI::StartDataLogging() {
    if (isLoggingData_) {
        return;
    }

    std::string filename = logFilename[0] != '\0' ? std::string(logFilename) : "";
    if (dataLogger_.startLogging(filename)) {
        isLoggingData_ = true;
    }
}

void FiberGratingAnalyzerUI::StopDataLogging() {
    if (!isLoggingData_) {
        return;
    }

    dataLogger_.stopLogging();
    isLoggingData_ = false;
}

void FiberGratingAnalyzerUI::UpdateData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors) {
    std::lock_guard<std::mutex> lock(dataMutex_);

    totalPackets_++;
    totalSensors_ += sensors.size();
    currentData_ = sensors;

    // 记录数据到CSV文件
    if (isLoggingData_) {
        dataLogger_.logSensorData(sensors);
    }

    // 更新历史数据用于图表显示
    auto currentTime = std::chrono::steady_clock::now();
    auto timePoint = std::chrono::duration_cast<std::chrono::milliseconds>(
        currentTime - startTime_).count() / 1000.0f;

    for (const auto& sensor : sensors) {
        int sensorId = sensor.channel * 100 + sensor.sequence;

        wavelengthHistory_[sensorId].push_back(sensor.wavelength);
        if (wavelengthHistory_[sensorId].size() > maxHistoryPoints_) {
            wavelengthHistory_[sensorId].erase(wavelengthHistory_[sensorId].begin());
        }

        if (sensor.hasPhysicalValue) {
            physicalValueHistory_[sensorId].push_back(sensor.physicalValue);
            if (physicalValueHistory_[sensorId].size() > maxHistoryPoints_) {
                physicalValueHistory_[sensorId].erase(physicalValueHistory_[sensorId].begin());
            }
        }
    }
}

void FiberGratingAnalyzerUI::RenderControlPanel() {
    ImGui::Text(u8"控制面板");

    if (!isReceiving_) {
        if (ImGui::Button(u8"开始接收")) {
            StartReceiving();
        }
    }
    else {
        if (ImGui::Button(u8"停止接收")) {
            StopReceiving();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"运行中...");
    }

    ImGui::SameLine();
    ImGui::Checkbox(u8"显示物理量", &showPhysicalValues_);
    ImGui::SameLine();
    ImGui::Checkbox(u8"显示图表", &showWavelengthCharts_);
    ImGui::SameLine();
    ImGui::Checkbox(u8"自动缩放", &autoScaleCharts_);
}

// 新增数据记录面板渲染函数
void FiberGratingAnalyzerUI::RenderDataLoggingPanel() {
    ImGui::Text(u8"数据记录");

    // 文件名输入
    ImGui::Text(u8"文件名 (留空使用自动生成):");
    ImGui::InputText("##filename", logFilename, sizeof(logFilename));

    // 控制按钮
    if (!isLoggingData_) {
        if (ImGui::Button(u8"开始记录")) {
            StartDataLogging();
        }
    }
    else {
        if (ImGui::Button(u8"停止记录")) {
            StopDataLogging();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"记录中...");
    }

    // 显示记录状态信息
    if (isLoggingData_) {
        ImGui::SameLine();
        ImGui::Text(u8"文件: %s", dataLogger_.getCurrentFilename().c_str());
        ImGui::SameLine();
        ImGui::Text(u8"已记录: %zu 条数据", dataLogger_.getLoggedCount());
    }

    // 温度校准设置
    if (ImGui::TreeNode(u8"温度校准设置")) {
        ImGui::InputFloat(u8"温度系数 (nm/°C)", &tempCalibration_.coefficient);
        ImGui::InputFloat(u8"参考波长 (nm)", &tempCalibration_.referenceWavelength);
        ImGui::InputFloat(u8"参考温度 (°C)", &tempCalibration_.referenceTemperature);
        ImGui::TreePop();
    }
}

void FiberGratingAnalyzerUI::RenderDataDisplay() {
    ImGui::Text(u8"实时数据");

    std::lock_guard<std::mutex> lock(dataMutex_);

    if (currentData_.empty()) {
        ImGui::Text(u8"暂无数据");
        return;
    }

    // 创建表格显示数据
    if (ImGui::BeginTable(u8"SensorData", showPhysicalValues_ ? 5 : 4,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        // 表头
        ImGui::TableSetupColumn(u8"通道");
        ImGui::TableSetupColumn(u8"序列号");
        ImGui::TableSetupColumn(u8"波长 (nm)");
        if (showPhysicalValues_) {
            ImGui::TableSetupColumn(u8"物理量");
        }
        ImGui::TableSetupColumn(u8"状态");
        ImGui::TableHeadersRow();

        for (const auto& sensor : currentData_) {
            ImGui::TableNextRow();

            // 通道列
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", sensor.channel);

            // 序列号列
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", sensor.sequence);

            // 波长列
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", sensor.wavelength);

            // 物理量列
            if (showPhysicalValues_) {
                ImGui::TableSetColumnIndex(3);
                if (sensor.hasPhysicalValue) {
                    // 应用温度校准 [6](@ref)
                    float calibratedValue = sensor.physicalValue;
                    //ApplyTemperatureCalibration(const_cast<FiberGratingAnalyzer::SensorData&>(sensor));
                    ImGui::Text("%.4f", calibratedValue);
                }
                else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "N/A");
                }
            }

            // 状态列
            int statusColumn = showPhysicalValues_ ? 4 : 3;
            ImGui::TableSetColumnIndex(statusColumn);

            // 根据波长值判断状态 [2](@ref)
            if (sensor.wavelength < 1520.0 || sensor.wavelength > 1620.0) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"异常");
            }
            else {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"正常");
            }
        }

        ImGui::EndTable();
    }
}

void FiberGratingAnalyzerUI::RenderStatistics() {
    ImGui::Text(u8"统计信息");

    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        currentTime - startTime_).count();

    ImGui::Text(u8"运行时间: %lld 秒", elapsedSeconds);
    ImGui::Text(u8"接收数据包: %d", totalPackets_);
    ImGui::Text(u8"传感器数据总数: %d", totalSensors_);

    if (elapsedSeconds > 0) {
        float packetRate = static_cast<float>(totalPackets_) / elapsedSeconds;
        float sensorRate = static_cast<float>(totalSensors_) / elapsedSeconds;
        ImGui::Text(u8"数据率: %.2f 包/秒, %.2f 传感器/秒", packetRate, sensorRate);
    }
}

void FiberGratingAnalyzerUI::RenderCharts() {
    if (!showWavelengthCharts_) return;

    std::lock_guard<std::mutex> lock(dataMutex_);

    if (wavelengthHistory_.empty()) return;

    ImGui::Text(u8"波长变化趋势");

    for (const auto& [sensorId, wavelengths] : wavelengthHistory_) {
        if (wavelengths.empty()) continue;

        int channel = sensorId / 100;
        int sequence = sensorId % 100;

        std::string label = u8"通道 " + std::to_string(channel) +
            u8" - 传感器 " + std::to_string(sequence);

        if (ImGui::TreeNode(label.c_str())) {
            // 计算图表范围
            float minVal = *std::min_element(wavelengths.begin(), wavelengths.end());
            float maxVal = *std::max_element(wavelengths.begin(), wavelengths.end());
            float range = maxVal - minVal;

            if (range < 0.1f) range = 0.1f; // 最小范围

            ImVec2 graphSize(-1, 100);

            // 波长图表
            ImGui::Text(u8"波长 (nm): %.3f", wavelengths.back());
            if (ImGui::BeginChild(("WaveChart_" + std::to_string(sensorId)).c_str(),
                graphSize, false)) {
                ImGui::PlotLines("", wavelengths.data(), wavelengths.size(),
                    0, nullptr, minVal - range * 0.1f,
                    maxVal + range * 0.1f, graphSize);
            }
            ImGui::EndChild();

            // 物理量图表（如果有）
            if (showPhysicalValues_ && physicalValueHistory_.count(sensorId)) {
                const auto& physicalValues = physicalValueHistory_.at(sensorId);
                if (!physicalValues.empty()) {
                    ImGui::Text(u8"物理量: %.4f", physicalValues.back());
                    if (ImGui::BeginChild(("PhysChart_" + std::to_string(sensorId)).c_str(),
                        graphSize, false)) {
                        float pMin = *std::min_element(physicalValues.begin(), physicalValues.end());
                        float pMax = *std::max_element(physicalValues.begin(), physicalValues.end());
                        float pRange = pMax - pMin;
                        if (pRange < 0.1f) pRange = 0.1f;

                        ImGui::PlotLines("", physicalValues.data(), physicalValues.size(),
                            0, nullptr, pMin - pRange * 0.1f,
                            pMax + pRange * 0.1f, graphSize);
                    }
                    ImGui::EndChild();
                }
            }

            ImGui::TreePop();
        }
    }
}

void FiberGratingAnalyzerUI::ApplyTemperatureCalibration(FiberGratingAnalyzer::SensorData& sensor) {
    if (sensor.hasPhysicalValue) {
        // 基于布拉格光栅温度特性进行校准 [6](@ref)
        float wavelengthShift = sensor.wavelength - tempCalibration_.referenceWavelength;
        float temperatureChange = wavelengthShift / tempCalibration_.coefficient;
        sensor.physicalValue = tempCalibration_.referenceTemperature + temperatureChange;
    }
}