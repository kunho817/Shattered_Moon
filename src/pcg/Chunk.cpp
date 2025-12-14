#include "pcg/Chunk.h"
#include "renderer/DX12Core.h"

#include <algorithm>
#include <cmath>

namespace PCG
{
    Chunk::Chunk(ChunkCoord coord)
        : m_Coord(coord)
        , m_LOD(0)
        , m_NeedsRebuild(false)
        , m_MinHeight(0.0f)
        , m_MaxHeight(0.0f)
    {
    }

    // ============================================================================
    // Generation
    // ============================================================================

    void Chunk::Generate(const HeightmapGenerator& generator, const HeightmapSettings& settings)
    {
        // Allocate height data
        const int vertexCount = SIZE + 1;
        m_Heights.resize(vertexCount * vertexCount);

        // Calculate world offset for this chunk
        float worldOffsetX = static_cast<float>(m_Coord.X) * GetWorldSize();
        float worldOffsetZ = static_cast<float>(m_Coord.Z) * GetWorldSize();

        // Create a local generator (HeightmapGenerator needs to be non-const for FBM)
        HeightmapGenerator localGen;

        // Create settings for single-point sampling
        HeightmapSettings localSettings = settings;

        // Initialize noise for consistent generation
        PerlinNoise baseNoise(settings.Seed);
        FBM fbm(&baseNoise);

        // Generate heights using FBM directly
        for (int z = 0; z <= SIZE; ++z)
        {
            for (int x = 0; x <= SIZE; ++x)
            {
                float worldX = worldOffsetX + static_cast<float>(x) * SCALE;
                float worldZ = worldOffsetZ + static_cast<float>(z) * SCALE;

                // Sample noise at world coordinates
                float height;
                if (settings.ApplyDomainWarp)
                {
                    height = fbm.WarpedSample(worldX, worldZ, settings.Noise, settings.WarpStrength);
                }
                else
                {
                    height = fbm.Sample(worldX, worldZ, settings.Noise);
                }

                // Remap from [-1, 1] to [0, 1]
                height = (height + 1.0f) * 0.5f;

                // Apply height range
                height = settings.MinHeight + height * (settings.MaxHeight - settings.MinHeight);

                m_Heights[z * vertexCount + x] = height;
            }
        }

        UpdateHeightBounds();
        m_NeedsRebuild = true;
    }

    void Chunk::GenerateCustom(std::function<float(float, float)> heightFunc)
    {
        const int vertexCount = SIZE + 1;
        m_Heights.resize(vertexCount * vertexCount);

        float worldOffsetX = static_cast<float>(m_Coord.X) * GetWorldSize();
        float worldOffsetZ = static_cast<float>(m_Coord.Z) * GetWorldSize();

        for (int z = 0; z <= SIZE; ++z)
        {
            for (int x = 0; x <= SIZE; ++x)
            {
                float worldX = worldOffsetX + static_cast<float>(x) * SCALE;
                float worldZ = worldOffsetZ + static_cast<float>(z) * SCALE;

                m_Heights[z * vertexCount + x] = heightFunc(worldX, worldZ);
            }
        }

        UpdateHeightBounds();
        m_NeedsRebuild = true;
    }

    bool Chunk::BuildMesh(SM::DX12Core* core)
    {
        if (!core || m_Heights.empty())
        {
            return false;
        }

        // Calculate LOD step (1, 2, 4, 8, etc.)
        int lodStep = 1 << m_LOD;

        // Generate mesh data
        std::vector<SM::Vertex> vertices;
        std::vector<uint32_t> indices;

        GenerateVertices(vertices, lodStep);
        GenerateIndices(indices, lodStep);

        // Create mesh data structure
        SM::MeshData meshData;
        meshData.Vertices = std::move(vertices);
        meshData.Indices = std::move(indices);

        // Build GPU mesh
        bool result = m_Mesh.Create(core, meshData);
        if (result)
        {
            m_NeedsRebuild = false;
        }

        return result;
    }

    bool Chunk::RebuildMesh(SM::DX12Core* core)
    {
        return BuildMesh(core);
    }

    // ============================================================================
    // Accessors
    // ============================================================================

    float Chunk::GetHeight(int localX, int localZ) const
    {
        if (localX < 0 || localX > SIZE || localZ < 0 || localZ > SIZE)
        {
            return 0.0f;
        }

        const int vertexCount = SIZE + 1;
        return m_Heights[localZ * vertexCount + localX];
    }

    float Chunk::GetHeightInterpolated(float localX, float localZ) const
    {
        if (m_Heights.empty())
        {
            return 0.0f;
        }

        // Clamp to valid range
        localX = std::clamp(localX, 0.0f, static_cast<float>(SIZE));
        localZ = std::clamp(localZ, 0.0f, static_cast<float>(SIZE));

        int x0 = static_cast<int>(localX);
        int z0 = static_cast<int>(localZ);
        int x1 = std::min(x0 + 1, SIZE);
        int z1 = std::min(z0 + 1, SIZE);

        float fx = localX - static_cast<float>(x0);
        float fz = localZ - static_cast<float>(z0);

        float h00 = GetHeight(x0, z0);
        float h10 = GetHeight(x1, z0);
        float h01 = GetHeight(x0, z1);
        float h11 = GetHeight(x1, z1);

        // Bilinear interpolation
        float h0 = h00 * (1.0f - fx) + h10 * fx;
        float h1 = h01 * (1.0f - fx) + h11 * fx;

        return h0 * (1.0f - fz) + h1 * fz;
    }

    // ============================================================================
    // LOD
    // ============================================================================

    void Chunk::SetLOD(int level)
    {
        level = std::clamp(level, 0, 4); // Max LOD level is 4 (step size = 16)

        if (m_LOD != level)
        {
            m_LOD = level;
            m_NeedsRebuild = true;
        }
    }

    // ============================================================================
    // Private Methods
    // ============================================================================

    void Chunk::GenerateVertices(std::vector<SM::Vertex>& vertices, int lodStep) const
    {
        const int vertexCount = SIZE + 1;
        const int lodVertexCount = (SIZE / lodStep) + 1;

        vertices.reserve(lodVertexCount * lodVertexCount);

        float worldOffsetX = static_cast<float>(m_Coord.X) * GetWorldSize();
        float worldOffsetZ = static_cast<float>(m_Coord.Z) * GetWorldSize();

        // Calculate UV scale based on chunk size
        float uvScale = 1.0f / static_cast<float>(SIZE);

        for (int z = 0; z <= SIZE; z += lodStep)
        {
            for (int x = 0; x <= SIZE; x += lodStep)
            {
                float height = GetHeight(x, z);

                // Calculate world position
                DirectX::XMFLOAT3 position(
                    worldOffsetX + static_cast<float>(x) * SCALE,
                    height,
                    worldOffsetZ + static_cast<float>(z) * SCALE
                );

                // Calculate normal
                DirectX::XMFLOAT3 normal = CalculateNormal(x, z);

                // Calculate UV coordinates
                DirectX::XMFLOAT2 texCoord(
                    static_cast<float>(x) * uvScale,
                    static_cast<float>(z) * uvScale
                );

                // Calculate vertex color based on height and slope
                DirectX::XMFLOAT4 color = CalculateVertexColor(height, normal);

                vertices.emplace_back(position, normal, texCoord, color);
            }
        }
    }

    void Chunk::GenerateIndices(std::vector<uint32_t>& indices, int lodStep) const
    {
        const int lodVertexCount = (SIZE / lodStep) + 1;
        const int quadCount = lodVertexCount - 1;

        // Two triangles per quad, 3 indices per triangle
        indices.reserve(quadCount * quadCount * 6);

        for (int z = 0; z < quadCount; ++z)
        {
            for (int x = 0; x < quadCount; ++x)
            {
                uint32_t topLeft = z * lodVertexCount + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * lodVertexCount + x;
                uint32_t bottomRight = bottomLeft + 1;

                // First triangle (top-left, bottom-left, bottom-right)
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);

                // Second triangle (top-left, bottom-right, top-right)
                indices.push_back(topLeft);
                indices.push_back(bottomRight);
                indices.push_back(topRight);
            }
        }
    }

    DirectX::XMFLOAT3 Chunk::CalculateNormal(int x, int z) const
    {
        // Get heights of neighboring vertices
        float hL = GetHeight(x - 1, z);  // Left
        float hR = GetHeight(x + 1, z);  // Right
        float hD = GetHeight(x, z - 1);  // Down (closer in Z)
        float hU = GetHeight(x, z + 1);  // Up (further in Z)

        // Handle edge cases by using current height
        if (x == 0) hL = GetHeight(x, z);
        if (x == SIZE) hR = GetHeight(x, z);
        if (z == 0) hD = GetHeight(x, z);
        if (z == SIZE) hU = GetHeight(x, z);

        // Calculate normal using central differences
        float nx = (hL - hR) * 0.5f;
        float ny = SCALE; // This controls the "steepness" of the normal
        float nz = (hD - hU) * 0.5f;

        // Normalize
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 0.0001f)
        {
            nx /= len;
            ny /= len;
            nz /= len;
        }
        else
        {
            nx = 0.0f;
            ny = 1.0f;
            nz = 0.0f;
        }

        return DirectX::XMFLOAT3(nx, ny, nz);
    }

    DirectX::XMFLOAT4 Chunk::CalculateVertexColor(float height, const DirectX::XMFLOAT3& normal) const
    {
        // Normalize height to [0, 1] range based on chunk bounds
        float heightRange = m_MaxHeight - m_MinHeight;
        float normalizedHeight = (heightRange > 0.001f)
            ? (height - m_MinHeight) / heightRange
            : 0.5f;

        // Calculate slope (0 = flat, 1 = vertical)
        float slope = 1.0f - normal.y;

        // Define terrain colors
        // Deep water: dark blue
        DirectX::XMFLOAT3 deepWater(0.05f, 0.1f, 0.3f);
        // Shallow water: light blue
        DirectX::XMFLOAT3 shallowWater(0.1f, 0.3f, 0.5f);
        // Sand: tan
        DirectX::XMFLOAT3 sand(0.76f, 0.70f, 0.50f);
        // Grass: green
        DirectX::XMFLOAT3 grass(0.2f, 0.5f, 0.15f);
        // Rock: gray
        DirectX::XMFLOAT3 rock(0.5f, 0.5f, 0.5f);
        // Snow: white
        DirectX::XMFLOAT3 snow(0.95f, 0.95f, 0.98f);

        // Height thresholds
        const float waterLevel = 0.2f;
        const float sandLevel = 0.25f;
        const float grassLevel = 0.6f;
        const float rockLevel = 0.8f;

        DirectX::XMFLOAT3 color;

        if (normalizedHeight < waterLevel)
        {
            // Deep to shallow water
            float t = normalizedHeight / waterLevel;
            color.x = deepWater.x + t * (shallowWater.x - deepWater.x);
            color.y = deepWater.y + t * (shallowWater.y - deepWater.y);
            color.z = deepWater.z + t * (shallowWater.z - deepWater.z);
        }
        else if (normalizedHeight < sandLevel)
        {
            // Sand
            float t = (normalizedHeight - waterLevel) / (sandLevel - waterLevel);
            color.x = shallowWater.x + t * (sand.x - shallowWater.x);
            color.y = shallowWater.y + t * (sand.y - shallowWater.y);
            color.z = shallowWater.z + t * (sand.z - shallowWater.z);
        }
        else if (normalizedHeight < grassLevel)
        {
            // Grass (with slope-based rock blending)
            float t = (normalizedHeight - sandLevel) / (grassLevel - sandLevel);
            color.x = sand.x + t * (grass.x - sand.x);
            color.y = sand.y + t * (grass.y - sand.y);
            color.z = sand.z + t * (grass.z - sand.z);

            // Blend with rock on steep slopes
            if (slope > 0.3f)
            {
                float rockBlend = (slope - 0.3f) / 0.4f;
                rockBlend = std::clamp(rockBlend, 0.0f, 1.0f);
                color.x = color.x + rockBlend * (rock.x - color.x);
                color.y = color.y + rockBlend * (rock.y - color.y);
                color.z = color.z + rockBlend * (rock.z - color.z);
            }
        }
        else if (normalizedHeight < rockLevel)
        {
            // Rock
            float t = (normalizedHeight - grassLevel) / (rockLevel - grassLevel);
            color.x = grass.x + t * (rock.x - grass.x);
            color.y = grass.y + t * (rock.y - grass.y);
            color.z = grass.z + t * (rock.z - grass.z);
        }
        else
        {
            // Snow
            float t = (normalizedHeight - rockLevel) / (1.0f - rockLevel);
            color.x = rock.x + t * (snow.x - rock.x);
            color.y = rock.y + t * (snow.y - rock.y);
            color.z = rock.z + t * (snow.z - rock.z);
        }

        return DirectX::XMFLOAT4(color.x, color.y, color.z, 1.0f);
    }

    void Chunk::UpdateHeightBounds()
    {
        if (m_Heights.empty())
        {
            m_MinHeight = 0.0f;
            m_MaxHeight = 0.0f;
            return;
        }

        m_MinHeight = m_Heights[0];
        m_MaxHeight = m_Heights[0];

        for (float h : m_Heights)
        {
            m_MinHeight = std::min(m_MinHeight, h);
            m_MaxHeight = std::max(m_MaxHeight, h);
        }
    }

} // namespace PCG
