#pragma once

/**
 * @file ChunkManager.h
 * @brief Dynamic chunk loading and management for procedural terrain
 *
 * Manages terrain chunks around the camera position, handling:
 * - Dynamic loading/unloading based on view distance
 * - LOD level management
 * - Chunk generation and mesh building
 */

#include "pcg/Chunk.h"
#include "pcg/TerrainLOD.h"
#include "pcg/HeightmapGenerator.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <queue>
#include <DirectXMath.h>

namespace SM
{
    class DX12Core;
    class Renderer;
}

namespace PCG
{
    /**
     * @brief Configuration for the chunk manager
     */
    struct ChunkManagerConfig
    {
        float ViewDistance = 300.0f;       ///< Maximum view distance for chunks
        float UnloadDistance = 350.0f;     ///< Distance at which chunks are unloaded
        int MaxChunksPerFrame = 2;         ///< Max chunks to generate per frame
        int MaxMeshBuildsPerFrame = 4;     ///< Max mesh builds per frame
        bool AsyncGeneration = false;       ///< Enable async chunk generation (future)

        HeightmapSettings TerrainSettings; ///< Settings for terrain generation
    };

    /**
     * @brief Manages terrain chunks dynamically based on camera position
     *
     * The ChunkManager handles the lifecycle of terrain chunks:
     * 1. Determines which chunks should be visible based on camera position
     * 2. Queues new chunks for generation
     * 3. Updates LOD levels based on distance
     * 4. Unloads chunks that are too far away
     */
    class ChunkManager
    {
    public:
        ChunkManager();
        ~ChunkManager() = default;

        // Prevent copying
        ChunkManager(const ChunkManager&) = delete;
        ChunkManager& operator=(const ChunkManager&) = delete;

        // ====================================================================
        // Initialization
        // ====================================================================

        /**
         * @brief Initialize the chunk manager
         * @param core DX12 core for GPU resource creation
         * @param config Configuration settings
         * @return true if initialization succeeded
         */
        bool Initialize(SM::DX12Core* core, const ChunkManagerConfig& config = ChunkManagerConfig());

        /**
         * @brief Shutdown and release all resources
         */
        void Shutdown();

        // ====================================================================
        // Update
        // ====================================================================

        /**
         * @brief Update chunk loading/unloading based on camera position
         * @param cameraPosition Current camera position in world space
         *
         * This should be called every frame to:
         * - Queue new chunks for generation
         * - Update LOD levels
         * - Unload distant chunks
         * - Process pending operations
         */
        void Update(const DirectX::XMFLOAT3& cameraPosition);

        /**
         * @brief Force immediate loading of chunks around a position
         * @param position World position
         * @param radius Radius in world units
         *
         * Useful for initial loading or teleportation.
         */
        void ForceLoadAround(const DirectX::XMFLOAT3& position, float radius);

        // ====================================================================
        // Rendering
        // ====================================================================

        /**
         * @brief Get all visible chunks for rendering
         * @return Vector of pointers to visible chunks
         */
        const std::vector<Chunk*>& GetVisibleChunks() const { return m_VisibleChunks; }

        /**
         * @brief Get chunk count for debugging
         */
        size_t GetLoadedChunkCount() const { return m_Chunks.size(); }

        /**
         * @brief Get pending generation count
         */
        size_t GetPendingCount() const { return m_PendingGeneration.size(); }

        // ====================================================================
        // Configuration
        // ====================================================================

        /**
         * @brief Set the view distance
         * @param distance New view distance in world units
         */
        void SetViewDistance(float distance);

        /**
         * @brief Set custom LOD distances
         * @param distances Vector of LOD threshold distances
         */
        void SetLODDistances(const std::vector<float>& distances);

        /**
         * @brief Get current configuration
         */
        const ChunkManagerConfig& GetConfig() const { return m_Config; }

        /**
         * @brief Get the LOD system
         */
        const TerrainLOD& GetLOD() const { return m_LOD; }

        // ====================================================================
        // Terrain Queries
        // ====================================================================

        /**
         * @brief Get terrain height at world position
         * @param worldX World X coordinate
         * @param worldZ World Z coordinate
         * @return Height at position, or 0 if no chunk loaded
         */
        float GetHeightAt(float worldX, float worldZ) const;

        /**
         * @brief Get chunk at world position
         * @param worldX World X coordinate
         * @param worldZ World Z coordinate
         * @return Pointer to chunk, or nullptr if not loaded
         */
        Chunk* GetChunkAt(float worldX, float worldZ);
        const Chunk* GetChunkAt(float worldX, float worldZ) const;

        /**
         * @brief Get chunk by coordinate
         * @param coord Chunk coordinate
         * @return Pointer to chunk, or nullptr if not loaded
         */
        Chunk* GetChunk(const ChunkCoord& coord);
        const Chunk* GetChunk(const ChunkCoord& coord) const;

    private:
        // ====================================================================
        // Internal Methods
        // ====================================================================

        /**
         * @brief Convert world position to chunk coordinate
         */
        ChunkCoord WorldToChunkCoord(float worldX, float worldZ) const;

        /**
         * @brief Calculate distance from camera to chunk center
         */
        float CalculateChunkDistance(const ChunkCoord& coord, const DirectX::XMFLOAT3& cameraPos) const;

        /**
         * @brief Determine which chunks should be loaded
         */
        void DetermineVisibleChunks(const DirectX::XMFLOAT3& cameraPosition);

        /**
         * @brief Queue a chunk for generation
         */
        void QueueChunkGeneration(const ChunkCoord& coord);

        /**
         * @brief Process pending chunk generations
         */
        void ProcessPendingGenerations();

        /**
         * @brief Process pending mesh builds
         */
        void ProcessPendingMeshBuilds();

        /**
         * @brief Unload chunks beyond unload distance
         */
        void UnloadDistantChunks(const DirectX::XMFLOAT3& cameraPosition);

        /**
         * @brief Update LOD levels for all chunks
         */
        void UpdateChunkLODs(const DirectX::XMFLOAT3& cameraPosition);

        /**
         * @brief Update the visible chunks list
         */
        void UpdateVisibleChunksList();

        /**
         * @brief Create and generate a new chunk
         */
        std::unique_ptr<Chunk> CreateChunk(const ChunkCoord& coord);

    private:
        // Configuration
        ChunkManagerConfig m_Config;
        TerrainLOD m_LOD;

        // DX12 resources
        SM::DX12Core* m_Core = nullptr;

        // Terrain generation
        HeightmapGenerator m_Generator;

        // Chunk storage
        std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkHash> m_Chunks;

        // Generation queues
        std::queue<ChunkCoord> m_PendingGeneration;  ///< Chunks waiting to be generated
        std::queue<ChunkCoord> m_PendingMeshBuild;   ///< Chunks waiting for mesh build

        // Visible chunks (updated each frame)
        std::vector<Chunk*> m_VisibleChunks;

        // Camera tracking
        ChunkCoord m_LastCameraChunk;
        bool m_Initialized = false;
    };

} // namespace PCG
