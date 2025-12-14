#pragma once

/**
 * @file PCG.h
 * @brief Master include file for Procedural Content Generation system
 *
 * Include this file to access all PCG functionality:
 * - Noise algorithms (Perlin, Simplex, Worley, Value)
 * - FBM (Fractal Brownian Motion)
 * - Heightmap generation
 * - Biome classification
 */

#include "Noise.h"
#include "FBM.h"
#include "HeightmapGenerator.h"
#include "Biome.h"

/**
 * @namespace PCG
 * @brief Procedural Content Generation
 *
 * The PCG namespace contains all procedural generation functionality
 * for terrain, textures, and other game content.
 *
 * ## Quick Start
 *
 * ### Generate Basic Noise
 * ```cpp
 * PCG::PerlinNoise noise(12345);  // Seed for reproducibility
 * float value = noise.Sample(x, y);  // Returns [-1, 1]
 * ```
 *
 * ### Generate Terrain Heightmap
 * ```cpp
 * PCG::HeightmapSettings settings = PCG::HeightmapSettings::Island(256);
 * settings.Seed = 12345;
 *
 * PCG::HeightmapGenerator generator;
 * std::vector<float> heightmap = generator.Generate(settings);
 *
 * // Apply erosion for realism
 * PCG::ErosionSettings erosion;
 * erosion.Iterations = 50000;
 * generator.ApplyErosion(heightmap, settings.Width, settings.Height, erosion);
 * ```
 *
 * ### Classify Terrain into Biomes
 * ```cpp
 * PCG::BiomeMap biomeMap;
 * PCG::BiomeGenerator biomeGen;
 * PCG::BiomeGenerator::Settings biomeSettings;
 * biomeSettings.Width = settings.Width;
 * biomeSettings.Height = settings.Height;
 *
 * std::vector<PCG::BiomeType> biomes = biomeGen.Generate(heightmap, biomeSettings, biomeMap);
 * ```
 *
 * ## Noise Types
 *
 * | Type | Use Case | Output Range |
 * |------|----------|--------------|
 * | Perlin | Smooth terrain, clouds | [-1, 1] |
 * | Simplex | Similar to Perlin, faster in 3D | [-1, 1] |
 * | Worley | Cellular patterns, stone textures | [0, 1] |
 * | Value | Simple, fast noise | [0, 1] |
 *
 * ## FBM Variations
 *
 * | Type | Description |
 * |------|-------------|
 * | Standard | Sum of octaves |
 * | Ridged | Sharp ridge features |
 * | Turbulence | Swirling patterns |
 * | Billow | Soft, cloud-like shapes |
 */

namespace PCG {

    /**
     * @brief PCG system version
     */
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;

    /**
     * @brief Utility function to remap a value from one range to another
     * @param value Input value
     * @param inMin Input range minimum
     * @param inMax Input range maximum
     * @param outMin Output range minimum
     * @param outMax Output range maximum
     * @return Remapped value
     */
    inline float Remap(float value, float inMin, float inMax, float outMin, float outMax) {
        return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
    }

    /**
     * @brief Smoothstep interpolation
     */
    inline float Smoothstep(float edge0, float edge1, float x) {
        x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return x * x * (3.0f - 2.0f * x);
    }

    /**
     * @brief Smoother step interpolation (Ken Perlin's improved version)
     */
    inline float Smootherstep(float edge0, float edge1, float x) {
        x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
    }

} // namespace PCG
