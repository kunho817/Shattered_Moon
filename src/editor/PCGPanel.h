#pragma once

/**
 * @file PCGPanel.h
 * @brief PCG (Procedural Content Generation) parameter panel for the editor
 *
 * Provides UI for controlling terrain generation parameters including
 * noise settings, FBM settings, biome configuration, and erosion.
 */

#include "pcg/HeightmapGenerator.h"
#include "pcg/FBM.h"

namespace PCG
{
    class ChunkManager;
}

namespace SM
{
    /**
     * @brief PCG parameter control panel
     *
     * Allows real-time editing of procedural terrain generation parameters:
     * - Noise settings (seed, type, frequency)
     * - FBM settings (octaves, persistence, lacunarity)
     * - Terrain settings (height range, falloff)
     * - Biome configuration
     * - Erosion parameters
     */
    class PCGPanel
    {
    public:
        PCGPanel();
        ~PCGPanel() = default;

        /**
         * @brief Draw the PCG panel using ImGui
         * @param chunkManager Reference to chunk manager for regeneration
         */
        void Draw(PCG::ChunkManager* chunkManager);

        // ====================================================================
        // Settings access
        // ====================================================================

        /**
         * @brief Get current heightmap settings
         */
        PCG::HeightmapSettings& GetSettings() { return m_Settings; }
        const PCG::HeightmapSettings& GetSettings() const { return m_Settings; }

        /**
         * @brief Get current FBM settings
         */
        PCG::FBMSettings& GetFBMSettings() { return m_Settings.Noise; }
        const PCG::FBMSettings& GetFBMSettings() const { return m_Settings.Noise; }

        /**
         * @brief Check if settings were changed since last frame
         */
        bool WereSettingsChanged() const { return m_SettingsChanged; }

        /**
         * @brief Reset settings changed flag
         */
        void ClearSettingsChanged() { m_SettingsChanged = false; }

        // ====================================================================
        // Actions
        // ====================================================================

        /**
         * @brief Trigger terrain regeneration
         */
        void RegenerateTerrain();

        /**
         * @brief Randomize the seed value
         */
        void RandomizeSeed();

        // ====================================================================
        // Visibility
        // ====================================================================

        /**
         * @brief Set panel visibility
         */
        void SetVisible(bool visible) { m_Visible = visible; }

        /**
         * @brief Get panel visibility
         */
        bool IsVisible() const { return m_Visible; }

        /**
         * @brief Toggle visibility
         */
        void ToggleVisible() { m_Visible = !m_Visible; }

    private:
        /**
         * @brief Draw noise settings section
         */
        void DrawNoiseSettings();

        /**
         * @brief Draw FBM settings section
         */
        void DrawFBMSettings();

        /**
         * @brief Draw terrain settings section
         */
        void DrawTerrainSettings();

        /**
         * @brief Draw biome settings section
         */
        void DrawBiomeSettings();

        /**
         * @brief Draw erosion settings section
         */
        void DrawErosionSettings();

        /**
         * @brief Draw preset buttons
         */
        void DrawPresets();

        /**
         * @brief Draw action buttons
         */
        void DrawActions(PCG::ChunkManager* chunkManager);

        /**
         * @brief Draw noise preview visualization
         */
        void DrawNoisePreview();

    private:
        bool m_Visible = true;

        // Current settings
        PCG::HeightmapSettings m_Settings;
        PCG::ErosionSettings m_ErosionSettings;

        // State tracking
        bool m_SettingsChanged = false;
        bool m_NeedRegeneration = false;

        // Preview
        bool m_ShowPreview = false;
        std::vector<float> m_PreviewData;
        int m_PreviewSize = 64;

        // UI state
        bool m_AutoRegenerate = false;
        float m_RegenerateTimer = 0.0f;
        static constexpr float REGENERATE_DELAY = 0.5f;

        // Chunk manager reference (for regeneration)
        PCG::ChunkManager* m_ChunkManager = nullptr;
    };

} // namespace SM
