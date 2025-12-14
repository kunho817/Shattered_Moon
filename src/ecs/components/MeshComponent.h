#pragma once

/**
 * @file MeshComponent.h
 * @brief Mesh component for renderable entities
 *
 * Contains mesh reference and rendering properties.
 */

#include <cstdint>
#include <string>

namespace SM
{
    /**
     * @brief Identifier for mesh assets
     *
     * Simple handle/ID system for referencing mesh resources.
     * In a full engine, this would reference the asset manager.
     */
    using MeshID = std::uint32_t;

    /** Invalid mesh identifier */
    constexpr MeshID INVALID_MESH_ID = 0;

    // ============================================================================
    // MeshComponent
    // ============================================================================

    /**
     * @brief Component for mesh rendering
     *
     * Associates an entity with a mesh resource and rendering properties.
     */
    struct MeshComponent
    {
        /** ID of the mesh resource */
        MeshID MeshId = INVALID_MESH_ID;

        /** Whether this mesh should cast shadows */
        bool CastShadows = true;

        /** Whether this mesh receives shadows */
        bool ReceiveShadows = true;

        /** Whether the mesh is visible */
        bool Visible = true;

        /** Render layer/pass (for sorting) */
        std::uint8_t RenderLayer = 0;

        /** Optional mesh name for debugging */
        std::string DebugName;

        MeshComponent() = default;

        explicit MeshComponent(MeshID meshId)
            : MeshId(meshId)
        {
        }

        MeshComponent(MeshID meshId, bool castShadows, bool receiveShadows = true)
            : MeshId(meshId)
            , CastShadows(castShadows)
            , ReceiveShadows(receiveShadows)
        {
        }

        /**
         * @brief Check if the mesh is valid
         */
        bool IsValid() const
        {
            return MeshId != INVALID_MESH_ID;
        }
    };

    // ============================================================================
    // Primitive Mesh IDs
    // ============================================================================

    /**
     * @brief Reserved mesh IDs for built-in primitives
     *
     * These IDs are reserved for commonly used primitive meshes
     * that should be available without loading.
     */
    namespace PrimitiveMesh
    {
        constexpr MeshID Cube = 1;
        constexpr MeshID Sphere = 2;
        constexpr MeshID Plane = 3;
        constexpr MeshID Cylinder = 4;
        constexpr MeshID Cone = 5;
        constexpr MeshID Quad = 6;
        constexpr MeshID Custom = 100;  // First ID for custom meshes
    }

} // namespace SM
