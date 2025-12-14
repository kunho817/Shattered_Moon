#include "renderer/Mesh.h"

#include <cmath>
#include <unordered_map>

namespace SM
{
    // ============================================================================
    // MeshData Implementation
    // ============================================================================

    void MeshData::CalculateNormals()
    {
        if (Indices.empty())
        {
            return;
        }

        // Reset normals
        for (auto& v : Vertices)
        {
            v.Normal = DirectX::XMFLOAT3(0, 0, 0);
        }

        // Calculate face normals and accumulate
        for (size_t i = 0; i < Indices.size(); i += 3)
        {
            uint32_t i0 = Indices[i];
            uint32_t i1 = Indices[i + 1];
            uint32_t i2 = Indices[i + 2];

            DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&Vertices[i0].Position);
            DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&Vertices[i1].Position);
            DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&Vertices[i2].Position);

            DirectX::XMVECTOR e0 = DirectX::XMVectorSubtract(v1, v0);
            DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract(v2, v0);
            DirectX::XMVECTOR normal = DirectX::XMVector3Cross(e0, e1);

            // Accumulate to each vertex
            DirectX::XMVECTOR n0 = DirectX::XMLoadFloat3(&Vertices[i0].Normal);
            DirectX::XMVECTOR n1 = DirectX::XMLoadFloat3(&Vertices[i1].Normal);
            DirectX::XMVECTOR n2 = DirectX::XMLoadFloat3(&Vertices[i2].Normal);

            DirectX::XMStoreFloat3(&Vertices[i0].Normal, DirectX::XMVectorAdd(n0, normal));
            DirectX::XMStoreFloat3(&Vertices[i1].Normal, DirectX::XMVectorAdd(n1, normal));
            DirectX::XMStoreFloat3(&Vertices[i2].Normal, DirectX::XMVectorAdd(n2, normal));
        }

        // Normalize
        for (auto& v : Vertices)
        {
            DirectX::XMVECTOR n = DirectX::XMLoadFloat3(&v.Normal);
            n = DirectX::XMVector3Normalize(n);
            DirectX::XMStoreFloat3(&v.Normal, n);
        }
    }

    void MeshData::RecalculateSmoothNormals()
    {
        CalculateNormals();
    }

    // ============================================================================
    // Mesh Implementation
    // ============================================================================

    bool Mesh::Create(DX12Core* core, const MeshData& data)
    {
        if (!data.IsValid())
        {
            return false;
        }

        // Create vertex buffer
        if (!m_VertexBuffer.Initialize(
            core,
            data.GetVertexCount(),
            sizeof(Vertex),
            GPUBufferUsage::Upload,
            data.Vertices.data()))
        {
            return false;
        }

        // Create index buffer (if indices exist)
        if (!data.Indices.empty())
        {
            if (!m_IndexBuffer.Initialize(
                core,
                data.GetIndexCount(),
                true,  // 32-bit indices
                GPUBufferUsage::Upload,
                data.Indices.data()))
            {
                return false;
            }
        }

        return true;
    }

    // ============================================================================
    // MeshGenerator Implementation
    // ============================================================================

    namespace MeshGenerator
    {
        constexpr float PI = 3.14159265358979323846f;
        constexpr float TWO_PI = 2.0f * PI;

        MeshData CreateCube(float size, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            float hs = size * 0.5f; // half size

            // 24 vertices (4 per face for proper normals)
            data.Vertices = {
                // Front face (+Z)
                Vertex(-hs, -hs,  hs,  0,  0,  1, 0, 1, color.x, color.y, color.z, color.w),
                Vertex( hs, -hs,  hs,  0,  0,  1, 1, 1, color.x, color.y, color.z, color.w),
                Vertex( hs,  hs,  hs,  0,  0,  1, 1, 0, color.x, color.y, color.z, color.w),
                Vertex(-hs,  hs,  hs,  0,  0,  1, 0, 0, color.x, color.y, color.z, color.w),

                // Back face (-Z)
                Vertex( hs, -hs, -hs,  0,  0, -1, 0, 1, color.x, color.y, color.z, color.w),
                Vertex(-hs, -hs, -hs,  0,  0, -1, 1, 1, color.x, color.y, color.z, color.w),
                Vertex(-hs,  hs, -hs,  0,  0, -1, 1, 0, color.x, color.y, color.z, color.w),
                Vertex( hs,  hs, -hs,  0,  0, -1, 0, 0, color.x, color.y, color.z, color.w),

                // Top face (+Y)
                Vertex(-hs,  hs,  hs,  0,  1,  0, 0, 1, color.x, color.y, color.z, color.w),
                Vertex( hs,  hs,  hs,  0,  1,  0, 1, 1, color.x, color.y, color.z, color.w),
                Vertex( hs,  hs, -hs,  0,  1,  0, 1, 0, color.x, color.y, color.z, color.w),
                Vertex(-hs,  hs, -hs,  0,  1,  0, 0, 0, color.x, color.y, color.z, color.w),

                // Bottom face (-Y)
                Vertex(-hs, -hs, -hs,  0, -1,  0, 0, 1, color.x, color.y, color.z, color.w),
                Vertex( hs, -hs, -hs,  0, -1,  0, 1, 1, color.x, color.y, color.z, color.w),
                Vertex( hs, -hs,  hs,  0, -1,  0, 1, 0, color.x, color.y, color.z, color.w),
                Vertex(-hs, -hs,  hs,  0, -1,  0, 0, 0, color.x, color.y, color.z, color.w),

                // Right face (+X)
                Vertex( hs, -hs,  hs,  1,  0,  0, 0, 1, color.x, color.y, color.z, color.w),
                Vertex( hs, -hs, -hs,  1,  0,  0, 1, 1, color.x, color.y, color.z, color.w),
                Vertex( hs,  hs, -hs,  1,  0,  0, 1, 0, color.x, color.y, color.z, color.w),
                Vertex( hs,  hs,  hs,  1,  0,  0, 0, 0, color.x, color.y, color.z, color.w),

                // Left face (-X)
                Vertex(-hs, -hs, -hs, -1,  0,  0, 0, 1, color.x, color.y, color.z, color.w),
                Vertex(-hs, -hs,  hs, -1,  0,  0, 1, 1, color.x, color.y, color.z, color.w),
                Vertex(-hs,  hs,  hs, -1,  0,  0, 1, 0, color.x, color.y, color.z, color.w),
                Vertex(-hs,  hs, -hs, -1,  0,  0, 0, 0, color.x, color.y, color.z, color.w),
            };

            // 36 indices (6 faces * 2 triangles * 3 vertices)
            data.Indices = {
                0,  1,  2,   0,  2,  3,   // Front
                4,  5,  6,   4,  6,  7,   // Back
                8,  9,  10,  8,  10, 11,  // Top
                12, 13, 14,  12, 14, 15,  // Bottom
                16, 17, 18,  16, 18, 19,  // Right
                20, 21, 22,  20, 22, 23   // Left
            };

            return data;
        }

        MeshData CreateSphere(float radius, uint32_t slices, uint32_t stacks, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            // Generate vertices
            for (uint32_t stack = 0; stack <= stacks; ++stack)
            {
                float phi = PI * static_cast<float>(stack) / static_cast<float>(stacks);
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);

                for (uint32_t slice = 0; slice <= slices; ++slice)
                {
                    float theta = TWO_PI * static_cast<float>(slice) / static_cast<float>(slices);
                    float sinTheta = std::sin(theta);
                    float cosTheta = std::cos(theta);

                    float x = cosTheta * sinPhi;
                    float y = cosPhi;
                    float z = sinTheta * sinPhi;

                    float u = static_cast<float>(slice) / static_cast<float>(slices);
                    float v = static_cast<float>(stack) / static_cast<float>(stacks);

                    data.Vertices.emplace_back(
                        x * radius, y * radius, z * radius,
                        x, y, z,
                        u, v,
                        color.x, color.y, color.z, color.w
                    );
                }
            }

            // Generate indices
            for (uint32_t stack = 0; stack < stacks; ++stack)
            {
                for (uint32_t slice = 0; slice < slices; ++slice)
                {
                    uint32_t current = stack * (slices + 1) + slice;
                    uint32_t next = current + slices + 1;

                    // Two triangles per quad
                    data.Indices.push_back(current);
                    data.Indices.push_back(next);
                    data.Indices.push_back(current + 1);

                    data.Indices.push_back(current + 1);
                    data.Indices.push_back(next);
                    data.Indices.push_back(next + 1);
                }
            }

            return data;
        }

        MeshData CreatePlane(float width, float depth, uint32_t subdivisionsX, uint32_t subdivisionsZ, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            float halfWidth = width * 0.5f;
            float halfDepth = depth * 0.5f;

            float dx = width / static_cast<float>(subdivisionsX);
            float dz = depth / static_cast<float>(subdivisionsZ);

            float du = 1.0f / static_cast<float>(subdivisionsX);
            float dv = 1.0f / static_cast<float>(subdivisionsZ);

            // Generate vertices
            for (uint32_t z = 0; z <= subdivisionsZ; ++z)
            {
                float pz = -halfDepth + static_cast<float>(z) * dz;
                float v = static_cast<float>(z) * dv;

                for (uint32_t x = 0; x <= subdivisionsX; ++x)
                {
                    float px = -halfWidth + static_cast<float>(x) * dx;
                    float u = static_cast<float>(x) * du;

                    data.Vertices.emplace_back(
                        px, 0.0f, pz,
                        0.0f, 1.0f, 0.0f,
                        u, v,
                        color.x, color.y, color.z, color.w
                    );
                }
            }

            // Generate indices
            for (uint32_t z = 0; z < subdivisionsZ; ++z)
            {
                for (uint32_t x = 0; x < subdivisionsX; ++x)
                {
                    uint32_t current = z * (subdivisionsX + 1) + x;
                    uint32_t next = current + subdivisionsX + 1;

                    data.Indices.push_back(current);
                    data.Indices.push_back(next);
                    data.Indices.push_back(current + 1);

                    data.Indices.push_back(current + 1);
                    data.Indices.push_back(next);
                    data.Indices.push_back(next + 1);
                }
            }

            return data;
        }

        MeshData CreateCylinder(float radius, float height, uint32_t slices, bool caps, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            float halfHeight = height * 0.5f;

            // Side vertices
            for (uint32_t i = 0; i <= slices; ++i)
            {
                float theta = TWO_PI * static_cast<float>(i) / static_cast<float>(slices);
                float x = std::cos(theta);
                float z = std::sin(theta);
                float u = static_cast<float>(i) / static_cast<float>(slices);

                // Bottom vertex
                data.Vertices.emplace_back(
                    x * radius, -halfHeight, z * radius,
                    x, 0.0f, z,
                    u, 1.0f,
                    color.x, color.y, color.z, color.w
                );

                // Top vertex
                data.Vertices.emplace_back(
                    x * radius, halfHeight, z * radius,
                    x, 0.0f, z,
                    u, 0.0f,
                    color.x, color.y, color.z, color.w
                );
            }

            // Side indices
            for (uint32_t i = 0; i < slices; ++i)
            {
                uint32_t current = i * 2;
                uint32_t next = (i + 1) * 2;

                data.Indices.push_back(current);
                data.Indices.push_back(next);
                data.Indices.push_back(current + 1);

                data.Indices.push_back(current + 1);
                data.Indices.push_back(next);
                data.Indices.push_back(next + 1);
            }

            if (caps)
            {
                // Top cap
                uint32_t topCenterIndex = static_cast<uint32_t>(data.Vertices.size());
                data.Vertices.emplace_back(
                    0.0f, halfHeight, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.5f, 0.5f,
                    color.x, color.y, color.z, color.w
                );

                for (uint32_t i = 0; i <= slices; ++i)
                {
                    float theta = TWO_PI * static_cast<float>(i) / static_cast<float>(slices);
                    float x = std::cos(theta);
                    float z = std::sin(theta);

                    data.Vertices.emplace_back(
                        x * radius, halfHeight, z * radius,
                        0.0f, 1.0f, 0.0f,
                        x * 0.5f + 0.5f, z * 0.5f + 0.5f,
                        color.x, color.y, color.z, color.w
                    );
                }

                for (uint32_t i = 0; i < slices; ++i)
                {
                    data.Indices.push_back(topCenterIndex);
                    data.Indices.push_back(topCenterIndex + i + 1);
                    data.Indices.push_back(topCenterIndex + i + 2);
                }

                // Bottom cap
                uint32_t bottomCenterIndex = static_cast<uint32_t>(data.Vertices.size());
                data.Vertices.emplace_back(
                    0.0f, -halfHeight, 0.0f,
                    0.0f, -1.0f, 0.0f,
                    0.5f, 0.5f,
                    color.x, color.y, color.z, color.w
                );

                for (uint32_t i = 0; i <= slices; ++i)
                {
                    float theta = TWO_PI * static_cast<float>(i) / static_cast<float>(slices);
                    float x = std::cos(theta);
                    float z = std::sin(theta);

                    data.Vertices.emplace_back(
                        x * radius, -halfHeight, z * radius,
                        0.0f, -1.0f, 0.0f,
                        x * 0.5f + 0.5f, z * 0.5f + 0.5f,
                        color.x, color.y, color.z, color.w
                    );
                }

                for (uint32_t i = 0; i < slices; ++i)
                {
                    data.Indices.push_back(bottomCenterIndex);
                    data.Indices.push_back(bottomCenterIndex + i + 2);
                    data.Indices.push_back(bottomCenterIndex + i + 1);
                }
            }

            return data;
        }

        MeshData CreateCone(float radius, float height, uint32_t slices, bool cap, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            float halfHeight = height * 0.5f;

            // Calculate slope for normals
            float slope = radius / height;
            float normalY = 1.0f / std::sqrt(1.0f + slope * slope);
            float normalXZ = slope * normalY;

            // Tip vertex
            uint32_t tipIndex = 0;
            data.Vertices.emplace_back(
                0.0f, halfHeight, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.5f, 0.0f,
                color.x, color.y, color.z, color.w
            );

            // Base vertices
            for (uint32_t i = 0; i <= slices; ++i)
            {
                float theta = TWO_PI * static_cast<float>(i) / static_cast<float>(slices);
                float x = std::cos(theta);
                float z = std::sin(theta);
                float u = static_cast<float>(i) / static_cast<float>(slices);

                data.Vertices.emplace_back(
                    x * radius, -halfHeight, z * radius,
                    x * normalXZ, normalY, z * normalXZ,
                    u, 1.0f,
                    color.x, color.y, color.z, color.w
                );
            }

            // Side indices
            for (uint32_t i = 0; i < slices; ++i)
            {
                data.Indices.push_back(tipIndex);
                data.Indices.push_back(i + 2);
                data.Indices.push_back(i + 1);
            }

            if (cap)
            {
                // Bottom cap
                uint32_t centerIndex = static_cast<uint32_t>(data.Vertices.size());
                data.Vertices.emplace_back(
                    0.0f, -halfHeight, 0.0f,
                    0.0f, -1.0f, 0.0f,
                    0.5f, 0.5f,
                    color.x, color.y, color.z, color.w
                );

                for (uint32_t i = 0; i <= slices; ++i)
                {
                    float theta = TWO_PI * static_cast<float>(i) / static_cast<float>(slices);
                    float x = std::cos(theta);
                    float z = std::sin(theta);

                    data.Vertices.emplace_back(
                        x * radius, -halfHeight, z * radius,
                        0.0f, -1.0f, 0.0f,
                        x * 0.5f + 0.5f, z * 0.5f + 0.5f,
                        color.x, color.y, color.z, color.w
                    );
                }

                for (uint32_t i = 0; i < slices; ++i)
                {
                    data.Indices.push_back(centerIndex);
                    data.Indices.push_back(centerIndex + i + 2);
                    data.Indices.push_back(centerIndex + i + 1);
                }
            }

            return data;
        }

        MeshData CreateQuad(float width, float height, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            float hw = width * 0.5f;
            float hh = height * 0.5f;

            data.Vertices = {
                Vertex(-hw, -hh, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, color.x, color.y, color.z, color.w),
                Vertex( hw, -hh, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, color.x, color.y, color.z, color.w),
                Vertex( hw,  hh, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, color.x, color.y, color.z, color.w),
                Vertex(-hw,  hh, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, color.x, color.y, color.z, color.w),
            };

            data.Indices = { 0, 1, 2, 0, 2, 3 };

            return data;
        }

        MeshData CreateTorus(float outerRadius, float innerRadius, uint32_t slices, uint32_t stacks, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            for (uint32_t stack = 0; stack <= stacks; ++stack)
            {
                float u = static_cast<float>(stack) / static_cast<float>(stacks);
                float phi = u * TWO_PI;
                float cosPhi = std::cos(phi);
                float sinPhi = std::sin(phi);

                for (uint32_t slice = 0; slice <= slices; ++slice)
                {
                    float v = static_cast<float>(slice) / static_cast<float>(slices);
                    float theta = v * TWO_PI;
                    float cosTheta = std::cos(theta);
                    float sinTheta = std::sin(theta);

                    float x = (outerRadius + innerRadius * cosTheta) * cosPhi;
                    float y = innerRadius * sinTheta;
                    float z = (outerRadius + innerRadius * cosTheta) * sinPhi;

                    float nx = cosTheta * cosPhi;
                    float ny = sinTheta;
                    float nz = cosTheta * sinPhi;

                    data.Vertices.emplace_back(
                        x, y, z,
                        nx, ny, nz,
                        u, v,
                        color.x, color.y, color.z, color.w
                    );
                }
            }

            for (uint32_t stack = 0; stack < stacks; ++stack)
            {
                for (uint32_t slice = 0; slice < slices; ++slice)
                {
                    uint32_t current = stack * (slices + 1) + slice;
                    uint32_t next = current + slices + 1;

                    data.Indices.push_back(current);
                    data.Indices.push_back(next);
                    data.Indices.push_back(current + 1);

                    data.Indices.push_back(current + 1);
                    data.Indices.push_back(next);
                    data.Indices.push_back(next + 1);
                }
            }

            return data;
        }

        MeshData CreateGrid(float size, uint32_t divisions, const DirectX::XMFLOAT4& color)
        {
            MeshData data;

            float halfSize = size * 0.5f;
            float step = size / static_cast<float>(divisions);

            // Lines parallel to X axis
            for (uint32_t i = 0; i <= divisions; ++i)
            {
                float z = -halfSize + static_cast<float>(i) * step;

                data.Vertices.emplace_back(
                    -halfSize, 0.0f, z,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f,
                    color.x, color.y, color.z, color.w
                );

                data.Vertices.emplace_back(
                    halfSize, 0.0f, z,
                    0.0f, 1.0f, 0.0f,
                    1.0f, 0.0f,
                    color.x, color.y, color.z, color.w
                );
            }

            // Lines parallel to Z axis
            for (uint32_t i = 0; i <= divisions; ++i)
            {
                float x = -halfSize + static_cast<float>(i) * step;

                data.Vertices.emplace_back(
                    x, 0.0f, -halfSize,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f,
                    color.x, color.y, color.z, color.w
                );

                data.Vertices.emplace_back(
                    x, 0.0f, halfSize,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 1.0f,
                    color.x, color.y, color.z, color.w
                );
            }

            // Line indices (pairs)
            for (uint32_t i = 0; i < data.Vertices.size(); i += 2)
            {
                data.Indices.push_back(i);
                data.Indices.push_back(i + 1);
            }

            return data;
        }

    } // namespace MeshGenerator

} // namespace SM
