#include "core/Window.h"

#include <stdexcept>

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

            // TODO: Notify renderer of resize
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                PostMessage(m_Handle, WM_CLOSE, 0, 0);
                return 0;
            }
            if (wParam == VK_F11)
            {
                ToggleFullscreen();
                return 0;
            }
            break;

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
