#pragma once

/**
 * @file Chunk.h
 * @brief Terrain chunk system for procedural terrain rendering
 *
 * Defines the Chunk class which represents a single terrain tile
 * with height data and mesh generation capabilities.
 */

#include "pcg/HeightmapGenerator.h"
#include "renderer/Mesh.h"

#include <vector>
#include <cstdint>
#include <functional>

namespace SM
{
    // Forward declarations
    class DX12Core;
}

namespace PCG
{
    /**
     * @brief 2D coordinate for chunk identification
     *
     * Uses integer coordinates to identify chunks in a grid.
     * Chunk (0,0) is centered at world origin.
     */
    struct ChunkCoord
    {
        int X = 0;
        int Z = 0;

        ChunkCoord() = default;
        ChunkCoord(int x, int z) : X(x), Z(z) {}

        bool operator==(const ChunkCoord& other) const
        {
            return X == other.X && Z == other.Z;
        }

        bool operator!=(const ChunkCoord& other) const
        {
            return !(*this == other);
        }

        /**
         * @brief Get the world position of this chunk's corner
         * @param chunkSize Size of a chunk in world units
         * @return World position (X, 0, Z)
         */
        DirectX::XMFLOAT3 ToWorldPosition(float chunkSize) const
        {
            return DirectX::XMFLOAT3(
                static_cast<float>(X) * chunkSize,
                0.0f,
                static_cast<float>(Z) * chunkSize
            );
        }

        /**
         * @brief Get the center world position of this chunk
         * @param chunkSize Size of a chunk in world units
         * @return World center position
         */
        DirectX::XMFLOAT3 ToWorldCenter(float chunkSize) const
        {
            float halfSize = chunkSize * 0.5f;
            return DirectX::XMFLOAT3(
                static_cast<float>(X) * chunkSize + halfSize,
                0.0f,
                static_cast<float>(Z) * chunkSize + halfSize
            );
        }

        /**
         * @brief Calculate Manhattan distance to another chunk
         */
        int ManhattanDistance(const ChunkCoord& other) const
        {
            return std::abs(X - other.X) + std::abs(Z - other.Z);
        }

        /**
         * @brief Calculate Chebyshev (chessboard) distance to another chunk
         */
        int ChebyshevDistance(const ChunkCoord& other) const
        {
            return std::max(std::abs(X - other.X), std::abs(Z - other.Z));
        }
    };

    /**
     * @brief Hash function for ChunkCoord
     *
     * Allows ChunkCoord to be used as a key in unordered_map.
     */
    struct ChunkHash
    {
        size_t operator()(const ChunkCoord& coord) const
        {
            // Combine X and Z using a hash function
            // Using Cantor pairing function variant
            size_t h1 = std::hash<int>{}(coord.X);
            size_t h2 = std::hash<int>{}(coord.Z);
            return h1 ^ (h2 << 1);
        }
    };

    /**
     * @brief Terrain chunk containing height data and mesh
     *
     * Each chunk represents a square section of terrain with:
     * - Height values for each vertex
     * - GPU mesh for rendering
     * - LOD level for distance-based quality
     */
    class Chunk
    {
    public:
        // Constants
        static constexpr int SIZE = 32;          ///< Number of vertices per side (33x33 for 32x32 quads)
        static constexpr int VERTEX_COUNT = (SIZE + 1) * (SIZE + 1);
        static constexpr float SCALE = 1.0f;     ///< World units per vertex spacing

        /**
         * @brief Construct a chunk at the given coordinate
         * @param coord Chunk grid coordinate
         */
        explicit Chunk(ChunkCoord coord);

        /**
         * @brief Default destructor
         */
        ~Chunk() = default;

        // Prevent copying (chunks own GPU resources)
        Chunk(const Chunk&) = delete;
        Chunk& operator=(const Chunk&) = delete;

        // Allow moving
        Chunk(Chunk&&) noexcept = default;
        Chunk& operator=(Chunk&&) noexcept = default;

        // ====================================================================
        // Generation
        // ====================================================================

        /**
         * @brief Generate height data using the heightmap generator
         * @param generator HeightmapGenerator instance
         * @param settings Settings for generation
         */
        void Generate(const HeightmapGenerator& generator, const HeightmapSettings& settings);

        /**
         * @brief Generate height data using a custom height function
         * @param heightFunc Function taking (worldX, worldZ) and returning height
         */
        void GenerateCustom(std::function<float(float, float)> heightFunc);

        /**
         * @brief Build the GPU mesh from height data
         * @param core DX12 core for GPU resource creation
         * @return true if mesh creation succeeded
         */
        bool BuildMesh(SM::DX12Core* core);

        /**
         * @brief Rebuild mesh with current LOD level
         * @param core DX12 core for GPU resource creation
         * @return true if mesh recreation succeeded
         */
        bool RebuildMesh(SM::DX12Core* core);

        // ====================================================================
        // Accessors
        // ====================================================================

        /**
         * @brief Get the chunk's mesh
         * @return Reference to the mesh (may be invalid if not built)
         */
        SM::Mesh& GetMesh() { return m_Mesh; }
        const SM::Mesh& GetMesh() const { return m_Mesh; }

        /**
         * @brief Get the chunk coordinate
         */
        ChunkCoord GetCoord() const { return m_Coord; }

        /**
         * @brief Get the world position of this chunk's origin
         */
        DirectX::XMFLOAT3 GetWorldPosition() const
        {
            return m_Coord.ToWorldPosition(GetWorldSize());
        }

        /**
         * @brief Get the world size of this chunk
         */
        static float GetWorldSize()
        {
            return static_cast<float>(SIZE) * SCALE;
        }

        /**
         * @brief Check if height data has been generated
         */
        bool IsGenerated() const { return !m_Heights.empty(); }

        /**
         * @brief Check if mesh has been built
         */
        bool HasMesh() const { return m_Mesh.IsValid(); }

        /**
         * @brief Get height at local coordinates
         * @param localX Local X (0 to SIZE)
         * @param localZ Local Z (0 to SIZE)
         * @return Height value, or 0 if out of bounds
         */
        float GetHeight(int localX, int localZ) const;

        /**
         * @brief Get interpolated height at fractional local coordinates
         * @param localX Local X (0.0 to SIZE)
         * @param localZ Local Z (0.0 to SIZE)
         * @return Interpolated height value
         */
        float GetHeightInterpolated(float localX, float localZ) const;

        /**
         * @brief Get the minimum height in this chunk
         */
        float GetMinHeight() const { return m_MinHeight; }

        /**
         * @brief Get the maximum height in this chunk
         */
        float GetMaxHeight() const { return m_MaxHeight; }

        // ====================================================================
        // LOD (Level of Detail)
        // ====================================================================

        /**
         * @brief Set the LOD level (0 = highest detail)
         * @param level LOD level (0-4 typically)
         */
        void SetLOD(int level);

        /**
         * @brief Get the current LOD level
         */
        int GetLOD() const { return m_LOD; }

        /**
         * @brief Check if LOD changed since last mesh build
         */
        bool NeedsRebuild() const { return m_NeedsRebuild; }

    private:
        /**
         * @brief Generate vertex data from heights
         * @param vertices Output vertex array
         * @param lodStep Step size based on LOD level
         */
        void GenerateVertices(std::vector<SM::Vertex>& vertices, int lodStep) const;

        /**
         * @brief Generate index data for the mesh
         * @param indices Output index array
         * @param lodStep Step size based on LOD level
         */
        void GenerateIndices(std::vector<uint32_t>& indices, int lodStep) const;

        /**
         * @brief Calculate normal at a vertex position
         * @param x Local X coordinate
         * @param z Local Z coordinate
         * @return Normal vector
         */
        DirectX::XMFLOAT3 CalculateNormal(int x, int z) const;

        /**
         * @brief Calculate vertex color based on height and normal
         * @param height Height value
         * @param normal Normal vector
         * @return Vertex color (RGBA)
         */
        DirectX::XMFLOAT4 CalculateVertexColor(float height, const DirectX::XMFLOAT3& normal) const;

        /**
         * @brief Update min/max height values
         */
        void UpdateHeightBounds();

    private:
        ChunkCoord m_Coord;                  ///< Grid coordinate of this chunk
        int m_LOD = 0;                       ///< Current LOD level
        bool m_NeedsRebuild = false;         ///< Whether mesh needs rebuilding

        std::vector<float> m_Heights;        ///< Height data (SIZE+1)^2 elements
        float m_MinHeight = 0.0f;            ///< Minimum height in chunk
        float m_MaxHeight = 0.0f;            ///< Maximum height in chunk

        SM::Mesh m_Mesh;                     ///< GPU mesh
    };

} // namespace PCG
