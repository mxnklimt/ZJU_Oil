//FibreGratingAnalyzerUI.h
#ifndef FIBER_GRATING_ANALYZER_UI_H
#define FIBER_GRATING_ANALYZER_UI_H

#include "udp/FiberGratingAnalyzer.h"
#include "udp/CSVDataLogger.h"  // 新增包含
#include <imgui.h>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

class FiberGratingAnalyzerUI {
public:
    FiberGratingAnalyzerUI();
    ~FiberGratingAnalyzerUI();

    void ShowUI();
    void SetAnalyzer(FiberGratingAnalyzer* analyzer);

private:
    void StartReceiving();
    void StopReceiving();
    void UpdateData(const std::vector<FiberGratingAnalyzer::SensorData>& sensors);
    void RenderControlPanel();
    void RenderDataDisplay();
    void RenderStatistics();
    void RenderCharts();
    void ApplyTemperatureCalibration(FiberGratingAnalyzer::SensorData& sensor);
    void RenderDataLoggingPanel();  // 新增函数

private:
    FiberGratingAnalyzer* analyzer_;
    CSVDataLogger dataLogger_;  // 新增数据记录器
    std::atomic<bool> isReceiving_;
    std::atomic<bool> isLoggingData_;  // 新增日志状态
    std::thread dataThread_;
    mutable std::mutex dataMutex_;

    // 实时数据存储
    std::vector<FiberGratingAnalyzer::SensorData> currentData_;
    std::map<int, std::vector<float>> wavelengthHistory_;
    std::map<int, std::vector<float>> physicalValueHistory_;

    // 显示设置
    int maxHistoryPoints_ = 100;
    bool showPhysicalValues_ = true;
    bool showWavelengthCharts_ = true;
    bool autoScaleCharts_ = true;

    // 统计信息
    int totalPackets_ = 0;
    int totalSensors_ = 0;
    std::chrono::steady_clock::time_point startTime_;

    // 数据记录设置
    char logFilename[256] = "";  // 自定义文件名

    // 温度校准参数 [6](@ref)
    struct TemperatureCalibration {
        float coefficient = 0.01f; // 温度系数 (nm/°C)
        float referenceWavelength = 1550.0f; // 参考波长 (nm)
        float referenceTemperature = 25.0f; // 参考温度 (°C)
    } tempCalibration_;
};

#endif // FIBER_GRATING_ANALYZER_UI_H
