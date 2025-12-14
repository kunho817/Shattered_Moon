#include "core/Window.h"
#include "gameplay/Input.h"

#include <stdexcept>
#include <vector>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM

// Forward declare ImGui handler
extern "C" LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace SM
{
    // Static member initialization
    bool Window::s_ClassRegistered = false;

    Window::Window()
        : m_Instance(GetModuleHandle(nullptr))
    {
    }

    Window::~Window()
    {
        Shutdown();
    }

    bool Window::Initialize(const WindowConfig& config)
    {
        m_Title = config.title;
        m_Width = config.width;
        m_Height = config.height;
        m_Fullscreen = config.fullscreen;
        m_Resizable = config.resizable;

        // Register window class
        if (!RegisterWindowClass())
        {
            return false;
        }

        // Create window
        if (!CreateMainWindow())
        {
            return false;
        }

        // Set window handle for input system
        Input::Get().SetWindowHandle(m_Handle);

        // Register for raw mouse input
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01;  // HID_USAGE_PAGE_GENERIC
        rid.usUsage = 0x02;      // HID_USAGE_GENERIC_MOUSE
        rid.dwFlags = 0;
        rid.hwndTarget = m_Handle;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));

        // Show and update window
        ShowWindow(m_Handle, SW_SHOW);
        UpdateWindow(m_Handle);

        return true;
    }

    void Window::Shutdown()
    {
        if (m_Handle)
        {
            DestroyWindow(m_Handle);
            m_Handle = nullptr;
        }
    }

    bool Window::RegisterWindowClass()
    {
        if (s_ClassRegistered)
        {
            return true;
        }

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = m_Instance;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = CLASS_NAME;
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassExW(&wc))
        {
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS)
            {
                return false;
            }
        }

        s_ClassRegistered = true;
        return true;
    }

    bool Window::CreateMainWindow()
    {
        // Calculate window style
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (!m_Resizable)
        {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }

        // Calculate window size to get desired client area
        RECT rect = { 0, 0, m_Width, m_Height };
        AdjustWindowRect(&rect, style, FALSE);

        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;

        // Center window on screen
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenWidth - windowWidth) / 2;
        int y = (screenHeight - windowHeight) / 2;

        // Create window
        m_Handle = CreateWindowExW(
            0,                          // Extended style
            CLASS_NAME,                 // Window class name
            m_Title.c_str(),            // Window title
            style,                      // Window style
            x, y,                       // Position
            windowWidth, windowHeight,  // Size
            nullptr,                    // Parent window
            nullptr,                    // Menu
            m_Instance,                 // Instance
            this                        // User data (pointer to this)
        );

        if (!m_Handle)
        {
            return false;
        }

        // Store windowed mode info for fullscreen toggle
        GetWindowRect(m_Handle, &m_WindowedRect);
        m_WindowedStyle = style;

        // Apply fullscreen if requested
        if (m_Fullscreen)
        {
            m_Fullscreen = false; // Reset so toggle works correctly
            ToggleFullscreen();
        }

        return true;
    }

    bool Window::ProcessMessages()
    {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    void Window::Clear(float r, float g, float b)
    {
        // Simple GDI clear for now - will be replaced by DX12
        HDC hdc = GetDC(m_Handle);
        if (hdc)
        {
            RECT rect;
            GetClientRect(m_Handle, &rect);

            // Convert float color to COLORREF
            int red = static_cast<int>(r * 255.0f);
            int green = static_cast<int>(g * 255.0f);
            int blue = static_cast<int>(b * 255.0f);

            HBRUSH brush = CreateSolidBrush(RGB(red, green, blue));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);

            ReleaseDC(m_Handle, hdc);
        }
    }

    void Window::SetTitle(const std::wstring& title)
    {
        m_Title = title;
        if (m_Handle)
        {
            SetWindowTextW(m_Handle, title.c_str());
        }
    }

    void Window::ToggleFullscreen()
    {
        if (m_Fullscreen)
        {
            // Restore windowed mode
            SetWindowLongPtr(m_Handle, GWL_STYLE, m_WindowedStyle);
            SetWindowPos(
                m_Handle,
                HWND_NOTOPMOST,
                m_WindowedRect.left,
                m_WindowedRect.top,
                m_WindowedRect.right - m_WindowedRect.left,
                m_WindowedRect.bottom - m_WindowedRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE
            );
            ShowWindow(m_Handle, SW_NORMAL);
        }
        else
        {
            // Save current window position
            GetWindowRect(m_Handle, &m_WindowedRect);
            m_WindowedStyle = static_cast<DWORD>(GetWindowLongPtr(m_Handle, GWL_STYLE));

            // Switch to fullscreen
            SetWindowLongPtr(m_Handle, GWL_STYLE, WS_POPUP | WS_VISIBLE);

            HMONITOR monitor = MonitorFromWindow(m_Handle, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO monitorInfo = {};
            monitorInfo.cbSize = sizeof(monitorInfo);
            GetMonitorInfo(monitor, &monitorInfo);

            SetWindowPos(
                m_Handle,
                HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE
            );
            ShowWindow(m_Handle, SW_MAXIMIZE);
        }

        m_Fullscreen = !m_Fullscreen;
    }

    LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Window* window = nullptr;

        if (msg == WM_NCCREATE)
        {
            // Get the Window pointer from creation parameters
            CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
            window = reinterpret_cast<Window*>(create->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        else
        {
            window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (window)
        {
            return window->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // Let ImGui process messages first
        if (ImGui_ImplWin32_WndProcHandler(m_Handle, msg, wParam, lParam))
        {
            return true;
        }

        // Get input system reference
        Input& input = Input::Get();

        switch (msg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_CLOSE:
            DestroyWindow(m_Handle);
            return 0;

        case WM_SIZE:
        {
            m_Width = LOWORD(lParam);
            m_Height = HIWORD(lParam);
            m_Minimized = (wParam == SIZE_MINIMIZED);

            // Notify via callback if set
            if (m_ResizeCallback && m_Width > 0 && m_Height > 0)
            {
                m_ResizeCallback(static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));
            }
            return 0;
        }

        // Keyboard input
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            // Forward to input system
            input.ProcessKeyDown(static_cast<uint32_t>(wParam));

            // Handle special keys
            if (wParam == VK_F11)
            {
                ToggleFullscreen();
                return 0;
            }
            break;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            input.ProcessKeyUp(static_cast<uint32_t>(wParam));
            break;
        }

        // Mouse movement
        case WM_MOUSEMOVE:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            input.ProcessMouseMove(x, y);
            break;
        }

        // Mouse buttons
        case WM_LBUTTONDOWN:
            input.ProcessMouseButton(0, true);
            break;
        case WM_LBUTTONUP:
            input.ProcessMouseButton(0, false);
            break;
        case WM_RBUTTONDOWN:
            input.ProcessMouseButton(1, true);
            break;
        case WM_RBUTTONUP:
            input.ProcessMouseButton(1, false);
            break;
        case WM_MBUTTONDOWN:
            input.ProcessMouseButton(2, true);
            break;
        case WM_MBUTTONUP:
            input.ProcessMouseButton(2, false);
            break;
        case WM_XBUTTONDOWN:
        {
            int button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
            input.ProcessMouseButton(button, true);
            break;
        }
        case WM_XBUTTONUP:
        {
            int button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
            input.ProcessMouseButton(button, false);
            break;
        }

        // Mouse wheel
        case WM_MOUSEWHEEL:
        {
            float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            input.ProcessMouseWheel(delta);
            break;
        }

        // Raw input (for mouse delta in captured mode)
        case WM_INPUT:
        {
            UINT size = 0;
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

            if (size > 0)
            {
                std::vector<BYTE> rawdata(size);
                if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawdata.data(), &size, sizeof(RAWINPUTHEADER)) == size)
                {
                    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawdata.data());
                    if (raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        input.ProcessRawMouseInput(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
                    }
                }
            }
            break;
        }

        // Focus handling
        case WM_KILLFOCUS:
        {
            // Release mouse capture when losing focus
            if (input.IsMouseCaptured())
            {
                input.SetMouseCapture(false);
            }
            // Reset input state
            input.Reset();
            break;
        }

        case WM_GETMINMAXINFO:
        {
            // Set minimum window size
            MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            minMaxInfo->ptMinTrackSize.x = 320;
            minMaxInfo->ptMinTrackSize.y = 240;
            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_Handle, &ps);
            // GDI painting is handled in Clear() for now
            (void)hdc;
            EndPaint(m_Handle, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            // Prevent flickering by handling background erase ourselves
            return 1;
        }

        return DefWindowProc(m_Handle, msg, wParam, lParam);
    }

} // namespace SM
