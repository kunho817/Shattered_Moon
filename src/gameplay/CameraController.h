#pragma once

/**
 * @file CameraController.h
 * @brief Camera controller classes for different camera behaviors
 *
 * Provides various camera controller implementations:
 * - FPS style (first-person shooter)
 * - Orbit style (third-person around target)
 */

#include "gameplay/Camera.h"
#include "gameplay/Input.h"
#include "gameplay/InputAction.h"

namespace SM
{
    /**
     * @brief FPS-style camera controller
     *
     * Provides first-person camera control with:
     * - WASD movement relative to camera direction
     * - Mouse look (when captured)
     * - Sprint modifier
     * - Smooth movement
     */
    class FPSCameraController
    {
    public:
        /**
         * @brief Construct an FPS camera controller
         * @param camera Reference to the camera to control
         */
        explicit FPSCameraController(GameCamera& camera);

        /**
         * @brief Update the camera based on input
         * @param deltaTime Time since last frame in seconds
         */
        void Update(float deltaTime);

        // ====================================================================
        // Settings
        // ====================================================================

        /**
         * @brief Set the base movement speed
         * @param speed Speed in units per second
         */
        void SetMoveSpeed(float speed) { m_MoveSpeed = speed; }

        /**
         * @brief Get the base movement speed
         */
        float GetMoveSpeed() const { return m_MoveSpeed; }

        /**
         * @brief Set the mouse sensitivity
         * @param sensitivity Degrees per pixel of mouse movement
         */
        void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }

        /**
         * @brief Get the mouse sensitivity
         */
        float GetMouseSensitivity() const { return m_MouseSensitivity; }

        /**
         * @brief Set the sprint speed multiplier
         * @param multiplier Sprint speed multiplier (e.g., 2.0 for double speed)
         */
        void SetSprintMultiplier(float multiplier) { m_SprintMultiplier = multiplier; }

        /**
         * @brief Get the sprint speed multiplier
         */
        float GetSprintMultiplier() const { return m_SprintMultiplier; }

        /**
         * @brief Enable or disable movement smoothing
         * @param enabled true to enable smoothing
         */
        void SetSmoothingEnabled(bool enabled) { m_SmoothingEnabled = enabled; }

        /**
         * @brief Set movement smoothing factor
         * @param factor Smoothing factor (0-1, lower = smoother but more lag)
         */
        void SetSmoothingFactor(float factor) { m_SmoothingFactor = factor; }

        /**
         * @brief Enable or disable vertical movement (Q/E)
         * @param enabled true to enable vertical movement
         */
        void SetVerticalMovementEnabled(bool enabled) { m_VerticalMovementEnabled = enabled; }

        /**
         * @brief Check if mouse capture mode is active
         */
        bool IsMouseCaptured() const { return Input::Get().IsMouseCaptured(); }

    private:
        /**
         * @brief Handle mouse look input
         * @param deltaTime Time since last frame
         */
        void HandleMouseLook(float deltaTime);

        /**
         * @brief Handle keyboard movement input
         * @param deltaTime Time since last frame
         */
        void HandleMovement(float deltaTime);

    private:
        GameCamera& m_Camera;

        // Movement settings
        float m_MoveSpeed = 10.0f;
        float m_SprintMultiplier = 2.0f;
        float m_MouseSensitivity = 0.15f;

        // Smoothing
        bool m_SmoothingEnabled = true;
        float m_SmoothingFactor = 0.2f;
        Vector3 m_Velocity;

        // Options
        bool m_VerticalMovementEnabled = true;
    };

    /**
     * @brief Orbit-style camera controller
     *
     * Provides third-person camera control that orbits around a target:
     * - Mouse drag to rotate around target
     * - Mouse wheel to zoom in/out
     * - Smooth rotation and zoom
     */
    class OrbitCameraController
    {
    public:
        /**
         * @brief Construct an orbit camera controller
         * @param camera Reference to the camera to control
         */
        explicit OrbitCameraController(GameCamera& camera);

        /**
         * @brief Update the camera based on input
         * @param deltaTime Time since last frame in seconds
         */
        void Update(float deltaTime);

        // ====================================================================
        // Target and distance
        // ====================================================================

        /**
         * @brief Set the target point to orbit around
         * @param target Target position
         */
        void SetTarget(const Vector3& target);

        /**
         * @brief Set the target point to orbit around
         * @param x Target X position
         * @param y Target Y position
         * @param z Target Z position
         */
        void SetTarget(float x, float y, float z);

        /**
         * @brief Get the current target position
         */
        const Vector3& GetTarget() const { return m_Target; }

        /**
         * @brief Set the distance from target
         * @param distance Distance in units
         */
        void SetDistance(float distance);

        /**
         * @brief Get the current distance from target
         */
        float GetDistance() const { return m_Distance; }

        /**
         * @brief Set the distance limits
         * @param minDistance Minimum distance
         * @param maxDistance Maximum distance
         */
        void SetDistanceLimits(float minDistance, float maxDistance);

        // ====================================================================
        // Rotation
        // ====================================================================

        /**
         * @brief Set the azimuth (horizontal) angle
         * @param degrees Angle in degrees
         */
        void SetAzimuth(float degrees);

        /**
         * @brief Get the azimuth angle
         */
        float GetAzimuth() const { return m_Azimuth; }

        /**
         * @brief Set the elevation (vertical) angle
         * @param degrees Angle in degrees
         */
        void SetElevation(float degrees);

        /**
         * @brief Get the elevation angle
         */
        float GetElevation() const { return m_Elevation; }

        /**
         * @brief Set elevation angle limits
         * @param minElevation Minimum elevation (degrees)
         * @param maxElevation Maximum elevation (degrees)
         */
        void SetElevationLimits(float minElevation, float maxElevation);

        // ====================================================================
        // Settings
        // ====================================================================

        /**
         * @brief Set the rotation speed
         * @param speed Degrees per pixel of mouse movement
         */
        void SetRotationSpeed(float speed) { m_RotationSpeed = speed; }

        /**
         * @brief Get the rotation speed
         */
        float GetRotationSpeed() const { return m_RotationSpeed; }

        /**
         * @brief Set the zoom speed
         * @param speed Units per wheel tick
         */
        void SetZoomSpeed(float speed) { m_ZoomSpeed = speed; }

        /**
         * @brief Get the zoom speed
         */
        float GetZoomSpeed() const { return m_ZoomSpeed; }

        /**
         * @brief Set the pan speed (for middle mouse drag)
         * @param speed Units per pixel of mouse movement
         */
        void SetPanSpeed(float speed) { m_PanSpeed = speed; }

        /**
         * @brief Enable or disable auto-rotation
         * @param enabled true to enable auto-rotation
         * @param speed Rotation speed in degrees per second
         */
        void SetAutoRotate(bool enabled, float speed = 20.0f);

        /**
         * @brief Enable or disable smoothing
         * @param enabled true to enable smoothing
         */
        void SetSmoothingEnabled(bool enabled) { m_SmoothingEnabled = enabled; }

        /**
         * @brief Set smoothing factor
         * @param factor Smoothing factor (0-1, lower = smoother)
         */
        void SetSmoothingFactor(float factor) { m_SmoothingFactor = factor; }

    private:
        /**
         * @brief Update camera position from spherical coordinates
         */
        void UpdateCameraPosition();

        /**
         * @brief Handle rotation input
         * @param deltaTime Time since last frame
         */
        void HandleRotation(float deltaTime);

        /**
         * @brief Handle zoom input
         * @param deltaTime Time since last frame
         */
        void HandleZoom(float deltaTime);

        /**
         * @brief Handle pan input
         * @param deltaTime Time since last frame
         */
        void HandlePan(float deltaTime);

    private:
        GameCamera& m_Camera;

        // Orbit parameters
        Vector3 m_Target;
        float m_Distance = 50.0f;
        float m_Azimuth = 0.0f;      // Horizontal angle (degrees)
        float m_Elevation = 30.0f;   // Vertical angle (degrees)

        // Target values for smoothing
        float m_TargetDistance = 50.0f;
        float m_TargetAzimuth = 0.0f;
        float m_TargetElevation = 30.0f;

        // Limits
        float m_MinDistance = 5.0f;
        float m_MaxDistance = 500.0f;
        float m_MinElevation = -89.0f;
        float m_MaxElevation = 89.0f;

        // Speed settings
        float m_RotationSpeed = 0.3f;
        float m_ZoomSpeed = 5.0f;
        float m_PanSpeed = 0.01f;

        // Auto rotation
        bool m_AutoRotate = false;
        float m_AutoRotateSpeed = 20.0f;

        // Smoothing
        bool m_SmoothingEnabled = true;
        float m_SmoothingFactor = 0.15f;

        // Drag state
        bool m_IsDragging = false;
    };

} // namespace SM
