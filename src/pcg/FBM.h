#pragma once

/**
 * @file FBM.h
 * @brief Fractal Brownian Motion (fBm) for layered noise generation
 *
 * FBM combines multiple octaves of noise at different frequencies and amplitudes
 * to create more natural-looking, detailed results.
 *
 * Includes variations:
 * - Standard FBM: Simple octave summation
 * - Ridged FBM: Creates ridge-like features (good for mountains)
 * - Turbulence: Uses absolute values for swirling patterns
 * - Billow: Smooth, cloud-like formations
 */

#include "Noise.h"
#include <memory>

namespace PCG {

    /**
     * @brief Settings for FBM generation
     */
    struct FBMSettings {
        int Octaves = 6;              ///< Number of noise layers (more = more detail, slower)
        float Frequency = 1.0f;       ///< Starting frequency (lower = larger features)
        float Amplitude = 1.0f;       ///< Starting amplitude (overall scale)
        float Lacunarity = 2.0f;      ///< Frequency multiplier per octave (typical: 2.0)
        float Persistence = 0.5f;     ///< Amplitude multiplier per octave (typical: 0.5)

        // Advanced settings
        float Gain = 0.5f;            ///< Used in ridged noise for sharpness
        float Offset = 1.0f;          ///< Used in ridged noise for ridge height

        /**
         * @brief Create default settings optimized for terrain
         */
        static FBMSettings Terrain() {
            FBMSettings s;
            s.Octaves = 8;
            s.Frequency = 0.005f;
            s.Lacunarity = 2.0f;
            s.Persistence = 0.5f;
            return s;
        }

        /**
         * @brief Create settings for cloud-like patterns
         */
        static FBMSettings Clouds() {
            FBMSettings s;
            s.Octaves = 5;
            s.Frequency = 0.01f;
            s.Lacunarity = 2.5f;
            s.Persistence = 0.4f;
            return s;
        }

        /**
         * @brief Create settings for sharp mountain ridges
         */
        static FBMSettings Mountains() {
            FBMSettings s;
            s.Octaves = 6;
            s.Frequency = 0.003f;
            s.Lacunarity = 2.2f;
            s.Persistence = 0.6f;
            s.Gain = 0.5f;
            s.Offset = 1.0f;
            return s;
        }
    };

    /**
     * @brief Fractal Brownian Motion generator
     *
     * Combines multiple octaves of a base noise function to create
     * more complex, natural-looking patterns.
     */
    class FBM {
    public:
        /**
         * @brief Construct FBM with an existing noise generator
         * @param baseNoise Pointer to noise generator (FBM does not take ownership)
         */
        explicit FBM(INoise* baseNoise);

        /**
         * @brief Construct FBM with an owned noise generator
         * @param baseNoise Unique pointer to noise generator (FBM takes ownership)
         */
        explicit FBM(std::unique_ptr<INoise> baseNoise);

        /**
         * @brief Set a new base noise generator (non-owning)
         */
        void SetBaseNoise(INoise* noise);

        /**
         * @brief Set a new base noise generator (owning)
         */
        void SetBaseNoise(std::unique_ptr<INoise> noise);

        /**
         * @brief Get the base noise generator
         */
        INoise* GetBaseNoise() const { return m_BaseNoise; }

        // ====================================================================
        // Standard FBM Sampling
        // ====================================================================

        /**
         * @brief Sample 2D FBM noise
         * @param x X coordinate
         * @param y Y coordinate
         * @param settings FBM parameters
         * @return Noise value, approximately in range [-1, 1]
         */
        float Sample(float x, float y, const FBMSettings& settings) const;

        /**
         * @brief Sample 3D FBM noise
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         * @param settings FBM parameters
         * @return Noise value, approximately in range [-1, 1]
         */
        float Sample(float x, float y, float z, const FBMSettings& settings) const;

        // ====================================================================
        // Ridged FBM (Mountain-like features)
        // ====================================================================

        /**
         * @brief Sample 2D ridged FBM
         *
         * Creates sharp ridge-like features by inverting and squaring noise values.
         * Great for mountain terrain.
         *
         * @param x X coordinate
         * @param y Y coordinate
         * @param settings FBM parameters (Gain and Offset are used)
         * @return Noise value in range [0, 1]
         */
        float Ridged(float x, float y, const FBMSettings& settings) const;

        /**
         * @brief Sample 3D ridged FBM
         */
        float Ridged(float x, float y, float z, const FBMSettings& settings) const;

        // ====================================================================
        // Turbulence (Swirling patterns)
        // ====================================================================

        /**
         * @brief Sample 2D turbulence
         *
         * Uses absolute value of noise, creating swirling, fire-like patterns.
         *
         * @param x X coordinate
         * @param y Y coordinate
         * @param settings FBM parameters
         * @return Noise value in range [0, 1]
         */
        float Turbulence(float x, float y, const FBMSettings& settings) const;

        /**
         * @brief Sample 3D turbulence
         */
        float Turbulence(float x, float y, float z, const FBMSettings& settings) const;

        // ====================================================================
        // Billow (Cloud-like features)
        // ====================================================================

        /**
         * @brief Sample 2D billow noise
         *
         * Similar to turbulence but with smoothing, creates billowy cloud shapes.
         *
         * @param x X coordinate
         * @param y Y coordinate
         * @param settings FBM parameters
         * @return Noise value in range [0, 1]
         */
        float Billow(float x, float y, const FBMSettings& settings) const;

        /**
         * @brief Sample 3D billow noise
         */
        float Billow(float x, float y, float z, const FBMSettings& settings) const;

        // ====================================================================
        // Domain Warping
        // ====================================================================

        /**
         * @brief Sample 2D FBM with domain warping
         *
         * Distorts the input coordinates using additional noise layers,
         * creating more organic, swirly patterns.
         *
         * @param x X coordinate
         * @param y Y coordinate
         * @param settings FBM parameters
         * @param warpStrength How much to distort (0 = no warp, 1 = normal warp)
         * @return Noise value, approximately in range [-1, 1]
         */
        float WarpedSample(float x, float y, const FBMSettings& settings,
                          float warpStrength = 0.5f) const;

        /**
         * @brief Sample 2D FBM with multiple domain warping iterations
         *
         * Applies domain warping recursively for more complex distortion.
         *
         * @param x X coordinate
         * @param y Y coordinate
         * @param settings FBM parameters
         * @param warpIterations Number of warp passes (1-3 typical)
         * @param warpStrength How much to distort per iteration
         * @return Noise value, approximately in range [-1, 1]
         */
        float MultiWarpedSample(float x, float y, const FBMSettings& settings,
                                int warpIterations = 2, float warpStrength = 0.5f) const;

    private:
        INoise* m_BaseNoise = nullptr;
        std::unique_ptr<INoise> m_OwnedNoise;

        /**
         * @brief Calculate weight normalization factor
         * @param octaves Number of octaves
         * @param persistence Amplitude reduction per octave
         * @return Normalization factor to keep output roughly in [-1, 1]
         */
        float CalculateNormalization(int octaves, float persistence) const;
    };

    /**
     * @brief Pre-configured FBM generators for common use cases
     */
    class FBMPresets {
    public:
        /**
         * @brief Create FBM optimized for terrain height generation
         * @param seed Random seed
         */
        static std::unique_ptr<FBM> CreateTerrainFBM(uint32_t seed = 0);

        /**
         * @brief Create FBM optimized for cloud/fog generation
         * @param seed Random seed
         */
        static std::unique_ptr<FBM> CreateCloudFBM(uint32_t seed = 0);

        /**
         * @brief Create FBM optimized for detail/texture overlay
         * @param seed Random seed
         */
        static std::unique_ptr<FBM> CreateDetailFBM(uint32_t seed = 0);
    };

} // namespace PCG
