#pragma once

/**
 * @file HeightmapGenerator.h
 * @brief Procedural heightmap generation for terrain
 *
 * Generates 2D height arrays using layered noise with various post-processing
 * options like erosion, falloff, and terrain type blending.
 */

#include "FBM.h"
#include <vector>
#include <functional>

namespace PCG {

    /**
     * @brief Settings for heightmap generation
     */
    struct HeightmapSettings {
        // Basic parameters
        uint32_t Seed = 12345;        ///< Random seed for reproducibility
        int Width = 256;              ///< Width in samples
        int Height = 256;             ///< Height in samples

        // Noise settings
        FBMSettings Noise;            ///< FBM parameters for base terrain

        // Height range
        float MinHeight = 0.0f;       ///< Minimum terrain height
        float MaxHeight = 100.0f;     ///< Maximum terrain height

        // Terrain type weights (should sum to 1.0)
        float MountainWeight = 0.3f;  ///< Mountain regions weight
        float HillWeight = 0.3f;      ///< Hill regions weight
        float PlainWeight = 0.25f;    ///< Plains regions weight
        float OceanWeight = 0.15f;    ///< Ocean/lowland regions weight

        // Post-processing
        bool ApplyFalloffMap = false; ///< Apply island-style falloff
        bool ApplyTerracing = false;  ///< Apply stepped terraces
        int TerraceCount = 8;         ///< Number of terrace levels

        // Domain warping
        bool ApplyDomainWarp = false; ///< Apply domain warping for organic shapes
        float WarpStrength = 0.3f;    ///< Domain warp strength

        /**
         * @brief Create settings for island generation
         */
        static HeightmapSettings Island(int size = 256) {
            HeightmapSettings s;
            s.Width = size;
            s.Height = size;
            s.Noise = FBMSettings::Terrain();
            s.ApplyFalloffMap = true;
            s.MountainWeight = 0.2f;
            s.HillWeight = 0.3f;
            s.PlainWeight = 0.3f;
            s.OceanWeight = 0.2f;
            return s;
        }

        /**
         * @brief Create settings for continental terrain
         */
        static HeightmapSettings Continent(int size = 512) {
            HeightmapSettings s;
            s.Width = size;
            s.Height = size;
            s.Noise = FBMSettings::Terrain();
            s.Noise.Octaves = 10;
            s.MountainWeight = 0.35f;
            s.HillWeight = 0.25f;
            s.PlainWeight = 0.25f;
            s.OceanWeight = 0.15f;
            s.ApplyDomainWarp = true;
            s.WarpStrength = 0.2f;
            return s;
        }

        /**
         * @brief Create settings for mountainous terrain
         */
        static HeightmapSettings Mountains(int size = 256) {
            HeightmapSettings s;
            s.Width = size;
            s.Height = size;
            s.Noise = FBMSettings::Mountains();
            s.MountainWeight = 0.6f;
            s.HillWeight = 0.3f;
            s.PlainWeight = 0.1f;
            s.OceanWeight = 0.0f;
            s.MinHeight = 500.0f;
            s.MaxHeight = 4000.0f;
            return s;
        }
    };

    /**
     * @brief Erosion simulation settings
     */
    struct ErosionSettings {
        int Iterations = 50000;       ///< Number of droplet simulations
        int MaxDropletLifetime = 30;  ///< Maximum steps per droplet
        float Inertia = 0.05f;        ///< Momentum (0 = none, 1 = full)
        float SedimentCapacity = 4.0f;///< Max sediment a droplet can carry
        float MinSedimentCapacity = 0.01f; ///< Minimum capacity threshold
        float DepositSpeed = 0.3f;    ///< How fast sediment deposits
        float ErodeSpeed = 0.3f;      ///< How fast terrain erodes
        float EvaporateSpeed = 0.01f; ///< Water evaporation rate
        float Gravity = 4.0f;         ///< Affects droplet speed on slopes
        int ErosionRadius = 3;        ///< Erosion brush radius
    };

    /**
     * @brief Procedural heightmap generator
     *
     * Creates terrain heightmaps using FBM noise with various
     * post-processing effects for realistic terrain.
     */
    class HeightmapGenerator {
    public:
        HeightmapGenerator();
        ~HeightmapGenerator() = default;

        // ====================================================================
        // Generation
        // ====================================================================

        /**
         * @brief Generate a heightmap with the given settings
         * @param settings Generation parameters
         * @return Vector of height values (row-major, size = Width * Height)
         */
        std::vector<float> Generate(const HeightmapSettings& settings);

        /**
         * @brief Generate heightmap using a custom noise function
         * @param settings Generation parameters
         * @param noiseFunc Custom function (x, y) -> height
         * @return Vector of height values
         */
        std::vector<float> GenerateCustom(const HeightmapSettings& settings,
                                          std::function<float(float, float)> noiseFunc);

        // ====================================================================
        // Post-Processing
        // ====================================================================

        /**
         * @brief Apply hydraulic erosion simulation
         * @param heightmap Height data to modify (in-place)
         * @param width Map width
         * @param height Map height
         * @param settings Erosion parameters
         */
        void ApplyErosion(std::vector<float>& heightmap, int width, int height,
                          const ErosionSettings& settings = ErosionSettings());

        /**
         * @brief Normalize heightmap to [0, 1] range
         * @param heightmap Height data to modify (in-place)
         */
        void Normalize(std::vector<float>& heightmap);

        /**
         * @brief Remap heightmap from [0, 1] to [minHeight, maxHeight]
         * @param heightmap Height data to modify (in-place)
         * @param minHeight Target minimum
         * @param maxHeight Target maximum
         */
        void Remap(std::vector<float>& heightmap, float minHeight, float maxHeight);

        /**
         * @brief Apply island-style falloff (edges go to 0)
         * @param heightmap Height data to modify (in-place)
         * @param width Map width
         * @param height Map height
         * @param falloffStrength How strong the falloff is (0-1)
         */
        void ApplyFalloff(std::vector<float>& heightmap, int width, int height,
                          float falloffStrength = 1.0f);

        /**
         * @brief Apply circular falloff (radial distance from center)
         * @param heightmap Height data to modify (in-place)
         * @param width Map width
         * @param height Map height
         * @param centerX Center X (0-1)
         * @param centerY Center Y (0-1)
         * @param radius Falloff radius (0-1)
         */
        void ApplyRadialFalloff(std::vector<float>& heightmap, int width, int height,
                                float centerX = 0.5f, float centerY = 0.5f,
                                float radius = 0.5f);

        /**
         * @brief Apply stepped terracing effect
         * @param heightmap Height data to modify (in-place)
         * @param levels Number of terrace levels
         */
        void ApplyTerracing(std::vector<float>& heightmap, int levels);

        /**
         * @brief Smooth the heightmap using gaussian blur
         * @param heightmap Height data to modify (in-place)
         * @param width Map width
         * @param height Map height
         * @param radius Blur radius
         */
        void Smooth(std::vector<float>& heightmap, int width, int height, int radius = 1);

        /**
         * @brief Apply thermal erosion (slumping)
         * @param heightmap Height data to modify (in-place)
         * @param width Map width
         * @param height Map height
         * @param iterations Number of iterations
         * @param talusAngle Maximum stable slope angle
         */
        void ApplyThermalErosion(std::vector<float>& heightmap, int width, int height,
                                  int iterations = 50, float talusAngle = 0.5f);

        // ====================================================================
        // Utility Functions
        // ====================================================================

        /**
         * @brief Generate a falloff map
         * @param width Map width
         * @param height Map height
         * @return Falloff values (0 at edges, 1 at center)
         */
        std::vector<float> GenerateFalloffMap(int width, int height);

        /**
         * @brief Calculate normal vectors from heightmap
         * @param heightmap Input height data
         * @param width Map width
         * @param height Map height
         * @param heightScale World-space height scale
         * @return Vector of normals (3 floats per pixel: x, y, z)
         */
        std::vector<float> CalculateNormals(const std::vector<float>& heightmap,
                                            int width, int height,
                                            float heightScale = 1.0f);

        /**
         * @brief Get height at a point with bilinear interpolation
         * @param heightmap Height data
         * @param width Map width
         * @param height Map height
         * @param x X coordinate (0 to width-1)
         * @param y Y coordinate (0 to height-1)
         * @return Interpolated height
         */
        float SampleBilinear(const std::vector<float>& heightmap,
                             int width, int height,
                             float x, float y) const;

        /**
         * @brief Calculate slope at a point
         * @param heightmap Height data
         * @param width Map width
         * @param height Map height
         * @param x X coordinate
         * @param y Y coordinate
         * @return Slope value (0 = flat, 1 = 45 degrees)
         */
        float CalculateSlope(const std::vector<float>& heightmap,
                             int width, int height,
                             int x, int y) const;

    private:
        std::unique_ptr<FBM> m_FBM;
        std::unique_ptr<INoise> m_BaseNoise;

        // Helper for erosion
        void InitErosionBrushes(int radius, std::vector<std::vector<int>>& brushIndices,
                                std::vector<std::vector<float>>& brushWeights,
                                int mapSize);

        float CalculateGradient(const std::vector<float>& heightmap,
                                int width, int height,
                                float posX, float posY,
                                float& gradX, float& gradY) const;
    };

    /**
     * @brief Data structure for heightmap with metadata
     */
    struct Heightmap {
        std::vector<float> Data;      ///< Height values
        int Width = 0;                ///< Width in samples
        int Height = 0;               ///< Height in samples
        float MinHeight = 0.0f;       ///< Minimum height value
        float MaxHeight = 1.0f;       ///< Maximum height value
        uint32_t Seed = 0;            ///< Seed used for generation

        /**
         * @brief Get height at integer coordinates
         */
        float At(int x, int y) const {
            if (x < 0 || x >= Width || y < 0 || y >= Height) return 0.0f;
            return Data[y * Width + x];
        }

        /**
         * @brief Set height at integer coordinates
         */
        void Set(int x, int y, float value) {
            if (x < 0 || x >= Width || y < 0 || y >= Height) return;
            Data[y * Width + x] = value;
        }

        /**
         * @brief Get the index for given coordinates
         */
        int Index(int x, int y) const {
            return y * Width + x;
        }

        /**
         * @brief Check if coordinates are valid
         */
        bool IsValid(int x, int y) const {
            return x >= 0 && x < Width && y >= 0 && y < Height;
        }
    };

} // namespace PCG
