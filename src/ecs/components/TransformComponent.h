#pragma once

/**
 * @file TransformComponent.h
 * @brief Transform component for spatial positioning
 *
 * Contains position, rotation, and scale data for entities.
 */

#include <cmath>
#include <DirectXMath.h>

namespace SM
{
    // ============================================================================
    // Vector3 Helper Structure
    // ============================================================================

    /**
     * @brief Simple 3D vector for transform data
     *
     * Basic vector implementation for position, rotation (euler), and scale.
     * For a production engine, use DirectXMath or similar optimized math library.
     */
    struct Vector3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        Vector3() = default;
        Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
        Vector3(const DirectX::XMFLOAT3& v) : x(v.x), y(v.y), z(v.z) {}

        // DirectX conversion functions
        DirectX::XMFLOAT3 ToXMFLOAT3() const { return DirectX::XMFLOAT3(x, y, z); }
        DirectX::XMVECTOR ToXMVECTOR() const
        {
            DirectX::XMFLOAT3 f = ToXMFLOAT3();
            return DirectX::XMLoadFloat3(&f);
        }

        static Vector3 FromXMVECTOR(DirectX::FXMVECTOR v)
        {
            DirectX::XMFLOAT3 f;
            DirectX::XMStoreFloat3(&f, v);
            return Vector3(f.x, f.y, f.z);
        }

        // Basic operations
        Vector3 operator+(const Vector3& other) const
        {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }

        Vector3 operator-(const Vector3& other) const
        {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }

        Vector3 operator*(float scalar) const
        {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }

        Vector3 operator/(float scalar) const
        {
            return Vector3(x / scalar, y / scalar, z / scalar);
        }

        Vector3& operator+=(const Vector3& other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vector3& operator-=(const Vector3& other)
        {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        Vector3& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        float LengthSquared() const
        {
            return x * x + y * y + z * z;
        }

        float Length() const
        {
            return std::sqrt(LengthSquared());
        }

        Vector3 Normalized() const
        {
            float len = Length();
            if (len > 0.0001f)
            {
                return *this * (1.0f / len);
            }
            return Vector3();
        }

        static float Dot(const Vector3& a, const Vector3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        static Vector3 Cross(const Vector3& a, const Vector3& b)
        {
            return Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
        {
            return a + (b - a) * t;
        }

        // Common vectors
        static Vector3 Zero() { return Vector3(0.0f, 0.0f, 0.0f); }
        static Vector3 One() { return Vector3(1.0f, 1.0f, 1.0f); }
        static Vector3 Up() { return Vector3(0.0f, 1.0f, 0.0f); }
        static Vector3 Down() { return Vector3(0.0f, -1.0f, 0.0f); }
        static Vector3 Forward() { return Vector3(0.0f, 0.0f, 1.0f); }
        static Vector3 Back() { return Vector3(0.0f, 0.0f, -1.0f); }
        static Vector3 Right() { return Vector3(1.0f, 0.0f, 0.0f); }
        static Vector3 Left() { return Vector3(-1.0f, 0.0f, 0.0f); }
    };

    // ============================================================================
    // Quaternion Helper Structure
    // ============================================================================

    /**
     * @brief Simple quaternion for rotation representation
     *
     * Basic quaternion implementation. For a production engine,
     * use DirectXMath or similar optimized math library.
     */
    struct Quaternion
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;

        Quaternion() = default;
        Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

        /**
         * @brief Create quaternion from euler angles (in radians)
         */
        static Quaternion FromEuler(float pitch, float yaw, float roll)
        {
            float cy = std::cos(yaw * 0.5f);
            float sy = std::sin(yaw * 0.5f);
            float cp = std::cos(pitch * 0.5f);
            float sp = std::sin(pitch * 0.5f);
            float cr = std::cos(roll * 0.5f);
            float sr = std::sin(roll * 0.5f);

            Quaternion q;
            q.w = cr * cp * cy + sr * sp * sy;
            q.x = sr * cp * cy - cr * sp * sy;
            q.y = cr * sp * cy + sr * cp * sy;
            q.z = cr * cp * sy - sr * sp * cy;
            return q;
        }

        /**
         * @brief Create quaternion from euler angles vector (in radians)
         */
        static Quaternion FromEuler(const Vector3& euler)
        {
            return FromEuler(euler.x, euler.y, euler.z);
        }

        /**
         * @brief Convert to euler angles (in radians)
         */
        Vector3 ToEuler() const
        {
            Vector3 euler;

            // Roll (x-axis rotation)
            float sinr_cosp = 2.0f * (w * x + y * z);
            float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
            euler.z = std::atan2(sinr_cosp, cosr_cosp);

            // Pitch (y-axis rotation)
            float sinp = 2.0f * (w * y - z * x);
            if (std::abs(sinp) >= 1.0f)
            {
                euler.x = std::copysign(3.14159265f / 2.0f, sinp);
            }
            else
            {
                euler.x = std::asin(sinp);
            }

            // Yaw (z-axis rotation)
            float siny_cosp = 2.0f * (w * z + x * y);
            float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
            euler.y = std::atan2(siny_cosp, cosy_cosp);

            return euler;
        }

        Quaternion operator*(const Quaternion& other) const
        {
            return Quaternion(
                w * other.x + x * other.w + y * other.z - z * other.y,
                w * other.y - x * other.z + y * other.w + z * other.x,
                w * other.z + x * other.y - y * other.x + z * other.w,
                w * other.w - x * other.x - y * other.y - z * other.z
            );
        }

        static Quaternion Identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }
    };

    // ============================================================================
    // TransformComponent
    // ============================================================================

    /**
     * @brief Component for entity spatial transformation
     *
     * Contains position, rotation, and scale. This is typically one of the
     * most commonly used components in a game.
     */
    struct TransformComponent
    {
        /** World position */
        Vector3 Position = Vector3::Zero();

        /** Rotation as quaternion */
        Quaternion Rotation = Quaternion::Identity();

        /** Scale factors */
        Vector3 Scale = Vector3::One();

        TransformComponent() = default;

        TransformComponent(const Vector3& position)
            : Position(position)
        {
        }

        TransformComponent(const Vector3& position, const Quaternion& rotation)
            : Position(position)
            , Rotation(rotation)
        {
        }

        TransformComponent(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
            : Position(position)
            , Rotation(rotation)
            , Scale(scale)
        {
        }

        /**
         * @brief Get euler angles in radians
         */
        Vector3 GetEulerAngles() const
        {
            return Rotation.ToEuler();
        }

        /**
         * @brief Set rotation from euler angles in radians
         */
        void SetEulerAngles(const Vector3& euler)
        {
            Rotation = Quaternion::FromEuler(euler);
        }

        /**
         * @brief Get the forward direction vector
         */
        Vector3 GetForward() const
        {
            // Simplified - for full implementation, rotate Vector3::Forward by quaternion
            float yaw = GetEulerAngles().y;
            return Vector3(std::sin(yaw), 0.0f, std::cos(yaw));
        }

        /**
         * @brief Get the right direction vector
         */
        Vector3 GetRight() const
        {
            float yaw = GetEulerAngles().y;
            return Vector3(std::cos(yaw), 0.0f, -std::sin(yaw));
        }

        /**
         * @brief Translate by a delta
         */
        void Translate(const Vector3& delta)
        {
            Position += delta;
        }
    };

} // namespace SM
