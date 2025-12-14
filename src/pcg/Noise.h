#pragma once

/**
 * @file Noise.h
 * @brief Procedural noise generation algorithms for terrain and texture generation
 *
 * Provides multiple noise algorithms:
 * - Perlin Noise: Smooth, gradient-based noise
 * - Simplex Noise: Improved Perlin with better performance in higher dimensions
 * - Worley Noise: Cellular/Voronoi-based noise for organic patterns
 *
 * All noise functions return values in range [-1, 1] or [0, 1] depending on the algorithm
 */

#include <array>
#include <cstdint>
#include <memory>
#include <random>

namespace PCG {

    /**
     * @brief Base interface for all noise generators
     */
    class INoise {
    public:
        virtual ~INoise() = default;

        /**
         * @brief Sample 2D noise at the given coordinates
         * @param x X coordinate
         * @param y Y coordinate
         * @return Noise value, typically in range [-1, 1]
         */
        virtual float Sample(float x, float y) const = 0;

        /**
         * @brief Sample 3D noise at the given coordinates
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         * @return Noise value, typically in range [-1, 1]
         */
        virtual float Sample(float x, float y, float z) const = 0;

        /**
         * @brief Get the current seed
         * @return The seed used for noise generation
         */
        virtual uint32_t GetSeed() const = 0;
    };

    /**
     * @brief Classic Perlin noise implementation
     *
     * Produces smooth, continuous noise suitable for terrain generation.
     * Output range: [-1, 1] (approximately, can slightly exceed)
     */
    class PerlinNoise : public INoise {
    public:
        /**
         * @brief Construct Perlin noise generator with optional seed
         * @param seed Random seed for reproducible results (0 = random)
         */
        explicit PerlinNoise(uint32_t seed = 0);

        /**
         * @brief Re-initialize with a new seed
         * @param seed New seed value
         */
        void SetSeed(uint32_t seed);

        float Sample(float x, float y) const override;
        float Sample(float x, float y, float z) const override;
        uint32_t GetSeed() const override { return m_Seed; }

    private:
        uint32_t m_Seed;
        std::array<int, 512> m_Permutation;

        /**
         * @brief Smoothstep function for interpolation
         * @param t Value in range [0, 1]
         * @return Smoothed value
         */
        float Fade(float t) const;

        /**
         * @brief Linear interpolation
         */
        float Lerp(float a, float b, float t) const;

        /**
         * @brief Compute gradient dot product
         * @param hash Hash value to select gradient
         * @param x, y, z Offset from grid point
         * @return Dot product with selected gradient
         */
        float Grad(int hash, float x, float y, float z) const;

        /**
         * @brief Initialize the permutation table
         */
        void InitializePermutation();
    };

    /**
     * @brief Simplex noise implementation
     *
     * Improved noise algorithm with:
     * - Lower computational complexity than Perlin
     * - Better visual isotropy
     * - No noticeable directional artifacts
     *
     * Output range: approximately [-1, 1]
     */
    class SimplexNoise : public INoise {
    public:
        /**
         * @brief Construct Simplex noise generator with optional seed
         * @param seed Random seed for reproducible results (0 = random)
         */
        explicit SimplexNoise(uint32_t seed = 0);

        /**
         * @brief Re-initialize with a new seed
         * @param seed New seed value
         */
        void SetSeed(uint32_t seed);

        float Sample(float x, float y) const override;
        float Sample(float x, float y, float z) const override;
        uint32_t GetSeed() const override { return m_Seed; }

    private:
        uint32_t m_Seed;
        std::array<int, 512> m_Permutation;
        std::array<int, 512> m_PermMod12;

        // Gradient vectors for 2D
        static constexpr float s_Grad2[12][2] = {
            {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
            {1, 0}, {-1, 0}, {1, 0}, {-1, 0},
            {0, 1}, {0, -1}, {0, 1}, {0, -1}
        };

        // Gradient vectors for 3D (normalized to edges of a cube)
        static constexpr float s_Grad3[12][3] = {
            {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
            {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
            {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
        };

        // Skewing factors for 2D
        static constexpr float F2 = 0.3660254037844386f;  // (sqrt(3) - 1) / 2
        static constexpr float G2 = 0.21132486540518713f; // (3 - sqrt(3)) / 6

        // Skewing factors for 3D
        static constexpr float F3 = 1.0f / 3.0f;
        static constexpr float G3 = 1.0f / 6.0f;

        void InitializePermutation();
        float Dot2(const float* g, float x, float y) const;
        float Dot3(const float* g, float x, float y, float z) const;
        int FastFloor(float x) const;
    };

    /**
     * @brief Worley (Cellular/Voronoi) noise implementation
     *
     * Creates cellular patterns based on distance to randomly distributed points.
     * Useful for: organic textures, stone patterns, cellular structures
     *
     * Output range: [0, 1]
     */
    class WorleyNoise : public INoise {
    public:
        /**
         * @brief Distance calculation method
         */
        enum class DistanceFunction {
            Euclidean,   ///< Standard distance (circular cells)
            Manhattan,   ///< Taxicab distance (diamond cells)
            Chebyshev    ///< Chessboard distance (square cells)
        };

        /**
         * @brief Return value type
         */
        enum class ReturnType {
            F1,          ///< Distance to closest point
            F2,          ///< Distance to second closest point
            F2MinusF1,   ///< F2 - F1 (cell boundaries)
            F1PlusF2     ///< F1 + F2
        };

        /**
         * @brief Construct Worley noise generator
         * @param seed Random seed for reproducible results (0 = random)
         */
        explicit WorleyNoise(uint32_t seed = 0);

        /**
         * @brief Re-initialize with a new seed
         * @param seed New seed value
         */
        void SetSeed(uint32_t seed);

        /**
         * @brief Set the distance function to use
         * @param func Distance calculation method
         */
        void SetDistanceFunction(DistanceFunction func) { m_DistanceFunc = func; }

        /**
         * @brief Set the return type
         * @param type What distance value to return
         */
        void SetReturnType(ReturnType type) { m_ReturnType = type; }

        float Sample(float x, float y) const override;
        float Sample(float x, float y, float z) const override;
        uint32_t GetSeed() const override { return m_Seed; }

    private:
        uint32_t m_Seed;
        DistanceFunction m_DistanceFunc = DistanceFunction::Euclidean;
        ReturnType m_ReturnType = ReturnType::F1;

        /**
         * @brief Hash function for cell coordinates
         */
        uint32_t Hash(int32_t x, int32_t y) const;
        uint32_t Hash(int32_t x, int32_t y, int32_t z) const;

        /**
         * @brief Calculate distance based on current function
         */
        float Distance(float dx, float dy) const;
        float Distance(float dx, float dy, float dz) const;

        /**
         * @brief Generate random point position within a cell
         */
        void GetCellPoint(int32_t cellX, int32_t cellY, float& px, float& py) const;
        void GetCellPoint(int32_t cellX, int32_t cellY, int32_t cellZ,
                          float& px, float& py, float& pz) const;
    };

    /**
     * @brief Value noise implementation (simple integer-based noise)
     *
     * Simpler than Perlin, uses random values at grid points with interpolation.
     * Less smooth but faster to compute.
     *
     * Output range: [0, 1]
     */
    class ValueNoise : public INoise {
    public:
        explicit ValueNoise(uint32_t seed = 0);
        void SetSeed(uint32_t seed);

        float Sample(float x, float y) const override;
        float Sample(float x, float y, float z) const override;
        uint32_t GetSeed() const override { return m_Seed; }

    private:
        uint32_t m_Seed;
        std::array<float, 512> m_RandomValues;

        void InitializeRandomValues();
        int Hash(int x, int y) const;
        int Hash(int x, int y, int z) const;
        float SmoothStep(float t) const;
    };

    /**
     * @brief Factory for creating noise generators
     */
    class NoiseFactory {
    public:
        enum class NoiseType {
            Perlin,
            Simplex,
            Worley,
            Value
        };

        /**
         * @brief Create a noise generator of the specified type
         * @param type Type of noise to create
         * @param seed Random seed
         * @return Unique pointer to the noise generator
         */
        static std::unique_ptr<INoise> Create(NoiseType type, uint32_t seed = 0);
    };

} // namespace PCG
