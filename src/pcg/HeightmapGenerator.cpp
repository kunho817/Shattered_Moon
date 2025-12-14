#include "HeightmapGenerator.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace PCG {

    HeightmapGenerator::HeightmapGenerator()
    {
    }

    // ============================================================================
    // Generation
    // ============================================================================

    std::vector<float> HeightmapGenerator::Generate(const HeightmapSettings& settings)
    {
        // Initialize noise generators
        m_BaseNoise = std::make_unique<PerlinNoise>(settings.Seed);
        m_FBM = std::make_unique<FBM>(m_BaseNoise.get());

        std::vector<float> heightmap(settings.Width * settings.Height);

        // Create additional noise for terrain variety
        SimplexNoise mountainNoise(settings.Seed + 1);
        PerlinNoise hillNoise(settings.Seed + 2);
        FBM mountainFBM(&mountainNoise);
        FBM hillFBM(&hillNoise);

        FBMSettings mountainSettings = FBMSettings::Mountains();
        FBMSettings hillSettings;
        hillSettings.Octaves = 4;
        hillSettings.Frequency = settings.Noise.Frequency * 2.0f;
        hillSettings.Persistence = 0.4f;

        // Generate heightmap
        for (int y = 0; y < settings.Height; ++y) {
            for (int x = 0; x < settings.Width; ++x) {
                float nx = static_cast<float>(x);
                float ny = static_cast<float>(y);

                float baseValue;

                if (settings.ApplyDomainWarp) {
                    baseValue = m_FBM->WarpedSample(nx, ny, settings.Noise, settings.WarpStrength);
                } else {
                    baseValue = m_FBM->Sample(nx, ny, settings.Noise);
                }

                // Remap base value from [-1, 1] to [0, 1]
                baseValue = (baseValue + 1.0f) * 0.5f;

                // Add terrain type variation
                float mountainValue = mountainFBM.Ridged(nx, ny, mountainSettings);
                float hillValue = (hillFBM.Sample(nx, ny, hillSettings) + 1.0f) * 0.5f;
                float plainValue = baseValue * 0.3f;

                // Blend terrain types based on weights and base noise
                float blendFactor = baseValue;

                float finalHeight;
                if (blendFactor > 0.7f) {
                    // Mountain region
                    float t = (blendFactor - 0.7f) / 0.3f;
                    finalHeight = hillValue + t * (mountainValue - hillValue);
                    finalHeight *= settings.MountainWeight + settings.HillWeight;
                } else if (blendFactor > 0.3f) {
                    // Hill region
                    float t = (blendFactor - 0.3f) / 0.4f;
                    finalHeight = plainValue + t * (hillValue - plainValue);
                    finalHeight *= settings.HillWeight + settings.PlainWeight;
                } else {
                    // Plain/Ocean region
                    float t = blendFactor / 0.3f;
                    float oceanFloor = settings.OceanWeight * 0.2f;
                    finalHeight = oceanFloor + t * (plainValue - oceanFloor);
                    finalHeight *= settings.PlainWeight + settings.OceanWeight;
                }

                // Store normalized [0, 1] value
                heightmap[y * settings.Width + x] = std::clamp(finalHeight, 0.0f, 1.0f);
            }
        }

        // Apply post-processing
        if (settings.ApplyFalloffMap) {
            ApplyFalloff(heightmap, settings.Width, settings.Height);
        }

        if (settings.ApplyTerracing) {
            ApplyTerracing(heightmap, settings.TerraceCount);
        }

        // Remap to final height range
        Remap(heightmap, settings.MinHeight, settings.MaxHeight);

        return heightmap;
    }

    std::vector<float> HeightmapGenerator::GenerateCustom(const HeightmapSettings& settings,
                                                          std::function<float(float, float)> noiseFunc)
    {
        std::vector<float> heightmap(settings.Width * settings.Height);

        for (int y = 0; y < settings.Height; ++y) {
            for (int x = 0; x < settings.Width; ++x) {
                float nx = static_cast<float>(x);
                float ny = static_cast<float>(y);

                heightmap[y * settings.Width + x] = noiseFunc(nx, ny);
            }
        }

        Normalize(heightmap);

        if (settings.ApplyFalloffMap) {
            ApplyFalloff(heightmap, settings.Width, settings.Height);
        }

        Remap(heightmap, settings.MinHeight, settings.MaxHeight);

        return heightmap;
    }

    // ============================================================================
    // Post-Processing: Erosion
    // ============================================================================

    void HeightmapGenerator::InitErosionBrushes(int radius,
                                                 std::vector<std::vector<int>>& brushIndices,
                                                 std::vector<std::vector<float>>& brushWeights,
                                                 int mapSize)
    {
        brushIndices.resize(mapSize);
        brushWeights.resize(mapSize);

        std::vector<int> xOffsets;
        std::vector<int> yOffsets;
        std::vector<float> weights;

        float weightSum = 0;
        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                float sqrDst = static_cast<float>(x * x + y * y);
                if (sqrDst < radius * radius) {
                    xOffsets.push_back(x);
                    yOffsets.push_back(y);
                    float weight = 1 - std::sqrt(sqrDst) / radius;
                    weights.push_back(weight);
                    weightSum += weight;
                }
            }
        }

        // Normalize weights
        for (float& w : weights) {
            w /= weightSum;
        }

        int mapWidth = static_cast<int>(std::sqrt(mapSize));

        for (int i = 0; i < mapSize; ++i) {
            int centerX = i % mapWidth;
            int centerY = i / mapWidth;

            if (centerX < radius || centerX >= mapWidth - radius ||
                centerY < radius || centerY >= mapWidth - radius) {
                // Near edge, use smaller brush
                brushIndices[i].push_back(i);
                brushWeights[i].push_back(1.0f);
            } else {
                for (size_t j = 0; j < xOffsets.size(); ++j) {
                    int idx = (centerY + yOffsets[j]) * mapWidth + (centerX + xOffsets[j]);
                    brushIndices[i].push_back(idx);
                    brushWeights[i].push_back(weights[j]);
                }
            }
        }
    }

    float HeightmapGenerator::CalculateGradient(const std::vector<float>& heightmap,
                                                 int width, int height,
                                                 float posX, float posY,
                                                 float& gradX, float& gradY) const
    {
        int coordX = static_cast<int>(posX);
        int coordY = static_cast<int>(posY);

        // Calculate droplet's offset inside cell
        float x = posX - coordX;
        float y = posY - coordY;

        // Get heights of 4 corners
        int nodeIdx = coordY * width + coordX;
        float heightNW = heightmap[nodeIdx];
        float heightNE = heightmap[nodeIdx + 1];
        float heightSW = heightmap[nodeIdx + width];
        float heightSE = heightmap[nodeIdx + width + 1];

        // Calculate gradient
        gradX = (heightNE - heightNW) * (1 - y) + (heightSE - heightSW) * y;
        gradY = (heightSW - heightNW) * (1 - x) + (heightSE - heightNE) * x;

        // Calculate interpolated height
        return heightNW * (1 - x) * (1 - y) + heightNE * x * (1 - y) +
               heightSW * (1 - x) * y + heightSE * x * y;
    }

    void HeightmapGenerator::ApplyErosion(std::vector<float>& heightmap, int width, int height,
                                           const ErosionSettings& settings)
    {
        int mapSize = width * height;

        // Pre-compute erosion brush data
        std::vector<std::vector<int>> erosionBrushIndices;
        std::vector<std::vector<float>> erosionBrushWeights;
        InitErosionBrushes(settings.ErosionRadius, erosionBrushIndices,
                           erosionBrushWeights, mapSize);

        std::mt19937 rng(12345);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int iteration = 0; iteration < settings.Iterations; ++iteration) {
            // Spawn droplet at random position
            float posX = dist(rng) * (width - 2) + 1;
            float posY = dist(rng) * (height - 2) + 1;
            float dirX = 0;
            float dirY = 0;
            float speed = 1;
            float water = 1;
            float sediment = 0;

            for (int lifetime = 0; lifetime < settings.MaxDropletLifetime; ++lifetime) {
                int nodeX = static_cast<int>(posX);
                int nodeY = static_cast<int>(posY);
                int dropletIndex = nodeY * width + nodeX;

                // Calculate droplet's offset inside cell
                float cellOffsetX = posX - nodeX;
                float cellOffsetY = posY - nodeY;

                // Calculate gradient and height
                float gradientX, gradientY;
                float heightAtPos = CalculateGradient(heightmap, width, height,
                                                       posX, posY, gradientX, gradientY);

                // Update direction with inertia
                dirX = dirX * settings.Inertia - gradientX * (1 - settings.Inertia);
                dirY = dirY * settings.Inertia - gradientY * (1 - settings.Inertia);

                // Normalize direction
                float len = std::sqrt(dirX * dirX + dirY * dirY);
                if (len > 0.0001f) {
                    dirX /= len;
                    dirY /= len;
                }

                // Move droplet
                float newPosX = posX + dirX;
                float newPosY = posY + dirY;

                // Check bounds
                if (newPosX < 1 || newPosX >= width - 2 ||
                    newPosY < 1 || newPosY >= height - 2) {
                    break;
                }

                // Calculate new height
                float newHeight = CalculateGradient(heightmap, width, height,
                                                     newPosX, newPosY, gradientX, gradientY);
                float deltaHeight = newHeight - heightAtPos;

                // Calculate sediment capacity
                float sedimentCapacity = std::max(-deltaHeight * speed * water *
                                                   settings.SedimentCapacity,
                                                   settings.MinSedimentCapacity);

                // Deposit or erode
                if (sediment > sedimentCapacity || deltaHeight > 0) {
                    // Deposit sediment
                    float amountToDeposit = (deltaHeight > 0) ? std::min(deltaHeight, sediment) :
                                            (sediment - sedimentCapacity) * settings.DepositSpeed;
                    sediment -= amountToDeposit;

                    // Deposit to 4 nodes
                    heightmap[dropletIndex] += amountToDeposit * (1 - cellOffsetX) * (1 - cellOffsetY);
                    heightmap[dropletIndex + 1] += amountToDeposit * cellOffsetX * (1 - cellOffsetY);
                    heightmap[dropletIndex + width] += amountToDeposit * (1 - cellOffsetX) * cellOffsetY;
                    heightmap[dropletIndex + width + 1] += amountToDeposit * cellOffsetX * cellOffsetY;
                } else {
                    // Erode
                    float amountToErode = std::min((sedimentCapacity - sediment) * settings.ErodeSpeed,
                                                    -deltaHeight);

                    // Use erosion brush
                    const auto& brushIndices = erosionBrushIndices[dropletIndex];
                    const auto& brushWeights = erosionBrushWeights[dropletIndex];

                    for (size_t i = 0; i < brushIndices.size(); ++i) {
                        int idx = brushIndices[i];
                        float erodeAmount = amountToErode * brushWeights[i];
                        float deltaSediment = std::min(heightmap[idx], erodeAmount);
                        heightmap[idx] -= deltaSediment;
                        sediment += deltaSediment;
                    }
                }

                // Update speed and water
                speed = std::sqrt(std::max(0.0f, speed * speed + deltaHeight * settings.Gravity));
                water *= (1 - settings.EvaporateSpeed);

                posX = newPosX;
                posY = newPosY;
            }
        }
    }

    // ============================================================================
    // Post-Processing: Normalization and Remapping
    // ============================================================================

    void HeightmapGenerator::Normalize(std::vector<float>& heightmap)
    {
        if (heightmap.empty()) return;

        float minVal = heightmap[0];
        float maxVal = heightmap[0];

        for (float h : heightmap) {
            minVal = std::min(minVal, h);
            maxVal = std::max(maxVal, h);
        }

        float range = maxVal - minVal;
        if (range < 0.0001f) {
            std::fill(heightmap.begin(), heightmap.end(), 0.5f);
            return;
        }

        for (float& h : heightmap) {
            h = (h - minVal) / range;
        }
    }

    void HeightmapGenerator::Remap(std::vector<float>& heightmap, float minHeight, float maxHeight)
    {
        float range = maxHeight - minHeight;
        for (float& h : heightmap) {
            h = minHeight + h * range;
        }
    }

    // ============================================================================
    // Post-Processing: Falloff
    // ============================================================================

    std::vector<float> HeightmapGenerator::GenerateFalloffMap(int width, int height)
    {
        std::vector<float> falloff(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Normalize to [-1, 1]
                float nx = static_cast<float>(x) / width * 2.0f - 1.0f;
                float ny = static_cast<float>(y) / height * 2.0f - 1.0f;

                // Use max of abs values for square falloff
                float value = std::max(std::abs(nx), std::abs(ny));

                // Apply smoothstep for smoother transition
                float a = 3.0f;
                float b = 2.2f;
                value = std::pow(value, a) / (std::pow(value, a) + std::pow(b - b * value, a));

                falloff[y * width + x] = 1.0f - value;
            }
        }

        return falloff;
    }

    void HeightmapGenerator::ApplyFalloff(std::vector<float>& heightmap, int width, int height,
                                           float falloffStrength)
    {
        std::vector<float> falloff = GenerateFalloffMap(width, height);

        for (size_t i = 0; i < heightmap.size(); ++i) {
            float falloffValue = 1.0f - (1.0f - falloff[i]) * falloffStrength;
            heightmap[i] *= falloffValue;
        }
    }

    void HeightmapGenerator::ApplyRadialFalloff(std::vector<float>& heightmap, int width, int height,
                                                 float centerX, float centerY, float radius)
    {
        float cx = centerX * width;
        float cy = centerY * height;
        float maxDist = radius * std::max(width, height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float dx = x - cx;
                float dy = y - cy;
                float dist = std::sqrt(dx * dx + dy * dy);

                float falloff = 1.0f - std::clamp(dist / maxDist, 0.0f, 1.0f);
                falloff = falloff * falloff; // Smoother falloff

                heightmap[y * width + x] *= falloff;
            }
        }
    }

    // ============================================================================
    // Post-Processing: Terracing
    // ============================================================================

    void HeightmapGenerator::ApplyTerracing(std::vector<float>& heightmap, int levels)
    {
        for (float& h : heightmap) {
            h = std::round(h * levels) / levels;
        }
    }

    // ============================================================================
    // Post-Processing: Smoothing
    // ============================================================================

    void HeightmapGenerator::Smooth(std::vector<float>& heightmap, int width, int height, int radius)
    {
        std::vector<float> temp(heightmap.size());

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float sum = 0;
                int count = 0;

                for (int dy = -radius; dy <= radius; ++dy) {
                    for (int dx = -radius; dx <= radius; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            sum += heightmap[ny * width + nx];
                            count++;
                        }
                    }
                }

                temp[y * width + x] = sum / count;
            }
        }

        heightmap = std::move(temp);
    }

    // ============================================================================
    // Post-Processing: Thermal Erosion
    // ============================================================================

    void HeightmapGenerator::ApplyThermalErosion(std::vector<float>& heightmap, int width, int height,
                                                  int iterations, float talusAngle)
    {
        for (int iter = 0; iter < iterations; ++iter) {
            for (int y = 1; y < height - 1; ++y) {
                for (int x = 1; x < width - 1; ++x) {
                    float h = heightmap[y * width + x];
                    float maxDiff = 0;
                    int maxIdx = -1;

                    // Check 4 neighbors
                    int neighbors[4] = {
                        (y - 1) * width + x,
                        (y + 1) * width + x,
                        y * width + (x - 1),
                        y * width + (x + 1)
                    };

                    for (int i = 0; i < 4; ++i) {
                        float diff = h - heightmap[neighbors[i]];
                        if (diff > talusAngle && diff > maxDiff) {
                            maxDiff = diff;
                            maxIdx = neighbors[i];
                        }
                    }

                    if (maxIdx >= 0) {
                        float move = (maxDiff - talusAngle) * 0.5f;
                        heightmap[y * width + x] -= move;
                        heightmap[maxIdx] += move;
                    }
                }
            }
        }
    }

    // ============================================================================
    // Utility Functions
    // ============================================================================

    std::vector<float> HeightmapGenerator::CalculateNormals(const std::vector<float>& heightmap,
                                                             int width, int height,
                                                             float heightScale)
    {
        std::vector<float> normals(width * height * 3);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Get neighboring heights
                float hL = (x > 0) ? heightmap[y * width + (x - 1)] : heightmap[y * width + x];
                float hR = (x < width - 1) ? heightmap[y * width + (x + 1)] : heightmap[y * width + x];
                float hD = (y > 0) ? heightmap[(y - 1) * width + x] : heightmap[y * width + x];
                float hU = (y < height - 1) ? heightmap[(y + 1) * width + x] : heightmap[y * width + x];

                // Calculate normal
                float nx = (hL - hR) * heightScale;
                float ny = 2.0f;
                float nz = (hD - hU) * heightScale;

                // Normalize
                float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                nx /= len;
                ny /= len;
                nz /= len;

                int idx = (y * width + x) * 3;
                normals[idx] = nx;
                normals[idx + 1] = ny;
                normals[idx + 2] = nz;
            }
        }

        return normals;
    }

    float HeightmapGenerator::SampleBilinear(const std::vector<float>& heightmap,
                                              int width, int height,
                                              float x, float y) const
    {
        int x0 = static_cast<int>(x);
        int y0 = static_cast<int>(y);
        int x1 = std::min(x0 + 1, width - 1);
        int y1 = std::min(y0 + 1, height - 1);
        x0 = std::max(0, x0);
        y0 = std::max(0, y0);

        float fx = x - x0;
        float fy = y - y0;

        float h00 = heightmap[y0 * width + x0];
        float h10 = heightmap[y0 * width + x1];
        float h01 = heightmap[y1 * width + x0];
        float h11 = heightmap[y1 * width + x1];

        return h00 * (1 - fx) * (1 - fy) +
               h10 * fx * (1 - fy) +
               h01 * (1 - fx) * fy +
               h11 * fx * fy;
    }

    float HeightmapGenerator::CalculateSlope(const std::vector<float>& heightmap,
                                              int width, int height,
                                              int x, int y) const
    {
        if (x <= 0 || x >= width - 1 || y <= 0 || y >= height - 1) {
            return 0.0f;
        }

        float hL = heightmap[y * width + (x - 1)];
        float hR = heightmap[y * width + (x + 1)];
        float hD = heightmap[(y - 1) * width + x];
        float hU = heightmap[(y + 1) * width + x];

        float dX = (hR - hL) * 0.5f;
        float dY = (hU - hD) * 0.5f;

        return std::sqrt(dX * dX + dY * dY);
    }

} // namespace PCG
