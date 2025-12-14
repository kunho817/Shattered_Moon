#pragma once

/**
 * @file Components.h
 * @brief Convenience header including all basic components
 *
 * Include this file to get access to all standard ECS components.
 */

#include "TransformComponent.h"
#include "MeshComponent.h"
#include "MaterialComponent.h"
#include "TagComponent.h"

namespace SM
{
    // ============================================================================
    // Additional Common Components
    // ============================================================================

    /**
     * @brief Component for velocity and linear movement
     */
    struct VelocityComponent
    {
        Vector3 Linear = Vector3::Zero();
        Vector3 Angular = Vector3::Zero();

        VelocityComponent() = default;
        VelocityComponent(const Vector3& linear) : Linear(linear) {}
        VelocityComponent(const Vector3& linear, const Vector3& angular)
            : Linear(linear), Angular(angular) {}
    };

    /**
     * @brief Component for physics simulation properties
     */
    struct RigidbodyComponent
    {
        float Mass = 1.0f;
        float Drag = 0.0f;
        float AngularDrag = 0.05f;
        bool UseGravity = true;
        bool IsKinematic = false;
        bool FreezeRotation = false;

        RigidbodyComponent() = default;
        explicit RigidbodyComponent(float mass) : Mass(mass) {}
    };

    /**
     * @brief Component for simple bounding box collision
     */
    struct BoxColliderComponent
    {
        Vector3 Center = Vector3::Zero();
        Vector3 Size = Vector3::One();
        bool IsTrigger = false;

        BoxColliderComponent() = default;
        BoxColliderComponent(const Vector3& size) : Size(size) {}
        BoxColliderComponent(const Vector3& center, const Vector3& size)
            : Center(center), Size(size) {}
    };

    /**
     * @brief Component for camera properties
     */
    struct CameraComponent
    {
        float FieldOfView = 60.0f;  // In degrees
        float NearPlane = 0.1f;
        float FarPlane = 1000.0f;
        float AspectRatio = 16.0f / 9.0f;
        bool IsOrthographic = false;
        float OrthographicSize = 5.0f;  // Half-height in orthographic mode
        bool IsActive = true;
        int Priority = 0;  // Higher priority cameras render on top

        CameraComponent() = default;
    };

    /**
     * @brief Component for point/directional/spot lights
     */
    struct LightComponent
    {
        enum class LightType : std::uint8_t
        {
            Directional,
            Point,
            Spot
        };

        LightType Type = LightType::Point;
        Color LightColor = Color::White();
        float Intensity = 1.0f;
        float Range = 10.0f;           // For point/spot lights
        float SpotAngle = 45.0f;       // For spot lights (in degrees)
        float SpotOuterAngle = 60.0f;  // Outer cone angle for soft edges
        bool CastShadows = true;
        bool Enabled = true;

        LightComponent() = default;
        LightComponent(LightType type, const Color& color = Color::White(), float intensity = 1.0f)
            : Type(type), LightColor(color), Intensity(intensity) {}
    };

    /**
     * @brief Component for audio sources
     */
    struct AudioSourceComponent
    {
        std::uint32_t SoundId = 0;
        float Volume = 1.0f;
        float Pitch = 1.0f;
        float MinDistance = 1.0f;
        float MaxDistance = 100.0f;
        bool Loop = false;
        bool PlayOnStart = false;
        bool Is3D = true;
        bool IsPlaying = false;

        AudioSourceComponent() = default;
        explicit AudioSourceComponent(std::uint32_t soundId) : SoundId(soundId) {}
    };

    /**
     * @brief Component for parent-child relationships
     */
    struct HierarchyComponent
    {
        EntityID Parent = INVALID_ENTITY;
        std::vector<EntityID> Children;

        HierarchyComponent() = default;
        explicit HierarchyComponent(EntityID parent) : Parent(parent) {}

        bool HasParent() const { return Parent != INVALID_ENTITY; }
        bool HasChildren() const { return !Children.empty(); }

        void AddChild(EntityID child)
        {
            Children.push_back(child);
        }

        void RemoveChild(EntityID child)
        {
            Children.erase(
                std::remove(Children.begin(), Children.end(), child),
                Children.end()
            );
        }
    };

} // namespace SM
