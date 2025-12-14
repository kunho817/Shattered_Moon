#pragma once

/**
 * @file Camera.h
 * @brief Camera system for 3D rendering
 *
 * Provides a flexible camera class supporting both perspective
 * and orthographic projections with various transform operations.
 */

#include "ecs/components/TransformComponent.h"  // For Vector3
#include <DirectXMath.h>

namespace SM
{
    /**
     * @brief 4x4 Matrix wrapper
     */
    struct Matrix4x4
    {
        DirectX::XMFLOAT4X4 m;

        Matrix4x4()
        {
            DirectX::XMStoreFloat4x4(&m, DirectX::XMMatrixIdentity());
        }

        Matrix4x4(const DirectX::XMMATRIX& mat)
        {
            DirectX::XMStoreFloat4x4(&m, mat);
        }

        DirectX::XMMATRIX ToXMMATRIX() const
        {
            return DirectX::XMLoadFloat4x4(&m);
        }

        static Matrix4x4 Identity()
        {
            return Matrix4x4(DirectX::XMMatrixIdentity());
        }
    };

    /**
     * @brief Projection type enumeration
     */
    enum class ProjectionType
    {
        Perspective,
        Orthographic
    };

    /**
     * @brief Flexible 3D camera class
     *
     * Supports both perspective and orthographic projections.
     * Uses a pitch/yaw rotation model for FPS-style camera control.
     */
    class GameCamera
    {
    public:
        GameCamera();
        ~GameCamera() = default;

        // ====================================================================
        // Transform operations
        // ====================================================================

        /**
         * @brief Set the camera position
         * @param position World position
         */
        void SetPosition(const Vector3& position);

        /**
         * @brief Set the camera position
         * @param x X position
         * @param y Y position
         * @param z Z position
         */
        void SetPosition(float x, float y, float z);

        /**
         * @brief Move the camera by an offset
         * @param offset Movement offset
         */
        void Move(const Vector3& offset);

        /**
         * @brief Move the camera relative to its orientation
         * @param forward Forward movement
         * @param right Right movement
         * @param up Up movement
         */
        void MoveRelative(float forward, float right, float up);

        /**
         * @brief Set the camera rotation using pitch and yaw
         * @param pitchDegrees Pitch angle (up/down) in degrees
         * @param yawDegrees Yaw angle (left/right) in degrees
         */
        void SetRotation(float pitchDegrees, float yawDegrees);

        /**
         * @brief Rotate the camera by delta angles
         * @param deltaPitchDegrees Pitch change in degrees
         * @param deltaYawDegrees Yaw change in degrees
         */
        void Rotate(float deltaPitchDegrees, float deltaYawDegrees);

        /**
         * @brief Make the camera look at a target point
         * @param target Target position to look at
         */
        void LookAt(const Vector3& target);

        /**
         * @brief Make the camera look at a target point
         * @param x Target X position
         * @param y Target Y position
         * @param z Target Z position
         */
        void LookAt(float x, float y, float z);

        // ====================================================================
        // Projection settings
        // ====================================================================

        /**
         * @brief Set perspective projection
         * @param fovDegrees Field of view in degrees
         * @param aspectRatio Width / height
         * @param nearZ Near clipping plane
         * @param farZ Far clipping plane
         */
        void SetPerspective(float fovDegrees, float aspectRatio, float nearZ, float farZ);

        /**
         * @brief Set orthographic projection
         * @param width View width
         * @param height View height
         * @param nearZ Near clipping plane
         * @param farZ Far clipping plane
         */
        void SetOrthographic(float width, float height, float nearZ, float farZ);

        /**
         * @brief Update aspect ratio (for window resize)
         * @param aspectRatio New aspect ratio
         */
        void SetAspectRatio(float aspectRatio);

        // ====================================================================
        // Getters
        // ====================================================================

        /**
         * @brief Get the camera position
         */
        const Vector3& GetPosition() const { return m_Position; }

        /**
         * @brief Get the forward direction vector
         */
        const Vector3& GetForward() const { return m_Forward; }

        /**
         * @brief Get the right direction vector
         */
        const Vector3& GetRight() const { return m_Right; }

        /**
         * @brief Get the up direction vector
         */
        const Vector3& GetUp() const { return m_Up; }

        /**
         * @brief Get the pitch angle in degrees
         */
        float GetPitch() const { return m_Pitch; }

        /**
         * @brief Get the yaw angle in degrees
         */
        float GetYaw() const { return m_Yaw; }

        /**
         * @brief Get the field of view in degrees
         */
        float GetFOV() const { return m_FOV; }

        /**
         * @brief Get the near clipping plane distance
         */
        float GetNearZ() const { return m_NearZ; }

        /**
         * @brief Get the far clipping plane distance
         */
        float GetFarZ() const { return m_FarZ; }

        /**
         * @brief Get the current projection type
         */
        ProjectionType GetProjectionType() const { return m_ProjectionType; }

        /**
         * @brief Get the view matrix
         * @return View transformation matrix
         */
        const Matrix4x4& GetViewMatrix() const;

        /**
         * @brief Get the projection matrix
         * @return Projection transformation matrix
         */
        const Matrix4x4& GetProjectionMatrix() const;

        /**
         * @brief Get the combined view-projection matrix
         * @return Combined transformation matrix
         */
        Matrix4x4 GetViewProjectionMatrix() const;

        /**
         * @brief Get the view matrix as DirectX XMMATRIX
         */
        DirectX::XMMATRIX GetViewMatrixXM() const;

        /**
         * @brief Get the projection matrix as DirectX XMMATRIX
         */
        DirectX::XMMATRIX GetProjectionMatrixXM() const;

        /**
         * @brief Get position as XMFLOAT3
         */
        DirectX::XMFLOAT3 GetPositionXM() const { return m_Position.ToXMFLOAT3(); }

        // ====================================================================
        // Constraints
        // ====================================================================

        /**
         * @brief Set pitch angle limits
         * @param minPitch Minimum pitch (degrees)
         * @param maxPitch Maximum pitch (degrees)
         */
        void SetPitchLimits(float minPitch, float maxPitch);

        /**
         * @brief Enable or disable pitch limiting
         * @param enable true to enable limits
         */
        void SetPitchLimitEnabled(bool enable) { m_PitchLimitEnabled = enable; }

    private:
        /**
         * @brief Update direction vectors from pitch/yaw
         */
        void UpdateVectors();

        /**
         * @brief Mark view matrix as needing update
         */
        void InvalidateViewMatrix() { m_ViewDirty = true; }

        /**
         * @brief Mark projection matrix as needing update
         */
        void InvalidateProjectionMatrix() { m_ProjectionDirty = true; }

        /**
         * @brief Rebuild view matrix if dirty
         */
        void UpdateViewMatrix() const;

        /**
         * @brief Rebuild projection matrix if dirty
         */
        void UpdateProjectionMatrix() const;

    private:
        // Position and orientation
        Vector3 m_Position;
        Vector3 m_Forward;
        Vector3 m_Right;
        Vector3 m_Up;

        // Euler angles (degrees)
        float m_Pitch = 0.0f;   // Up/down rotation
        float m_Yaw = -90.0f;   // Left/right rotation (start facing -Z in LH)

        // Pitch limits
        float m_MinPitch = -89.0f;
        float m_MaxPitch = 89.0f;
        bool m_PitchLimitEnabled = true;

        // Projection parameters
        ProjectionType m_ProjectionType = ProjectionType::Perspective;
        float m_FOV = 45.0f;           // Field of view (degrees)
        float m_AspectRatio = 16.0f / 9.0f;
        float m_NearZ = 0.1f;
        float m_FarZ = 1000.0f;
        float m_OrthoWidth = 10.0f;    // For orthographic
        float m_OrthoHeight = 10.0f;

        // Cached matrices
        mutable Matrix4x4 m_ViewMatrix;
        mutable Matrix4x4 m_ProjectionMatrix;
        mutable bool m_ViewDirty = true;
        mutable bool m_ProjectionDirty = true;
    };

} // namespace SM
