#pragma once

/**
 * @file Input.h
 * @brief Input handling system for keyboard and mouse
 *
 * Provides a centralized input system that processes raw Win32 input
 * events and exposes a clean API for querying input state.
 */

#include <array>
#include <cstdint>

namespace SM
{
    /**
     * @brief Virtual key codes for input handling
     *
     * Provides a platform-independent way to reference keyboard keys
     * and mouse buttons.
     */
    enum class KeyCode : uint32_t
    {
        // Alphabet keys (A-Z)
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Number keys (0-9)
        Num0, Num1, Num2, Num3, Num4,
        Num5, Num6, Num7, Num8, Num9,

        // Special keys
        Space,
        Enter,
        Escape,
        Tab,
        Backspace,
        Delete,
        Insert,
        Home,
        End,
        PageUp,
        PageDown,

        // Modifier keys
        Shift,
        LeftShift,
        RightShift,
        Ctrl,
        LeftCtrl,
        RightCtrl,
        Alt,
        LeftAlt,
        RightAlt,

        // Arrow keys
        Up,
        Down,
        Left,
        Right,

        // Function keys (F1-F12)
        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,

        // Numpad keys
        Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
        Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
        NumpadAdd,
        NumpadSubtract,
        NumpadMultiply,
        NumpadDivide,
        NumpadEnter,
        NumpadDecimal,
        NumLock,

        // Other keys
        CapsLock,
        ScrollLock,
        PrintScreen,
        Pause,

        // Punctuation and symbols
        Minus,          // -
        Equals,         // =
        LeftBracket,    // [
        RightBracket,   // ]
        Backslash,      //
        Semicolon,      // ;
        Apostrophe,     // '
        Comma,          // ,
        Period,         // .
        Slash,          // /
        Grave,          // `

        // Mouse buttons
        MouseLeft,
        MouseRight,
        MouseMiddle,
        MouseButton4,
        MouseButton5,

        // Must be last - represents the count of key codes
        Count
    };

    /**
     * @brief Mouse button indices
     */
    enum class MouseButton : uint32_t
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4,
        Count = 5
    };

    /**
     * @brief Input system singleton class
     *
     * Manages all input state including keyboard keys and mouse.
     * Call Update() at the start of each frame to update state tracking.
     */
    class Input
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the Input instance
         */
        static Input& Get();

        // Prevent copying
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        /**
         * @brief Update input state at the start of each frame
         *
         * Must be called once per frame before checking input state.
         * Copies current state to previous state and resets per-frame values.
         */
        void Update();

        /**
         * @brief Reset all input state
         *
         * Clears all key and mouse button states.
         */
        void Reset();

        // ====================================================================
        // Raw input event handlers (called from Window)
        // ====================================================================

        /**
         * @brief Process a key down event
         * @param virtualKeyCode Win32 virtual key code (VK_*)
         */
        void ProcessKeyDown(uint32_t virtualKeyCode);

        /**
         * @brief Process a key up event
         * @param virtualKeyCode Win32 virtual key code (VK_*)
         */
        void ProcessKeyUp(uint32_t virtualKeyCode);

        /**
         * @brief Process a mouse move event
         * @param x New X position (client coordinates)
         * @param y New Y position (client coordinates)
         */
        void ProcessMouseMove(int x, int y);

        /**
         * @brief Process a mouse button event
         * @param button Button index (0=left, 1=right, 2=middle)
         * @param down true if button pressed, false if released
         */
        void ProcessMouseButton(int button, bool down);

        /**
         * @brief Process a mouse wheel event
         * @param delta Wheel delta (positive = up, negative = down)
         */
        void ProcessMouseWheel(float delta);

        /**
         * @brief Process a raw mouse input event (for delta tracking)
         * @param deltaX Raw X movement
         * @param deltaY Raw Y movement
         */
        void ProcessRawMouseInput(int deltaX, int deltaY);

        // ====================================================================
        // Keyboard state queries
        // ====================================================================

        /**
         * @brief Check if a key is currently held down
         * @param key Key code to check
         * @return true if key is currently down
         */
        bool IsKeyDown(KeyCode key) const;

        /**
         * @brief Check if a key was pressed this frame
         * @param key Key code to check
         * @return true if key was pressed this frame
         */
        bool IsKeyPressed(KeyCode key) const;

        /**
         * @brief Check if a key was released this frame
         * @param key Key code to check
         * @return true if key was released this frame
         */
        bool IsKeyReleased(KeyCode key) const;

        /**
         * @brief Check if any key is currently down
         * @return true if any key is down
         */
        bool AnyKeyDown() const;

        // ====================================================================
        // Mouse state queries
        // ====================================================================

        /**
         * @brief Get the current mouse X position (client coordinates)
         */
        int GetMouseX() const { return m_MouseX; }

        /**
         * @brief Get the current mouse Y position (client coordinates)
         */
        int GetMouseY() const { return m_MouseY; }

        /**
         * @brief Get the mouse X movement since last frame
         */
        int GetMouseDeltaX() const { return m_MouseDeltaX; }

        /**
         * @brief Get the mouse Y movement since last frame
         */
        int GetMouseDeltaY() const { return m_MouseDeltaY; }

        /**
         * @brief Get the mouse wheel delta for this frame
         */
        float GetMouseWheelDelta() const { return m_MouseWheelDelta; }

        /**
         * @brief Check if a mouse button is currently down
         * @param button Button index (0=left, 1=right, 2=middle)
         * @return true if button is down
         */
        bool IsMouseButtonDown(int button) const;

        /**
         * @brief Check if a mouse button is currently down
         * @param button Mouse button enum
         * @return true if button is down
         */
        bool IsMouseButtonDown(MouseButton button) const;

        /**
         * @brief Check if a mouse button was pressed this frame
         * @param button Button index
         * @return true if button was pressed this frame
         */
        bool IsMouseButtonPressed(int button) const;

        /**
         * @brief Check if a mouse button was released this frame
         * @param button Button index
         * @return true if button was released this frame
         */
        bool IsMouseButtonReleased(int button) const;

        // ====================================================================
        // Mouse capture
        // ====================================================================

        /**
         * @brief Set mouse capture mode
         * @param capture true to capture mouse, false to release
         *
         * When captured, mouse is hidden and confined to the window center.
         * Mouse delta tracking continues to work.
         */
        void SetMouseCapture(bool capture);

        /**
         * @brief Check if mouse is captured
         * @return true if mouse is captured
         */
        bool IsMouseCaptured() const { return m_MouseCaptured; }

        /**
         * @brief Set the window handle for mouse capture operations
         * @param hwnd Window handle
         */
        void SetWindowHandle(void* hwnd) { m_WindowHandle = hwnd; }

        // ====================================================================
        // Utility
        // ====================================================================

        /**
         * @brief Convert a virtual key code to a KeyCode
         * @param virtualKeyCode Win32 VK_* code
         * @return Corresponding KeyCode, or KeyCode::Count if unknown
         */
        static KeyCode VirtualKeyToKeyCode(uint32_t virtualKeyCode);

        /**
         * @brief Get the name of a key code as a string
         * @param key Key code
         * @return String name of the key
         */
        static const char* KeyCodeToString(KeyCode key);

    private:
        Input();
        ~Input() = default;

        /**
         * @brief Update mouse capture state
         */
        void UpdateMouseCapture();

    private:
        // Key states
        static constexpr size_t KEY_COUNT = static_cast<size_t>(KeyCode::Count);
        std::array<bool, KEY_COUNT> m_CurrentKeyState;
        std::array<bool, KEY_COUNT> m_PreviousKeyState;

        // Mouse position (client coordinates)
        int m_MouseX = 0;
        int m_MouseY = 0;
        int m_PreviousMouseX = 0;
        int m_PreviousMouseY = 0;

        // Mouse delta (for captured mode)
        int m_MouseDeltaX = 0;
        int m_MouseDeltaY = 0;
        int m_RawMouseDeltaX = 0;
        int m_RawMouseDeltaY = 0;

        // Mouse wheel
        float m_MouseWheelDelta = 0.0f;

        // Mouse buttons
        static constexpr size_t MOUSE_BUTTON_COUNT = static_cast<size_t>(MouseButton::Count);
        std::array<bool, MOUSE_BUTTON_COUNT> m_CurrentMouseButtons;
        std::array<bool, MOUSE_BUTTON_COUNT> m_PreviousMouseButtons;

        // Mouse capture
        bool m_MouseCaptured = false;
        void* m_WindowHandle = nullptr;
    };

} // namespace SM
