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
std::vector<uint8_t> dualAxisDeviceAddresses = { 0x0C, 0x02 };//经过init以后，使得所有串口号进入
static std::thread pollingThread;
std::vector<uint8_t> collectdualAxisDeveiceAddresses;
static std::unordered_map<uint8_t, bool> displayFlags;
std::mutex dataMutex;
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

    // 创建一个水平布局，左侧为子窗口
    ImGui::BeginChild("LeftSidebar", ImVec2(200, 0), true); // 左侧固定宽度为200像素

    ImGui::Text(u8"显示设备:");
    for (uint8_t addr : collectdualAxisDeveiceAddresses)
    {
        if (displayFlags.find(addr) == displayFlags.end())
            displayFlags[addr] = false;

        char label[32];
        sprintf_s(label, u8"地址: 0x%02X", addr);
        ImGui::Checkbox(label, &displayFlags[addr]);
    }

    ImGui::EndChild(); // 结束左侧子窗口

    ImGui::SameLine(); // 右侧紧跟着左侧显示

    // 右侧主区域
    ImGui::BeginChild("MainArea", ImVec2(0, 0), true); // 剩余区域自动占满

    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
    static int selectedPortIndex = 0;
    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
    static int selectedBaudIndex = 4;
    static bool isConnected = false;
    static std::atomic<bool> collecting = false;

    ShowSerialPortSelector(availablePorts, selectedPortIndex, u8"串口号");
    ShowBaudRateSelector(baudRates, IM_ARRAYSIZE(baudRates), selectedBaudIndex, u8"波特率");

    if (!isConnected)
    {
        if (ImGui::Button(u8"连接"))
        {
            try {
                DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
                serialDualAxis.open(availablePorts[selectedPortIndex], baudRate);
                isConnected = true;
                collecting = true;
                initializedualAxisParsers();
                pollingThread = std::thread([] {
                    while (collecting)
                    {
                        for (uint8_t addr : dualAxisDeviceAddresses) {
                            auto& parser = *dualAxisParsers[addr];
                            auto angles = parser.readAngles();
                            std::cout << "扫描中...addr: " << std::hex << (int)addr << std::endl;
                            std::cout << "angles.filtered_horizontal = " << angles.filtered_horizontal << std::endl;
                            loadingAddress(addr, angles.filtered_horizontal);
                        }
                        break;
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
            collecting = false;
            if (pollingThread.joinable())
                pollingThread.join();
            isConnected = false;
            dualAxisParsers.clear();
            displayFlags.clear();
        }
    }


    ImGui::Separator();
ImGui::Text(u8"设备配置");

static std::unordered_map<uint8_t, int> deviceBaudIndexMap;
static std::unordered_map<uint8_t, int> deviceNewAddrMap;
const char* deviceBaudRates[] = { "9600", "19200", "38400", "57600", "115200" };

for (uint8_t addr : collectdualAxisDeveiceAddresses)
{
    if (!displayFlags[addr])
        continue; // 只显示勾选中的设备

    ImGui::PushID(addr); // 避免控件重复冲突

    ImGui::Separator();
    char title[32];
    sprintf_s(title, u8"设备 0x%02X", addr);
    ImGui::Text(title);

    if (deviceBaudIndexMap.find(addr) == deviceBaudIndexMap.end())
        deviceBaudIndexMap[addr] = 0; // 默认选中9600
    if (deviceNewAddrMap.find(addr) == deviceNewAddrMap.end())
        deviceNewAddrMap[addr] = addr; // 默认当前地址

    // 波特率选择
    ImGui::Text(u8"设置波特率:");
    ImGui::Combo("##BaudRate", &deviceBaudIndexMap[addr], deviceBaudRates, IM_ARRAYSIZE(deviceBaudRates));

    // 地址输入框
    ImGui::Text(u8"新地址:");
    ImGui::InputInt("##NewAddress", &deviceNewAddrMap[addr]);
    if (deviceNewAddrMap[addr] < 0) deviceNewAddrMap[addr] = 0;
    if (deviceNewAddrMap[addr] > 255) deviceNewAddrMap[addr] = 255;

    if (ImGui::Button(u8"应用设置")) {
        try {
            uint8_t baudCode = 0xB8;
            switch (deviceBaudIndexMap[addr]) {
            case 0: baudCode = 0xB2; break; // 9600
            case 1: baudCode = 0xB4; break; // 19200
            case 2: baudCode = 0xB6; break; // 38400
            case 3: baudCode = 0xB7; break; // 57600
            case 4: baudCode = 0xB8; break; // 115200
            }

            uint8_t newAddr = static_cast<uint8_t>(deviceNewAddrMap[addr]);

            auto it = dualAxisParsers.find(addr);
            if (it != dualAxisParsers.end()) {
                it->second->changeSerialConfig(baudCode, 0x00, newAddr); // 无校验
                ImGui::TextColored(ImVec4(0, 1, 0, 1), u8"设置成功！");
            } else {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), u8"设备解析器不存在");
            }

        } catch (const std::exception& e) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"设置失败: %s", e.what());
        }
    }

    ImGui::PopID();
}

    ImGui::EndChild(); // 结束右侧主区域

    ImGui::End(); // 结束整个窗口
}

//void ShowDualAxisSensor()
//{
//    static std::vector<std::string> availablePorts = listAvailableSerialPorts();
//    static int selectedPortIndex = 0;
//    static const char* baudRates[] = { "9600", "19200", "38400", "57600", "115200" };
//    static int selectedBaudIndex = 4;
//    static bool isConnected = false;
//    static std::atomic<bool> collecting = false;
//	
//    // 串口选择下拉框
//    ShowSerialPortSelector(availablePorts, selectedPortIndex, u8"串口号");
//    // 波特率选择下拉框
//    ShowBaudRateSelector(baudRates, IM_ARRAYSIZE(baudRates), selectedBaudIndex, u8"波特率");
//
//    if (!isConnected)
//    {
//        if (ImGui::Button(u8"连接"))
//        {
//            try
//            {
//				DWORD baudRate = std::stoi(baudRates[selectedBaudIndex]);
//                //打开所选的串口号，使用选择的波特率
//				serialDualAxis.open(availablePorts[selectedPortIndex], baudRate);
//                isConnected = true;
//                collecting = true;
//                // 初始化设备地址列表
//                initializedualAxisParsers();
//                // 启动数据采集线程
//                pollingThread = std::thread([] {
//                    while (collecting)
//                    {
//                        for (uint8_t addr : dualAxisDeviceAddresses) {
//                            auto& parser = *dualAxisParsers[addr];
//                            auto angles = parser.readAngles();
//                            std::cout << "扫描中...addr: " << std::hex <<(int)addr << std::endl;
//							std::cout <<"angles.filtered_horizontal = " << angles.filtered_horizontal << std::endl;
//							loadingAddress(addr, angles.filtered_horizontal);
//
//                        }
//
//                    }
//
//                    });
//            }
//            catch (const std::exception& e) {
//                ImGui::TextColored(ImVec4(1, 0, 0, 1), u8"扫描失败: %s", e.what());
//            }
//        }
//    }
//    else
//    {
//        if (ImGui::Button(u8"停止扫描"))
//        {
//            collecting = false;//停止数据采集
//			if (pollingThread.joinable())
//				pollingThread.join(); // 等待线程结束
//            isConnected = false;
//			dualAxisParsers.clear(); // 清空解析器
//			displayFlags.clear(); // 清空显示标志
//        }
//        
//    }
//    for (uint8_t addr : collectdualAxisDeveiceAddresses)
//    {
//        if (displayFlags.find(addr) == displayFlags.end())
//            displayFlags[addr] = false;
//        char label[32];
//		sprintf_s(label, u8"设备地址: 0x%02X", addr);
//        ImGui::Checkbox(label, &displayFlags[addr]);
//    }
//}
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
            ImPlot::ShowDemoWindow();
            ImPlot3D::ShowDemoWindow();
            //ShowDualAxisSensorSettings();
            ShowDualAxisSensor();

        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
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
