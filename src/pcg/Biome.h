#pragma once

/**
 * @file Biome.h
 * @brief Biome classification and mapping system
 *
 * Provides a system for classifying terrain into biomes based on
 * height, moisture, temperature, and other environmental factors.
 */

#include <array>
#include <vector>
#include <string>
#include <cstdint>

namespace PCG {

    /**
     * @brief Available biome types
     */
    enum class BiomeType : uint8_t {
        // Water biomes
        DeepOcean,
        Ocean,
        CoastalWater,

        // Low elevation
        Beach,
        Marsh,
        Plains,
        Grassland,

        // Medium elevation
        Forest,
        DeciduousForest,
        ConiferousForest,
        Jungle,
        Savanna,
        Desert,

        // High elevation
        Hills,
        Mountains,
        SnowPeaks,

        // Special
        Tundra,
        Volcanic,
        Wasteland,

        Count  // Number of biome types
    };

    /**
     * @brief Convert biome type to string
     */
    inline const char* BiomeTypeToString(BiomeType type) {
        switch (type) {
            case BiomeType::DeepOcean: return "Deep Ocean";
            case BiomeType::Ocean: return "Ocean";
            case BiomeType::CoastalWater: return "Coastal Water";
            case BiomeType::Beach: return "Beach";
            case BiomeType::Marsh: return "Marsh";
            case BiomeType::Plains: return "Plains";
            case BiomeType::Grassland: return "Grassland";
            case BiomeType::Forest: return "Forest";
            case BiomeType::DeciduousForest: return "Deciduous Forest";
            case BiomeType::ConiferousForest: return "Coniferous Forest";
            case BiomeType::Jungle: return "Jungle";
            case BiomeType::Savanna: return "Savanna";
            case BiomeType::Desert: return "Desert";
            case BiomeType::Hills: return "Hills";
            case BiomeType::Mountains: return "Mountains";
            case BiomeType::SnowPeaks: return "Snow Peaks";
            case BiomeType::Tundra: return "Tundra";
            case BiomeType::Volcanic: return "Volcanic";
            case BiomeType::Wasteland: return "Wasteland";
            default: return "Unknown";
        }
    }

    /**
     * @brief Data for a single biome type
     */
    struct BiomeData {
        BiomeType Type = BiomeType::Plains;
        std::string Name;

        // Height thresholds (normalized 0-1)
        float MinHeight = 0.0f;
        float MaxHeight = 1.0f;

        // Moisture thresholds (0 = dry, 1 = wet)
        float MinMoisture = 0.0f;
        float MaxMoisture = 1.0f;

        // Temperature thresholds (0 = cold, 1 = hot)
        float MinTemperature = 0.0f;
        float MaxTemperature = 1.0f;

        // Visual properties
        float Color[4] = {0.5f, 0.5f, 0.5f, 1.0f};  // RGBA

        // Gameplay properties
        float MovementSpeed = 1.0f;      // Movement speed multiplier
        float ResourceDensity = 1.0f;    // Resource spawn rate
        float DangerLevel = 0.0f;        // Enemy spawn rate
        bool IsWater = false;
        bool IsPassable = true;

        /**
         * @brief Check if environmental conditions match this biome
         */
        bool Matches(float height, float moisture, float temperature) const {
            return height >= MinHeight && height <= MaxHeight &&
                   moisture >= MinMoisture && moisture <= MaxMoisture &&
                   temperature >= MinTemperature && temperature <= MaxTemperature;
        }

        /**
         * @brief Calculate how well conditions match this biome (0-1)
         */
        float MatchScore(float height, float moisture, float temperature) const {
            if (!Matches(height, moisture, temperature)) return 0.0f;

            // Calculate center of biome ranges
            float heightCenter = (MinHeight + MaxHeight) * 0.5f;
            float moistureCenter = (MinMoisture + MaxMoisture) * 0.5f;
            float tempCenter = (MinTemperature + MaxTemperature) * 0.5f;

            // Calculate distance from center (lower is better)
            float heightDist = std::abs(height - heightCenter) / (MaxHeight - MinHeight + 0.001f);
            float moistureDist = std::abs(moisture - moistureCenter) / (MaxMoisture - MinMoisture + 0.001f);
            float tempDist = std::abs(temperature - tempCenter) / (MaxTemperature - MinTemperature + 0.001f);

            return 1.0f - (heightDist + moistureDist + tempDist) / 3.0f;
        }
    };

    /**
     * @brief Biome classification and lookup system
     */
    class BiomeMap {
    public:
        BiomeMap();

        /**
         * @brief Get biome type based on height only (simple mode)
         * @param height Normalized height (0-1)
         * @return Biome type
         */
        BiomeType GetBiome(float height) const;

        /**
         * @brief Get biome type based on height and moisture
         * @param height Normalized height (0-1)
         * @param moisture Moisture level (0-1, 0=dry, 1=wet)
         * @return Biome type
         */
        BiomeType GetBiome(float height, float moisture) const;

        /**
         * @brief Get biome type based on all environmental factors
         * @param height Normalized height (0-1)
         * @param moisture Moisture level (0-1)
         * @param temperature Temperature level (0-1, 0=cold, 1=hot)
         * @return Biome type
         */
        BiomeType GetBiome(float height, float moisture, float temperature) const;

        /**
         * @brief Get the data for a biome type
         */
        const BiomeData& GetBiomeData(BiomeType type) const;

        /**
         * @brief Get biome color for a given point
         */
        void GetBiomeColor(float height, float moisture, float temperature,
                           float& r, float& g, float& b, float& a) const;

        /**
         * @brief Get biome color for a given point (simple mode)
         */
        void GetBiomeColor(float height, float& r, float& g, float& b, float& a) const;

        /**
         * @brief Set custom biome data
         */
        void SetBiomeData(BiomeType type, const BiomeData& data);

        /**
         * @brief Set height threshold for water level
         */
        void SetWaterLevel(float level) { m_WaterLevel = level; }

        /**
         * @brief Get water level threshold
         */
        float GetWaterLevel() const { return m_WaterLevel; }

    private:
        std::array<BiomeData, static_cast<size_t>(BiomeType::Count)> m_BiomeData;
        float m_WaterLevel = 0.3f;

        void InitializeDefaultBiomes();
    };

    /**
     * @brief Generate a biome map from heightmap and other data
     */
    class BiomeGenerator {
    public:
        struct Settings {
            int Width = 256;
            int Height = 256;
            uint32_t Seed = 12345;

            // Noise settings for moisture map
            float MoistureFrequency = 0.02f;
            int MoistureOctaves = 4;

            // Noise settings for temperature map
            float TemperatureFrequency = 0.01f;
            int TemperatureOctaves = 3;
            float TemperatureLatitudeInfluence = 0.5f;  // How much latitude affects temperature
        };

        BiomeGenerator();

        /**
         * @brief Generate a biome map from a heightmap
         * @param heightmap Input height data (normalized 0-1)
         * @param settings Generation settings
         * @param biomeMap Biome classification rules
         * @return Vector of BiomeType for each point
         */
        std::vector<BiomeType> Generate(const std::vector<float>& heightmap,
                                        const Settings& settings,
                                        const BiomeMap& biomeMap);

        /**
         * @brief Generate moisture map
         */
        std::vector<float> GenerateMoistureMap(const Settings& settings);

        /**
         * @brief Generate temperature map
         */
        std::vector<float> GenerateTemperatureMap(const Settings& settings);

        /**
         * @brief Generate a color map from biome data
         * @param biomes Biome type array
         * @param biomeMap Biome data source
         * @return RGBA color data (4 floats per pixel)
         */
        std::vector<float> GenerateColorMap(const std::vector<BiomeType>& biomes,
                                            const BiomeMap& biomeMap);

    private:
        // Cached maps
        std::vector<float> m_MoistureMap;
        std::vector<float> m_TemperatureMap;
    };

    /**
     * @brief Simple biome preset configurations
     */
    class BiomePresets {
    public:
        /**
         * @brief Create biome map for Earth-like terrain
         */
        static BiomeMap CreateEarthLike();

        /**
         * @brief Create biome map for alien/fantasy terrain
         */
        static BiomeMap CreateAlien();

        /**
         * @brief Create biome map for desert world
         */
        static BiomeMap CreateDesertWorld();

        /**
         * @brief Create biome map for ice world
         */
        static BiomeMap CreateIceWorld();

        /**
         * @brief Create biome map based on moon/lunar terrain
         */
        static BiomeMap CreateLunar();
    };

} // namespace PCG
