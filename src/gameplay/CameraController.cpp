#include "gameplay/CameraController.h"

#include <cmath>
#include <algorithm>

namespace SM
{
    // Constants
    static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;

    // ========================================================================
    // FPSCameraController
    // ========================================================================

    FPSCameraController::FPSCameraController(GameCamera& camera)
        : m_Camera(camera)
        , m_Velocity(0.0f, 0.0f, 0.0f)
    {
    }

    void FPSCameraController::Update(float deltaTime)
    {
        Input& input = Input::Get();

        // Toggle mouse capture on right mouse button
        if (input.IsMouseButtonPressed(static_cast<int>(MouseButton::Right)))
        {
            input.SetMouseCapture(!input.IsMouseCaptured());
        }

        // Handle mouse look (only when captured)
        if (input.IsMouseCaptured())
        {
            HandleMouseLook(deltaTime);
        }

        // Handle keyboard movement
        HandleMovement(deltaTime);
    }

    void FPSCameraController::HandleMouseLook(float deltaTime)
    {
        (void)deltaTime; // Not used for mouse look (frame-independent)

        Input& input = Input::Get();

        float deltaX = static_cast<float>(input.GetMouseDeltaX());
        float deltaY = static_cast<float>(input.GetMouseDeltaY());

        if (deltaX != 0.0f || deltaY != 0.0f)
        {
            // Apply sensitivity and rotate camera
            float yawDelta = deltaX * m_MouseSensitivity;
            float pitchDelta = -deltaY * m_MouseSensitivity; // Invert Y for natural feel

            m_Camera.Rotate(pitchDelta, yawDelta);
        }
    }

    void FPSCameraController::HandleMovement(float deltaTime)
    {
        InputMapper& mapper = InputMapper::Get();
        Input& input = Input::Get();

        // Get movement input
        float moveForward = mapper.GetAxisValue("MoveForward");
        float moveRight = mapper.GetAxisValue("MoveRight");
        float moveUp = m_VerticalMovementEnabled ? mapper.GetAxisValue("MoveUp") : 0.0f;

        // Calculate target velocity
        Vector3 targetVelocity(0.0f, 0.0f, 0.0f);

        if (moveForward != 0.0f || moveRight != 0.0f || moveUp != 0.0f)
        {
            // Get camera vectors for movement
            Vector3 forward = m_Camera.GetForward();
            Vector3 right = m_Camera.GetRight();
            Vector3 up = Vector3::Up();

            // Calculate movement direction
            Vector3 moveDir = forward * moveForward + right * moveRight + up * moveUp;

            // Normalize if moving diagonally
            float length = moveDir.Length();
            if (length > 1.0f)
            {
                moveDir = moveDir / length;
            }

            // Calculate speed (with sprint modifier)
            float speed = m_MoveSpeed;
            if (mapper.IsActionDown("Sprint") || input.IsKeyDown(KeyCode::Shift))
            {
                speed *= m_SprintMultiplier;
            }

            targetVelocity = moveDir * speed;
        }

        // Apply smoothing
        if (m_SmoothingEnabled)
        {
            float t = std::min(1.0f, m_SmoothingFactor * 60.0f * deltaTime);
            m_Velocity = Vector3::Lerp(m_Velocity, targetVelocity, t);
        }
        else
        {
            m_Velocity = targetVelocity;
        }

        // Apply movement
        if (m_Velocity.LengthSquared() > 0.0001f)
        {
            m_Camera.Move(m_Velocity * deltaTime);
        }
    }

    // ========================================================================
    // OrbitCameraController
    // ========================================================================

    OrbitCameraController::OrbitCameraController(GameCamera& camera)
        : m_Camera(camera)
        , m_Target(0.0f, 0.0f, 0.0f)
    {
        UpdateCameraPosition();
    }

    void OrbitCameraController::Update(float deltaTime)
    {
        // Handle auto-rotation
        if (m_AutoRotate && !m_IsDragging)
        {
            m_TargetAzimuth += m_AutoRotateSpeed * deltaTime;
            while (m_TargetAzimuth > 360.0f) m_TargetAzimuth -= 360.0f;
        }

        // Handle input
        HandleRotation(deltaTime);
        HandleZoom(deltaTime);
        HandlePan(deltaTime);

        // Apply smoothing
        if (m_SmoothingEnabled)
        {
            float t = std::min(1.0f, m_SmoothingFactor * 60.0f * deltaTime);
            m_Azimuth += (m_TargetAzimuth - m_Azimuth) * t;
            m_Elevation += (m_TargetElevation - m_Elevation) * t;
            m_Distance += (m_TargetDistance - m_Distance) * t;
        }
        else
        {
            m_Azimuth = m_TargetAzimuth;
            m_Elevation = m_TargetElevation;
            m_Distance = m_TargetDistance;
        }

        // Update camera position
        UpdateCameraPosition();
    }

    void OrbitCameraController::SetTarget(const Vector3& target)
    {
        m_Target = target;
        UpdateCameraPosition();
    }

    void OrbitCameraController::SetTarget(float x, float y, float z)
    {
        SetTarget(Vector3(x, y, z));
    }

    void OrbitCameraController::SetDistance(float distance)
    {
        m_Distance = std::clamp(distance, m_MinDistance, m_MaxDistance);
        m_TargetDistance = m_Distance;
        UpdateCameraPosition();
    }

    void OrbitCameraController::SetDistanceLimits(float minDistance, float maxDistance)
    {
        m_MinDistance = minDistance;
        m_MaxDistance = maxDistance;
        m_Distance = std::clamp(m_Distance, m_MinDistance, m_MaxDistance);
        m_TargetDistance = std::clamp(m_TargetDistance, m_MinDistance, m_MaxDistance);
    }

    void OrbitCameraController::SetAzimuth(float degrees)
    {
        m_Azimuth = degrees;
        m_TargetAzimuth = degrees;
        UpdateCameraPosition();
    }

    void OrbitCameraController::SetElevation(float degrees)
    {
        m_Elevation = std::clamp(degrees, m_MinElevation, m_MaxElevation);
        m_TargetElevation = m_Elevation;
        UpdateCameraPosition();
    }

    void OrbitCameraController::SetElevationLimits(float minElevation, float maxElevation)
    {
        m_MinElevation = minElevation;
        m_MaxElevation = maxElevation;
        m_Elevation = std::clamp(m_Elevation, m_MinElevation, m_MaxElevation);
        m_TargetElevation = std::clamp(m_TargetElevation, m_MinElevation, m_MaxElevation);
    }

    void OrbitCameraController::SetAutoRotate(bool enabled, float speed)
    {
        m_AutoRotate = enabled;
        m_AutoRotateSpeed = speed;
    }

    void OrbitCameraController::UpdateCameraPosition()
    {
        // Convert spherical coordinates to Cartesian
        float azimuthRad = m_Azimuth * DEG_TO_RAD;
        float elevationRad = m_Elevation * DEG_TO_RAD;

        float cosElevation = std::cos(elevationRad);
        float sinElevation = std::sin(elevationRad);
        float cosAzimuth = std::cos(azimuthRad);
        float sinAzimuth = std::sin(azimuthRad);

        // Calculate camera position relative to target
        Vector3 offset;
        offset.x = m_Distance * cosElevation * sinAzimuth;
        offset.y = m_Distance * sinElevation;
        offset.z = m_Distance * cosElevation * cosAzimuth;

        Vector3 newPosition = m_Target + offset;

        // Update camera
        m_Camera.SetPosition(newPosition);
        m_Camera.LookAt(m_Target);
    }

    void OrbitCameraController::HandleRotation(float deltaTime)
    {
        (void)deltaTime;

        Input& input = Input::Get();

        // Check for drag with right mouse button
        bool rightButtonDown = input.IsMouseButtonDown(static_cast<int>(MouseButton::Right));

        if (rightButtonDown)
        {
            m_IsDragging = true;

            float deltaX = static_cast<float>(input.GetMouseDeltaX());
            float deltaY = static_cast<float>(input.GetMouseDeltaY());

            if (deltaX != 0.0f || deltaY != 0.0f)
            {
                // Rotate around target
                m_TargetAzimuth -= deltaX * m_RotationSpeed;
                m_TargetElevation += deltaY * m_RotationSpeed;

                // Clamp elevation
                m_TargetElevation = std::clamp(m_TargetElevation, m_MinElevation, m_MaxElevation);

                // Wrap azimuth
                while (m_TargetAzimuth > 360.0f) m_TargetAzimuth -= 360.0f;
                while (m_TargetAzimuth < 0.0f) m_TargetAzimuth += 360.0f;
            }
        }
        else
        {
            m_IsDragging = false;
        }
    }

    void OrbitCameraController::HandleZoom(float deltaTime)
    {
        (void)deltaTime;

        Input& input = Input::Get();

        float wheelDelta = input.GetMouseWheelDelta();
        if (wheelDelta != 0.0f)
        {
            // Zoom in/out
            m_TargetDistance -= wheelDelta * m_ZoomSpeed;
            m_TargetDistance = std::clamp(m_TargetDistance, m_MinDistance, m_MaxDistance);
        }
    }

    void OrbitCameraController::HandlePan(float deltaTime)
    {
        (void)deltaTime;

        Input& input = Input::Get();

        // Pan with middle mouse button
        bool middleButtonDown = input.IsMouseButtonDown(static_cast<int>(MouseButton::Middle));

        if (middleButtonDown)
        {
            float deltaX = static_cast<float>(input.GetMouseDeltaX());
            float deltaY = static_cast<float>(input.GetMouseDeltaY());

            if (deltaX != 0.0f || deltaY != 0.0f)
            {
                // Get camera right and up vectors for panning
                Vector3 right = m_Camera.GetRight();
                Vector3 up = m_Camera.GetUp();

                // Calculate pan offset scaled by distance (pan faster when zoomed out)
                float panScale = m_PanSpeed * m_Distance;
                Vector3 panOffset = right * (-deltaX * panScale) + up * (deltaY * panScale);

                // Move target
                m_Target += panOffset;
            }
        }
    }

} // namespace SM
