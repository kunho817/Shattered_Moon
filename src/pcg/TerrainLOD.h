#pragma once

/**
 * @file TerrainLOD.h
 * @brief Level of Detail system for terrain rendering
 *
 * Manages LOD levels for terrain chunks based on distance from camera.
 * Supports multiple LOD levels with configurable distance thresholds.
 */

#include <vector>
#include <cstdint>

namespace PCG
{
    /**
     * @brief Configuration for a single LOD level
     */
    struct LODLevel
    {
        int Level;            ///< LOD level (0 = highest detail)
        float MaxDistance;    ///< Maximum distance for this LOD level
        int MeshResolution;   ///< Mesh resolution at this level (vertices per side)

        LODLevel() : Level(0), MaxDistance(100.0f), MeshResolution(33) {}
        LODLevel(int level, float maxDist, int resolution)
            : Level(level), MaxDistance(maxDist), MeshResolution(resolution) {}
    };

    /**
     * @brief Terrain LOD management system
     *
     * Determines appropriate LOD levels based on camera distance
     * and provides mesh resolution information for each level.
     */
    class TerrainLOD
    {
    public:
        TerrainLOD();
        ~TerrainLOD() = default;

        // ====================================================================
        // Configuration
        // ====================================================================

        /**
         * @brief Set LOD levels with custom configuration
         * @param levels Vector of LOD level configurations (sorted by distance)
         */
        void SetLevels(const std::vector<LODLevel>& levels);

        /**
         * @brief Set LOD distances only (auto-calculates resolutions)
         * @param distances Maximum distances for each LOD level
         * @param baseResolution Resolution at LOD 0 (default: 33 for 32x32 quads)
         */
        void SetDistances(const std::vector<float>& distances, int baseResolution = 33);

        /**
         * @brief Create default LOD levels
         * @param viewDistance Maximum view distance
         * @param numLevels Number of LOD levels to create
         */
        void SetupDefault(float viewDistance = 500.0f, int numLevels = 5);

        // ====================================================================
        // LOD Queries
        // ====================================================================

        /**
         * @brief Get the appropriate LOD level for a given distance
         * @param distance Distance from camera to object
         * @return LOD level (0 = highest detail)
         */
        int GetLODLevel(float distance) const;

        /**
         * @brief Get the mesh step size for a LOD level
         * @param lodLevel LOD level
         * @return Step size (1, 2, 4, 8, etc.)
         *
         * Step size determines how many vertices are skipped when
         * generating the mesh. Step 1 = full detail, Step 2 = half, etc.
         */
        int GetMeshStep(int lodLevel) const;

        /**
         * @brief Get the mesh resolution for a LOD level
         * @param lodLevel LOD level
         * @return Number of vertices per side
         */
        int GetMeshResolution(int lodLevel) const;

        /**
         * @brief Get maximum distance for a LOD level
         * @param lodLevel LOD level
         * @return Maximum distance, or FLT_MAX for highest level
         */
        float GetLODDistance(int lodLevel) const;

        /**
         * @brief Get total number of LOD levels
         */
        int GetLevelCount() const { return static_cast<int>(m_Levels.size()); }

        // ====================================================================
        // LOD Transition Helpers
        // ====================================================================

        /**
         * @brief Calculate blend factor for LOD transition
         * @param distance Current distance
         * @param lodLevel Current LOD level
         * @return Blend factor (0 = current LOD, 1 = next LOD)
         *
         * Used for smooth LOD transitions to minimize popping.
         */
        float GetTransitionFactor(float distance, int lodLevel) const;

        /**
         * @brief Check if distance is in LOD transition zone
         * @param distance Current distance
         * @param lodLevel Current LOD level
         * @param transitionWidth Width of transition zone (default: 10% of LOD distance)
         * @return true if in transition zone
         */
        bool IsInTransition(float distance, int lodLevel, float transitionWidth = 0.1f) const;

        // ====================================================================
        // Accessors
        // ====================================================================

        /**
         * @brief Get all LOD levels
         */
        const std::vector<LODLevel>& GetLevels() const { return m_Levels; }

        /**
         * @brief Get the maximum view distance
         */
        float GetMaxDistance() const;

    private:
        std::vector<LODLevel> m_Levels;
    };

    /**
     * @brief Factory for common LOD configurations
     */
    namespace LODPresets
    {
        /**
         * @brief Create LOD for close-up terrain (small maps)
         * @return TerrainLOD configured for small view distances
         */
        TerrainLOD CreateCloseRange();

        /**
         * @brief Create LOD for medium-range terrain
         * @return TerrainLOD configured for medium view distances
         */
        TerrainLOD CreateMediumRange();

        /**
         * @brief Create LOD for large terrain (open worlds)
         * @return TerrainLOD configured for large view distances
         */
        TerrainLOD CreateLongRange();

        /**
         * @brief Create LOD optimized for performance
         * @return TerrainLOD with aggressive LOD distances
         */
        TerrainLOD CreatePerformance();

        /**
         * @brief Create LOD optimized for quality
         * @return TerrainLOD with conservative LOD distances
         */
        TerrainLOD CreateQuality();
    }

} // namespace PCG
