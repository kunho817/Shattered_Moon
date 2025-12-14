#include "Biome.h"
#include "Noise.h"
#include "FBM.h"
#include <algorithm>
#include <cmath>

namespace PCG {

    // ============================================================================
    // BiomeMap Implementation
    // ============================================================================

    BiomeMap::BiomeMap()
    {
        InitializeDefaultBiomes();
    }

    void BiomeMap::InitializeDefaultBiomes()
    {
        // Deep Ocean
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::DeepOcean)];
            b.Type = BiomeType::DeepOcean;
            b.Name = "Deep Ocean";
            b.MinHeight = 0.0f; b.MaxHeight = 0.15f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.05f; b.Color[1] = 0.1f; b.Color[2] = 0.3f; b.Color[3] = 1.0f;
            b.IsWater = true;
            b.IsPassable = false;
        }

        // Ocean
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Ocean)];
            b.Type = BiomeType::Ocean;
            b.Name = "Ocean";
            b.MinHeight = 0.15f; b.MaxHeight = 0.25f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.1f; b.Color[1] = 0.2f; b.Color[2] = 0.5f; b.Color[3] = 1.0f;
            b.IsWater = true;
            b.IsPassable = false;
        }

        // Coastal Water
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::CoastalWater)];
            b.Type = BiomeType::CoastalWater;
            b.Name = "Coastal Water";
            b.MinHeight = 0.25f; b.MaxHeight = 0.3f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.2f; b.Color[1] = 0.4f; b.Color[2] = 0.6f; b.Color[3] = 1.0f;
            b.IsWater = true;
            b.IsPassable = true;
            b.MovementSpeed = 0.5f;
        }

        // Beach
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Beach)];
            b.Type = BiomeType::Beach;
            b.Name = "Beach";
            b.MinHeight = 0.3f; b.MaxHeight = 0.35f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.9f; b.Color[1] = 0.85f; b.Color[2] = 0.6f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.9f;
        }

        // Marsh
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Marsh)];
            b.Type = BiomeType::Marsh;
            b.Name = "Marsh";
            b.MinHeight = 0.3f; b.MaxHeight = 0.4f;
            b.MinMoisture = 0.7f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.3f; b.MaxTemperature = 0.8f;
            b.Color[0] = 0.3f; b.Color[1] = 0.4f; b.Color[2] = 0.25f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.6f;
        }

        // Plains
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Plains)];
            b.Type = BiomeType::Plains;
            b.Name = "Plains";
            b.MinHeight = 0.35f; b.MaxHeight = 0.5f;
            b.MinMoisture = 0.2f; b.MaxMoisture = 0.5f;
            b.MinTemperature = 0.3f; b.MaxTemperature = 0.7f;
            b.Color[0] = 0.6f; b.Color[1] = 0.7f; b.Color[2] = 0.3f; b.Color[3] = 1.0f;
            b.ResourceDensity = 1.2f;
        }

        // Grassland
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Grassland)];
            b.Type = BiomeType::Grassland;
            b.Name = "Grassland";
            b.MinHeight = 0.35f; b.MaxHeight = 0.55f;
            b.MinMoisture = 0.4f; b.MaxMoisture = 0.7f;
            b.MinTemperature = 0.3f; b.MaxTemperature = 0.7f;
            b.Color[0] = 0.4f; b.Color[1] = 0.6f; b.Color[2] = 0.2f; b.Color[3] = 1.0f;
            b.ResourceDensity = 1.3f;
        }

        // Forest
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Forest)];
            b.Type = BiomeType::Forest;
            b.Name = "Forest";
            b.MinHeight = 0.4f; b.MaxHeight = 0.65f;
            b.MinMoisture = 0.5f; b.MaxMoisture = 0.8f;
            b.MinTemperature = 0.3f; b.MaxTemperature = 0.7f;
            b.Color[0] = 0.15f; b.Color[1] = 0.4f; b.Color[2] = 0.1f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.8f;
            b.ResourceDensity = 1.5f;
        }

        // Deciduous Forest
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::DeciduousForest)];
            b.Type = BiomeType::DeciduousForest;
            b.Name = "Deciduous Forest";
            b.MinHeight = 0.4f; b.MaxHeight = 0.6f;
            b.MinMoisture = 0.5f; b.MaxMoisture = 0.8f;
            b.MinTemperature = 0.4f; b.MaxTemperature = 0.7f;
            b.Color[0] = 0.2f; b.Color[1] = 0.5f; b.Color[2] = 0.15f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.8f;
        }

        // Coniferous Forest
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::ConiferousForest)];
            b.Type = BiomeType::ConiferousForest;
            b.Name = "Coniferous Forest";
            b.MinHeight = 0.5f; b.MaxHeight = 0.7f;
            b.MinMoisture = 0.4f; b.MaxMoisture = 0.7f;
            b.MinTemperature = 0.2f; b.MaxTemperature = 0.5f;
            b.Color[0] = 0.1f; b.Color[1] = 0.3f; b.Color[2] = 0.15f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.7f;
        }

        // Jungle
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Jungle)];
            b.Type = BiomeType::Jungle;
            b.Name = "Jungle";
            b.MinHeight = 0.35f; b.MaxHeight = 0.55f;
            b.MinMoisture = 0.8f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.7f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.05f; b.Color[1] = 0.35f; b.Color[2] = 0.05f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.5f;
            b.DangerLevel = 0.6f;
        }

        // Savanna
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Savanna)];
            b.Type = BiomeType::Savanna;
            b.Name = "Savanna";
            b.MinHeight = 0.35f; b.MaxHeight = 0.5f;
            b.MinMoisture = 0.2f; b.MaxMoisture = 0.5f;
            b.MinTemperature = 0.6f; b.MaxTemperature = 0.9f;
            b.Color[0] = 0.7f; b.Color[1] = 0.6f; b.Color[2] = 0.2f; b.Color[3] = 1.0f;
        }

        // Desert
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Desert)];
            b.Type = BiomeType::Desert;
            b.Name = "Desert";
            b.MinHeight = 0.3f; b.MaxHeight = 0.6f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 0.2f;
            b.MinTemperature = 0.6f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.9f; b.Color[1] = 0.8f; b.Color[2] = 0.5f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.7f;
            b.ResourceDensity = 0.3f;
            b.DangerLevel = 0.4f;
        }

        // Hills
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Hills)];
            b.Type = BiomeType::Hills;
            b.Name = "Hills";
            b.MinHeight = 0.55f; b.MaxHeight = 0.7f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.4f; b.Color[1] = 0.5f; b.Color[2] = 0.3f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.7f;
        }

        // Mountains
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Mountains)];
            b.Type = BiomeType::Mountains;
            b.Name = "Mountains";
            b.MinHeight = 0.7f; b.MaxHeight = 0.85f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.5f; b.Color[1] = 0.45f; b.Color[2] = 0.4f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.4f;
            b.DangerLevel = 0.5f;
        }

        // Snow Peaks
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::SnowPeaks)];
            b.Type = BiomeType::SnowPeaks;
            b.Name = "Snow Peaks";
            b.MinHeight = 0.85f; b.MaxHeight = 1.0f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 1.0f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.95f; b.Color[1] = 0.95f; b.Color[2] = 1.0f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.3f;
            b.DangerLevel = 0.7f;
        }

        // Tundra
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Tundra)];
            b.Type = BiomeType::Tundra;
            b.Name = "Tundra";
            b.MinHeight = 0.35f; b.MaxHeight = 0.6f;
            b.MinMoisture = 0.3f; b.MaxMoisture = 0.7f;
            b.MinTemperature = 0.0f; b.MaxTemperature = 0.3f;
            b.Color[0] = 0.7f; b.Color[1] = 0.75f; b.Color[2] = 0.7f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.8f;
            b.ResourceDensity = 0.5f;
        }

        // Volcanic
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Volcanic)];
            b.Type = BiomeType::Volcanic;
            b.Name = "Volcanic";
            b.MinHeight = 0.6f; b.MaxHeight = 0.9f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 0.2f;
            b.MinTemperature = 0.8f; b.MaxTemperature = 1.0f;
            b.Color[0] = 0.3f; b.Color[1] = 0.15f; b.Color[2] = 0.1f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.5f;
            b.DangerLevel = 0.9f;
            b.ResourceDensity = 0.8f;
        }

        // Wasteland
        {
            BiomeData& b = m_BiomeData[static_cast<size_t>(BiomeType::Wasteland)];
            b.Type = BiomeType::Wasteland;
            b.Name = "Wasteland";
            b.MinHeight = 0.3f; b.MaxHeight = 0.7f;
            b.MinMoisture = 0.0f; b.MaxMoisture = 0.15f;
            b.MinTemperature = 0.3f; b.MaxTemperature = 0.7f;
            b.Color[0] = 0.5f; b.Color[1] = 0.45f; b.Color[2] = 0.35f; b.Color[3] = 1.0f;
            b.MovementSpeed = 0.9f;
            b.ResourceDensity = 0.2f;
            b.DangerLevel = 0.3f;
        }
    }

    BiomeType BiomeMap::GetBiome(float height) const
    {
        // Simple height-based classification
        if (height < 0.15f) return BiomeType::DeepOcean;
        if (height < 0.25f) return BiomeType::Ocean;
        if (height < 0.3f) return BiomeType::CoastalWater;
        if (height < 0.35f) return BiomeType::Beach;
        if (height < 0.5f) return BiomeType::Plains;
        if (height < 0.55f) return BiomeType::Grassland;
        if (height < 0.65f) return BiomeType::Forest;
        if (height < 0.75f) return BiomeType::Hills;
        if (height < 0.85f) return BiomeType::Mountains;
        return BiomeType::SnowPeaks;
    }

    BiomeType BiomeMap::GetBiome(float height, float moisture) const
    {
        // Height-based water biomes
        if (height < 0.15f) return BiomeType::DeepOcean;
        if (height < 0.25f) return BiomeType::Ocean;
        if (height < 0.3f) return BiomeType::CoastalWater;
        if (height < 0.35f) return BiomeType::Beach;

        // High altitude always mountain/snow
        if (height > 0.85f) return BiomeType::SnowPeaks;
        if (height > 0.7f) return BiomeType::Mountains;
        if (height > 0.55f) return BiomeType::Hills;

        // Mid-range depends on moisture
        if (moisture < 0.2f) return BiomeType::Desert;
        if (moisture < 0.4f) return BiomeType::Savanna;
        if (moisture < 0.6f) return BiomeType::Plains;
        if (moisture < 0.8f) return BiomeType::Forest;
        return BiomeType::Marsh;
    }

    BiomeType BiomeMap::GetBiome(float height, float moisture, float temperature) const
    {
        // Find best matching biome
        BiomeType bestBiome = BiomeType::Plains;
        float bestScore = -1.0f;

        for (size_t i = 0; i < static_cast<size_t>(BiomeType::Count); ++i) {
            const BiomeData& biome = m_BiomeData[i];
            float score = biome.MatchScore(height, moisture, temperature);
            if (score > bestScore) {
                bestScore = score;
                bestBiome = biome.Type;
            }
        }

        // If no good match, fall back to simple classification
        if (bestScore < 0.1f) {
            return GetBiome(height, moisture);
        }

        return bestBiome;
    }

    const BiomeData& BiomeMap::GetBiomeData(BiomeType type) const
    {
        return m_BiomeData[static_cast<size_t>(type)];
    }

    void BiomeMap::GetBiomeColor(float height, float moisture, float temperature,
                                  float& r, float& g, float& b, float& a) const
    {
        BiomeType biome = GetBiome(height, moisture, temperature);
        const BiomeData& data = GetBiomeData(biome);
        r = data.Color[0];
        g = data.Color[1];
        b = data.Color[2];
        a = data.Color[3];
    }

    void BiomeMap::GetBiomeColor(float height, float& r, float& g, float& b, float& a) const
    {
        BiomeType biome = GetBiome(height);
        const BiomeData& data = GetBiomeData(biome);
        r = data.Color[0];
        g = data.Color[1];
        b = data.Color[2];
        a = data.Color[3];
    }

    void BiomeMap::SetBiomeData(BiomeType type, const BiomeData& data)
    {
        m_BiomeData[static_cast<size_t>(type)] = data;
    }

    // ============================================================================
    // BiomeGenerator Implementation
    // ============================================================================

    BiomeGenerator::BiomeGenerator()
    {
    }

    std::vector<BiomeType> BiomeGenerator::Generate(const std::vector<float>& heightmap,
                                                     const Settings& settings,
                                                     const BiomeMap& biomeMap)
    {
        // Generate moisture and temperature maps
        m_MoistureMap = GenerateMoistureMap(settings);
        m_TemperatureMap = GenerateTemperatureMap(settings);

        std::vector<BiomeType> biomes(settings.Width * settings.Height);

        for (int y = 0; y < settings.Height; ++y) {
            for (int x = 0; x < settings.Width; ++x) {
                int idx = y * settings.Width + x;

                float height = heightmap[idx];
                float moisture = m_MoistureMap[idx];
                float temperature = m_TemperatureMap[idx];

                biomes[idx] = biomeMap.GetBiome(height, moisture, temperature);
            }
        }

        return biomes;
    }

    std::vector<float> BiomeGenerator::GenerateMoistureMap(const Settings& settings)
    {
        std::vector<float> moisture(settings.Width * settings.Height);

        SimplexNoise noise(settings.Seed + 100);
        FBM fbm(&noise);

        FBMSettings fbmSettings;
        fbmSettings.Frequency = settings.MoistureFrequency;
        fbmSettings.Octaves = settings.MoistureOctaves;
        fbmSettings.Persistence = 0.5f;

        for (int y = 0; y < settings.Height; ++y) {
            for (int x = 0; x < settings.Width; ++x) {
                float value = fbm.Sample(static_cast<float>(x), static_cast<float>(y), fbmSettings);
                // Remap from [-1, 1] to [0, 1]
                moisture[y * settings.Width + x] = (value + 1.0f) * 0.5f;
            }
        }

        return moisture;
    }

    std::vector<float> BiomeGenerator::GenerateTemperatureMap(const Settings& settings)
    {
        std::vector<float> temperature(settings.Width * settings.Height);

        PerlinNoise noise(settings.Seed + 200);
        FBM fbm(&noise);

        FBMSettings fbmSettings;
        fbmSettings.Frequency = settings.TemperatureFrequency;
        fbmSettings.Octaves = settings.TemperatureOctaves;
        fbmSettings.Persistence = 0.4f;

        for (int y = 0; y < settings.Height; ++y) {
            for (int x = 0; x < settings.Width; ++x) {
                // Base temperature from noise
                float noiseValue = fbm.Sample(static_cast<float>(x), static_cast<float>(y), fbmSettings);
                noiseValue = (noiseValue + 1.0f) * 0.5f;

                // Latitude influence (warmer at equator/center, colder at poles/edges)
                float normalizedY = static_cast<float>(y) / settings.Height;
                float latitudeTemp = 1.0f - 2.0f * std::abs(normalizedY - 0.5f);

                // Blend noise and latitude
                float temp = noiseValue * (1.0f - settings.TemperatureLatitudeInfluence) +
                             latitudeTemp * settings.TemperatureLatitudeInfluence;

                temperature[y * settings.Width + x] = std::clamp(temp, 0.0f, 1.0f);
            }
        }

        return temperature;
    }

    std::vector<float> BiomeGenerator::GenerateColorMap(const std::vector<BiomeType>& biomes,
                                                         const BiomeMap& biomeMap)
    {
        std::vector<float> colors(biomes.size() * 4);

        for (size_t i = 0; i < biomes.size(); ++i) {
            const BiomeData& data = biomeMap.GetBiomeData(biomes[i]);
            colors[i * 4 + 0] = data.Color[0];
            colors[i * 4 + 1] = data.Color[1];
            colors[i * 4 + 2] = data.Color[2];
            colors[i * 4 + 3] = data.Color[3];
        }

        return colors;
    }

    // ============================================================================
    // BiomePresets Implementation
    // ============================================================================

    BiomeMap BiomePresets::CreateEarthLike()
    {
        return BiomeMap(); // Default is Earth-like
    }

    BiomeMap BiomePresets::CreateAlien()
    {
        BiomeMap map;

        // Modify colors for alien look
        BiomeData data;

        data = map.GetBiomeData(BiomeType::Forest);
        data.Color[0] = 0.6f; data.Color[1] = 0.1f; data.Color[2] = 0.4f;
        map.SetBiomeData(BiomeType::Forest, data);

        data = map.GetBiomeData(BiomeType::Plains);
        data.Color[0] = 0.3f; data.Color[1] = 0.5f; data.Color[2] = 0.6f;
        map.SetBiomeData(BiomeType::Plains, data);

        data = map.GetBiomeData(BiomeType::Ocean);
        data.Color[0] = 0.4f; data.Color[1] = 0.1f; data.Color[2] = 0.3f;
        map.SetBiomeData(BiomeType::Ocean, data);

        return map;
    }

    BiomeMap BiomePresets::CreateDesertWorld()
    {
        BiomeMap map;

        // Most biomes become desert variations
        for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
            BiomeType type = static_cast<BiomeType>(i);
            BiomeData data = map.GetBiomeData(type);

            if (!data.IsWater) {
                // Shift colors toward sand/brown
                data.Color[0] = data.Color[0] * 0.5f + 0.45f;
                data.Color[1] = data.Color[1] * 0.5f + 0.35f;
                data.Color[2] = data.Color[2] * 0.3f + 0.2f;
                data.ResourceDensity *= 0.5f;
                map.SetBiomeData(type, data);
            }
        }

        return map;
    }

    BiomeMap BiomePresets::CreateIceWorld()
    {
        BiomeMap map;

        for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
            BiomeType type = static_cast<BiomeType>(i);
            BiomeData data = map.GetBiomeData(type);

            // Shift colors toward white/blue
            float brightness = (data.Color[0] + data.Color[1] + data.Color[2]) / 3.0f;
            data.Color[0] = brightness * 0.8f + 0.2f;
            data.Color[1] = brightness * 0.85f + 0.15f;
            data.Color[2] = brightness * 0.9f + 0.1f;
            data.MovementSpeed *= 0.7f;
            map.SetBiomeData(type, data);
        }

        return map;
    }

    BiomeMap BiomePresets::CreateLunar()
    {
        BiomeMap map;

        // Gray scale everything, no water
        for (int i = 0; i < static_cast<int>(BiomeType::Count); ++i) {
            BiomeType type = static_cast<BiomeType>(i);
            BiomeData data = map.GetBiomeData(type);

            float gray = (data.Color[0] + data.Color[1] + data.Color[2]) / 3.0f;
            gray = gray * 0.5f + 0.2f; // Keep somewhat dark

            data.Color[0] = gray;
            data.Color[1] = gray;
            data.Color[2] = gray;
            data.IsWater = false;
            data.IsPassable = true;
            data.ResourceDensity = 0.3f;
            map.SetBiomeData(type, data);
        }

        return map;
    }

} // namespace PCG
