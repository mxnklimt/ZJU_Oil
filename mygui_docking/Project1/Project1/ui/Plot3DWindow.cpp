

// V2.0
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

#include "Plot3DWindow.h"
#include"imgui.h"
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
struct Vec3 {
    float x, y, z;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};
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

void Application::ExcelGetData()
{

}
std::vector<std::string> Application::listAvailableSerialPorts() {
    std::vector<std::string> ports;
    for (int i = 1; i <= 20; ++i) {
        std::string portName = "COM" + std::to_string(i);
        std::wstring wPortName = L"\\\\.\\" + std::wstring(portName.begin(), portName.end());

        HANDLE h = CreateFileW(
            wPortName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        if (h != INVALID_HANDLE_VALUE) {
            ports.push_back(portName);
            CloseHandle(h);
        }
    }
    return ports;
}
void Application::ShowJY61P()
{
    // ImGui::Begin() 返回 false 表示窗口不可见（如被折叠），必须 return
	// 防止Imgui报错child窗口未结束 
    if (!ImGui::Begin("JY61P")) {
        ImGui::End();
        return;
    }
    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;
    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
    static int selectedBaudIndex = 0;
    static bool isConnected = false;
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
    // 波特率选择下拉框
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
       
    ImGui::Text(u8"连接状态: %s", isConnected ? u8"已连接" : u8"未连接");

    ImGui::End(); //  一定记得调用

}
void Application::JY61PInit(int port)
{

    OpenCOMDevice(iComPort, iBaud);
}

void Application::ShowADXL355() {
    // ImGui::Begin() 返回 false 表示窗口不可见（如被折叠），必须 return
	// 防止Imgui报错child窗口未结束 
    if (!ImGui::Begin("ADXL355")) {
        ImGui::End();
        return;
    }

    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;
    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
    static int selectedBaudIndex = 0;
    static bool isConnected = false;

    // 串口选择下拉框
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

    // 波特率选择下拉框
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

    // 连接或断开
    if (!isConnected) {
        if (ImGui::Button(u8"连接")) {
            try {
                DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
                serialManager.open(availablePorts[selectedPortIndex], baudRate);
                isConnected = true;
                collectingADXL355 = true;

                adxl355Thread = std::thread([] {
                    while (collectingADXL355) {
                        try {
                            auto cmd = parser.generateReadAccelerationCommand();
                            serialManager.send(cmd);
                            auto response = serialManager.receiveADXL355Response();
                            auto data = parser.parseAccelerationResponse(response);

                            {
                                std::lock_guard<std::mutex> lock(ADXL355Mutex);
                                extern ADXL355Data adxl355Data;
                                adxl355Data.dataQue.push_back(data);
                                if (adxl355Data.dataQue.size() > MAX_POINTS) {
                                    adxl355Data.dataQue.pop_front();
                                }
                            }

                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                        catch (const std::exception& e) {
                            std::cerr << "ADXL355线程错误: " << e.what() << std::endl;
                        }
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
            if (adxl355Thread.joinable())
                adxl355Thread.join();
            serialManager.close();
            isConnected = false;
        }

        // 显示加速度数据
        std::lock_guard<std::mutex> lock(ADXL355Mutex);
        extern ADXL355Data adxl355Data;

        if (!adxl355Data.dataQue.empty()) {
            const auto& data = adxl355Data.dataQue.back();
            extern ImFont* DataFont;
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
        }
    }

    ImGui::Text(u8"连接状态: %s", isConnected ? u8"已连接" : u8"未连接");

    ImGui::End(); //  一定记得调用
}

void Application::ShowWindow()
{
    static bool show_plot2d = true;
    static bool ADXL355 = true;
	static bool JY61P = true; // 默认显示JY61P数据
    static bool show_plot3d_2_window = true;  // 注意：控制的是独立窗口
    static bool show_plot3d_2 = true;
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
    // 读取excel数据
    bool readfile = false;
    std::vector<float> dValues, eValues;
    std::vector<std::tm> times;
    if (!readfile)
    {
        
        ReadFile::readColumnsDandEFloat("data.xlsx", "Sheet1", dValues, eValues);
        /* for (size_t i = 0; i < dValues.size(); ++i) {
             std::cout << "Row " << (i + 2) << ": D=" << dValues[i] << ", E=" << eValues[i] << std::endl;
         }*/
		dValues_save = dValues; // 保存数据
		eValues_save = eValues; // 保存数据
        ReadFile::readColumnCTimeOnly("data.xlsx", "Sheet1", times);
		times_save = times; // 保存时间数据
		readfile = true; // 只读取一次
        /*for (const auto& t : times) {
            std::cout << "时间: "
                << std::setw(2) << std::setfill('0') << t.tm_hour << ":"
                << std::setw(2) << std::setfill('0') << t.tm_min << ":"
                << std::setw(2) << std::setfill('0') << t.tm_sec
                << std::endl;
        }*/
    }

    //--------------------------------------------------------------------------------------------------------------------------------
	// 绘制管道位移和岸坡沉降的2D图形
    if (show_plot2d) {
        static std::vector<float> time_xf;
        static std::vector<float> y_d, y_e;

        // 使用全局变量 times_save 计算时间轴
        if (time_xf.size() != times_save.size()) {
            size_t n = times_save.size();
            time_xf.resize(n);
            y_d.resize(n);
            y_e.resize(n);

            for (size_t i = 0; i < n; ++i) {
                const std::tm& t = times_save[i];
                float seconds = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
                time_xf[i] = seconds;
                y_d[i] = dValues_save[i];
                y_e[i] = eValues_save[i];
            }
        }

        // 时间格式化，显示 HH:MM:SS
        ImPlotFormatter TimeFormatter = [](double seconds, char* buffer, int size, void*) -> int {
            int h = static_cast<int>(seconds) / 3600;
            int m = (static_cast<int>(seconds) % 3600) / 60;
            int s = static_cast<int>(seconds) % 60;
            return snprintf(buffer, size, "%02d:%02d:%02d", h, m, s);
            };

        int count = static_cast<int>(time_xf.size());

        if (ImGui::CollapsingHeader(u8"管道水平位移")) {
            if (ImPlot::BeginPlot(u8"管道水平位移图")) {
                ImPlot::SetupAxes(u8"时间", u8"管道水平位移");
                ImPlot::SetupAxisFormat(ImAxis_X1, TimeFormatter);

                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 0.4f, 1.0f, 1.0f));
                ImPlot::PlotLine(u8"管道水平位移##line", time_xf.data(), y_d.data(), count);
                ImPlot::PopStyleColor();

                ImPlot::EndPlot();
            }
        }
        
        if (ImGui::CollapsingHeader(u8"岸坡沉降位移")) {
            if (ImPlot::BeginPlot(u8"岸坡沉降位移图")) {
                ImPlot::SetupAxes(u8"时间", u8"岸坡沉降位移");
                ImPlot::SetupAxisFormat(ImAxis_X1, TimeFormatter);

                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 86400.0, ImGuiCond_Always);

                ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
                ImPlot::PlotLine(u8"岸坡沉降位移##line", time_xf.data(), y_e.data(), count);
                ImPlot::PopStyleColor();

                ImPlot::EndPlot();
            }
        }
    }
    ImGui::End(); // 主窗口结束
    //--------------------------------------------------------------------------------------------------------------------------------
    // ADXL355
    if (ADXL355) {
		Application::ShowADXL355();
    }
    //ImGui::End(); // 主窗口结束


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
    //--------------------------------------------------------------------------------------------------------------------------------
}

void Application::ShowWindow2()
{
    static bool show_plot2d = true;
    static bool show_plot3d_1 = false;
    static bool show_plot3d_2 = true;
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("3D Plot Demo", nullptr, ImGuiWindowFlags_MenuBar);

    // 创建一个选项卡栏
    if (ImGui::BeginTabBar("Tabs")) {
        // 2D 图形的选项卡
        if (ImGui::BeginTabItem("2D Plot")) {
            show_plot2d = true;
            show_plot3d_1 = false;
            show_plot3d_2 = false;

            // 绘制 2D 图形
            if (show_plot2d) {
                static float x_data[100];
                static float y_data1[100]; // sin(x)
                static float y_data2[100]; // cos(x)

                // 初始化数据
                for (int i = 0; i < 100; ++i) {
                    x_data[i] = i * 0.1f;
                    y_data1[i] = sinf(x_data[i]);
                    y_data2[i] = cosf(x_data[i]);
                }

                if (ImPlot::BeginPlot("Sine & Cosine Plot")) {
                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.2f, 0.4f, 1.0f, 1.0f)); // 蓝色
                    ImPlot::PlotLine("Sine Wave", x_data, y_data1, 100);
                    ImPlot::PopStyleColor();

                    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.85f, 0.1f, 1.0f)); // 黄色
                    ImPlot::PlotLine("Cosine Wave", x_data, y_data2, 100);
                    ImPlot::PopStyleColor();

                    ImPlot::EndPlot();
                }
            }
            ImGui::EndTabItem();
        }

        // 3D 图形 1 的选项卡
        if (ImGui::BeginTabItem("3D Plot 1")) {
            show_plot2d = false;
            show_plot3d_1 = true;
            show_plot3d_2 = false;

            // 绘制 3D 图形 1
            if (show_plot3d_1) {
                static float xs1[1001], ys1[1001], zs1[1001];
                for (int i = 0; i < 1001; i++) {
                    xs1[i] = i * 0.001f;
                    ys1[i] = 0.5f + 0.5f * cosf(50 * (xs1[i] + (float)ImGui::GetTime() / 10));
                    zs1[i] = 0.5f + 0.5f * sinf(50 * (xs1[i] + (float)ImGui::GetTime() / 10));
                }
                static double xs2[20], ys2[20], zs2[20];
                for (int i = 0; i < 20; i++) {
                    xs2[i] = i * 1 / 19.0f;
                    ys2[i] = xs2[i] * xs2[i];
                    zs2[i] = xs2[i] * ys2[i];
                }
                if (ImPlot3D::BeginPlot("Line Plots")) {
                    ImPlot3D::SetupAxes("x", "y", "z");
                    ImPlot3D::PlotLine("f(x)", xs1, ys1, zs1, 1001);
                    ImPlot3D::EndPlot();
                }
            }
            ImGui::EndTabItem();
        }

        // 3D 图形 2 的选项卡
        if (ImGui::BeginTabItem("3D Plot 2")) {
            show_plot2d = false;
            show_plot3d_1 = false;
            show_plot3d_2 = true;

            // 绘制 3D 图形 2
            if (show_plot3d_2) {
                Application::CylinderPlots();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}




