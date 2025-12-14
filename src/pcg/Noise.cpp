#include "Noise.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace PCG {

    // ============================================================================
    // Perlin Noise Implementation
    // ============================================================================

    PerlinNoise::PerlinNoise(uint32_t seed)
        : m_Seed(seed)
    {
        InitializePermutation();
    }

    void PerlinNoise::SetSeed(uint32_t seed)
    {
        m_Seed = seed;
        InitializePermutation();
    }

    void PerlinNoise::InitializePermutation()
    {
        // Fill first half with values 0-255
        std::array<int, 256> p;
        std::iota(p.begin(), p.end(), 0);

        // Shuffle using seed
        std::mt19937 rng(m_Seed == 0 ? std::random_device{}() : m_Seed);
        std::shuffle(p.begin(), p.end(), rng);

        // Duplicate the permutation array
        for (int i = 0; i < 256; ++i) {
            m_Permutation[i] = p[i];
            m_Permutation[256 + i] = p[i];
        }
    }

    float PerlinNoise::Fade(float t) const
    {
        // 6t^5 - 15t^4 + 10t^3 (improved Perlin fade function)
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float PerlinNoise::Lerp(float a, float b, float t) const
    {
        return a + t * (b - a);
    }

    float PerlinNoise::Grad(int hash, float x, float y, float z) const
    {
        // Convert low 4 bits of hash to one of 12 gradient directions
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    float PerlinNoise::Sample(float x, float y) const
    {
        return Sample(x, y, 0.0f);
    }

    float PerlinNoise::Sample(float x, float y, float z) const
    {
        // Find unit cube containing point
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        // Relative x, y, z of point within cube
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        // Compute fade curves
        float u = Fade(x);
        float v = Fade(y);
        float w = Fade(z);

        // Hash coordinates of cube corners
        int A = m_Permutation[X] + Y;
        int AA = m_Permutation[A] + Z;
        int AB = m_Permutation[A + 1] + Z;
        int B = m_Permutation[X + 1] + Y;
        int BA = m_Permutation[B] + Z;
        int BB = m_Permutation[B + 1] + Z;

        // Blend results from 8 corners of cube
        float res = Lerp(
            Lerp(
                Lerp(Grad(m_Permutation[AA], x, y, z),
                     Grad(m_Permutation[BA], x - 1, y, z), u),
                Lerp(Grad(m_Permutation[AB], x, y - 1, z),
                     Grad(m_Permutation[BB], x - 1, y - 1, z), u),
                v),
            Lerp(
                Lerp(Grad(m_Permutation[AA + 1], x, y, z - 1),
                     Grad(m_Permutation[BA + 1], x - 1, y, z - 1), u),
                Lerp(Grad(m_Permutation[AB + 1], x, y - 1, z - 1),
                     Grad(m_Permutation[BB + 1], x - 1, y - 1, z - 1), u),
                v),
            w);

        return res;
    }

    // ============================================================================
    // Simplex Noise Implementation
    // ============================================================================

    constexpr float SimplexNoise::s_Grad2[12][2];
    constexpr float SimplexNoise::s_Grad3[12][3];

    SimplexNoise::SimplexNoise(uint32_t seed)
        : m_Seed(seed)
    {
        InitializePermutation();
    }

    void SimplexNoise::SetSeed(uint32_t seed)
    {
        m_Seed = seed;
        InitializePermutation();
    }

    void SimplexNoise::InitializePermutation()
    {
        std::array<int, 256> p;
        std::iota(p.begin(), p.end(), 0);

        std::mt19937 rng(m_Seed == 0 ? std::random_device{}() : m_Seed);
        std::shuffle(p.begin(), p.end(), rng);

        for (int i = 0; i < 256; ++i) {
            m_Permutation[i] = p[i];
            m_Permutation[256 + i] = p[i];
            m_PermMod12[i] = m_Permutation[i] % 12;
            m_PermMod12[256 + i] = m_PermMod12[i];
        }
    }

    int SimplexNoise::FastFloor(float x) const
    {
        int xi = static_cast<int>(x);
        return x < xi ? xi - 1 : xi;
    }

    float SimplexNoise::Dot2(const float* g, float x, float y) const
    {
        return g[0] * x + g[1] * y;
    }

    float SimplexNoise::Dot3(const float* g, float x, float y, float z) const
    {
        return g[0] * x + g[1] * y + g[2] * z;
    }

    float SimplexNoise::Sample(float x, float y) const
    {
        float n0, n1, n2; // Noise contributions from three corners

        // Skew input space to determine which simplex cell we're in
        float s = (x + y) * F2;
        int i = FastFloor(x + s);
        int j = FastFloor(y + s);

        float t = (i + j) * G2;
        float X0 = i - t; // Unskew the cell origin back to (x,y) space
        float Y0 = j - t;
        float x0 = x - X0; // The x,y distances from the cell origin
        float y0 = y - Y0;

        // Determine which simplex we're in
        int i1, j1; // Offsets for second corner of simplex in (i,j) coords
        if (x0 > y0) {
            i1 = 1; j1 = 0; // Lower triangle, XY order: (0,0)->(1,0)->(1,1)
        } else {
            i1 = 0; j1 = 1; // Upper triangle, YX order: (0,0)->(0,1)->(1,1)
        }

        // Offsets for middle and last corner in (x,y) unskewed coords
        float x1 = x0 - i1 + G2;
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2;
        float y2 = y0 - 1.0f + 2.0f * G2;

        // Work out hashed gradient indices of three simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int gi0 = m_PermMod12[ii + m_Permutation[jj]];
        int gi1 = m_PermMod12[ii + i1 + m_Permutation[jj + j1]];
        int gi2 = m_PermMod12[ii + 1 + m_Permutation[jj + 1]];

        // Calculate contributions from three corners
        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 < 0) {
            n0 = 0.0f;
        } else {
            t0 *= t0;
            n0 = t0 * t0 * Dot2(s_Grad2[gi0], x0, y0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 < 0) {
            n1 = 0.0f;
        } else {
            t1 *= t1;
            n1 = t1 * t1 * Dot2(s_Grad2[gi1], x1, y1);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 < 0) {
            n2 = 0.0f;
        } else {
            t2 *= t2;
            n2 = t2 * t2 * Dot2(s_Grad2[gi2], x2, y2);
        }

        // Scale to [-1, 1]
        return 70.0f * (n0 + n1 + n2);
    }

    float SimplexNoise::Sample(float x, float y, float z) const
    {
        float n0, n1, n2, n3; // Noise contributions from four corners

        // Skew input space
        float s = (x + y + z) * F3;
        int i = FastFloor(x + s);
        int j = FastFloor(y + s);
        int k = FastFloor(z + s);

        float t = (i + j + k) * G3;
        float X0 = i - t;
        float Y0 = j - t;
        float Z0 = k - t;
        float x0 = x - X0;
        float y0 = y - Y0;
        float z0 = z - Z0;

        // Determine simplex
        int i1, j1, k1;
        int i2, j2, k2;

        if (x0 >= y0) {
            if (y0 >= z0) {
                i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
            } else if (x0 >= z0) {
                i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
            } else {
                i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
            }
        } else {
            if (y0 < z0) {
                i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
            } else if (x0 < z0) {
                i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
            } else {
                i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
            }
        }

        float x1 = x0 - i1 + G3;
        float y1 = y0 - j1 + G3;
        float z1 = z0 - k1 + G3;
        float x2 = x0 - i2 + 2.0f * G3;
        float y2 = y0 - j2 + 2.0f * G3;
        float z2 = z0 - k2 + 2.0f * G3;
        float x3 = x0 - 1.0f + 3.0f * G3;
        float y3 = y0 - 1.0f + 3.0f * G3;
        float z3 = z0 - 1.0f + 3.0f * G3;

        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int gi0 = m_PermMod12[ii + m_Permutation[jj + m_Permutation[kk]]];
        int gi1 = m_PermMod12[ii + i1 + m_Permutation[jj + j1 + m_Permutation[kk + k1]]];
        int gi2 = m_PermMod12[ii + i2 + m_Permutation[jj + j2 + m_Permutation[kk + k2]]];
        int gi3 = m_PermMod12[ii + 1 + m_Permutation[jj + 1 + m_Permutation[kk + 1]]];

        float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
        if (t0 < 0) {
            n0 = 0.0f;
        } else {
            t0 *= t0;
            n0 = t0 * t0 * Dot3(s_Grad3[gi0], x0, y0, z0);
        }

        float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
        if (t1 < 0) {
            n1 = 0.0f;
        } else {
            t1 *= t1;
            n1 = t1 * t1 * Dot3(s_Grad3[gi1], x1, y1, z1);
        }

        float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
        if (t2 < 0) {
            n2 = 0.0f;
        } else {
            t2 *= t2;
            n2 = t2 * t2 * Dot3(s_Grad3[gi2], x2, y2, z2);
        }

        float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
        if (t3 < 0) {
            n3 = 0.0f;
        } else {
            t3 *= t3;
            n3 = t3 * t3 * Dot3(s_Grad3[gi3], x3, y3, z3);
        }

        // Scale to [-1, 1]
        return 32.0f * (n0 + n1 + n2 + n3);
    }

    // ============================================================================
    // Worley Noise Implementation
    // ============================================================================

    WorleyNoise::WorleyNoise(uint32_t seed)
        : m_Seed(seed == 0 ? std::random_device{}() : seed)
    {
    }

    void WorleyNoise::SetSeed(uint32_t seed)
    {
        m_Seed = seed == 0 ? std::random_device{}() : seed;
    }

    uint32_t WorleyNoise::Hash(int32_t x, int32_t y) const
    {
        uint32_t h = m_Seed;
        h ^= static_cast<uint32_t>(x) * 374761393u;
        h ^= static_cast<uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return h;
    }

    uint32_t WorleyNoise::Hash(int32_t x, int32_t y, int32_t z) const
    {
        uint32_t h = m_Seed;
        h ^= static_cast<uint32_t>(x) * 374761393u;
        h ^= static_cast<uint32_t>(y) * 668265263u;
        h ^= static_cast<uint32_t>(z) * 1610612741u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return h;
    }

    float WorleyNoise::Distance(float dx, float dy) const
    {
        switch (m_DistanceFunc) {
            case DistanceFunction::Euclidean:
                return std::sqrt(dx * dx + dy * dy);
            case DistanceFunction::Manhattan:
                return std::abs(dx) + std::abs(dy);
            case DistanceFunction::Chebyshev:
                return std::max(std::abs(dx), std::abs(dy));
            default:
                return std::sqrt(dx * dx + dy * dy);
        }
    }

    float WorleyNoise::Distance(float dx, float dy, float dz) const
    {
        switch (m_DistanceFunc) {
            case DistanceFunction::Euclidean:
                return std::sqrt(dx * dx + dy * dy + dz * dz);
            case DistanceFunction::Manhattan:
                return std::abs(dx) + std::abs(dy) + std::abs(dz);
            case DistanceFunction::Chebyshev:
                return std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
            default:
                return std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    }

    void WorleyNoise::GetCellPoint(int32_t cellX, int32_t cellY, float& px, float& py) const
    {
        uint32_t h = Hash(cellX, cellY);
        px = static_cast<float>(cellX) + static_cast<float>(h & 0xFFFF) / 65535.0f;
        py = static_cast<float>(cellY) + static_cast<float>((h >> 16) & 0xFFFF) / 65535.0f;
    }

    void WorleyNoise::GetCellPoint(int32_t cellX, int32_t cellY, int32_t cellZ,
                                    float& px, float& py, float& pz) const
    {
        uint32_t h = Hash(cellX, cellY, cellZ);
        px = static_cast<float>(cellX) + static_cast<float>(h & 0x3FF) / 1023.0f;
        py = static_cast<float>(cellY) + static_cast<float>((h >> 10) & 0x3FF) / 1023.0f;
        pz = static_cast<float>(cellZ) + static_cast<float>((h >> 20) & 0x3FF) / 1023.0f;
    }

    float WorleyNoise::Sample(float x, float y) const
    {
        int32_t cellX = static_cast<int32_t>(std::floor(x));
        int32_t cellY = static_cast<int32_t>(std::floor(y));

        float minDist1 = 99999.0f;
        float minDist2 = 99999.0f;

        // Check 3x3 neighborhood
        for (int32_t dy = -1; dy <= 1; ++dy) {
            for (int32_t dx = -1; dx <= 1; ++dx) {
                float px, py;
                GetCellPoint(cellX + dx, cellY + dy, px, py);

                float dist = Distance(x - px, y - py);

                if (dist < minDist1) {
                    minDist2 = minDist1;
                    minDist1 = dist;
                } else if (dist < minDist2) {
                    minDist2 = dist;
                }
            }
        }

        float result;
        switch (m_ReturnType) {
            case ReturnType::F1:
                result = minDist1;
                break;
            case ReturnType::F2:
                result = minDist2;
                break;
            case ReturnType::F2MinusF1:
                result = minDist2 - minDist1;
                break;
            case ReturnType::F1PlusF2:
                result = (minDist1 + minDist2) * 0.5f;
                break;
            default:
                result = minDist1;
        }

        // Normalize to [0, 1] approximately
        return std::min(1.0f, result);
    }

    float WorleyNoise::Sample(float x, float y, float z) const
    {
        int32_t cellX = static_cast<int32_t>(std::floor(x));
        int32_t cellY = static_cast<int32_t>(std::floor(y));
        int32_t cellZ = static_cast<int32_t>(std::floor(z));

        float minDist1 = 99999.0f;
        float minDist2 = 99999.0f;

        // Check 3x3x3 neighborhood
        for (int32_t dz = -1; dz <= 1; ++dz) {
            for (int32_t dy = -1; dy <= 1; ++dy) {
                for (int32_t dx = -1; dx <= 1; ++dx) {
                    float px, py, pz;
                    GetCellPoint(cellX + dx, cellY + dy, cellZ + dz, px, py, pz);

                    float dist = Distance(x - px, y - py, z - pz);

                    if (dist < minDist1) {
                        minDist2 = minDist1;
                        minDist1 = dist;
                    } else if (dist < minDist2) {
                        minDist2 = dist;
                    }
                }
            }
        }

        float result;
        switch (m_ReturnType) {
            case ReturnType::F1:
                result = minDist1;
                break;
            case ReturnType::F2:
                result = minDist2;
                break;
            case ReturnType::F2MinusF1:
                result = minDist2 - minDist1;
                break;
            case ReturnType::F1PlusF2:
                result = (minDist1 + minDist2) * 0.5f;
                break;
            default:
                result = minDist1;
        }

        return std::min(1.0f, result);
    }

    // ============================================================================
    // Value Noise Implementation
    // ============================================================================

    ValueNoise::ValueNoise(uint32_t seed)
        : m_Seed(seed)
    {
        InitializeRandomValues();
    }

    void ValueNoise::SetSeed(uint32_t seed)
    {
        m_Seed = seed;
        InitializeRandomValues();
    }

    void ValueNoise::InitializeRandomValues()
    {
        std::mt19937 rng(m_Seed == 0 ? std::random_device{}() : m_Seed);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int i = 0; i < 256; ++i) {
            m_RandomValues[i] = dist(rng);
            m_RandomValues[256 + i] = m_RandomValues[i];
        }
    }

    int ValueNoise::Hash(int x, int y) const
    {
        int hash = x * 374761393 + y * 668265263;
        hash = (hash ^ (hash >> 13)) * 1274126177;
        return hash & 255;
    }

    int ValueNoise::Hash(int x, int y, int z) const
    {
        int hash = x * 374761393 + y * 668265263 + z * 1610612741;
        hash = (hash ^ (hash >> 13)) * 1274126177;
        return hash & 255;
    }

    float ValueNoise::SmoothStep(float t) const
    {
        return t * t * (3.0f - 2.0f * t);
    }

    float ValueNoise::Sample(float x, float y) const
    {
        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));

        float xf = x - xi;
        float yf = y - yi;

        float u = SmoothStep(xf);
        float v = SmoothStep(yf);

        float n00 = m_RandomValues[Hash(xi, yi)];
        float n10 = m_RandomValues[Hash(xi + 1, yi)];
        float n01 = m_RandomValues[Hash(xi, yi + 1)];
        float n11 = m_RandomValues[Hash(xi + 1, yi + 1)];

        float nx0 = n00 + u * (n10 - n00);
        float nx1 = n01 + u * (n11 - n01);

        return nx0 + v * (nx1 - nx0);
    }

    float ValueNoise::Sample(float x, float y, float z) const
    {
        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));
        int zi = static_cast<int>(std::floor(z));

        float xf = x - xi;
        float yf = y - yi;
        float zf = z - zi;

        float u = SmoothStep(xf);
        float v = SmoothStep(yf);
        float w = SmoothStep(zf);

        float n000 = m_RandomValues[Hash(xi, yi, zi)];
        float n100 = m_RandomValues[Hash(xi + 1, yi, zi)];
        float n010 = m_RandomValues[Hash(xi, yi + 1, zi)];
        float n110 = m_RandomValues[Hash(xi + 1, yi + 1, zi)];
        float n001 = m_RandomValues[Hash(xi, yi, zi + 1)];
        float n101 = m_RandomValues[Hash(xi + 1, yi, zi + 1)];
        float n011 = m_RandomValues[Hash(xi, yi + 1, zi + 1)];
        float n111 = m_RandomValues[Hash(xi + 1, yi + 1, zi + 1)];

        float nx00 = n000 + u * (n100 - n000);
        float nx10 = n010 + u * (n110 - n010);
        float nx01 = n001 + u * (n101 - n001);
        float nx11 = n011 + u * (n111 - n011);

        float nxy0 = nx00 + v * (nx10 - nx00);
        float nxy1 = nx01 + v * (nx11 - nx01);

        return nxy0 + w * (nxy1 - nxy0);
    }

    // ============================================================================
    // Noise Factory
    // ============================================================================

    std::unique_ptr<INoise> NoiseFactory::Create(NoiseType type, uint32_t seed)
    {
        switch (type) {
            case NoiseType::Perlin:
                return std::make_unique<PerlinNoise>(seed);
            case NoiseType::Simplex:
                return std::make_unique<SimplexNoise>(seed);
            case NoiseType::Worley:
                return std::make_unique<WorleyNoise>(seed);
            case NoiseType::Value:
                return std::make_unique<ValueNoise>(seed);
            default:
                return std::make_unique<PerlinNoise>(seed);
        }
    }

} // namespace PCG
