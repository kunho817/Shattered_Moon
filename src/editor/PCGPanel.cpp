#include "editor/PCGPanel.h"
#include "pcg/ChunkManager.h"
#include "pcg/Noise.h"

#include <imgui.h>
#include <random>
#include <chrono>

namespace SM
{
    PCGPanel::PCGPanel()
    {
        // Initialize with default terrain settings
        m_Settings = PCG::HeightmapSettings();
        m_Settings.Noise = PCG::FBMSettings::Terrain();
        m_Settings.MinHeight = 0.0f;
        m_Settings.MaxHeight = 50.0f;
    }

    void PCGPanel::Draw(PCG::ChunkManager* chunkManager)
    {
        if (!m_Visible)
        {
            return;
        }

        m_ChunkManager = chunkManager;

        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(320, 10), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Terrain Generation (PCG)", &m_Visible))
        {
            DrawNoiseSettings();
            DrawFBMSettings();
            DrawTerrainSettings();
            DrawErosionSettings();
            DrawPresets();
            DrawActions(chunkManager);

            if (m_ShowPreview)
            {
                DrawNoisePreview();
            }
        }
        ImGui::End();
    }

    void PCGPanel::RegenerateTerrain()
    {
        m_NeedRegeneration = true;
        m_SettingsChanged = true;
    }

    void PCGPanel::RandomizeSeed()
    {
        // Use high-resolution clock for random seed
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        m_Settings.Seed = static_cast<uint32_t>(duration.count() % UINT32_MAX);
        m_SettingsChanged = true;
    }

    void PCGPanel::DrawNoiseSettings()
    {
        if (ImGui::CollapsingHeader("Noise Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Seed
            int seed = static_cast<int>(m_Settings.Seed);
            if (ImGui::InputInt("Seed", &seed))
            {
                m_Settings.Seed = static_cast<uint32_t>(seed);
                m_SettingsChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Random"))
            {
                RandomizeSeed();
            }

            // Noise type selector (informational)
            const char* noiseTypes[] = { "Perlin", "Simplex", "Worley", "Value" };
            static int currentNoise = 0;
            ImGui::Combo("Noise Type", &currentNoise, noiseTypes, 4);
            ImGui::TextDisabled("(Currently using Perlin via FBM)");

            ImGui::Separator();
        }
    }

    void PCGPanel::DrawFBMSettings()
    {
        if (ImGui::CollapsingHeader("FBM Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            PCG::FBMSettings& fbm = m_Settings.Noise;

            // Octaves
            if (ImGui::SliderInt("Octaves", &fbm.Octaves, 1, 12))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Number of noise layers to combine");

            // Frequency
            if (ImGui::SliderFloat("Frequency", &fbm.Frequency, 0.001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Base frequency of the noise");

            // Persistence
            if (ImGui::SliderFloat("Persistence", &fbm.Persistence, 0.1f, 1.0f, "%.2f"))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Amplitude reduction per octave (roughness)");

            // Lacunarity
            if (ImGui::SliderFloat("Lacunarity", &fbm.Lacunarity, 1.0f, 4.0f, "%.2f"))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Frequency multiplication per octave");

            // Amplitude
            if (ImGui::SliderFloat("Amplitude", &fbm.Amplitude, 0.1f, 2.0f, "%.2f"))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Base amplitude of the noise");

            ImGui::Separator();
        }
    }

    void PCGPanel::DrawTerrainSettings()
    {
        if (ImGui::CollapsingHeader("Terrain Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Height range
            if (ImGui::DragFloatRange2("Height Range", &m_Settings.MinHeight, &m_Settings.MaxHeight,
                1.0f, -1000.0f, 1000.0f, "Min: %.1f", "Max: %.1f"))
            {
                m_SettingsChanged = true;
            }

            ImGui::Separator();
            ImGui::Text("Terrain Type Weights:");

            // Terrain type weights
            bool weightsChanged = false;
            weightsChanged |= ImGui::SliderFloat("Mountains", &m_Settings.MountainWeight, 0.0f, 1.0f, "%.2f");
            weightsChanged |= ImGui::SliderFloat("Hills", &m_Settings.HillWeight, 0.0f, 1.0f, "%.2f");
            weightsChanged |= ImGui::SliderFloat("Plains", &m_Settings.PlainWeight, 0.0f, 1.0f, "%.2f");
            weightsChanged |= ImGui::SliderFloat("Ocean", &m_Settings.OceanWeight, 0.0f, 1.0f, "%.2f");

            if (weightsChanged)
            {
                m_SettingsChanged = true;
            }

            // Normalize weights button
            if (ImGui::Button("Normalize Weights"))
            {
                float total = m_Settings.MountainWeight + m_Settings.HillWeight +
                              m_Settings.PlainWeight + m_Settings.OceanWeight;
                if (total > 0.0f)
                {
                    m_Settings.MountainWeight /= total;
                    m_Settings.HillWeight /= total;
                    m_Settings.PlainWeight /= total;
                    m_Settings.OceanWeight /= total;
                    m_SettingsChanged = true;
                }
            }

            ImGui::Separator();

            // Post-processing options
            if (ImGui::Checkbox("Apply Falloff Map", &m_Settings.ApplyFalloffMap))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Creates island-like edges that fall to sea level");

            if (ImGui::Checkbox("Apply Domain Warp", &m_Settings.ApplyDomainWarp))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Distorts terrain for more organic shapes");

            if (m_Settings.ApplyDomainWarp)
            {
                if (ImGui::SliderFloat("Warp Strength", &m_Settings.WarpStrength, 0.0f, 1.0f, "%.2f"))
                {
                    m_SettingsChanged = true;
                }
            }

            if (ImGui::Checkbox("Apply Terracing", &m_Settings.ApplyTerracing))
            {
                m_SettingsChanged = true;
            }
            ImGui::SetItemTooltip("Creates stepped plateau effect");

            if (m_Settings.ApplyTerracing)
            {
                if (ImGui::SliderInt("Terrace Count", &m_Settings.TerraceCount, 2, 20))
                {
                    m_SettingsChanged = true;
                }
            }

            ImGui::Separator();
        }
    }

    void PCGPanel::DrawErosionSettings()
    {
        if (ImGui::CollapsingHeader("Erosion Settings"))
        {
            ImGui::Text("Hydraulic Erosion:");

            ImGui::SliderInt("Iterations", &m_ErosionSettings.Iterations, 1000, 100000);
            ImGui::SetItemTooltip("Number of water droplet simulations");

            ImGui::SliderInt("Droplet Lifetime", &m_ErosionSettings.MaxDropletLifetime, 10, 100);

            ImGui::SliderFloat("Inertia", &m_ErosionSettings.Inertia, 0.0f, 1.0f, "%.2f");
            ImGui::SetItemTooltip("How much droplets maintain their direction");

            ImGui::SliderFloat("Sediment Capacity", &m_ErosionSettings.SedimentCapacity, 1.0f, 20.0f, "%.1f");
            ImGui::SetItemTooltip("Maximum sediment a droplet can carry");

            ImGui::SliderFloat("Deposit Speed", &m_ErosionSettings.DepositSpeed, 0.1f, 1.0f, "%.2f");
            ImGui::SliderFloat("Erode Speed", &m_ErosionSettings.ErodeSpeed, 0.1f, 1.0f, "%.2f");
            ImGui::SliderFloat("Evaporate Speed", &m_ErosionSettings.EvaporateSpeed, 0.001f, 0.1f, "%.3f");
            ImGui::SliderFloat("Gravity", &m_ErosionSettings.Gravity, 1.0f, 10.0f, "%.1f");
            ImGui::SliderInt("Erosion Radius", &m_ErosionSettings.ErosionRadius, 1, 8);

            ImGui::TextDisabled("(Erosion applied during chunk generation)");

            ImGui::Separator();
        }
    }

    void PCGPanel::DrawPresets()
    {
        if (ImGui::CollapsingHeader("Presets"))
        {
            ImGui::Text("Terrain Types:");

            if (ImGui::Button("Island", ImVec2(100, 0)))
            {
                m_Settings = PCG::HeightmapSettings::Island();
                m_SettingsChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Continent", ImVec2(100, 0)))
            {
                m_Settings = PCG::HeightmapSettings::Continent();
                m_SettingsChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Mountains", ImVec2(100, 0)))
            {
                m_Settings = PCG::HeightmapSettings::Mountains();
                m_SettingsChanged = true;
            }

            ImGui::Text("FBM Presets:");

            if (ImGui::Button("Default Terrain", ImVec2(100, 0)))
            {
                m_Settings.Noise = PCG::FBMSettings::Terrain();
                m_SettingsChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Mountains FBM", ImVec2(100, 0)))
            {
                m_Settings.Noise = PCG::FBMSettings::Mountains();
                m_SettingsChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clouds", ImVec2(100, 0)))
            {
                m_Settings.Noise = PCG::FBMSettings::Clouds();
                m_SettingsChanged = true;
            }

            ImGui::Separator();
        }
    }

    void PCGPanel::DrawActions(PCG::ChunkManager* chunkManager)
    {
        ImGui::Separator();

        // Auto-regenerate option
        ImGui::Checkbox("Auto Regenerate", &m_AutoRegenerate);
        ImGui::SetItemTooltip("Automatically regenerate when settings change");

        ImGui::SameLine();
        ImGui::Checkbox("Preview", &m_ShowPreview);

        // Manual regenerate button
        if (ImGui::Button("Regenerate Terrain", ImVec2(-1, 30)))
        {
            RegenerateTerrain();
        }

        // Apply changes if needed
        if (m_SettingsChanged && chunkManager)
        {
            if (m_AutoRegenerate || m_NeedRegeneration)
            {
                // Update chunk manager settings
                // Note: This would require adding a method to ChunkManager
                // to update settings and regenerate chunks
                // chunkManager->UpdateSettings(m_Settings);

                m_NeedRegeneration = false;
            }
        }

        // Status
        if (m_SettingsChanged)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Settings changed - regenerate to apply");
        }
        else
        {
            ImGui::TextDisabled("Settings applied");
        }
    }

    void PCGPanel::DrawNoisePreview()
    {
        if (ImGui::CollapsingHeader("Noise Preview", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Generate preview data
            if (m_PreviewData.empty() || m_SettingsChanged)
            {
                m_PreviewData.resize(m_PreviewSize * m_PreviewSize);

                PCG::PerlinNoise perlin(m_Settings.Seed);
                PCG::FBM fbm(&perlin);

                for (int y = 0; y < m_PreviewSize; ++y)
                {
                    for (int x = 0; x < m_PreviewSize; ++x)
                    {
                        float nx = static_cast<float>(x) / m_PreviewSize * 10.0f;
                        float ny = static_cast<float>(y) / m_PreviewSize * 10.0f;

                        float value = fbm.Sample(nx, ny, m_Settings.Noise);
                        value = (value + 1.0f) * 0.5f; // Normalize to 0-1

                        m_PreviewData[y * m_PreviewSize + x] = value;
                    }
                }
            }

            // Draw preview as a grayscale image (using plot as approximation)
            ImGui::Text("Preview Size: %dx%d", m_PreviewSize, m_PreviewSize);

            // Display as a histogram-style representation
            // Note: For proper image preview, you'd need to create a DX12 texture
            ImGui::PlotLines("##NoisePreview",
                m_PreviewData.data(),
                m_PreviewSize,
                0, nullptr,
                0.0f, 1.0f,
                ImVec2(-1, 100));

            // Show center row values
            ImGui::Text("Center row noise values (horizontal)");
        }
    }

} // namespace SM
