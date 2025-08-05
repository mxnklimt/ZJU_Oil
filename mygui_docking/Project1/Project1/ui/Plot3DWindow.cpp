#undef x  // 取消冲突宏

#include<iostream>
#include<vector>
#include <regex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include<cmath>

#define NOMINMAX
#include<Windows.h>
#include<algorithm>
#include<filesystem>
#include <atomic>
#include <map>
#include <unordered_map>
#include <chrono>
#include "Plot3DWindow.h"
#include"ImPlot3d/implot3d.h"
#include"ImPlot/implot.h"
#include"ImPlot/implot_internal.h"
#include "ImPlot3d/implot3d_internal.h"
#include "OpenXLSX/OpenXLSX.hpp"
#include "file/ReadFile.h"
#include"ui/sheetdata.h"
#include"data/Data.h"
#include"JY61P/REG.h"
#include"JY61P/Com.h"
#include"wit_c_sdk.h"
#include"DualAxisSensor/DualAxisSensorParser.h"
#include"data/CollectData.h"
#include"LaserSensor/LaserSensorProtocol.h"
#include"EMA/EmaFilter.h"
void Application::ShowWindow()
{
    static bool show_plot2d = true;
    static bool ADXL355 = true;
    static bool JY61P = true; // 默认显示JY61P数据
    static bool show_plot3d_2_window = true;  // 注意：控制的是独立窗口
    static bool show_plot3d_2 = true;
    static bool DualAxis = true;
    static bool SynchronizedCapture = true;
	static bool show_laser_sensor = true; // 激光传感器选项
    //--------------------------------------------------------------------------------------------------------------------------------
    // 主窗口
    ImGui::SetNextWindowPos(ImVec2(-1, -1), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_FirstUseEver);
    ImGui::Begin("2D Plot", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem(u8"管道位移/岸坡沉降", nullptr, &show_plot2d);
            ImGui::MenuItem("ADXL355", nullptr, &ADXL355);
            ImGui::MenuItem("JY61P", nullptr, &JY61P);
            ImGui::MenuItem("DualAxis", nullptr, &DualAxis);
			ImGui::MenuItem("LaserSensor", nullptr, &show_laser_sensor); // 显示激光传感器选项
            ImGui::MenuItem("SynchronizedCapture", nullptr, &SynchronizedCapture);
            ImGui::MenuItem("Show Custom 3D Plot 2", nullptr, &show_plot3d_2);
            ImGui::MenuItem("Show Custom 3D Plot 2 (Separate Window)", nullptr, &show_plot3d_2_window);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Style")) {
            if (ImGui::MenuItem("Dark")) ImGui::StyleColorsDark();
            if (ImGui::MenuItem("Light")) ImGui::StyleColorsLight();
            if (ImGui::MenuItem("Classic")) ImGui::StyleColorsClassic();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    //--------------------------------------------------------------------------------------------------------------------------------
    //读取Excel数据，显示图
    Application::ShowExcel();

    //-------------------------------------------------------------------------------
	if (show_laser_sensor) {
		// 显示激光传感器数据
		Application::ShowLaserSensor();
	}
    
    //--------------------------------------------------------------------------------------------------------------------------------
    // ADXL355
    if (ADXL355) {
        Application::ShowADXL355();
    }
    //--------------------------------------------------------------------------------------------------------------------------------
    // JY61P
    if (JY61P)
    {
        Application::ShowJY61P();
    }

    //--------------------------------------------------------------------------------------------------------------------------------
    // 独立窗口：3D管道图像，光源建模
    if (show_plot3d_2_window) {
        //ImGui::SetNextWindowSize(ImVec2(-1, -1), ImGuiCond_FirstUseEver);
        ImGui::Begin("3D Plot 2 - Cylinder", &show_plot3d_2_window); // 可关闭窗口
        Application::CylinderPlots();
        ImGui::End();
    }

    if (DualAxis)
    {
        Application::ShowDualAxisSensor();
    }
    if (SynchronizedCapture)
    {
        Application::ShowSynchronizedCapture();
    }
    //try
    //{
    //    RS485Manager rs485_DualAxis;
    //    rs485_DualAxis.open("COM7", 115200);

    //    DualAxisSensorParser sensor(rs485_DualAxis);

    //    // 读取角度
    //    auto angles = sensor.readAngles();
    //    std::cout << "Horizontal =  " << angles.filtered_horizontal << "°" << std::endl;
    //    std::cout << "Vertical = " << angles.filtered_vertical << "°" << std::endl;
    //    std::cout << "raw_horizonta = " << angles.raw_horizontal << "°" << std::endl;
    //    std::cout << "raw_vertical = " << angles.raw_vertical << "°" << std::endl;

    //    // 设置采样率
    //    sensor.setSamplingRate(DualAxisSensorParser::SamplingRate::ADS_10_Hz);

    //    // 执行校准
    //    sensor.performCalibration(DualAxisSensorParser::CalibrationCommand::CLEAR_CALIBRATION);
    //    // ... 其他校准步骤

    //    // 读取设备信息
    //    std::cout << "Device info: " << sensor.readDeviceInfo() << std::endl;
    //}
    //catch (const std::exception& e)
    //{
    //    std::cerr << "Error: " << e.what() << std::endl;
    //}
    //--------------------------------------------------------------------------------------------------------------------------------
}

void Application::ShowExcel()
{
    if (ImGui::Button(u8"加载Excel数据")) {
        loadExcelDataAsync();
    }

    checkLoadingStatus();

    std::vector<float> d, e;
    std::vector<std::tm> ts;

    {
        std::lock_guard<std::mutex> lock(sheetDataMutex);
        d = dValues_save;
        e = eValues_save;
        ts = times_save;
        size_t minSize = std::min({ d.size(), e.size(), ts.size() });
        d.resize(minSize);
        e.resize(minSize);
        ts.resize(minSize);

    }


    if (dataLoadingDone) {
        ImGui::Text(u8"数据加载完成，行数：%d", (int)d.size() + (int)e.size());

        if (d.size() > 0) {
            // 构造绘图数据
            static std::vector<float> time_xf, y_d, y_e;
            size_t count = d.size();
            time_xf.resize(count);
            y_d.resize(count);
            y_e.resize(count);

            for (size_t i = 0; i < count; ++i) {
                const std::tm& t = ts[i];
                float seconds = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
                time_xf[i] = seconds;
                y_d[i] = d[i];
                y_e[i] = e[i];
            }

            ImPlotFormatter TimeFormatter = [](double seconds, char* buf, int size, void*) -> int {
                int h = (int)seconds / 3600;
                int m = ((int)seconds % 3600) / 60;
                int s = (int)seconds % 60;
                return snprintf(buf, size, "%02d:%02d:%02d", h, m, s);
                };
            if (ImGui::CollapsingHeader(u8"管道水平位移")) {
                if (ImPlot::BeginPlot(u8"管道水平位移图")) {
                    ImPlot::SetupAxes(u8"时间", u8"水平位移");
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 86500.0, ImGuiCond_Always);  // 限制时间轴显示一天
                    ImPlot::SetupAxisFormat(ImAxis_X1, TimeFormatter);
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 0.4f, 1.0f, 1.0f));
                    ImPlot::PlotLine(u8"水平位移", time_xf.data(), y_d.data(), (int)count);
                    ImPlot::PopStyleColor();
                    ImPlot::EndPlot();
                }
            }

            if (ImGui::CollapsingHeader(u8"岸坡沉降位移")) {
                if (ImPlot::BeginPlot(u8"岸坡沉降位移图")) {
                    ImPlot::SetupAxes(u8"时间", u8"沉降位移");
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 86400.0, ImGuiCond_Always);  // 限制时间轴显示一天
                    ImPlot::SetupAxisFormat(ImAxis_X1, TimeFormatter);
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
                    ImPlot::PlotLine(u8"沉降位移", time_xf.data(), y_e.data(), (int)count);
                    ImPlot::PopStyleColor();
                    ImPlot::EndPlot();
                }
            }
        }
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"正在加载中...");
    }

    ImGui::End();
}
void Application::ShowSynchronizedCapture() {
    if (!ImGui::Begin(u8"同步采集控制")) {
        ImGui::End();
        return;
    }

    static std::atomic<bool> isSyncCollecting = false;
    static std::thread syncCollectionThread;

    // 用于显示当前记录条数
    static std::atomic<int> jy61pCount = 0;
    static std::atomic<int> adxl355Count = 0;
    static std::atomic<int> dualAxisCount = 0;

    //  显示状态栏
    {
        ImGui::Separator();
        ImGui::Text(u8"当前状态：");
        ImGui::SameLine();
        if (isSyncCollecting) {
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), u8"同步采集中");
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), u8"未采集");
        }

        ImGui::Text(u8"采样周期：1 秒");
        ImGui::Separator();
    }

    // 控制按钮区
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 200) * 0.5f);  // 居中按钮

    if (!isSyncCollecting) {
        if (ImGui::Button(u8"开始同步采集", ImVec2(200, 40))) {
            isSyncCollecting = true;

            syncCollectionThread = std::thread([] {
                std::vector<std::pair<std::chrono::system_clock::time_point, std::unordered_map<uint8_t, JY61PData::angle>>> jy61pBuffer;
                std::vector<std::pair<std::chrono::system_clock::time_point, std::map<uint8_t, ADXL355Parser::AccelerationData>>> adxl355Buffer;
                std::vector<std::pair<std::chrono::system_clock::time_point, std::map<uint8_t, DualAxisSensorParser::AngleData>>> dualAxisBuffer;

                while (isSyncCollecting) {
                    auto now = std::chrono::system_clock::now();

                    // JY61P
                    {
                        std::unordered_map<uint8_t, JY61PData::angle> snapshot;
                        std::lock_guard<std::mutex> lock(jy61pDataMutex);
                        for (auto& [addr, data] : jy61pDataMap)
                            if (!data.dataQue.empty()) snapshot[addr] = data.dataQue.back();
                        jy61pBuffer.emplace_back(now, snapshot);
                        jy61pCount = static_cast<int>(jy61pBuffer.size());
                    }

                    // ADXL355
                    {
                        std::map<uint8_t, ADXL355Parser::AccelerationData> snapshot;
                        std::lock_guard<std::mutex> lock(ADXL355Mutex);
                        for (auto& [addr, data] : adxl355DataMap)
                            if (!data.dataQue.empty()) snapshot[addr] = data.dataQue.back();
                        adxl355Buffer.emplace_back(now, snapshot);
                        adxl355Count = static_cast<int>(adxl355Buffer.size());
                    }

                    // Dual Axis
                    {
                        std::map<uint8_t, DualAxisSensorParser::AngleData> snapshot;
                        std::lock_guard<std::mutex> lock(dataMutex);
                        snapshot = dualAxisDataMap;
                        dualAxisBuffer.emplace_back(now, snapshot);
                        dualAxisCount = static_cast<int>(dualAxisBuffer.size());
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                SaveJY61PToXLSX(jy61pBuffer);
                SaveADXL355ToXLSX(adxl355Buffer);
                SaveDualAxisToXLSX(dualAxisBuffer);
                });
        }
    }
    else {
        if (ImGui::Button(u8" 停止采集并保存", ImVec2(200, 40))) {
            isSyncCollecting = false;
            if (syncCollectionThread.joinable()) syncCollectionThread.join();
        }

        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8" 同步采集中...");
        ImGui::TextColored(ImVec4(1, 1, 0, 1), u8" JY61P 已记录 %d 条", jy61pCount.load());
        ImGui::TextColored(ImVec4(0, 1, 1, 1), u8" ADXL355 已记录 %d 条", adxl355Count.load());
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), u8"双轴传感器 已记录 %d 条", dualAxisCount.load());
    }

    //  操作说明
    ImGui::Separator();
    ImGui::Text(u8"操作说明：");
    ImGui::BulletText(u8"每秒同步采集所有模块数据");
    ImGui::BulletText(u8"停止采集后自动保存为 .xlsx 文件");

    ImGui::End();
}




void SetJY61PAddress(uint8_t addr) {
    WitInit(WIT_PROTOCOL_MODBUS, addr);        // 切换地址
    WitSerialWriteRegister(SensorUartSend);    // 重新绑定串口写函数
    WitRegisterCallBack(CopeSensorData);       // 注册数据处理回调
}


//
////V1.0
void Application::CylinderPlots() {
    static constexpr int N = 240;
    static float radius = 381.0f / 1000.0f; // 半径，单位转换为米

    std::vector<Vec3> centers = {
        {0, 0, 0},
        {477, -2, 52},
        {954, -12, 105},
        {1430, -17, 163},
        {1906, -19, 222},
        {2383, -26, 279},
        {2858, -31, 346},
        {3337, -37, 318},
        {3816, -41, 284},
        {4294, -48, 235},
        {4769, -46, 170}
    };

    // 单位从 mm → m
    for (auto& p : centers) {
        p.x /= 1000.0f;
        p.y /= 1000.0f;
        p.z /= 1000.0f;
    }

    if (ImPlot3D::BeginPlot("Line Plots", ImVec2(-1, -1), 0)) {
        //ImPlot3D::SetupAxesLimits(0, 5, -1, 1, 0, 2);
        // 设置坐标范围：中间居中展示
        ImPlot3D::SetupAxesLimits(
            -0.2f, 5.4f,  // X轴范围（0～4.8居中）
            -1.4f, 1.2f,  // Y轴范围（Y比较小）
            -0.8f, 1.8f   // Z轴范围
        );
        //static ImVec4 colorPipe(0.3f, 0.6f, 0.9f, 0.8f); // color
        //static ImVec4 colorPipe(0.7f, 0.7f, 0.75f, 0.6f); // 类似金属铝管
        static ImVec4 colorPipe(0.75f, 0.75f, 0.78f, 0.8f); // 银灰色，略带反光
        // 锈蚀金属管道配色 (RGB值基于图片中的黄褐色锈迹)
        //static ImVec4 colorPipe(0.76f, 0.55f, 0.35f, 0.6f);
        //static ImVec4 colorPipe(0.6f, 0.7f, 0.9f, 0.7f); // 蓝灰色，像水管


        ImPlot3D::SetNextFillStyle(colorPipe);
        ImPlot3D::SetNextLineStyle(colorPipe, 1.5f);

        for (int i = 0; i < centers.size() - 1; ++i) {
            Vec3 A = centers[i];
            Vec3 B = centers[i + 1];

            Vec3 D = { B.x - A.x, B.y - A.y, B.z - A.z };
            float len = sqrtf(D.x * D.x + D.y * D.y + D.z * D.z);
            if (len < 1e-6f) continue;

            D.x /= len; D.y /= len; D.z /= len;

            Vec3 U = { -D.y, D.x, 0 };
            float uLen = sqrtf(U.x * U.x + U.y * U.y + U.z * U.z);
            if (uLen < 1e-6f) U = { 1, 0, 0 };
            else { U.x /= uLen; U.y /= uLen; U.z /= uLen; }

            Vec3 V = {
                D.y * U.z - D.z * U.y,
                D.z * U.x - D.x * U.z,
                D.x * U.y - D.y * U.x
            };

            float xs[N * 4], ys[N * 4], zs[N * 4];
            for (int j = 0; j < N; ++j) {
                float theta1 = 2.0f * IM_PI * j / N;
                float theta2 = 2.0f * IM_PI * (j + 1) / N;

                Vec3 p1 = {
                    A.x + radius * (cosf(theta1) * U.x + sinf(theta1) * V.x),
                    A.y + radius * (cosf(theta1) * U.y + sinf(theta1) * V.y),
                    A.z + radius * (cosf(theta1) * U.z + sinf(theta1) * V.z)
                };
                Vec3 p2 = {
                    A.x + radius * (cosf(theta2) * U.x + sinf(theta2) * V.x),
                    A.y + radius * (cosf(theta2) * U.y + sinf(theta2) * V.y),
                    A.z + radius * (cosf(theta2) * U.z + sinf(theta2) * V.z)
                };
                Vec3 q1_my = { p1.x + D.x * len, p1.y + D.y * len, p1.z + D.z * len };
                Vec3 q2_my = { p2.x + D.x * len, p2.y + D.y * len, p2.z + D.z * len };

                xs[j * 4 + 0] = p1.x; ys[j * 4 + 0] = p1.y; zs[j * 4 + 0] = p1.z;
                xs[j * 4 + 1] = p2.x; ys[j * 4 + 1] = p2.y; zs[j * 4 + 1] = p2.z;
                xs[j * 4 + 2] = q2_my.x; ys[j * 4 + 2] = q2_my.y; zs[j * 4 + 2] = q2_my.z;
                xs[j * 4 + 3] = q1_my.x; ys[j * 4 + 3] = q1_my.y; zs[j * 4 + 3] = q1_my.z;
            }

            /*for (int j = 0; j < N; ++j) {
                ImPlot3D::PlotQuad("PipeSeg", &xs[j * 4], &ys[j * 4], &zs[j * 4], 4);
            }*/
            // 假设一个光源方向，比如从上方和右前方斜射
            Vec3 lightDir = { 0.5f, 0.5f, 1.0f };
            float lightLen = sqrtf(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightLen; lightDir.y /= lightLen; lightDir.z /= lightLen;

            for (int j = 0; j < N; ++j) {
                // 小面片中心点（可以用p1）
                Vec3 p1 = { xs[j * 4 + 0], ys[j * 4 + 0], zs[j * 4 + 0] };
                Vec3 p2 = { xs[j * 4 + 1], ys[j * 4 + 1], zs[j * 4 + 1] };
                Vec3 p3 = { xs[j * 4 + 2], ys[j * 4 + 2], zs[j * 4 + 2] };

                // 用三个点估算法向量
                Vec3 u = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
                Vec3 v = { p3.x - p1.x, p3.y - p1.y, p3.z - p1.z };
                Vec3 normal = {
                    u.y * v.z - u.z * v.y,
                    u.z * v.x - u.x * v.z,
                    u.x * v.y - u.y * v.x
                };
                float nLen = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                if (nLen > 1e-6f) {
                    normal.x /= nLen; normal.y /= nLen; normal.z /= nLen;
                }

                // 亮度 = 法线和光照方向点积，调整到 0.3～1.0 之间
                float intensity = (normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
                intensity = 0.3f + 0.7f * std::max(0.0f, intensity);

                // 根据亮度动态调整颜色
                ImVec4 shadedColor(
                    colorPipe.x * intensity,
                    colorPipe.y * intensity,
                    colorPipe.z * intensity,
                    colorPipe.w
                );
                ImPlot3D::SetNextFillStyle(shadedColor);

                // 画这个小面片
                ImPlot3D::PlotQuad("PipeSeg", &xs[j * 4], &ys[j * 4], &zs[j * 4], 4);
            }

        }

      
        // ------------------------------
// 在每个圆心正上方 radius 距离处画一个红点
// ------------------------------
        std::vector<float> xs, ys, zs;
        for (const Vec3& c : centers) {
            xs.push_back(c.x);
            ys.push_back(c.y);
            zs.push_back(c.z + 1.2*radius);  // Z轴正上方
        }

        // 设置红色实心圆点样式
        ImPlot3D::SetNextMarkerStyle(ImPlot3DMarker_Circle,
            9.0f,                        // 大小
            ImVec4(1, 0, 0, 1),           // 填充颜色：红色
            0.0f,                         // 边框宽度
            ImVec4(0, 0, 0, 1));          // 外轮廓：黑色

        // 一次性绘制所有点
        ImPlot3D::PlotScatter("TopPoints", xs.data(), ys.data(), zs.data(), (int)xs.size());
        ImPlot3D::SetNextLineStyle(ImVec4(1, 0, 0, 1), 2.0f); // 红色线，线宽 2.0
        ImPlot3D::PlotLine("TopLine", xs.data(), ys.data(), zs.data(), (int)xs.size());


        ImPlot3D::EndPlot();
    }
}

void Application::ShowDualAxisSensor() {
    if (!ImGui::Begin(u8"双轴柔性传感器")) {
        ImGui::End();
        return;
    }

    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;
    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
    static int selectedBaudIndex = 4;
    static bool isConnected = false;

    static std::thread pollingThread;
    static std::atomic<bool> collecting = false;
    static std::unordered_map<uint8_t, bool> displayFlags;

    // 数据采集相关变量
    static std::atomic<bool> isCollectingData = false;
    static std::thread dataCollectionThread;
    static std::vector<std::pair<std::chrono::system_clock::time_point,
    std::map<uint8_t, DualAxisSensorParser::AngleData>>> collectedData;
    static std::mutex collectedDataMutex;
    static std::string saveFilePath = "DualAxisSensor_data.xlsx";

    // 串口选择下拉框
    ShowSerialPortSelector(availablePorts, selectedPortIndex);
    // 波特率选择下拉框
    ShowBaudRateSelector(baudRates, IM_ARRAYSIZE(baudRates), selectedBaudIndex);

    if (!isConnected) {
        if (ImGui::Button(u8"连接")) {
            try {
                DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
                //打开选择的串口号，使用选择的波特率
                serialDualAxis.open(availablePorts[selectedPortIndex], baudRate);
                isConnected = true;
                collecting = true;
				// 初始化设备地址列表
                initializedualAxisParsers();
				// 启动数据采集线程
                pollingThread = std::thread([] {
                    while (collecting) {
                        for (uint8_t addr : dualAxisDeviceAddresses) {
                            try {
                                auto& parser = *dualAxisParsers[addr];
                                auto angles = parser.readAngles();

                                // 使用外部的 emaFilterManager 和 alpha
                                emaFilterManager.update(addr, angles.filtered_horizontal, angles.filtered_vertical, alpha);
                                angles.EMA_horizontal = emaFilterManager.getHorizontal(addr);
                                angles.EMA_vertical = emaFilterManager.getVertical(addr);

                                {
                                    std::lock_guard<std::mutex> lock(dataMutex);
                                    dualAxisDataMap[addr] = angles;
                                }
                            }
                            catch (const std::exception& e) {
                                std::cerr << "[设备 0x" << std::hex << (int)addr << "] 读取失败: " << e.what() << std::endl;
                            }

                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }
                    });
            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"连接失败: %s", e.what());
            }
        }
    }
    else {
        if (ImGui::Button(u8"断开")) {
            collecting = false;
            isCollectingData = false;
            if (pollingThread.joinable()) pollingThread.join();
            if (dataCollectionThread.joinable()) dataCollectionThread.join();
            serialDualAxis.close();
            isConnected = false;
            dualAxisParsers.clear();
            dualAxisDataMap.clear();
            displayFlags.clear();
        }
        // 数据采集控制按钮
        if (!isCollectingData) {
            if (ImGui::Button(u8"开始采集")) {
                isCollectingData = true;
                collectedData.clear();

                // 启动数据采集线程
                dataCollectionThread = std::thread([&]() {
                    while (isCollectingData) {
                        auto now = std::chrono::system_clock::now();

                        std::map<uint8_t, DualAxisSensorParser::AngleData> currentData;
                        {
                            std::lock_guard<std::mutex> lock(dataMutex);
                            currentData = dualAxisDataMap;
                        }

                        {
                            std::lock_guard<std::mutex> lock(collectedDataMutex);
                            collectedData.emplace_back(now, currentData);
                        }

                        std::this_thread::sleep_for(std::chrono::seconds(10));
                    }
                    });
            }
        }
        else {
            if (ImGui::Button(u8"停止采集并保存")) {
                StopAndSaveDualAxisData(
                    isCollectingData,
                    dataCollectionThread,
                    dualAxisDeviceAddresses,
                    collectedData,
                    collectedDataMutex
                );
            }
            // 显示采集状态
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"正在采集数据... 已记录 %d 条",

                static_cast<int>(collectedData.size()));
        }

        ImGui::Separator();

        // 开始左右分栏布局

        ImGui::Columns(2, "MainColumns", true);
        ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.2f);
        ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() * 0.8f);
        // 左侧栏 - Checkbox选择
        ImGui::BeginChild("LeftPanel", ImVec2(0, 0), true);
        //ImGui::Text(u8"选择要显示的数据设备：");
        for (uint8_t addr : dualAxisDeviceAddresses) {
            if (displayFlags.find(addr) == displayFlags.end())
                displayFlags[addr] = false;

            char label[32];
            sprintf_s(label, sizeof(label), u8" 0x%02X", addr);
            ImGui::Checkbox(label, &displayFlags[addr]);
        }
        ImGui::EndChild();

        // 切换到右侧栏
        ImGui::NextColumn();

        // 右侧栏 - 数据显示
        ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
        {
            auto renderAngleCard = [](const char* label, float value) {
                ImGui::BeginChild(label, ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("000.0000 °").x) * 0.5f);
                ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), u8"%.4f °", value);
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(label).x) * 0.5f);
                ImGui::TextColored(ImVec4(1, 1, 1, 1), u8"%s", label);
                ImGui::EndChild();
                ImGui::NextColumn();
                };

            std::lock_guard<std::mutex> lock(dataMutex);
            for (uint8_t addr : dualAxisDeviceAddresses) {
                if (!displayFlags[addr]) continue;

                if (dualAxisDataMap.find(addr) != dualAxisDataMap.end()) {
                    auto& angles = dualAxisDataMap[addr];

                    ImGui::Separator();
                    ImGui::Text(u8"当前显示设备: 0x%02X", addr);
                    extern ImFont* DataFont;
                    ImGui::PushID(addr);  // 避免 BeginChild 重名
                    ImGui::PushFont(DataFont);
                    ImGui::Columns(2, nullptr, false);

                    renderAngleCard(u8"水平角度", angles.filtered_horizontal);
                    renderAngleCard(u8"垂直角度", angles.filtered_vertical);
                    renderAngleCard(u8"原始水平", angles.raw_horizontal);
                    renderAngleCard(u8"原始垂直", angles.raw_vertical);

                    ImGui::Columns(1);
                    ImGui::PopFont();
                    ImGui::PopID();
                }
                else {
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), u8"设备 0x%02X 暂无数据", addr);
                }
            }
        }
        ImGui::EndChild();

        // 结束分栏
        ImGui::Columns(1);
    }

    ImGui::Text(u8"连接状态: %s", isConnected ? u8"已连接" : u8"未连接");
    ImGui::End();
}


//：CreateFileW 在尝试打开不存在的串口时，Windows 系统默认会等待超时（约 2 秒）
std::vector<std::string> Application::listAvailableSerialPorts() {
    std::vector<std::string> ports;
    HKEY hKey;
    // 打开注册表路径
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD index = 0;
        TCHAR valueName[256];
        BYTE data[256];
        DWORD valueNameSize, dataSize, type;

        // 遍历所有注册表键值
        while (true) {
            valueNameSize = sizeof(valueName);
            dataSize = sizeof(data);
            if (RegEnumValue(hKey, index, valueName, &valueNameSize, NULL, &type, data, &dataSize) != ERROR_SUCCESS)
                break;

            if (type == REG_SZ) {
                // 将宽字符数据转换为字符串
                std::wstring wPortName(reinterpret_cast<wchar_t*>(data));
                ports.push_back(std::string(wPortName.begin(), wPortName.end()));
            }
            index++;
        }
        RegCloseKey(hKey);
    }
    return ports;
}

void Application::ShowJY61P() {
    if (!ImGui::Begin("JY61P")) {
        ImGui::End();
        return;
    }
    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;
    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
    static int selectedBaudIndex = 0;
    static bool isConnected = false;
    static std::unordered_map<uint8_t, bool> displayFlags;
    static std::thread pollingThread;

    static std::atomic<bool> collecting = false;

    // 新增采集控制相关
    static std::atomic<bool> isCollectingData = false;
    static std::thread dataCollectionThread;
    static std::vector<std::pair<std::chrono::system_clock::time_point,
        std::unordered_map<uint8_t, JY61PData::angle>>> collectedData;
    static std::mutex collectedDataMutex;

    // 串口选择
    if (ImGui::BeginCombo(u8"串口", availablePorts[selectedPortIndex].c_str())) {
        for (int n = 0; n < availablePorts.size(); n++) {
            bool isSelected = (selectedPortIndex == n);
            if (ImGui::Selectable(availablePorts[n].c_str(), isSelected))
                selectedPortIndex = n;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // 波特率选择
    if (ImGui::BeginCombo(u8"波特率", baudRates[selectedBaudIndex])) {
        for (int n = 0; n < IM_ARRAYSIZE(baudRates); n++) {
            bool isSelected = (selectedBaudIndex == n);
            if (ImGui::Selectable(baudRates[n], isSelected))
                selectedBaudIndex = n;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // 连接逻辑（启动轮询线程采集数据）
    if (!isConnected) {
        if (ImGui::Button(u8"连接")) {
            try {
                DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
                // 从"COM3"中提取数字3
                std::string port = availablePorts[selectedPortIndex];
                unsigned long portNumber = std::stoul(port.substr(3)); // 跳过"COM"前缀
                OpenCOMDevice(portNumber, baudRate);
                //OpenCOMDevice(availablePorts[selectedPortIndex].c_str(), baudRate);
                isConnected = true;
                collecting = true;

                pollingThread = std::thread([] {
                    while (collecting) {
                        for (uint8_t addr : jy61pDeviceAddresses) {
                            try {
                                WitInit(WIT_PROTOCOL_MODBUS, addr);
                                WitSerialWriteRegister(SensorUartSend);
                                WitRegisterCallBack(CopeSensorData);
                                WitReadReg(AX, 15);
                                Sleep(20);

                                JY61PData::angle temp;
                                for (int i = 0; i < 3; ++i) {
                                    temp.a[i] = sReg[AX + i] / 32768.0f * 16.0f;
                                    temp.w[i] = sReg[GX + i] / 32768.0f * 2000.0f;
                                    temp.Angle[i] = sReg[Roll + i] / 32768.0f * 180.0f;
                                }

                                std::lock_guard<std::mutex> lock(jy61pDataMutex);
                                jy61pDataMap[addr].dataQue.push_back(temp);
                                if (jy61pDataMap[addr].dataQue.size() > MAX_POINTS)
                                    jy61pDataMap[addr].dataQue.pop_front();
                            }
                            catch (const std::exception& e) {
                                std::cerr << "JY61P 地址 0x" << std::hex << (int)addr << " 读取失败: " << e.what() << std::endl;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(20));
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    });
            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "连接失败: %s", e.what());
            }
        }
    }
    else {
        if (ImGui::Button(u8"断开")) {
            collecting = false;
            if (pollingThread.joinable()) pollingThread.join();

            // 停止采集线程
            isCollectingData = false;
            if (dataCollectionThread.joinable()) dataCollectionThread.join();

            CloseCOMDevice();
            isConnected = false;
            jy61pDataMap.clear();
            displayFlags.clear();
        }

        // 采集控制按钮
        if (!isCollectingData) {
            if (ImGui::Button(u8"开始采集")) {
                isCollectingData = true;
                {
                    std::lock_guard<std::mutex> lock(collectedDataMutex);
                    collectedData.clear();
                }

                dataCollectionThread = std::thread([&]() {
                    while (isCollectingData) {
                        std::unordered_map<uint8_t, JY61PData::angle> snapshot;

                        {
                            std::lock_guard<std::mutex> lock(jy61pDataMutex);
                            for (auto& [addr, data] : jy61pDataMap) {
                                if (!data.dataQue.empty())
                                    snapshot[addr] = data.dataQue.back();
                            }
                        }

                        auto now = std::chrono::system_clock::now();
                        {
                            std::lock_guard<std::mutex> lock(collectedDataMutex);
                            collectedData.emplace_back(now, snapshot);
                        }
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    });
            }
        }
        else {
            if (ImGui::Button(u8"停止采集并保存")) {
                isCollectingData = false;
                if (dataCollectionThread.joinable())
                    dataCollectionThread.join();

                //try {
                //    std::string saveFilePath = generateUniqueFileName("JY61P_data");
                //    OpenXLSX::XLDocument doc;
                //    doc.create(saveFilePath, false);
                //    doc.open(saveFilePath);
                //    auto wks = doc.workbook().worksheet("Sheet1");

                //    // 写入表头
                //    wks.cell(1, 1).value() = "Time";
                //    int col = 2;
                //    for (uint8_t addr : jy61pDeviceAddresses) {
                //        std::stringstream ss;
                //        ss << "Device 0x" << std::uppercase << std::hex
                //            << std::setw(2) << std::setfill('0') << (int)addr;

                //        wks.cell(1, col++) = ss.str() + " Accel X";
                //        wks.cell(1, col++) = ss.str() + " Accel Y";
                //        wks.cell(1, col++) = ss.str() + " Accel Z";

                //        wks.cell(1, col++) = ss.str() + " Gyro X";
                //        wks.cell(1, col++) = ss.str() + " Gyro Y";
                //        wks.cell(1, col++) = ss.str() + " Gyro Z";

                //        wks.cell(1, col++) = ss.str() + " Angle X";
                //        wks.cell(1, col++) = ss.str() + " Angle Y";
                //        wks.cell(1, col++) = ss.str() + " Angle Z";
                //    }

                //    // 写入数据
                //    std::lock_guard<std::mutex> lock(collectedDataMutex);
                //    for (size_t row = 0; row < collectedData.size(); ++row) {
                //        const auto& [timestamp, snapshot] = collectedData[row];

                //        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                //        std::tm tm;
                //        localtime_s(&tm, &time_t);
                //        std::ostringstream oss;
                //        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
                //        wks.cell(row + 2, 1).value() = oss.str();

                //        int col = 2;
                //        for (uint8_t addr : jy61pDeviceAddresses) {
                //            if (snapshot.find(addr) != snapshot.end()) {
                //                const auto& angle = snapshot.at(addr);
                //                wks.cell(row + 2, col++) = angle.a[0];
                //                wks.cell(row + 2, col++) = angle.a[1];
                //                wks.cell(row + 2, col++) = angle.a[2];

                //                wks.cell(row + 2, col++) = angle.w[0];
                //                wks.cell(row + 2, col++) = angle.w[1];
                //                wks.cell(row + 2, col++) = angle.w[2];

                //                wks.cell(row + 2, col++) = angle.Angle[0];
                //                wks.cell(row + 2, col++) = angle.Angle[1];
                //                wks.cell(row + 2, col++) = angle.Angle[2];
                //            }
                //            else {
                //                col += 9; // 跳过无数据设备
                //            }
                //        }
                //    }

                //    doc.save();
                //    doc.close();

                //    ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"数据已保存到: %s", saveFilePath.c_str());
                //}
                //catch (const std::exception& e) {
                //    ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"保存失败: %s", e.what());
                //}
                try {
                    std::string saveFilePath = generateUniqueFileName("JY61P_data");
                    OpenXLSX::XLDocument doc;
                    doc.create(saveFilePath, false);
                    doc.open(saveFilePath);
                    auto wks = doc.workbook().worksheet("Sheet1");

                    // 写入表头
                    wks.cell(1, 1).value() = "Time";
                    wks.cell(1, 2).value() = "Device Addr";
                    wks.cell(1, 3).value() = "Accel X";
                    wks.cell(1, 4).value() = "Accel Y";
                    wks.cell(1, 5).value() = "Accel Z";
                    wks.cell(1, 6).value() = "Gyro X";
                    wks.cell(1, 7).value() = "Gyro Y";
                    wks.cell(1, 8).value() = "Gyro Z";
                    wks.cell(1, 9).value() = "Angle X";
                    wks.cell(1, 10).value() = "Angle Y";
                    wks.cell(1, 11).value() = "Angle Z";

                    // 写入数据
                    std::lock_guard<std::mutex> lock(collectedDataMutex);
                    int row = 2; // 从第2行开始写入数据

                    for (const auto& [timestamp, snapshot] : collectedData) {
                        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                        std::tm tm;
                        localtime_s(&tm, &time_t);
                        std::ostringstream oss;
                        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
                        std::string timeStr = oss.str();

                        for (uint8_t addr : jy61pDeviceAddresses) {
                            if (snapshot.find(addr) != snapshot.end()) {
                                const auto& angle = snapshot.at(addr);

                                // 写入时间戳
                                wks.cell(row, 1).value() = timeStr;

                                // 写入设备地址
                                std::stringstream addr_ss;
                                addr_ss << "0x" << std::uppercase << std::hex
                                    << std::setw(2) << std::setfill('0') << (int)addr;
                                wks.cell(row, 2).value() = addr_ss.str();

                                // 写入加速度数据
                                wks.cell(row, 3).value() = angle.a[0];
                                wks.cell(row, 4).value() = angle.a[1];
                                wks.cell(row, 5).value() = angle.a[2];

                                // 写入陀螺仪数据
                                wks.cell(row, 6).value() = angle.w[0];
                                wks.cell(row, 7).value() = angle.w[1];
                                wks.cell(row, 8).value() = angle.w[2];

                                // 写入角度数据
                                wks.cell(row, 9).value() = angle.Angle[0];
                                wks.cell(row, 10).value() = angle.Angle[1];
                                wks.cell(row, 11).value() = angle.Angle[2];

                                row++; // 移动到下一行
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

            // 显示采集状态
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"正在采集数据... 已记录 %d 条",
                static_cast<int>(collectedData.size()));
        }

        // 设备选择与显示
        if (!jy61pDeviceAddresses.empty()) {
            // 开始左右分栏布局，左侧20%，右侧80%
            ImGui::Columns(2, "MainColumns", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.2f);  // 左侧占20%
            ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() * 0.8f);  // 右侧占80%

            // 左侧栏 - Checkbox选择
            ImGui::BeginChild("LeftPanel", ImVec2(0, 0), true);
            ImGui::Separator();
            ImGui::Text(u8"选择要显示的数据设备：");
            for (uint8_t addr : jy61pDeviceAddresses) {
                if (displayFlags.find(addr) == displayFlags.end())
                    displayFlags[addr] = false;

                char label[32];
                sprintf_s(label, sizeof(label), u8" 0x%02X", addr);
                ImGui::Checkbox(label, &displayFlags[addr]);
            }
            ImGui::EndChild();

            // 切换到右侧栏
            ImGui::NextColumn();

            // 右侧栏 - 数据显示
            ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
            {
                std::lock_guard<std::mutex> lock(jy61pDataMutex);
                for (uint8_t addr : jy61pDeviceAddresses) {
                    if (!displayFlags[addr]) continue;
                    if (jy61pDataMap[addr].dataQue.empty()) continue;

                    auto& data = jy61pDataMap[addr].dataQue.back();
                    ImGui::Separator();
                    ImGui::Text(u8"0x%02X", addr);
                    extern ImFont* DataFont;
                    ImGui::PushID(addr);
                    ImGui::PushFont(DataFont);
                    ImGui::Columns(3, nullptr, false);

                    auto renderCard = [](const char* label, float value, ImVec4 color, const char* fmt) {
                        ImGui::BeginChild(label, ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                        ImGui::Dummy(ImVec2(0.0f, 10.0f));
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(fmt).x) * 0.5f);
                        ImGui::TextColored(color, fmt, value);
                        ImGui::Dummy(ImVec2(0.0f, 5.0f));
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(label).x) * 0.5f);
                        ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", label);
                        ImGui::EndChild();
                        ImGui::NextColumn();
                        };

                    renderCard(u8"加速度X", data.a[0], ImVec4(1.0f, 0.75f, 0.0f, 1.0f), "%.4f g");
                    renderCard(u8"加速度Y", data.a[1], ImVec4(1.0f, 0.75f, 0.0f, 1.0f), "%.4f g");
                    renderCard(u8"加速度Z", data.a[2], ImVec4(1.0f, 0.75f, 0.0f, 1.0f), "%.4f g");
                    renderCard(u8"角速度X", data.w[0], ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"%.4f °/s");
                    renderCard(u8"角速度Y", data.w[1], ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"%.4f °/s");
                    renderCard(u8"角速度Z", data.w[2], ImVec4(0.4f, 0.8f, 1.0f, 1.0f), u8"%.4f °/s");
                    renderCard(u8"角度X", data.Angle[0], ImVec4(0.6f, 1.0f, 0.6f, 1.0f), u8"%.4f °");
                    renderCard(u8"角度Y", data.Angle[1], ImVec4(0.6f, 1.0f, 0.6f, 1.0f), u8"%.4f °");
                    renderCard(u8"角度Z", data.Angle[2], ImVec4(0.6f, 1.0f, 0.6f, 1.0f), u8"%.4f °");

                    ImGui::Columns(1);
                    ImGui::PopFont();
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();

            // 结束分栏
            ImGui::Columns(1);
        }
    }

    ImGui::Text(u8"连接状态: %s", isConnected ? u8"已连接" : u8"未连接");
    ImGui::End();
}


void Application::JY61PInit(const std::string& portName)
{

    OpenCOMDevice(iComPort, iBaud);
    WitInit(WIT_PROTOCOL_MODBUS, iAddress);
    WitSerialWriteRegister(SensorUartSend);
    WitRegisterCallBack(CopeSensorData);
    WitDelayMsRegister(DelayMs);
    AutoScanSensor();
}

void Application::ShowADXL355() {
    if (!ImGui::Begin("ADXL355")) {
        ImGui::End();
        return;
    }

    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;
    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
    static int selectedBaudIndex = 4;
    static bool isConnected = false;
    static std::unordered_map<uint8_t, bool> deviceDisplayFlags;

    // 采集控制相关变量
    static std::atomic<bool> isCollectingADXL355 = false;
    static std::thread adxl355CollectionThread;
    static std::vector<std::pair<std::chrono::system_clock::time_point, std::unordered_map<uint8_t, ADXL355Parser::AccelerationData>>> adxl355CollectedData;
    static std::mutex adxl355CollectedDataMutex;

    // 串口选择
    ShowSerialPortSelector(availablePorts, selectedPortIndex);
    // 波特率选择
    ShowBaudRateSelector(baudRates, IM_ARRAYSIZE(baudRates), selectedBaudIndex);

    if (!isConnected) {
        if (ImGui::Button(u8"连接")) {
            try {
                DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
                serialManager.open(availablePorts[selectedPortIndex], baudRate);
                isConnected = true;
                collectingADXL355 = true;

                for (uint8_t addr : adxl355DeviceAddresses) {
                    adxl355Parsers[addr] = ADXL355Parser(addr);
                    adxl355DataMap[addr] = ADXL355Data();
                }

                adxl355PollingThread = std::thread([]() {
                    while (collectingADXL355) {
                        for (uint8_t addr : adxl355DeviceAddresses) {
                            try {
                                std::vector<uint8_t> cmd;
                                std::vector<uint8_t> response;
                                ADXL355Parser::AccelerationData data;

                                {
                                    std::lock_guard<std::mutex> lock(RS485SendRecvMutex);
                                    cmd = adxl355Parsers[addr].generateReadAccelerationCommand();
                                    serialManager.send(cmd);
                                    response = serialManager.receiveADXL355Response();
                                    data = adxl355Parsers[addr].parseAccelerationResponse(response);
                                }

                                {
                                    std::lock_guard<std::mutex> lock2(ADXL355Mutex);
                                    auto& dq = adxl355DataMap[addr].dataQue;
                                    dq.push_back(data);
                                    if (dq.size() > MAX_POINTS)
                                        dq.pop_front();
                                }

                                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                            }
                            catch (const std::exception& e) {
                                std::cerr << u8"[设备 0x" << std::hex << (int)addr << "] 采集异常: " << e.what() << std::endl;
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                    });
            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "连接失败: %s", e.what());
            }
        }
    }
    else {
        if (ImGui::Button(u8"断开")) {
            collectingADXL355 = false;
            if (adxl355PollingThread.joinable())
                adxl355PollingThread.join();
            isCollectingADXL355 = false;
            if (adxl355CollectionThread.joinable())
                adxl355CollectionThread.join();
            adxl355CollectedData.clear();

            serialManager.close();
            isConnected = false;
            adxl355Parsers.clear();
            adxl355DataMap.clear();
            deviceDisplayFlags.clear();
        }

        // 采集控制
        if (!isCollectingADXL355) {
            if (ImGui::Button(u8"开始采集")) {
                isCollectingADXL355 = true;
                {
                    std::lock_guard<std::mutex> lock(adxl355CollectedDataMutex);
                    adxl355CollectedData.clear();
                }

                adxl355CollectionThread = std::thread([] {
                    while (isCollectingADXL355) {
                        std::unordered_map<uint8_t, ADXL355Parser::AccelerationData> snapshot;

                        {
                            std::lock_guard<std::mutex> lock(ADXL355Mutex);
                            for (const auto& [addr, data] : adxl355DataMap) {
                                if (!data.dataQue.empty())
                                    snapshot[addr] = data.dataQue.back();
                            }
                        }

                        auto now = std::chrono::system_clock::now();
                        {
                            std::lock_guard<std::mutex> lock(adxl355CollectedDataMutex);
                            adxl355CollectedData.emplace_back(now, snapshot);
                        }

                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    });
            }
        }
        else {
            if (ImGui::Button(u8"停止采集并保存")) {
                isCollectingADXL355 = false;
                if (adxl355CollectionThread.joinable())
                    adxl355CollectionThread.join();

                //try {
                //    std::string saveFilePath = generateUniqueFileName("ADXL355_data");
                //    OpenXLSX::XLDocument doc;
                //    doc.create(saveFilePath, false);
                //    doc.open(saveFilePath);
                //    auto wks = doc.workbook().worksheet("Sheet1");

                //    // 写入表头
                //    wks.cell(1, 1).value() = "Time";
                //    int col = 2;
                //    for (uint8_t addr : adxl355DeviceAddresses) {
                //        std::stringstream ss;
                //        ss << "Device 0x" << std::uppercase << std::hex
                //            << std::setw(2) << std::setfill('0') << (int)addr;
                //        wks.cell(1, col++) = ss.str() + " Accel X";
                //        wks.cell(1, col++) = ss.str() + " Accel Y";
                //        wks.cell(1, col++) = ss.str() + " Accel Z";
                //    }

                //    // 写入数据
                //    std::lock_guard<std::mutex> lock(adxl355CollectedDataMutex);
                //    for (size_t row = 0; row < adxl355CollectedData.size(); ++row) {
                //        const auto& [timestamp, snapshot] = adxl355CollectedData[row];
                //        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                //        std::tm tm;
                //        localtime_s(&tm, &time_t);
                //        std::ostringstream oss;
                //        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
                //        wks.cell(row + 2, 1).value() = oss.str();

                //        int col = 2;
                //        for (uint8_t addr : adxl355DeviceAddresses) {
                //            if (snapshot.find(addr) != snapshot.end()) {
                //                const auto& data = snapshot.at(addr);
                //                wks.cell(row + 2, col++) = data.x;
                //                wks.cell(row + 2, col++) = data.y;
                //                wks.cell(row + 2, col++) = data.z;
                //            }
                //            else {
                //                col += 3;
                //            }
                //        }
                //    }

                //    doc.save();
                //    doc.close();

                //    ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"数据已保存到: %s", saveFilePath.c_str());
                //}
                try {
                    std::string saveFilePath = generateUniqueFileName("ADXL355_data");
                    OpenXLSX::XLDocument doc;
                    doc.create(saveFilePath, false);
                    doc.open(saveFilePath);
                    auto wks = doc.workbook().worksheet("Sheet1");

                    // 写入表头
                    wks.cell(1, 1).value() = "Time";
                    wks.cell(1, 2).value() = "Device Addr";
                    int col = 3;
                    wks.cell(1, col++).value() = "Accel X";
                    wks.cell(1, col++).value() = "Accel Y";
                    wks.cell(1, col++).value() = "Accel Z";

                    // 写入数据
                    std::lock_guard<std::mutex> lock(adxl355CollectedDataMutex);
                    int row = 2; // 从第2行开始写入数据
                    for (const auto& [timestamp, snapshot] : adxl355CollectedData) {
                        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                        std::tm tm;
                        localtime_s(&tm, &time_t);
                        std::ostringstream oss;
                        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

                        for (uint8_t addr : adxl355DeviceAddresses) {
                            if (snapshot.find(addr) != snapshot.end()) {
                                const auto& data = snapshot.at(addr);

                                // 写入时间戳
                                wks.cell(row, 1).value() = oss.str();

                                // 写入设备地址
                                std::stringstream addr_ss;
                                addr_ss << "0x" << std::uppercase << std::hex
                                    << std::setw(2) << std::setfill('0') << (int)addr;
                                wks.cell(row, 2).value() = addr_ss.str();

                                // 写入加速度数据
                                wks.cell(row, 3).value() = data.x;
                                wks.cell(row, 4).value() = data.y;
                                wks.cell(row, 5).value() = data.z;

                                row++; // 移动到下一行
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

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"正在采集数据... 已记录 %d 条",
                static_cast<int>(adxl355CollectedData.size()));
        }

        // 设备选择与显示
        // 设备选择与显示
        if (!adxl355DeviceAddresses.empty()) {
            // 开始左右分栏布局，左侧20%，右侧80%
            ImGui::Columns(2, "ADXL355DisplayColumns", false);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.2f);  // 左侧占20%
            ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() * 0.8f);  // 右侧占80%

            // 左侧栏 - Checkbox选择
            ImGui::BeginChild("LeftPanel", ImVec2(0, 0), true);
            ImGui::Separator();
            for (uint8_t addr : adxl355DeviceAddresses) {
                if (deviceDisplayFlags.find(addr) == deviceDisplayFlags.end())
                    deviceDisplayFlags[addr] = false;

                char label[32];
                sprintf_s(label, sizeof(label), u8" 0x%02X", addr);
                ImGui::Checkbox(label, &deviceDisplayFlags[addr]);
            }
            ImGui::EndChild();

            // 切换到右侧栏
            ImGui::NextColumn();

            // 右侧栏 - 数据显示
            ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
            {
                std::lock_guard<std::mutex> lock(ADXL355Mutex);
                for (uint8_t addr : adxl355DeviceAddresses) {
                    if (!deviceDisplayFlags[addr]) continue;

                    auto it = adxl355DataMap.find(addr);
                    if (it != adxl355DataMap.end() && !it->second.dataQue.empty()) {
                        const auto& data = it->second.dataQue.back();

                        extern ImFont* DataFont;
                        ImGui::Separator();
                        ImGui::Text(u8"设备 0x%02X", addr);
                        ImGui::PushID(addr);
                        ImGui::PushFont(DataFont);
                        ImGui::Columns(3, nullptr, false);

                        auto renderAccelCard = [](const char* label, float value) {
                            ImGui::BeginChild(label, ImVec2(0, 120), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                            ImGui::Dummy(ImVec2(0.0f, 10.0f));
                            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("0.0000 g").x) * 0.5f);
                            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.0f, 1.0f), "%.4f g", value);
                            ImGui::Dummy(ImVec2(0.0f, 5.0f));
                            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(label).x) * 0.5f);
                            ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", label);
                            ImGui::EndChild();
                            ImGui::NextColumn();
                            };

                        renderAccelCard(u8"加速度X", data.x);
                        renderAccelCard(u8"加速度Y", data.y);
                        renderAccelCard(u8"加速度Z", data.z);

                        ImGui::Columns(1);
                        ImGui::PopFont();
                        ImGui::PopID();
                    }
                    else {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "设备 0x%02X 无数据", addr);
                    }
                }
    }
            ImGui::EndChild();

            // 结束分栏
            ImGui::Columns(1);
        }
    }

    ImGui::Text(u8"连接状态: %s", isConnected ? u8"已连接" : u8"未连接");
    ImGui::End();
}

// 在应用程序类中添加激光传感器相关代码
void Application::ShowLaserSensor() {
    if (!ImGui::Begin("Laser Sensor")) {
        ImGui::End();
        return;
    }

    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;
    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
    static int selectedBaudIndex = 0;
    static bool isConnected = false;
    static std::unique_ptr<LaserSensorProtocol> laserSensor;

    // 采集控制相关变量
    static std::atomic<bool> isCollecting = false;
    static std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>> collectedData;

    // 串口选择
    ShowSerialPortSelector(availablePorts, selectedPortIndex);
    // 波特率选择
    ShowBaudRateSelector(baudRates, IM_ARRAYSIZE(baudRates), selectedBaudIndex);

    if (!isConnected) {
        if (ImGui::Button(u8"连接")) {
            try {
                DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
                serialManager.open(availablePorts[selectedPortIndex], baudRate);
                laserSensor = std::make_unique<LaserSensorProtocol>(serialManager);
                isConnected = true;
            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "连接失败: %s", e.what());
            }
        }
    }
    else {
        if (ImGui::Button(u8"断开")) {
            if (isCollecting) {
                isCollecting = false;
                collectedData = laserSensor->stopContinuousCollection();
            }
            serialManager.close();
            isConnected = false;
            laserSensor.reset();
        }

        // 单次测量
        if (ImGui::Button(u8"单次测量")) {
            try {
                uint16_t distance = laserSensor->getDistance();
                ImGui::Text("当前距离: %d mm", distance);
            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "测量失败: %s", e.what());
            }
        }

        // 连续采集控制
        if (!isCollecting) {
            if (ImGui::Button(u8"开始连续采集")) {
                isCollecting = true;
                laserSensor->startContinuousCollection();
            }
        }
        else {
            if (ImGui::Button(u8"停止采集并保存")) {
                isCollecting = false;
                collectedData = laserSensor->stopContinuousCollection();

                try {
                    std::string saveFilePath = generateUniqueFileName("LaserSensor_data");
                    laserSensor->saveDataToExcel(saveFilePath);
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"数据已保存到: %s", saveFilePath.c_str());
                }
                catch (const std::exception& e) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"保存失败: %s", e.what());
                }
            }

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"正在采集数据...");
        }
    }

    ImGui::Text(u8"连接状态: %s", isConnected ? u8"已连接" : u8"未连接");
    ImGui::End();
}


void ShowSerialPortSelector(const std::vector<std::string>& ports, int& selectedIndex, const char* label) {
    if (ImGui::BeginCombo(label, ports[selectedIndex].c_str())) {
        for (int i = 0; i < ports.size(); ++i) {
            bool isSelected = (selectedIndex == i);
            if (ImGui::Selectable(ports[i].c_str(), isSelected))
                selectedIndex = i;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void ShowBaudRateSelector(const char* const* baudRates, int baudRateCount, int& selectedIndex, const char* label) {
    if (ImGui::BeginCombo(label, baudRates[selectedIndex])) {
        for (int i = 0; i < baudRateCount; ++i) {
            bool isSelected = (selectedIndex == i);
            if (ImGui::Selectable(baudRates[i], isSelected))
                selectedIndex = i;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}



