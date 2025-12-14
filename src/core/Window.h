#pragma once

/**
 * @file Window.h
 * @brief Win32 Window management
 *
 * Handles window creation, message processing, and basic rendering operations.
 * Also forwards input events to the Input system.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <string>
#include <functional>

namespace SM
{
    /**
     * @brief Window configuration structure
     */
    struct WindowConfig
    {
        std::wstring title = L"Shattered Moon";
        int width = 1280;
        int height = 720;
        bool fullscreen = false;
        bool resizable = true;
    };

    /**
     * @brief Win32 Window wrapper class
     *
     * Manages a Win32 window including creation, destruction,
     * and message processing.
     */
    class Window
    {
    public:
        Window();
        ~Window();

        // Prevent copying
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        /**
         * @brief Initialize and create the window
         * @param config Window configuration
         * @return true if window was created successfully
         */
        bool Initialize(const WindowConfig& config = WindowConfig{});

        /**
         * @brief Shutdown and destroy the window
         */
        void Shutdown();

        /**
         * @brief Process window messages
         * @return false if WM_QUIT was received, true otherwise
         */
        bool ProcessMessages();

        /**
         * @brief Clear the window with a solid color (GDI fallback)
         * @param r Red component (0.0 - 1.0)
         * @param g Green component (0.0 - 1.0)
         * @param b Blue component (0.0 - 1.0)
         */
        void Clear(float r, float g, float b);

        /**
         * @brief Set the window title
         * @param title New window title
         */
        void SetTitle(const std::wstring& title);

        /**
         * @brief Get the native window handle
         * @return HWND handle
         */
        HWND GetHandle() const { return m_Handle; }

        /**
         * @brief Get the window width
         * @return Width in pixels
         */
        int GetWidth() const { return m_Width; }

        /**
         * @brief Get the window height
         * @return Height in pixels
         */
        int GetHeight() const { return m_Height; }

        /**
         * @brief Check if window is in fullscreen mode
         * @return true if fullscreen
         */
        bool IsFullscreen() const { return m_Fullscreen; }

        /**
         * @brief Check if the window is minimized
         * @return true if minimized
         */
        bool IsMinimized() const { return m_Minimized; }

        /**
         * @brief Toggle fullscreen mode
         */
        void ToggleFullscreen();

        /**
         * @brief Set resize callback
         * @param callback Function to call on window resize
         */
        void SetResizeCallback(std::function<void(uint32_t, uint32_t)> callback)
        {
            m_ResizeCallback = callback;
        }

    private:
        /**
         * @brief Register the window class
         * @return true if registration succeeded
         */
        bool RegisterWindowClass();

        /**
         * @brief Create the actual window
         * @return true if creation succeeded
         */
        bool CreateMainWindow();

        /**
         * @brief Static window procedure callback
         */
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        /**
         * @brief Instance window procedure
         */
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        // Window handle
        HWND m_Handle = nullptr;
        HINSTANCE m_Instance = nullptr;

        // Window properties
        std::wstring m_Title;
        int m_Width = 1280;
        int m_Height = 720;
        bool m_Fullscreen = false;
        bool m_Resizable = true;
        bool m_Minimized = false;

        // Windowed mode backup (for fullscreen toggle)
        RECT m_WindowedRect = {};
        DWORD m_WindowedStyle = 0;

        // Window class name
        static constexpr const wchar_t* CLASS_NAME = L"ShatteredMoonWindowClass";
        static bool s_ClassRegistered;

        // Callbacks
        std::function<void(uint32_t, uint32_t)> m_ResizeCallback;
    };

} // namespace SM
