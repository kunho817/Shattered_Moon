#pragma once

/**
 * @file InputAction.h
 * @brief Action-based input mapping system
 *
 * Provides a higher-level abstraction over raw input, allowing
 * named actions and axes to be bound to multiple input keys.
 */

#include "gameplay/Input.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace SM
{
    /**
     * @brief Represents a single input binding
     */
    struct InputBinding
    {
        KeyCode Key = KeyCode::Count;
        float Scale = 1.0f;  // Multiplier for axis value (use -1 for negative axis)
    };

    /**
     * @brief Represents an input action (button-style input)
     */
    struct InputAction
    {
        std::string Name;
        std::vector<InputBinding> Bindings;

        /**
         * @brief Check if this action is currently active
         * @return true if any bound key is down
         */
        bool IsActive() const;

        /**
         * @brief Check if this action was just pressed
         * @return true if any bound key was pressed this frame
         */
        bool WasPressed() const;

        /**
         * @brief Check if this action was just released
         * @return true if all bound keys were released this frame
         */
        bool WasReleased() const;
    };

    /**
     * @brief Represents an input axis (continuous value input)
     */
    struct InputAxis
    {
        std::string Name;
        std::vector<InputBinding> Bindings;

        /**
         * @brief Get the current value of this axis
         * @return Value in range [-1, 1] based on pressed keys
         */
        float GetValue() const;
    };

    /**
     * @brief Input mapping system for action and axis bindings
     *
     * Allows binding named actions and axes to keyboard/mouse inputs,
     * providing a layer of indirection for easy remapping.
     */
    class InputMapper
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the InputMapper instance
         */
        static InputMapper& Get();

        // Prevent copying
        InputMapper(const InputMapper&) = delete;
        InputMapper& operator=(const InputMapper&) = delete;

        /**
         * @brief Clear all bindings
         */
        void Clear();

        // ====================================================================
        // Action binding
        // ====================================================================

        /**
         * @brief Create or get an action
         * @param name Action name
         * @return Reference to the action
         */
        InputAction& CreateAction(const std::string& name);

        /**
         * @brief Bind a key to an action
         * @param actionName Action name
         * @param key Key to bind
         */
        void BindAction(const std::string& actionName, KeyCode key);

        /**
         * @brief Bind multiple keys to an action
         * @param actionName Action name
         * @param keys Keys to bind
         */
        void BindAction(const std::string& actionName, std::initializer_list<KeyCode> keys);

        /**
         * @brief Unbind a key from an action
         * @param actionName Action name
         * @param key Key to unbind
         */
        void UnbindAction(const std::string& actionName, KeyCode key);

        /**
         * @brief Remove all bindings from an action
         * @param actionName Action name
         */
        void ClearAction(const std::string& actionName);

        /**
         * @brief Check if an action exists
         * @param name Action name
         * @return true if action exists
         */
        bool HasAction(const std::string& name) const;

        /**
         * @brief Get an action by name
         * @param name Action name
         * @return Pointer to action, nullptr if not found
         */
        const InputAction* GetAction(const std::string& name) const;

        /**
         * @brief Check if an action is currently pressed (any key down)
         * @param actionName Action name
         * @return true if action is active
         */
        bool IsActionDown(const std::string& actionName) const;

        /**
         * @brief Check if an action was just pressed this frame
         * @param actionName Action name
         * @return true if action was pressed
         */
        bool IsActionPressed(const std::string& actionName) const;

        /**
         * @brief Check if an action was just released this frame
         * @param actionName Action name
         * @return true if action was released
         */
        bool IsActionReleased(const std::string& actionName) const;

        // ====================================================================
        // Axis binding
        // ====================================================================

        /**
         * @brief Create or get an axis
         * @param name Axis name
         * @return Reference to the axis
         */
        InputAxis& CreateAxis(const std::string& name);

        /**
         * @brief Bind a key to an axis with a scale
         * @param axisName Axis name
         * @param key Key to bind
         * @param scale Scale factor (1.0 for positive, -1.0 for negative)
         */
        void BindAxis(const std::string& axisName, KeyCode key, float scale = 1.0f);

        /**
         * @brief Bind positive and negative keys to an axis
         * @param axisName Axis name
         * @param positiveKey Key for positive direction
         * @param negativeKey Key for negative direction
         */
        void BindAxis(const std::string& axisName, KeyCode positiveKey, KeyCode negativeKey);

        /**
         * @brief Unbind a key from an axis
         * @param axisName Axis name
         * @param key Key to unbind
         */
        void UnbindAxis(const std::string& axisName, KeyCode key);

        /**
         * @brief Remove all bindings from an axis
         * @param axisName Axis name
         */
        void ClearAxis(const std::string& axisName);

        /**
         * @brief Check if an axis exists
         * @param name Axis name
         * @return true if axis exists
         */
        bool HasAxis(const std::string& name) const;

        /**
         * @brief Get an axis by name
         * @param name Axis name
         * @return Pointer to axis, nullptr if not found
         */
        const InputAxis* GetAxis(const std::string& name) const;

        /**
         * @brief Get the current value of an axis
         * @param axisName Axis name
         * @return Axis value in range [-1, 1], 0 if axis not found
         */
        float GetAxisValue(const std::string& axisName) const;

        // ====================================================================
        // Default bindings
        // ====================================================================

        /**
         * @brief Setup default game bindings
         *
         * Creates standard bindings for:
         * - Movement: WASD, arrows, QE for up/down
         * - Camera: Mouse look
         * - Actions: Space (jump), Shift (sprint), Escape (pause)
         */
        void SetupDefaultBindings();

        // ====================================================================
        // Utility
        // ====================================================================

        /**
         * @brief Get all actions
         * @return Map of action names to actions
         */
        const std::unordered_map<std::string, InputAction>& GetActions() const { return m_Actions; }

        /**
         * @brief Get all axes
         * @return Map of axis names to axes
         */
        const std::unordered_map<std::string, InputAxis>& GetAxes() const { return m_Axes; }

    private:
        InputMapper() = default;
        ~InputMapper() = default;

    private:
        std::unordered_map<std::string, InputAction> m_Actions;
        std::unordered_map<std::string, InputAxis> m_Axes;
    };

} // namespace SM
