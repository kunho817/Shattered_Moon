#include "pcg/TerrainLOD.h"
#include "pcg/Chunk.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace PCG
{
    TerrainLOD::TerrainLOD()
    {
        // Setup default LOD levels
        SetupDefault();
    }

    // ============================================================================
    // Configuration
    // ============================================================================

    void TerrainLOD::SetLevels(const std::vector<LODLevel>& levels)
    {
        m_Levels = levels;

        // Sort by distance to ensure proper ordering
        std::sort(m_Levels.begin(), m_Levels.end(),
            [](const LODLevel& a, const LODLevel& b) {
                return a.MaxDistance < b.MaxDistance;
            });

        // Update level indices
        for (size_t i = 0; i < m_Levels.size(); ++i)
        {
            m_Levels[i].Level = static_cast<int>(i);
        }
    }

    void TerrainLOD::SetDistances(const std::vector<float>& distances, int baseResolution)
    {
        m_Levels.clear();
        m_Levels.reserve(distances.size());

        for (size_t i = 0; i < distances.size(); ++i)
        {
            LODLevel level;
            level.Level = static_cast<int>(i);
            level.MaxDistance = distances[i];

            // Calculate resolution: halves with each LOD level
            int step = 1 << static_cast<int>(i);
            int quads = (baseResolution - 1) / step;
            level.MeshResolution = quads + 1;

            m_Levels.push_back(level);
        }
    }

    void TerrainLOD::SetupDefault(float viewDistance, int numLevels)
    {
        m_Levels.clear();
        m_Levels.reserve(numLevels);

        // Calculate base resolution from Chunk::SIZE
        const int baseResolution = Chunk::SIZE + 1;

        // Distribute LOD distances exponentially
        float distanceStep = viewDistance / static_cast<float>(numLevels);

        for (int i = 0; i < numLevels; ++i)
        {
            LODLevel level;
            level.Level = i;

            // Exponential distance distribution
            // LOD 0: closest, LOD n-1: farthest
            if (i == numLevels - 1)
            {
                level.MaxDistance = viewDistance;
            }
            else
            {
                // Use exponential curve for smoother transitions
                float t = static_cast<float>(i + 1) / static_cast<float>(numLevels);
                level.MaxDistance = viewDistance * (t * t);
            }

            // Resolution halves with each LOD level
            int step = 1 << i;
            int quads = Chunk::SIZE / step;
            level.MeshResolution = quads + 1;

            m_Levels.push_back(level);
        }
    }

    // ============================================================================
    // LOD Queries
    // ============================================================================

    int TerrainLOD::GetLODLevel(float distance) const
    {
        if (m_Levels.empty())
        {
            return 0;
        }

        for (const auto& level : m_Levels)
        {
            if (distance <= level.MaxDistance)
            {
                return level.Level;
            }
        }

        // Beyond max distance, return highest LOD level
        return static_cast<int>(m_Levels.size()) - 1;
    }

    int TerrainLOD::GetMeshStep(int lodLevel) const
    {
        // Clamp LOD level to valid range
        lodLevel = std::clamp(lodLevel, 0, static_cast<int>(m_Levels.size()) - 1);

        // Step doubles with each LOD level: 1, 2, 4, 8, 16
        return 1 << lodLevel;
    }

    int TerrainLOD::GetMeshResolution(int lodLevel) const
    {
        if (m_Levels.empty())
        {
            return Chunk::SIZE + 1;
        }

        lodLevel = std::clamp(lodLevel, 0, static_cast<int>(m_Levels.size()) - 1);
        return m_Levels[lodLevel].MeshResolution;
    }

    float TerrainLOD::GetLODDistance(int lodLevel) const
    {
        if (m_Levels.empty())
        {
            return std::numeric_limits<float>::max();
        }

        lodLevel = std::clamp(lodLevel, 0, static_cast<int>(m_Levels.size()) - 1);
        return m_Levels[lodLevel].MaxDistance;
    }

    // ============================================================================
    // LOD Transition Helpers
    // ============================================================================

    float TerrainLOD::GetTransitionFactor(float distance, int lodLevel) const
    {
        if (m_Levels.empty() || lodLevel >= static_cast<int>(m_Levels.size()) - 1)
        {
            return 0.0f;
        }

        float currentMaxDist = m_Levels[lodLevel].MaxDistance;
        float nextMinDist = currentMaxDist * 0.9f; // Transition starts at 90% of max distance

        if (distance < nextMinDist)
        {
            return 0.0f;
        }

        if (distance >= currentMaxDist)
        {
            return 1.0f;
        }

        // Linear interpolation in transition zone
        return (distance - nextMinDist) / (currentMaxDist - nextMinDist);
    }

    bool TerrainLOD::IsInTransition(float distance, int lodLevel, float transitionWidth) const
    {
        if (m_Levels.empty() || lodLevel >= static_cast<int>(m_Levels.size()) - 1)
        {
            return false;
        }

        float maxDist = m_Levels[lodLevel].MaxDistance;
        float transitionStart = maxDist * (1.0f - transitionWidth);

        return distance >= transitionStart && distance < maxDist;
    }

    float TerrainLOD::GetMaxDistance() const
    {
        if (m_Levels.empty())
        {
            return 0.0f;
        }

        return m_Levels.back().MaxDistance;
    }

    // ============================================================================
    // LOD Presets
    // ============================================================================

    namespace LODPresets
    {
        TerrainLOD CreateCloseRange()
        {
            TerrainLOD lod;
            lod.SetDistances({ 50.0f, 100.0f, 150.0f, 200.0f });
            return lod;
        }

        TerrainLOD CreateMediumRange()
        {
            TerrainLOD lod;
            lod.SetDistances({ 100.0f, 200.0f, 400.0f, 600.0f, 800.0f });
            return lod;
        }

        TerrainLOD CreateLongRange()
        {
            TerrainLOD lod;
            lod.SetDistances({ 200.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f });
            return lod;
        }

        TerrainLOD CreatePerformance()
        {
            TerrainLOD lod;
            // Aggressive LOD - drops detail quickly
            lod.SetDistances({ 30.0f, 60.0f, 120.0f, 250.0f, 500.0f });
            return lod;
        }

        TerrainLOD CreateQuality()
        {
            TerrainLOD lod;
            // Conservative LOD - maintains detail longer
            lod.SetDistances({ 150.0f, 300.0f, 500.0f, 750.0f, 1000.0f });
            return lod;
        }
    }

} // namespace PCG
