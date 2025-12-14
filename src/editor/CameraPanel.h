#pragma once

/**
 * @file CameraPanel.h
 * @brief Camera control panel for the editor
 *
 * Provides UI for controlling camera position, rotation, projection settings,
 * and camera controller parameters.
 */

namespace SM
{
    // Forward declarations
    class GameCamera;
    class FPSCameraController;
    class OrbitCameraController;

    /**
     * @brief Camera control panel for editor UI
     *
     * Displays and allows editing of:
     * - Camera position (X, Y, Z)
     * - Camera rotation (Pitch, Yaw)
     * - Projection settings (FOV, Near/Far planes)
     * - Controller settings (speed, sensitivity)
     */
    class CameraPanel
    {
    public:
        CameraPanel() = default;
        ~CameraPanel() = default;

        /**
         * @brief Draw the camera panel using ImGui
         * @param camera Reference to the game camera
         * @param fpsController Pointer to FPS controller (can be nullptr)
         * @param orbitController Pointer to orbit controller (can be nullptr)
         */
        void Draw(GameCamera& camera, FPSCameraController* fpsController, OrbitCameraController* orbitController);

        /**
         * @brief Simple draw overload for camera only
         * @param camera Reference to the game camera
         */
        void Draw(GameCamera& camera);

        // ====================================================================
        // Visibility
        // ====================================================================

        /**
         * @brief Set panel visibility
         */
        void SetVisible(bool visible) { m_Visible = visible; }

        /**
         * @brief Get panel visibility
         */
        bool IsVisible() const { return m_Visible; }

        /**
         * @brief Toggle visibility
         */
        void ToggleVisible() { m_Visible = !m_Visible; }

    private:
        /**
         * @brief Draw position controls
         */
        void DrawPositionControls(GameCamera& camera);

        /**
         * @brief Draw rotation controls
         */
        void DrawRotationControls(GameCamera& camera);

        /**
         * @brief Draw projection settings
         */
        void DrawProjectionSettings(GameCamera& camera);

        /**
         * @brief Draw FPS controller settings
         */
        void DrawFPSControllerSettings(FPSCameraController* controller);

        /**
         * @brief Draw orbit controller settings
         */
        void DrawOrbitControllerSettings(OrbitCameraController* controller);

        /**
         * @brief Draw camera presets
         */
        void DrawPresets(GameCamera& camera);

    private:
        bool m_Visible = true;

        // State for tracking changes
        bool m_PositionChanged = false;
        bool m_RotationChanged = false;
    };

} // namespace SM
