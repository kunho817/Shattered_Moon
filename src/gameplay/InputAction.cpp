#include "gameplay/InputAction.h"

namespace SM
{
    // ========================================================================
    // InputAction implementation
    // ========================================================================

    bool InputAction::IsActive() const
    {
        const Input& input = Input::Get();
        for (const auto& binding : Bindings)
        {
            if (input.IsKeyDown(binding.Key))
            {
                return true;
            }
        }
        return false;
    }

    bool InputAction::WasPressed() const
    {
        const Input& input = Input::Get();
        for (const auto& binding : Bindings)
        {
            if (input.IsKeyPressed(binding.Key))
            {
                return true;
            }
        }
        return false;
    }

    bool InputAction::WasReleased() const
    {
        const Input& input = Input::Get();

        // Check if all keys are now up
        bool anyWasDown = false;
        bool allAreUp = true;

        for (const auto& binding : Bindings)
        {
            if (input.IsKeyReleased(binding.Key))
            {
                anyWasDown = true;
            }
            if (input.IsKeyDown(binding.Key))
            {
                allAreUp = false;
            }
        }

        return anyWasDown && allAreUp;
    }

    // ========================================================================
    // InputAxis implementation
    // ========================================================================

    float InputAxis::GetValue() const
    {
        const Input& input = Input::Get();
        float value = 0.0f;

        for (const auto& binding : Bindings)
        {
            if (input.IsKeyDown(binding.Key))
            {
                value += binding.Scale;
            }
        }

        // Clamp to [-1, 1]
        if (value > 1.0f) value = 1.0f;
        if (value < -1.0f) value = -1.0f;

        return value;
    }

    // ========================================================================
    // InputMapper implementation
    // ========================================================================

    InputMapper& InputMapper::Get()
    {
        static InputMapper instance;
        return instance;
    }

    void InputMapper::Clear()
    {
        m_Actions.clear();
        m_Axes.clear();
    }

    InputAction& InputMapper::CreateAction(const std::string& name)
    {
        auto it = m_Actions.find(name);
        if (it != m_Actions.end())
        {
            return it->second;
        }

        InputAction& action = m_Actions[name];
        action.Name = name;
        return action;
    }

    void InputMapper::BindAction(const std::string& actionName, KeyCode key)
    {
        InputAction& action = CreateAction(actionName);

        // Check if already bound
        for (const auto& binding : action.Bindings)
        {
            if (binding.Key == key)
            {
                return;
            }
        }

        action.Bindings.push_back({ key, 1.0f });
    }

    void InputMapper::BindAction(const std::string& actionName, std::initializer_list<KeyCode> keys)
    {
        for (KeyCode key : keys)
        {
            BindAction(actionName, key);
        }
    }

    void InputMapper::UnbindAction(const std::string& actionName, KeyCode key)
    {
        auto it = m_Actions.find(actionName);
        if (it == m_Actions.end())
        {
            return;
        }

        auto& bindings = it->second.Bindings;
        bindings.erase(
            std::remove_if(bindings.begin(), bindings.end(),
                [key](const InputBinding& b) { return b.Key == key; }),
            bindings.end()
        );
    }

    void InputMapper::ClearAction(const std::string& actionName)
    {
        auto it = m_Actions.find(actionName);
        if (it != m_Actions.end())
        {
            it->second.Bindings.clear();
        }
    }

    bool InputMapper::HasAction(const std::string& name) const
    {
        return m_Actions.find(name) != m_Actions.end();
    }

    const InputAction* InputMapper::GetAction(const std::string& name) const
    {
        auto it = m_Actions.find(name);
        if (it != m_Actions.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool InputMapper::IsActionDown(const std::string& actionName) const
    {
        const InputAction* action = GetAction(actionName);
        return action ? action->IsActive() : false;
    }

    bool InputMapper::IsActionPressed(const std::string& actionName) const
    {
        const InputAction* action = GetAction(actionName);
        return action ? action->WasPressed() : false;
    }

    bool InputMapper::IsActionReleased(const std::string& actionName) const
    {
        const InputAction* action = GetAction(actionName);
        return action ? action->WasReleased() : false;
    }

    InputAxis& InputMapper::CreateAxis(const std::string& name)
    {
        auto it = m_Axes.find(name);
        if (it != m_Axes.end())
        {
            return it->second;
        }

        InputAxis& axis = m_Axes[name];
        axis.Name = name;
        return axis;
    }

    void InputMapper::BindAxis(const std::string& axisName, KeyCode key, float scale)
    {
        InputAxis& axis = CreateAxis(axisName);

        // Check if already bound with same scale
        for (auto& binding : axis.Bindings)
        {
            if (binding.Key == key)
            {
                binding.Scale = scale;
                return;
            }
        }

        axis.Bindings.push_back({ key, scale });
    }

    void InputMapper::BindAxis(const std::string& axisName, KeyCode positiveKey, KeyCode negativeKey)
    {
        BindAxis(axisName, positiveKey, 1.0f);
        BindAxis(axisName, negativeKey, -1.0f);
    }

    void InputMapper::UnbindAxis(const std::string& axisName, KeyCode key)
    {
        auto it = m_Axes.find(axisName);
        if (it == m_Axes.end())
        {
            return;
        }

        auto& bindings = it->second.Bindings;
        bindings.erase(
            std::remove_if(bindings.begin(), bindings.end(),
                [key](const InputBinding& b) { return b.Key == key; }),
            bindings.end()
        );
    }

    void InputMapper::ClearAxis(const std::string& axisName)
    {
        auto it = m_Axes.find(axisName);
        if (it != m_Axes.end())
        {
            it->second.Bindings.clear();
        }
    }

    bool InputMapper::HasAxis(const std::string& name) const
    {
        return m_Axes.find(name) != m_Axes.end();
    }

    const InputAxis* InputMapper::GetAxis(const std::string& name) const
    {
        auto it = m_Axes.find(name);
        if (it != m_Axes.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    float InputMapper::GetAxisValue(const std::string& axisName) const
    {
        const InputAxis* axis = GetAxis(axisName);
        return axis ? axis->GetValue() : 0.0f;
    }

    void InputMapper::SetupDefaultBindings()
    {
        // Clear existing bindings
        Clear();

        // Movement axes
        BindAxis("MoveForward", KeyCode::W, KeyCode::S);
        BindAxis("MoveRight", KeyCode::D, KeyCode::A);
        BindAxis("MoveUp", KeyCode::E, KeyCode::Q);

        // Alternative arrow key bindings for movement
        BindAxis("MoveForward", KeyCode::Up, 1.0f);
        BindAxis("MoveForward", KeyCode::Down, -1.0f);
        BindAxis("MoveRight", KeyCode::Right, 1.0f);
        BindAxis("MoveRight", KeyCode::Left, -1.0f);

        // Look axes (for gamepad, mouse is handled separately)
        BindAxis("LookUp", KeyCode::Numpad8, KeyCode::Numpad2);
        BindAxis("LookRight", KeyCode::Numpad6, KeyCode::Numpad4);

        // Common actions
        BindAction("Sprint", KeyCode::Shift);
        BindAction("Sprint", KeyCode::LeftShift);
        BindAction("Jump", KeyCode::Space);
        BindAction("Crouch", KeyCode::LeftCtrl);
        BindAction("Crouch", KeyCode::C);

        // Camera actions
        BindAction("CameraCapture", KeyCode::MouseRight);
        BindAction("CameraZoomIn", KeyCode::MouseMiddle);

        // UI actions
        BindAction("Pause", KeyCode::Escape);
        BindAction("Menu", KeyCode::Tab);

        // Debug actions
        BindAction("ToggleWireframe", KeyCode::F1);
        BindAction("ToggleDebugInfo", KeyCode::F3);
        BindAction("ResetCamera", KeyCode::R);
    }

} // namespace SM
