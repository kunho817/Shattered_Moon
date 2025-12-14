#pragma once

/**
 * @file Mesh.h
 * @brief Mesh and primitive geometry generation
 *
 * Defines vertex structure and provides primitive mesh
 * generation utilities for common shapes.
 */

#include "renderer/DX12Core.h"
#include "renderer/GPUBuffer.h"

#include <vector>
#include <cstdint>
#include <DirectXMath.h>

namespace SM
{
    // Forward declarations
    class DX12Core;

    /**
     * @brief Standard vertex structure
     *
     * Matches the input layout in BasicVertex.hlsl
     */
    struct Vertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT2 TexCoord;
        DirectX::XMFLOAT4 Color;

        Vertex()
            : Position(0.0f, 0.0f, 0.0f)
            , Normal(0.0f, 1.0f, 0.0f)
            , TexCoord(0.0f, 0.0f)
            , Color(1.0f, 1.0f, 1.0f, 1.0f)
        {
        }

        Vertex(
            const DirectX::XMFLOAT3& pos,
            const DirectX::XMFLOAT3& normal,
            const DirectX::XMFLOAT2& uv,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f))
            : Position(pos)
            , Normal(normal)
            , TexCoord(uv)
            , Color(color)
        {
        }

        Vertex(
            float px, float py, float pz,
            float nx, float ny, float nz,
            float u, float v,
            float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
            : Position(px, py, pz)
            , Normal(nx, ny, nz)
            , TexCoord(u, v)
            , Color(r, g, b, a)
        {
        }
    };

    /**
     * @brief Mesh data in CPU memory
     *
     * Contains vertex and index data before GPU upload.
     */
    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;

        /**
         * @brief Clear all mesh data
         */
        void Clear()
        {
            Vertices.clear();
            Indices.clear();
        }

        /**
         * @brief Check if mesh has data
         */
        bool IsValid() const
        {
            return !Vertices.empty();
        }

        /**
         * @brief Get vertex count
         */
        uint32_t GetVertexCount() const
        {
            return static_cast<uint32_t>(Vertices.size());
        }

        /**
         * @brief Get index count
         */
        uint32_t GetIndexCount() const
        {
            return static_cast<uint32_t>(Indices.size());
        }

        /**
         * @brief Calculate normals (for meshes without normals)
         */
        void CalculateNormals();

        /**
         * @brief Recalculate normals using smooth averaging
         */
        void RecalculateSmoothNormals();
    };

    /**
     * @brief GPU Mesh resource
     *
     * Contains vertex and index buffers on the GPU.
     */
    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        // Prevent copying
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        // Allow moving
        Mesh(Mesh&&) noexcept = default;
        Mesh& operator=(Mesh&&) noexcept = default;

        /**
         * @brief Create mesh from mesh data
         * @param core DX12 core reference
         * @param data Mesh data
         * @return true if successful
         */
        bool Create(DX12Core* core, const MeshData& data);

        /**
         * @brief Check if mesh is valid
         */
        bool IsValid() const { return m_VertexBuffer.IsValid(); }

        /**
         * @brief Get vertex buffer view
         */
        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const
        {
            return m_VertexBuffer.GetView();
        }

        /**
         * @brief Get index buffer view
         */
        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const
        {
            return m_IndexBuffer.GetView();
        }

        /**
         * @brief Get vertex count
         */
        uint32_t GetVertexCount() const { return m_VertexBuffer.GetVertexCount(); }

        /**
         * @brief Get index count
         */
        uint32_t GetIndexCount() const { return m_IndexBuffer.GetIndexCount(); }

        /**
         * @brief Check if mesh has indices
         */
        bool HasIndices() const { return m_IndexBuffer.GetIndexCount() > 0; }

    private:
        VertexBuffer m_VertexBuffer;
        IndexBuffer m_IndexBuffer;
    };

    // ============================================================================
    // Primitive Mesh Generation
    // ============================================================================

    namespace MeshGenerator
    {
        /**
         * @brief Create a cube mesh
         * @param size Cube size (edge length)
         * @param color Vertex color
         * @return Mesh data
         */
        MeshData CreateCube(float size = 1.0f, const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1));

        /**
         * @brief Create a sphere mesh
         * @param radius Sphere radius
         * @param slices Number of horizontal slices
         * @param stacks Number of vertical stacks
         * @param color Vertex color
         * @return Mesh data
         */
        MeshData CreateSphere(
            float radius = 0.5f,
            uint32_t slices = 32,
            uint32_t stacks = 16,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1)
        );

        /**
         * @brief Create a plane mesh
         * @param width Plane width
         * @param depth Plane depth
         * @param subdivisionsX Subdivisions along X
         * @param subdivisionsZ Subdivisions along Z
         * @param color Vertex color
         * @return Mesh data
         */
        MeshData CreatePlane(
            float width = 1.0f,
            float depth = 1.0f,
            uint32_t subdivisionsX = 1,
            uint32_t subdivisionsZ = 1,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1)
        );

        /**
         * @brief Create a cylinder mesh
         * @param radius Cylinder radius
         * @param height Cylinder height
         * @param slices Number of slices
         * @param caps Include top and bottom caps
         * @param color Vertex color
         * @return Mesh data
         */
        MeshData CreateCylinder(
            float radius = 0.5f,
            float height = 1.0f,
            uint32_t slices = 32,
            bool caps = true,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1)
        );

        /**
         * @brief Create a cone mesh
         * @param radius Base radius
         * @param height Cone height
         * @param slices Number of slices
         * @param cap Include bottom cap
         * @param color Vertex color
         * @return Mesh data
         */
        MeshData CreateCone(
            float radius = 0.5f,
            float height = 1.0f,
            uint32_t slices = 32,
            bool cap = true,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1)
        );

        /**
         * @brief Create a quad (single face plane)
         * @param width Quad width
         * @param height Quad height
         * @param color Vertex color
         * @return Mesh data
         */
        MeshData CreateQuad(
            float width = 1.0f,
            float height = 1.0f,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1)
        );

        /**
         * @brief Create a torus mesh
         * @param outerRadius Distance from center to tube center
         * @param innerRadius Tube radius
         * @param slices Number of slices around the tube
         * @param stacks Number of stacks around the torus
         * @param color Vertex color
         * @return Mesh data
         */
        MeshData CreateTorus(
            float outerRadius = 0.5f,
            float innerRadius = 0.2f,
            uint32_t slices = 32,
            uint32_t stacks = 16,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(1, 1, 1, 1)
        );

        /**
         * @brief Create a grid mesh (line grid for debugging)
         * @param size Grid size
         * @param divisions Number of divisions
         * @param color Grid color
         * @return Mesh data (use line list topology)
         */
        MeshData CreateGrid(
            float size = 10.0f,
            uint32_t divisions = 10,
            const DirectX::XMFLOAT4& color = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
        );

    } // namespace MeshGenerator

} // namespace SM
