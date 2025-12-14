#include "gameplay/Input.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace SM
{
    Input::Input()
    {
        m_CurrentKeyState.fill(false);
        m_PreviousKeyState.fill(false);
        m_CurrentMouseButtons.fill(false);
        m_PreviousMouseButtons.fill(false);
    }

    Input& Input::Get()
    {
        static Input instance;
        return instance;
    }

    void Input::Update()
    {
        // Copy current state to previous state
        m_PreviousKeyState = m_CurrentKeyState;
        m_PreviousMouseButtons = m_CurrentMouseButtons;

        // Update mouse delta
        if (m_MouseCaptured)
        {
            // Use raw input delta for captured mode
            m_MouseDeltaX = m_RawMouseDeltaX;
            m_MouseDeltaY = m_RawMouseDeltaY;
        }
        else
        {
            // Use position delta for non-captured mode
            m_MouseDeltaX = m_MouseX - m_PreviousMouseX;
            m_MouseDeltaY = m_MouseY - m_PreviousMouseY;
        }

        // Store previous mouse position
        m_PreviousMouseX = m_MouseX;
        m_PreviousMouseY = m_MouseY;

        // Reset per-frame values
        m_RawMouseDeltaX = 0;
        m_RawMouseDeltaY = 0;
        m_MouseWheelDelta = 0.0f;

        // Update mouse capture if needed
        UpdateMouseCapture();
    }

    void Input::Reset()
    {
        m_CurrentKeyState.fill(false);
        m_PreviousKeyState.fill(false);
        m_CurrentMouseButtons.fill(false);
        m_PreviousMouseButtons.fill(false);
        m_MouseX = m_MouseY = 0;
        m_PreviousMouseX = m_PreviousMouseY = 0;
        m_MouseDeltaX = m_MouseDeltaY = 0;
        m_RawMouseDeltaX = m_RawMouseDeltaY = 0;
        m_MouseWheelDelta = 0.0f;
    }

    void Input::ProcessKeyDown(uint32_t virtualKeyCode)
    {
        KeyCode key = VirtualKeyToKeyCode(virtualKeyCode);
        if (key != KeyCode::Count)
        {
            m_CurrentKeyState[static_cast<size_t>(key)] = true;
        }
    }

    void Input::ProcessKeyUp(uint32_t virtualKeyCode)
    {
        KeyCode key = VirtualKeyToKeyCode(virtualKeyCode);
        if (key != KeyCode::Count)
        {
            m_CurrentKeyState[static_cast<size_t>(key)] = false;
        }
    }

    void Input::ProcessMouseMove(int x, int y)
    {
        m_MouseX = x;
        m_MouseY = y;
    }

    void Input::ProcessMouseButton(int button, bool down)
    {
        if (button >= 0 && button < static_cast<int>(MOUSE_BUTTON_COUNT))
        {
            m_CurrentMouseButtons[button] = down;

            // Also update corresponding KeyCode
            KeyCode keyCode = KeyCode::Count;
            switch (button)
            {
            case 0: keyCode = KeyCode::MouseLeft; break;
            case 1: keyCode = KeyCode::MouseRight; break;
            case 2: keyCode = KeyCode::MouseMiddle; break;
            case 3: keyCode = KeyCode::MouseButton4; break;
            case 4: keyCode = KeyCode::MouseButton5; break;
            }

            if (keyCode != KeyCode::Count)
            {
                m_CurrentKeyState[static_cast<size_t>(keyCode)] = down;
            }
        }
    }

    void Input::ProcessMouseWheel(float delta)
    {
        m_MouseWheelDelta += delta;
    }

    void Input::ProcessRawMouseInput(int deltaX, int deltaY)
    {
        m_RawMouseDeltaX += deltaX;
        m_RawMouseDeltaY += deltaY;
    }

    bool Input::IsKeyDown(KeyCode key) const
    {
        if (key == KeyCode::Count)
        {
            return false;
        }
        return m_CurrentKeyState[static_cast<size_t>(key)];
    }

    bool Input::IsKeyPressed(KeyCode key) const
    {
        if (key == KeyCode::Count)
        {
            return false;
        }
        size_t index = static_cast<size_t>(key);
        return m_CurrentKeyState[index] && !m_PreviousKeyState[index];
    }

    bool Input::IsKeyReleased(KeyCode key) const
    {
        if (key == KeyCode::Count)
        {
            return false;
        }
        size_t index = static_cast<size_t>(key);
        return !m_CurrentKeyState[index] && m_PreviousKeyState[index];
    }

    bool Input::AnyKeyDown() const
    {
        for (size_t i = 0; i < KEY_COUNT; ++i)
        {
            if (m_CurrentKeyState[i])
            {
                return true;
            }
        }
        return false;
    }

    bool Input::IsMouseButtonDown(int button) const
    {
        if (button >= 0 && button < static_cast<int>(MOUSE_BUTTON_COUNT))
        {
            return m_CurrentMouseButtons[button];
        }
        return false;
    }

    bool Input::IsMouseButtonDown(MouseButton button) const
    {
        return IsMouseButtonDown(static_cast<int>(button));
    }

    bool Input::IsMouseButtonPressed(int button) const
    {
        if (button >= 0 && button < static_cast<int>(MOUSE_BUTTON_COUNT))
        {
            return m_CurrentMouseButtons[button] && !m_PreviousMouseButtons[button];
        }
        return false;
    }

    bool Input::IsMouseButtonReleased(int button) const
    {
        if (button >= 0 && button < static_cast<int>(MOUSE_BUTTON_COUNT))
        {
            return !m_CurrentMouseButtons[button] && m_PreviousMouseButtons[button];
        }
        return false;
    }

    void Input::SetMouseCapture(bool capture)
    {
        if (m_MouseCaptured == capture)
        {
            return;
        }

        m_MouseCaptured = capture;

        if (m_WindowHandle)
        {
            HWND hwnd = static_cast<HWND>(m_WindowHandle);

            if (capture)
            {
                // Capture mouse
                SetCapture(hwnd);
                ShowCursor(FALSE);

                // Center cursor in window
                RECT rect;
                GetClientRect(hwnd, &rect);
                POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
                ClientToScreen(hwnd, &center);
                SetCursorPos(center.x, center.y);

                // Update our stored position
                m_MouseX = (rect.right - rect.left) / 2;
                m_MouseY = (rect.bottom - rect.top) / 2;
                m_PreviousMouseX = m_MouseX;
                m_PreviousMouseY = m_MouseY;
            }
            else
            {
                // Release mouse
                ReleaseCapture();
                ShowCursor(TRUE);
            }
        }

        // Reset deltas
        m_RawMouseDeltaX = 0;
        m_RawMouseDeltaY = 0;
        m_MouseDeltaX = 0;
        m_MouseDeltaY = 0;
    }

    void Input::UpdateMouseCapture()
    {
        // Keep cursor centered when captured
        if (m_MouseCaptured && m_WindowHandle)
        {
            HWND hwnd = static_cast<HWND>(m_WindowHandle);
            RECT rect;
            GetClientRect(hwnd, &rect);
            POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
            ClientToScreen(hwnd, &center);
            SetCursorPos(center.x, center.y);

            // Update stored position to center
            m_MouseX = (rect.right - rect.left) / 2;
            m_MouseY = (rect.bottom - rect.top) / 2;
        }
    }

    KeyCode Input::VirtualKeyToKeyCode(uint32_t virtualKeyCode)
    {
        // Alphabet keys
        if (virtualKeyCode >= 'A' && virtualKeyCode <= 'Z')
        {
            return static_cast<KeyCode>(virtualKeyCode - 'A' + static_cast<uint32_t>(KeyCode::A));
        }

        // Number keys
        if (virtualKeyCode >= '0' && virtualKeyCode <= '9')
        {
            return static_cast<KeyCode>(virtualKeyCode - '0' + static_cast<uint32_t>(KeyCode::Num0));
        }

        // Function keys
        if (virtualKeyCode >= VK_F1 && virtualKeyCode <= VK_F12)
        {
            return static_cast<KeyCode>(virtualKeyCode - VK_F1 + static_cast<uint32_t>(KeyCode::F1));
        }

        // Numpad keys
        if (virtualKeyCode >= VK_NUMPAD0 && virtualKeyCode <= VK_NUMPAD9)
        {
            return static_cast<KeyCode>(virtualKeyCode - VK_NUMPAD0 + static_cast<uint32_t>(KeyCode::Numpad0));
        }

        // Special keys
        switch (virtualKeyCode)
        {
        case VK_SPACE:      return KeyCode::Space;
        case VK_RETURN:     return KeyCode::Enter;
        case VK_ESCAPE:     return KeyCode::Escape;
        case VK_TAB:        return KeyCode::Tab;
        case VK_BACK:       return KeyCode::Backspace;
        case VK_DELETE:     return KeyCode::Delete;
        case VK_INSERT:     return KeyCode::Insert;
        case VK_HOME:       return KeyCode::Home;
        case VK_END:        return KeyCode::End;
        case VK_PRIOR:      return KeyCode::PageUp;
        case VK_NEXT:       return KeyCode::PageDown;

        // Modifiers
        case VK_SHIFT:      return KeyCode::Shift;
        case VK_LSHIFT:     return KeyCode::LeftShift;
        case VK_RSHIFT:     return KeyCode::RightShift;
        case VK_CONTROL:    return KeyCode::Ctrl;
        case VK_LCONTROL:   return KeyCode::LeftCtrl;
        case VK_RCONTROL:   return KeyCode::RightCtrl;
        case VK_MENU:       return KeyCode::Alt;
        case VK_LMENU:      return KeyCode::LeftAlt;
        case VK_RMENU:      return KeyCode::RightAlt;

        // Arrow keys
        case VK_UP:         return KeyCode::Up;
        case VK_DOWN:       return KeyCode::Down;
        case VK_LEFT:       return KeyCode::Left;
        case VK_RIGHT:      return KeyCode::Right;

        // Numpad special
        case VK_ADD:        return KeyCode::NumpadAdd;
        case VK_SUBTRACT:   return KeyCode::NumpadSubtract;
        case VK_MULTIPLY:   return KeyCode::NumpadMultiply;
        case VK_DIVIDE:     return KeyCode::NumpadDivide;
        case VK_DECIMAL:    return KeyCode::NumpadDecimal;
        case VK_NUMLOCK:    return KeyCode::NumLock;

        // Lock keys
        case VK_CAPITAL:    return KeyCode::CapsLock;
        case VK_SCROLL:     return KeyCode::ScrollLock;
        case VK_SNAPSHOT:   return KeyCode::PrintScreen;
        case VK_PAUSE:      return KeyCode::Pause;

        // OEM keys (US keyboard layout)
        case VK_OEM_MINUS:  return KeyCode::Minus;
        case VK_OEM_PLUS:   return KeyCode::Equals;
        case VK_OEM_4:      return KeyCode::LeftBracket;
        case VK_OEM_6:      return KeyCode::RightBracket;
        case VK_OEM_5:      return KeyCode::Backslash;
        case VK_OEM_1:      return KeyCode::Semicolon;
        case VK_OEM_7:      return KeyCode::Apostrophe;
        case VK_OEM_COMMA:  return KeyCode::Comma;
        case VK_OEM_PERIOD: return KeyCode::Period;
        case VK_OEM_2:      return KeyCode::Slash;
        case VK_OEM_3:      return KeyCode::Grave;

        // Mouse buttons (typically handled separately, but included for completeness)
        case VK_LBUTTON:    return KeyCode::MouseLeft;
        case VK_RBUTTON:    return KeyCode::MouseRight;
        case VK_MBUTTON:    return KeyCode::MouseMiddle;
        case VK_XBUTTON1:   return KeyCode::MouseButton4;
        case VK_XBUTTON2:   return KeyCode::MouseButton5;

        default:
            return KeyCode::Count;
        }
    }

    const char* Input::KeyCodeToString(KeyCode key)
    {
        switch (key)
        {
        case KeyCode::A: return "A";
        case KeyCode::B: return "B";
        case KeyCode::C: return "C";
        case KeyCode::D: return "D";
        case KeyCode::E: return "E";
        case KeyCode::F: return "F";
        case KeyCode::G: return "G";
        case KeyCode::H: return "H";
        case KeyCode::I: return "I";
        case KeyCode::J: return "J";
        case KeyCode::K: return "K";
        case KeyCode::L: return "L";
        case KeyCode::M: return "M";
        case KeyCode::N: return "N";
        case KeyCode::O: return "O";
        case KeyCode::P: return "P";
        case KeyCode::Q: return "Q";
        case KeyCode::R: return "R";
        case KeyCode::S: return "S";
        case KeyCode::T: return "T";
        case KeyCode::U: return "U";
        case KeyCode::V: return "V";
        case KeyCode::W: return "W";
        case KeyCode::X: return "X";
        case KeyCode::Y: return "Y";
        case KeyCode::Z: return "Z";

        case KeyCode::Num0: return "0";
        case KeyCode::Num1: return "1";
        case KeyCode::Num2: return "2";
        case KeyCode::Num3: return "3";
        case KeyCode::Num4: return "4";
        case KeyCode::Num5: return "5";
        case KeyCode::Num6: return "6";
        case KeyCode::Num7: return "7";
        case KeyCode::Num8: return "8";
        case KeyCode::Num9: return "9";

        case KeyCode::Space: return "Space";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Escape: return "Escape";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Delete: return "Delete";
        case KeyCode::Insert: return "Insert";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";
        case KeyCode::PageUp: return "PageUp";
        case KeyCode::PageDown: return "PageDown";

        case KeyCode::Shift: return "Shift";
        case KeyCode::LeftShift: return "LeftShift";
        case KeyCode::RightShift: return "RightShift";
        case KeyCode::Ctrl: return "Ctrl";
        case KeyCode::LeftCtrl: return "LeftCtrl";
        case KeyCode::RightCtrl: return "RightCtrl";
        case KeyCode::Alt: return "Alt";
        case KeyCode::LeftAlt: return "LeftAlt";
        case KeyCode::RightAlt: return "RightAlt";

        case KeyCode::Up: return "Up";
        case KeyCode::Down: return "Down";
        case KeyCode::Left: return "Left";
        case KeyCode::Right: return "Right";

        case KeyCode::F1: return "F1";
        case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3";
        case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5";
        case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7";
        case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9";
        case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11";
        case KeyCode::F12: return "F12";

        case KeyCode::Numpad0: return "Numpad0";
        case KeyCode::Numpad1: return "Numpad1";
        case KeyCode::Numpad2: return "Numpad2";
        case KeyCode::Numpad3: return "Numpad3";
        case KeyCode::Numpad4: return "Numpad4";
        case KeyCode::Numpad5: return "Numpad5";
        case KeyCode::Numpad6: return "Numpad6";
        case KeyCode::Numpad7: return "Numpad7";
        case KeyCode::Numpad8: return "Numpad8";
        case KeyCode::Numpad9: return "Numpad9";
        case KeyCode::NumpadAdd: return "NumpadAdd";
        case KeyCode::NumpadSubtract: return "NumpadSubtract";
        case KeyCode::NumpadMultiply: return "NumpadMultiply";
        case KeyCode::NumpadDivide: return "NumpadDivide";
        case KeyCode::NumpadEnter: return "NumpadEnter";
        case KeyCode::NumpadDecimal: return "NumpadDecimal";
        case KeyCode::NumLock: return "NumLock";

        case KeyCode::CapsLock: return "CapsLock";
        case KeyCode::ScrollLock: return "ScrollLock";
        case KeyCode::PrintScreen: return "PrintScreen";
        case KeyCode::Pause: return "Pause";

        case KeyCode::Minus: return "Minus";
        case KeyCode::Equals: return "Equals";
        case KeyCode::LeftBracket: return "LeftBracket";
        case KeyCode::RightBracket: return "RightBracket";
        case KeyCode::Backslash: return "Backslash";
        case KeyCode::Semicolon: return "Semicolon";
        case KeyCode::Apostrophe: return "Apostrophe";
        case KeyCode::Comma: return "Comma";
        case KeyCode::Period: return "Period";
        case KeyCode::Slash: return "Slash";
        case KeyCode::Grave: return "Grave";

        case KeyCode::MouseLeft: return "MouseLeft";
        case KeyCode::MouseRight: return "MouseRight";
        case KeyCode::MouseMiddle: return "MouseMiddle";
        case KeyCode::MouseButton4: return "MouseButton4";
        case KeyCode::MouseButton5: return "MouseButton5";

        default: return "Unknown";
        }
    }

} // namespace SM
