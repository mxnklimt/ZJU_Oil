// Dear ImGui: standalone example application for Win32 + OpenGL 3

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// This is provided for completeness, however it is strongly recommended you use OpenGL with SDL or GLFW.
#include <stdexcept>
#include<Auto.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>
#include "DualAxisSensor/DualAxisSensorParser.h"
#include "RS485/RS485Manager.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <map>
#include <Thread>
#include <unordered_map>
#include <mutex>
//define chinese font
ImFont* DataFont = nullptr;
//：CreateFileW 在尝试打开不存在的串口时，Windows 系统默认会等待超时（约 2 秒）
std::vector<std::string> listAvailableSerialPorts() {
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

RS485Manager serialDualAxis;
std::map<uint8_t, std::unique_ptr<DualAxisSensorParser>> dualAxisParsers;
std::vector<uint8_t> dualAxisDeviceAddresses = {  };//经过init以后，使得所有串口号进入
static std::thread pollingThread;
std::vector<uint8_t> collectdualAxisDeveiceAddresses;
static std::unordered_map<uint8_t, bool> displayFlags;
std::mutex dataMutex;
std::map<uint8_t, DualAxisSensorParser::AngleData> dualAxisDataMap;
void initAllPossibleDualAxisAddresses() {
    dualAxisDeviceAddresses.clear();
    for (uint8_t addr = 1; addr <= 127; ++addr) {
        dualAxisDeviceAddresses.push_back(addr);
    }
}
void initializedualAxisParsers() {
    initAllPossibleDualAxisAddresses();
    //遍历设备地址列表
    for (uint8_t addr : dualAxisDeviceAddresses) {
        //std::make_unique智能指针构造函数
        //为每个设备地址创建一个DualAxisSensorParser实例，并且初始化地址和串口对象
        dualAxisParsers[addr] = std::make_unique<DualAxisSensorParser>(serialDualAxis, addr);
        // 设置采样率
        //dualAxisParsers[addr]->setSamplingRate(DualAxisSensorParser::SamplingRate::ADS_100_Hz);
    }
}
void loadingAddress(uint8_t addr,double data1)
{
    if (data1 != 0 && data1<360 && data1>-360)
    {
		collectdualAxisDeveiceAddresses.push_back(addr);//将扫描到有数据的，已经连接的设备地址存入
    }
}


void ShowDualAxisSensor()
{
    if (!ImGui::Begin(u8"双轴柔性传感器")) {
        ImGui::End();
        return;
    }

    // ===== 左侧子窗口：设备 checkbox + 设置按钮 =====
    ImGui::BeginChild("LeftSidebar", ImVec2(200, 0), true);
    ImGui::Text(u8"显示设备:");

    static std::unordered_map<uint8_t, bool> configPopupOpen;

    for (uint8_t addr : collectdualAxisDeveiceAddresses)
    {
        if (displayFlags.find(addr) == displayFlags.end())
            displayFlags[addr] = false;

        ImGui::PushID(addr);
        char label[32];
        sprintf_s(label, u8"地址: 0x%02X", addr);
        ImGui::Checkbox(label, &displayFlags[addr]);
        ImGui::SameLine();

        if (ImGui::Button(u8"设置")) {
            configPopupOpen[addr] = true;
        }
        ImGui::PopID();
    }

    ImGui::EndChild(); // 左侧结束

    ImGui::SameLine();

    // ===== 右侧子窗口：串口选择 + 扫描控制 + 数据显示 =====
    ImGui::BeginChild("MainArea", ImVec2(0, 0), true);

    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;

    static const char* baudRates[] = {
        "2400","4800","9600", "14400","19200","28800",
        "38400", "57600", "115200","128000","153600",
        "230400","250000", "256000","345600","691200"
    };
    static int selectedBaudIndex = 2;

    static bool isConnected = false;
    static std::atomic<bool> collecting = false;
    static std::thread pollingThread;
    static std::thread dataThread;
    static bool scaning = true;
    ImGui::Text(u8"串口配置");
    ShowSerialPortSelector(availablePorts, selectedPortIndex, u8"串口号");
    ShowBaudRateSelector(baudRates, IM_ARRAYSIZE(baudRates), selectedBaudIndex, u8"波特率");

    if (!isConnected)
    {
        if (ImGui::Button(u8"连接并扫描"))
        {
            try {
                DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
                serialDualAxis.open(availablePorts[selectedPortIndex], baudRate);
                isConnected = true;
                collecting = true;

                collectdualAxisDeveiceAddresses.clear();
                dualAxisDataMap.clear();
                displayFlags.clear();

                initializedualAxisParsers();

                pollingThread = std::thread([&] {
                    while (collecting) {
                        if (scaning == true)
                        {
							scaning == false;
                        }
						if (scaning == true)
						{
                            for (uint8_t addr : dualAxisDeviceAddresses) {
                                try {
                                    auto& parser = *dualAxisParsers[addr];
                                    auto angles = parser.readAngles();

                                    // 只接受有效角度
                                    if (angles.filtered_horizontal != 0 &&
                                        angles.filtered_horizontal < 360 &&
                                        angles.filtered_horizontal > -360) {

                                        std::lock_guard<std::mutex> lock(dataMutex);
                                        dualAxisDataMap[addr] = angles;

                                        if (std::find(collectdualAxisDeveiceAddresses.begin(), collectdualAxisDeveiceAddresses.end(), addr) == collectdualAxisDeveiceAddresses.end()) {
                                            collectdualAxisDeveiceAddresses.push_back(addr);
                                        }
                                    }
                                }
                                catch (const std::exception& e) {
                                    std::cerr << "读取失败: 0x" << std::hex << (int)addr << " 错误: " << e.what() << std::endl;
                                }
                            }
						}
                        else if (scaning == false)
                        {
							for (uint8_t addr : collectdualAxisDeveiceAddresses)
							{
								try {
									auto& parser = *dualAxisParsers[addr];
									auto angles = parser.readAngles();
									std::lock_guard<std::mutex> lock(dataMutex);
									dualAxisDataMap[addr] = angles;
									std::cout <<"filtered_horizontal = " << angles.filtered_horizontal << std::endl;
									std::cout <<"filtered_vertical = " << angles.filtered_vertical << std::endl;
								}
								catch (const std::exception& e) {
									std::cerr << "读取失败: 0x" << std::hex << (int)addr << " 错误: " << e.what() << std::endl;
								}
							}
                        }
                        
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                    });


            }
            catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"扫描失败: %s", e.what());
            }
        }
    }
    else
    {
        if (ImGui::Button(u8"停止扫描"))
        {
            scaning = true;
            collecting = false;
            if (pollingThread.joinable())
                pollingThread.join();
            isConnected = false;
            dualAxisParsers.clear();
			collectdualAxisDeveiceAddresses.clear();
            displayFlags.clear();
        }
    }

    ImGui::Separator();
    ImGui::Text(u8"已选设备数据：");

    std::lock_guard<std::mutex> lock(dataMutex);

    for (uint8_t addr : collectdualAxisDeveiceAddresses) {
        if (!displayFlags[addr]) continue;

        if (dualAxisDataMap.find(addr) != dualAxisDataMap.end()) {
            auto& angles = dualAxisDataMap[addr];

            ImGui::Separator();
            ImGui::Text(u8"当前显示设备: 0x%02X", addr);
            extern ImFont* DataFont;
            ImGui::PushID(addr);
            ImGui::PushFont(DataFont);
            ImGui::Columns(2, nullptr, false);

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

    ImGui::EndChild(); // 右侧结束

    // ===== 配置弹窗逻辑 =====
    static std::unordered_map<uint8_t, int> deviceBaudIndexMap;
    static std::unordered_map<uint8_t, int> deviceNewAddrMap;

    const char* deviceBaudRates[] = {
        "2400","4800","9600", "14400","19200","28800",
        "38400", "57600", "115200","128000","153600",
        "230400","250000", "256000","345600","691200"
    };

    for (uint8_t addr : collectdualAxisDeveiceAddresses)
    {
        if (!configPopupOpen[addr])
            continue;

        char popupLabel[64];
        sprintf_s(popupLabel, "ConfigPopup_0x%02X", addr);
        ImGui::OpenPopup(popupLabel);

        if (ImGui::BeginPopupModal(popupLabel, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text(u8"配置设备 0x%02X", addr);

            if (deviceBaudIndexMap.find(addr) == deviceBaudIndexMap.end())
                deviceBaudIndexMap[addr] = 2;
            if (deviceNewAddrMap.find(addr) == deviceNewAddrMap.end())
                deviceNewAddrMap[addr] = addr;

            ImGui::Text(u8"波特率:");
            ImGui::Combo("##BaudRate", &deviceBaudIndexMap[addr], deviceBaudRates, IM_ARRAYSIZE(deviceBaudRates));

            ImGui::Text(u8"新地址:");
            ImGui::InputInt("##NewAddress", &deviceNewAddrMap[addr]);
            if (deviceNewAddrMap[addr] < 0) deviceNewAddrMap[addr] = 0;
            if (deviceNewAddrMap[addr] > 255) deviceNewAddrMap[addr] = 255;

            if (ImGui::Button(u8"应用设置"))
            {
                try {
                    uint8_t baudCode = 0xB0 + deviceBaudIndexMap[addr];
                    uint8_t newAddr = static_cast<uint8_t>(deviceNewAddrMap[addr]);

                    auto it = dualAxisParsers.find(addr);
                    if (it != dualAxisParsers.end()) {
                        it->second->changeSerialConfig(baudCode, 0x00, newAddr);
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"设置成功！");
                        std::cout << "设置成功" << std::endl;
                    }
                    else {
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), u8"找不到该设备的解析器");
                        std::cout << "找不到该设备的解析器" << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"设置失败: %s", e.what());
                    std::cout << "设置失败" << std::endl;
                }
            }

            if (ImGui::Button(u8"关闭")) {
                configPopupOpen[addr] = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::End(); // 主窗口
}







// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Support function for multi-viewports
// Unlike most other backend combination, we need specific hooks to combine Win32+OpenGL.
// We could in theory decide to support Win32-specific code in OpenGL backend via e.g. an hypothetical ImGui_ImplOpenGL3_InitForRawWin32().
static void Hook_Renderer_CreateWindow(ImGuiViewport* viewport)
{
    assert(viewport->RendererUserData == NULL);

    WGL_WindowData* data = IM_NEW(WGL_WindowData);
    CreateDeviceWGL((HWND)viewport->PlatformHandle, data);
    viewport->RendererUserData = data;
}

static void Hook_Renderer_DestroyWindow(ImGuiViewport* viewport)
{
    if (viewport->RendererUserData != NULL)
    {
        WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData;
        CleanupDeviceWGL((HWND)viewport->PlatformHandle, data);
        IM_DELETE(data);
        viewport->RendererUserData = NULL;
    }
}

static void Hook_Platform_RenderWindow(ImGuiViewport* viewport, void*)
{
    // Activate the platform window DC in the OpenGL rendering context
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        wglMakeCurrent(data->hDC, g_hRC);
}

static void Hook_Renderer_SwapBuffers(ImGuiViewport* viewport, void*)
{
    if (WGL_WindowData* data = (WGL_WindowData*)viewport->RendererUserData)
        ::SwapBuffers(data->hDC);
}

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui Win32+OpenGL3 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImPlot3D::CreateContext();

    //load chinese font
    ImFont* DataFont = nullptr;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows
    ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\simkai.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    DataFont = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\simkai.ttf", 25.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    // Win32+GL needs specific hooks for viewport, as there are specific things needed to tie Win32 and GL api.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        IM_ASSERT(platform_io.Renderer_CreateWindow == NULL);
        IM_ASSERT(platform_io.Renderer_DestroyWindow == NULL);
        IM_ASSERT(platform_io.Renderer_SwapBuffers == NULL);
        IM_ASSERT(platform_io.Platform_RenderWindow == NULL);
        platform_io.Renderer_CreateWindow = Hook_Renderer_CreateWindow;
        platform_io.Renderer_DestroyWindow = Hook_Renderer_DestroyWindow;
        platform_io.Renderer_SwapBuffers = Hook_Renderer_SwapBuffers;
        platform_io.Platform_RenderWindow = Hook_Platform_RenderWindow;
    }

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        //ImPlot::ShowDemoWindow();
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            //ImPlot::ShowDemoWindow();
            //ImPlot3D::ShowDemoWindow();
            //ShowDualAxisSensorSettings();
            ShowDualAxisSensor();

        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            //ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            //ImGui::Text("Hello from another window!");
            //if (ImGui::Button("Close Me"))
            //    show_another_window = false;
            //ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            // Restore the OpenGL rendering context to the main window DC, since platform windows might have changed it.
            wglMakeCurrent(g_MainWindow.hDC, g_hRC);
        }

        // Present
        ::SwapBuffers(g_MainWindow.hDC);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (!g_hRC)
        g_hRC = wglCreateContext(data->hDC);
    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
