#include "pcg/ChunkManager.h"
#include "renderer/DX12Core.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace PCG
{
    ChunkManager::ChunkManager()
        : m_LastCameraChunk(INT_MAX, INT_MAX)
    {
    }

    // ============================================================================
    // Initialization
    // ============================================================================

    bool ChunkManager::Initialize(SM::DX12Core* core, const ChunkManagerConfig& config)
    {
        if (!core)
        {
            std::cerr << "[ChunkManager] Cannot initialize: DX12Core is null" << std::endl;
            return false;
        }

        m_Core = core;
        m_Config = config;

        // Setup LOD system
        m_LOD.SetupDefault(config.ViewDistance, 5);

        // Initialize terrain settings if not set
        if (m_Config.TerrainSettings.Width == 0)
        {
            m_Config.TerrainSettings = HeightmapSettings::Continent(Chunk::SIZE);
            m_Config.TerrainSettings.MinHeight = 0.0f;
            m_Config.TerrainSettings.MaxHeight = 50.0f;
        }

        m_Initialized = true;

        std::cout << "[ChunkManager] Initialized with view distance: " << config.ViewDistance << std::endl;

        return true;
    }

    void ChunkManager::Shutdown()
    {
        // Clear all chunks
        m_Chunks.clear();

        // Clear queues
        while (!m_PendingGeneration.empty()) m_PendingGeneration.pop();
        while (!m_PendingMeshBuild.empty()) m_PendingMeshBuild.pop();

        m_VisibleChunks.clear();
        m_Initialized = false;
        m_Core = nullptr;

        std::cout << "[ChunkManager] Shutdown complete" << std::endl;
    }

    // ============================================================================
    // Update
    // ============================================================================

    void ChunkManager::Update(const DirectX::XMFLOAT3& cameraPosition)
    {
        if (!m_Initialized)
        {
            return;
        }

        // Determine which chunks should be loaded
        DetermineVisibleChunks(cameraPosition);

        // Process pending generations (limited per frame)
        ProcessPendingGenerations();

        // Process pending mesh builds (limited per frame)
        ProcessPendingMeshBuilds();

        // Update LOD levels for existing chunks
        UpdateChunkLODs(cameraPosition);

        // Unload chunks that are too far away
        UnloadDistantChunks(cameraPosition);

        // Update visible chunks list for rendering
        UpdateVisibleChunksList();

        // Update last camera chunk
        m_LastCameraChunk = WorldToChunkCoord(cameraPosition.x, cameraPosition.z);
    }

    void ChunkManager::ForceLoadAround(const DirectX::XMFLOAT3& position, float radius)
    {
        if (!m_Initialized)
        {
            return;
        }

        ChunkCoord centerChunk = WorldToChunkCoord(position.x, position.z);
        float chunkSize = Chunk::GetWorldSize();
        int chunkRadius = static_cast<int>(std::ceil(radius / chunkSize));

        for (int z = -chunkRadius; z <= chunkRadius; ++z)
        {
            for (int x = -chunkRadius; x <= chunkRadius; ++x)
            {
                ChunkCoord coord(centerChunk.X + x, centerChunk.Z + z);

                // Check if already loaded
                if (m_Chunks.find(coord) != m_Chunks.end())
                {
                    continue;
                }

                // Create and generate immediately
                auto chunk = CreateChunk(coord);
                if (chunk)
                {
                    chunk->BuildMesh(m_Core);
                    m_Chunks[coord] = std::move(chunk);
                }
            }
        }

        UpdateVisibleChunksList();
    }

    // ============================================================================
    // Configuration
    // ============================================================================

    void ChunkManager::SetViewDistance(float distance)
    {
        m_Config.ViewDistance = distance;
        m_Config.UnloadDistance = distance * 1.2f; // 20% buffer

        // Update LOD system
        m_LOD.SetupDefault(distance, 5);
    }

    void ChunkManager::SetLODDistances(const std::vector<float>& distances)
    {
        m_LOD.SetDistances(distances);
    }

    // ============================================================================
    // Terrain Queries
    // ============================================================================

    float ChunkManager::GetHeightAt(float worldX, float worldZ) const
    {
        const Chunk* chunk = GetChunkAt(worldX, worldZ);
        if (!chunk || !chunk->IsGenerated())
        {
            return 0.0f;
        }

        // Convert to local coordinates
        float chunkSize = Chunk::GetWorldSize();
        float localX = std::fmod(worldX, chunkSize);
        float localZ = std::fmod(worldZ, chunkSize);

        // Handle negative coordinates
        if (localX < 0.0f) localX += chunkSize;
        if (localZ < 0.0f) localZ += chunkSize;

        // Convert to vertex coordinates
        float vertexX = localX / Chunk::SCALE;
        float vertexZ = localZ / Chunk::SCALE;

        return chunk->GetHeightInterpolated(vertexX, vertexZ);
    }

    Chunk* ChunkManager::GetChunkAt(float worldX, float worldZ)
    {
        ChunkCoord coord = WorldToChunkCoord(worldX, worldZ);
        return GetChunk(coord);
    }

    const Chunk* ChunkManager::GetChunkAt(float worldX, float worldZ) const
    {
        ChunkCoord coord = WorldToChunkCoord(worldX, worldZ);
        return GetChunk(coord);
    }

    Chunk* ChunkManager::GetChunk(const ChunkCoord& coord)
    {
        auto it = m_Chunks.find(coord);
        if (it != m_Chunks.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    const Chunk* ChunkManager::GetChunk(const ChunkCoord& coord) const
    {
        auto it = m_Chunks.find(coord);
        if (it != m_Chunks.end())
        {
            return it->second.get();
        }
        return nullptr;
    }

    // ============================================================================
    // Internal Methods
    // ============================================================================

    ChunkCoord ChunkManager::WorldToChunkCoord(float worldX, float worldZ) const
    {
        float chunkSize = Chunk::GetWorldSize();

        int x = static_cast<int>(std::floor(worldX / chunkSize));
        int z = static_cast<int>(std::floor(worldZ / chunkSize));

        return ChunkCoord(x, z);
    }

    float ChunkManager::CalculateChunkDistance(const ChunkCoord& coord, const DirectX::XMFLOAT3& cameraPos) const
    {
        DirectX::XMFLOAT3 chunkCenter = coord.ToWorldCenter(Chunk::GetWorldSize());

        float dx = chunkCenter.x - cameraPos.x;
        float dz = chunkCenter.z - cameraPos.z;

        return std::sqrt(dx * dx + dz * dz);
    }

    void ChunkManager::DetermineVisibleChunks(const DirectX::XMFLOAT3& cameraPosition)
    {
        ChunkCoord centerChunk = WorldToChunkCoord(cameraPosition.x, cameraPosition.z);

        // Calculate how many chunks we need in each direction
        float chunkSize = Chunk::GetWorldSize();
        int chunkRadius = static_cast<int>(std::ceil(m_Config.ViewDistance / chunkSize)) + 1;

        // Check all chunks in range
        for (int z = -chunkRadius; z <= chunkRadius; ++z)
        {
            for (int x = -chunkRadius; x <= chunkRadius; ++x)
            {
                ChunkCoord coord(centerChunk.X + x, centerChunk.Z + z);

                // Calculate distance to chunk
                float distance = CalculateChunkDistance(coord, cameraPosition);

                // Skip chunks beyond view distance
                if (distance > m_Config.ViewDistance)
                {
                    continue;
                }

                // Check if chunk needs to be loaded
                if (m_Chunks.find(coord) == m_Chunks.end())
                {
                    QueueChunkGeneration(coord);
                }
            }
        }
    }

    void ChunkManager::QueueChunkGeneration(const ChunkCoord& coord)
    {
        // Check if already in queue (simple linear search for now)
        // In a production system, use a set for O(1) lookup
        std::queue<ChunkCoord> tempQueue = m_PendingGeneration;
        while (!tempQueue.empty())
        {
            if (tempQueue.front() == coord)
            {
                return; // Already queued
            }
            tempQueue.pop();
        }

        m_PendingGeneration.push(coord);
    }

    void ChunkManager::ProcessPendingGenerations()
    {
        int generated = 0;

        while (!m_PendingGeneration.empty() && generated < m_Config.MaxChunksPerFrame)
        {
            ChunkCoord coord = m_PendingGeneration.front();
            m_PendingGeneration.pop();

            // Skip if already loaded (might have been loaded by ForceLoadAround)
            if (m_Chunks.find(coord) != m_Chunks.end())
            {
                continue;
            }

            // Create and generate chunk
            auto chunk = CreateChunk(coord);
            if (chunk)
            {
                m_Chunks[coord] = std::move(chunk);
                m_PendingMeshBuild.push(coord);
                generated++;
            }
        }
    }

    void ChunkManager::ProcessPendingMeshBuilds()
    {
        int built = 0;

        while (!m_PendingMeshBuild.empty() && built < m_Config.MaxMeshBuildsPerFrame)
        {
            ChunkCoord coord = m_PendingMeshBuild.front();
            m_PendingMeshBuild.pop();

            auto it = m_Chunks.find(coord);
            if (it != m_Chunks.end())
            {
                Chunk* chunk = it->second.get();
                if (chunk && chunk->IsGenerated() && !chunk->HasMesh())
                {
                    if (chunk->BuildMesh(m_Core))
                    {
                        built++;
                    }
                }
            }
        }
    }

    void ChunkManager::UnloadDistantChunks(const DirectX::XMFLOAT3& cameraPosition)
    {
        std::vector<ChunkCoord> toUnload;

        for (const auto& pair : m_Chunks)
        {
            float distance = CalculateChunkDistance(pair.first, cameraPosition);

            if (distance > m_Config.UnloadDistance)
            {
                toUnload.push_back(pair.first);
            }
        }

        for (const ChunkCoord& coord : toUnload)
        {
            m_Chunks.erase(coord);
        }
    }

    void ChunkManager::UpdateChunkLODs(const DirectX::XMFLOAT3& cameraPosition)
    {
        for (auto& pair : m_Chunks)
        {
            Chunk* chunk = pair.second.get();
            if (!chunk)
            {
                continue;
            }

            float distance = CalculateChunkDistance(pair.first, cameraPosition);
            int newLOD = m_LOD.GetLODLevel(distance);

            if (chunk->GetLOD() != newLOD)
            {
                chunk->SetLOD(newLOD);

                // Rebuild mesh if LOD changed and chunk has mesh
                if (chunk->NeedsRebuild() && chunk->HasMesh())
                {
                    chunk->RebuildMesh(m_Core);
                }
            }
        }
    }

    void ChunkManager::UpdateVisibleChunksList()
    {
        m_VisibleChunks.clear();
        m_VisibleChunks.reserve(m_Chunks.size());

        for (auto& pair : m_Chunks)
        {
            Chunk* chunk = pair.second.get();
            if (chunk && chunk->HasMesh())
            {
                m_VisibleChunks.push_back(chunk);
            }
        }
    }

    std::unique_ptr<Chunk> ChunkManager::CreateChunk(const ChunkCoord& coord)
    {
        auto chunk = std::make_unique<Chunk>(coord);

        // Generate height data using a custom function based on settings
        chunk->GenerateCustom([this](float worldX, float worldZ) -> float {
            // Use FBM for terrain generation
            static PerlinNoise noise(m_Config.TerrainSettings.Seed);
            static FBM fbm(&noise);

            float height;
            if (m_Config.TerrainSettings.ApplyDomainWarp)
            {
                height = fbm.WarpedSample(worldX, worldZ,
                    m_Config.TerrainSettings.Noise,
                    m_Config.TerrainSettings.WarpStrength);
            }
            else
            {
                height = fbm.Sample(worldX, worldZ, m_Config.TerrainSettings.Noise);
            }

            // Remap from [-1, 1] to height range
            height = (height + 1.0f) * 0.5f;
            return m_Config.TerrainSettings.MinHeight +
                   height * (m_Config.TerrainSettings.MaxHeight - m_Config.TerrainSettings.MinHeight);
        });

        return chunk;
    }

} // namespace PCG
