#include "FBM.h"
#include <cmath>
#include <algorithm>

namespace PCG {

    // ============================================================================
    // Constructor / Setup
    // ============================================================================

    FBM::FBM(INoise* baseNoise)
        : m_BaseNoise(baseNoise)
    {
    }

    FBM::FBM(std::unique_ptr<INoise> baseNoise)
        : m_OwnedNoise(std::move(baseNoise))
        , m_BaseNoise(m_OwnedNoise.get())
    {
    }

    void FBM::SetBaseNoise(INoise* noise)
    {
        m_OwnedNoise.reset();
        m_BaseNoise = noise;
    }

    void FBM::SetBaseNoise(std::unique_ptr<INoise> noise)
    {
        m_OwnedNoise = std::move(noise);
        m_BaseNoise = m_OwnedNoise.get();
    }

    float FBM::CalculateNormalization(int octaves, float persistence) const
    {
        float maxValue = 0.0f;
        float amplitude = 1.0f;

        for (int i = 0; i < octaves; ++i) {
            maxValue += amplitude;
            amplitude *= persistence;
        }

        return maxValue;
    }

    // ============================================================================
    // Standard FBM
    // ============================================================================

    float FBM::Sample(float x, float y, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float normalization = CalculateNormalization(settings.Octaves, settings.Persistence);

        for (int i = 0; i < settings.Octaves; ++i) {
            sum += m_BaseNoise->Sample(x * frequency, y * frequency) * amplitude;
            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        return sum / normalization;
    }

    float FBM::Sample(float x, float y, float z, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float normalization = CalculateNormalization(settings.Octaves, settings.Persistence);

        for (int i = 0; i < settings.Octaves; ++i) {
            sum += m_BaseNoise->Sample(x * frequency, y * frequency, z * frequency) * amplitude;
            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        return sum / normalization;
    }

    // ============================================================================
    // Ridged FBM
    // ============================================================================

    float FBM::Ridged(float x, float y, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float weight = 1.0f;

        for (int i = 0; i < settings.Octaves; ++i) {
            // Get noise value and create ridge by inverting
            float noise = m_BaseNoise->Sample(x * frequency, y * frequency);
            noise = settings.Offset - std::abs(noise);  // Invert
            noise *= noise;  // Square for sharpness
            noise *= weight;  // Apply weight from previous octave

            sum += noise * amplitude;

            // Calculate weight for next octave
            weight = std::clamp(noise * settings.Gain, 0.0f, 1.0f);

            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        // Normalize to [0, 1]
        return std::clamp(sum * 1.25f, 0.0f, 1.0f);
    }

    float FBM::Ridged(float x, float y, float z, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float weight = 1.0f;

        for (int i = 0; i < settings.Octaves; ++i) {
            float noise = m_BaseNoise->Sample(x * frequency, y * frequency, z * frequency);
            noise = settings.Offset - std::abs(noise);
            noise *= noise;
            noise *= weight;

            sum += noise * amplitude;

            weight = std::clamp(noise * settings.Gain, 0.0f, 1.0f);

            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        return std::clamp(sum * 1.25f, 0.0f, 1.0f);
    }

    // ============================================================================
    // Turbulence
    // ============================================================================

    float FBM::Turbulence(float x, float y, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float normalization = CalculateNormalization(settings.Octaves, settings.Persistence);

        for (int i = 0; i < settings.Octaves; ++i) {
            // Use absolute value of noise
            sum += std::abs(m_BaseNoise->Sample(x * frequency, y * frequency)) * amplitude;
            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        return sum / normalization;
    }

    float FBM::Turbulence(float x, float y, float z, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float normalization = CalculateNormalization(settings.Octaves, settings.Persistence);

        for (int i = 0; i < settings.Octaves; ++i) {
            sum += std::abs(m_BaseNoise->Sample(x * frequency, y * frequency, z * frequency)) * amplitude;
            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        return sum / normalization;
    }

    // ============================================================================
    // Billow
    // ============================================================================

    float FBM::Billow(float x, float y, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float normalization = CalculateNormalization(settings.Octaves, settings.Persistence);

        for (int i = 0; i < settings.Octaves; ++i) {
            // Billow: abs(noise) * 2 - 1, creates puffy, cloud-like shapes
            float noise = m_BaseNoise->Sample(x * frequency, y * frequency);
            noise = std::abs(noise) * 2.0f - 1.0f;
            sum += noise * amplitude;
            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        // Remap to [0, 1]
        return (sum / normalization + 1.0f) * 0.5f;
    }

    float FBM::Billow(float x, float y, float z, const FBMSettings& settings) const
    {
        if (!m_BaseNoise) return 0.0f;

        float sum = 0.0f;
        float frequency = settings.Frequency;
        float amplitude = settings.Amplitude;
        float normalization = CalculateNormalization(settings.Octaves, settings.Persistence);

        for (int i = 0; i < settings.Octaves; ++i) {
            float noise = m_BaseNoise->Sample(x * frequency, y * frequency, z * frequency);
            noise = std::abs(noise) * 2.0f - 1.0f;
            sum += noise * amplitude;
            frequency *= settings.Lacunarity;
            amplitude *= settings.Persistence;
        }

        return (sum / normalization + 1.0f) * 0.5f;
    }

    // ============================================================================
    // Domain Warping
    // ============================================================================

    float FBM::WarpedSample(float x, float y, const FBMSettings& settings,
                           float warpStrength) const
    {
        if (!m_BaseNoise) return 0.0f;

        // Calculate warp offsets using different coordinate offsets
        float qx = Sample(x, y, settings);
        float qy = Sample(x + 5.2f, y + 1.3f, settings);

        // Apply warp
        float warpedX = x + warpStrength * qx * 4.0f;
        float warpedY = y + warpStrength * qy * 4.0f;

        return Sample(warpedX, warpedY, settings);
    }

    float FBM::MultiWarpedSample(float x, float y, const FBMSettings& settings,
                                 int warpIterations, float warpStrength) const
    {
        if (!m_BaseNoise) return 0.0f;

        float px = x;
        float py = y;

        for (int i = 0; i < warpIterations; ++i) {
            // Different offsets for each iteration to prevent patterns
            float offsetX = static_cast<float>(i) * 1.7f + 0.5f;
            float offsetY = static_cast<float>(i) * 2.3f + 1.1f;

            float qx = Sample(px + offsetX, py, settings);
            float qy = Sample(px, py + offsetY, settings);

            px = x + warpStrength * qx * 4.0f;
            py = y + warpStrength * qy * 4.0f;
        }

        return Sample(px, py, settings);
    }

    // ============================================================================
    // FBM Presets
    // ============================================================================

    std::unique_ptr<FBM> FBMPresets::CreateTerrainFBM(uint32_t seed)
    {
        auto noise = std::make_unique<PerlinNoise>(seed);
        return std::make_unique<FBM>(std::move(noise));
    }

    std::unique_ptr<FBM> FBMPresets::CreateCloudFBM(uint32_t seed)
    {
        auto noise = std::make_unique<SimplexNoise>(seed);
        return std::make_unique<FBM>(std::move(noise));
    }

    std::unique_ptr<FBM> FBMPresets::CreateDetailFBM(uint32_t seed)
    {
        auto noise = std::make_unique<ValueNoise>(seed);
        return std::make_unique<FBM>(std::move(noise));
    }

} // namespace PCG
