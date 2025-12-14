#include "gameplay/Camera.h"

#include <cmath>
#include <algorithm>

namespace SM
{
    // Constants
    static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;

    GameCamera::GameCamera()
        : m_Position(0.0f, 0.0f, 0.0f)
        , m_Forward(0.0f, 0.0f, 1.0f)
        , m_Right(1.0f, 0.0f, 0.0f)
        , m_Up(0.0f, 1.0f, 0.0f)
    {
        UpdateVectors();
    }

    void GameCamera::SetPosition(const Vector3& position)
    {
        m_Position = position;
        InvalidateViewMatrix();
    }

    void GameCamera::SetPosition(float x, float y, float z)
    {
        SetPosition(Vector3(x, y, z));
    }

    void GameCamera::Move(const Vector3& offset)
    {
        m_Position += offset;
        InvalidateViewMatrix();
    }

    void GameCamera::MoveRelative(float forward, float right, float up)
    {
        // Move relative to camera orientation
        Vector3 movement = m_Forward * forward + m_Right * right + m_Up * up;
        Move(movement);
    }

    void GameCamera::SetRotation(float pitchDegrees, float yawDegrees)
    {
        m_Pitch = pitchDegrees;
        m_Yaw = yawDegrees;

        // Apply pitch limits
        if (m_PitchLimitEnabled)
        {
            m_Pitch = std::clamp(m_Pitch, m_MinPitch, m_MaxPitch);
        }

        UpdateVectors();
        InvalidateViewMatrix();
    }

    void GameCamera::Rotate(float deltaPitchDegrees, float deltaYawDegrees)
    {
        m_Pitch += deltaPitchDegrees;
        m_Yaw += deltaYawDegrees;

        // Apply pitch limits
        if (m_PitchLimitEnabled)
        {
            m_Pitch = std::clamp(m_Pitch, m_MinPitch, m_MaxPitch);
        }

        // Wrap yaw to [-180, 180]
        while (m_Yaw > 180.0f) m_Yaw -= 360.0f;
        while (m_Yaw < -180.0f) m_Yaw += 360.0f;

        UpdateVectors();
        InvalidateViewMatrix();
    }

    void GameCamera::LookAt(const Vector3& target)
    {
        LookAt(target.x, target.y, target.z);
    }

    void GameCamera::LookAt(float x, float y, float z)
    {
        Vector3 target(x, y, z);
        Vector3 direction = (target - m_Position).Normalized();

        // Calculate yaw (rotation around Y axis)
        // In left-handed system, looking down +Z is yaw = 0
        m_Yaw = std::atan2(direction.x, direction.z) * RAD_TO_DEG;

        // Calculate pitch (rotation around X axis)
        float horizontalDistance = std::sqrt(direction.x * direction.x + direction.z * direction.z);
        m_Pitch = std::atan2(direction.y, horizontalDistance) * RAD_TO_DEG;

        // Apply pitch limits
        if (m_PitchLimitEnabled)
        {
            m_Pitch = std::clamp(m_Pitch, m_MinPitch, m_MaxPitch);
        }

        UpdateVectors();
        InvalidateViewMatrix();
    }

    void GameCamera::SetPerspective(float fovDegrees, float aspectRatio, float nearZ, float farZ)
    {
        m_ProjectionType = ProjectionType::Perspective;
        m_FOV = fovDegrees;
        m_AspectRatio = aspectRatio;
        m_NearZ = nearZ;
        m_FarZ = farZ;
        InvalidateProjectionMatrix();
    }

    void GameCamera::SetOrthographic(float width, float height, float nearZ, float farZ)
    {
        m_ProjectionType = ProjectionType::Orthographic;
        m_OrthoWidth = width;
        m_OrthoHeight = height;
        m_NearZ = nearZ;
        m_FarZ = farZ;
        InvalidateProjectionMatrix();
    }

    void GameCamera::SetAspectRatio(float aspectRatio)
    {
        m_AspectRatio = aspectRatio;
        if (m_ProjectionType == ProjectionType::Perspective)
        {
            InvalidateProjectionMatrix();
        }
    }

    const Matrix4x4& GameCamera::GetViewMatrix() const
    {
        if (m_ViewDirty)
        {
            UpdateViewMatrix();
        }
        return m_ViewMatrix;
    }

    const Matrix4x4& GameCamera::GetProjectionMatrix() const
    {
        if (m_ProjectionDirty)
        {
            UpdateProjectionMatrix();
        }
        return m_ProjectionMatrix;
    }

    Matrix4x4 GameCamera::GetViewProjectionMatrix() const
    {
        DirectX::XMMATRIX view = GetViewMatrixXM();
        DirectX::XMMATRIX proj = GetProjectionMatrixXM();
        return Matrix4x4(DirectX::XMMatrixMultiply(view, proj));
    }

    DirectX::XMMATRIX GameCamera::GetViewMatrixXM() const
    {
        if (m_ViewDirty)
        {
            UpdateViewMatrix();
        }
        return m_ViewMatrix.ToXMMATRIX();
    }

    DirectX::XMMATRIX GameCamera::GetProjectionMatrixXM() const
    {
        if (m_ProjectionDirty)
        {
            UpdateProjectionMatrix();
        }
        return m_ProjectionMatrix.ToXMMATRIX();
    }

    void GameCamera::SetPitchLimits(float minPitch, float maxPitch)
    {
        m_MinPitch = minPitch;
        m_MaxPitch = maxPitch;

        // Clamp current pitch if limits enabled
        if (m_PitchLimitEnabled)
        {
            float newPitch = std::clamp(m_Pitch, m_MinPitch, m_MaxPitch);
            if (newPitch != m_Pitch)
            {
                m_Pitch = newPitch;
                UpdateVectors();
                InvalidateViewMatrix();
            }
        }
    }

    void GameCamera::UpdateVectors()
    {
        // Convert to radians
        float pitchRad = m_Pitch * DEG_TO_RAD;
        float yawRad = m_Yaw * DEG_TO_RAD;

        // Calculate forward vector (left-handed coordinate system)
        // Yaw 0 = looking down +Z axis
        // Pitch 0 = horizontal
        float cosPitch = std::cos(pitchRad);
        float sinPitch = std::sin(pitchRad);
        float cosYaw = std::cos(yawRad);
        float sinYaw = std::sin(yawRad);

        m_Forward.x = sinYaw * cosPitch;
        m_Forward.y = sinPitch;
        m_Forward.z = cosYaw * cosPitch;
        m_Forward = m_Forward.Normalized();

        // Calculate right vector (cross product of world up and forward)
        Vector3 worldUp(0.0f, 1.0f, 0.0f);
        m_Right = Vector3::Cross(worldUp, m_Forward).Normalized();

        // Calculate up vector (cross product of forward and right)
        m_Up = Vector3::Cross(m_Forward, m_Right).Normalized();
    }

    void GameCamera::UpdateViewMatrix() const
    {
        // Create look-at matrix
        DirectX::XMVECTOR eye = m_Position.ToXMVECTOR();
        DirectX::XMVECTOR target = (m_Position + m_Forward).ToXMVECTOR();
        DirectX::XMVECTOR up = m_Up.ToXMVECTOR();

        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, target, up);
        DirectX::XMStoreFloat4x4(&m_ViewMatrix.m, view);

        m_ViewDirty = false;
    }

    void GameCamera::UpdateProjectionMatrix() const
    {
        DirectX::XMMATRIX proj;

        if (m_ProjectionType == ProjectionType::Perspective)
        {
            float fovRad = m_FOV * DEG_TO_RAD;
            proj = DirectX::XMMatrixPerspectiveFovLH(fovRad, m_AspectRatio, m_NearZ, m_FarZ);
        }
        else
        {
            proj = DirectX::XMMatrixOrthographicLH(m_OrthoWidth, m_OrthoHeight, m_NearZ, m_FarZ);
        }

        DirectX::XMStoreFloat4x4(&m_ProjectionMatrix.m, proj);

        m_ProjectionDirty = false;
    }

} // namespace SM
