#pragma once

/**
 * @file StatsPanel.h
 * @brief Performance statistics panel for the editor
 *
 * Displays real-time performance metrics including FPS, frame times,
 * memory usage, and rendering statistics.
 */

#include <array>
#include <cstdint>
#include <chrono>

namespace SM
{
    // Forward declarations
    class Engine;
    class Renderer;

    namespace PCG
    {
        class ChunkManager;
    }

    /**
     * @brief Performance statistics display panel
     *
     * Provides a visual display of engine performance metrics including:
     * - FPS and frame time
     * - CPU/GPU timing
     * - Memory usage
     * - Draw calls and triangle counts
     * - Chunk loading statistics
     */
    class StatsPanel
    {
    public:
        StatsPanel();
        ~StatsPanel() = default;

        /**
         * @brief Update statistics with new frame data
         * @param deltaTime Time since last frame in seconds
         */
        void Update(float deltaTime);

        /**
         * @brief Update frame timing statistics
         * @param deltaTime Frame delta time in seconds
         * @param updateTime Update time in milliseconds
         * @param renderTime Render time in milliseconds
         */
        void UpdateFrameTime(float deltaTime, float updateTime, float renderTime);

        /**
         * @brief Draw the statistics panel using ImGui
         */
        void Draw();

        /**
         * @brief Draw the statistics panel using ImGui
         * @param chunkManager Optional chunk manager for terrain stats
         */
        void Draw(PCG::ChunkManager* chunkManager);

        /**
         * @brief Set the engine reference for gathering stats
         * @param engine Pointer to engine
         */
        void SetEngine(Engine* engine) { m_Engine = engine; }

        /**
         * @brief Set the renderer reference for rendering stats
         * @param renderer Pointer to renderer
         */
        void SetRenderer(Renderer* renderer) { m_Renderer = renderer; }

        /**
         * @brief Set the chunk manager reference for terrain stats
         * @param chunkManager Pointer to chunk manager
         */
        void SetChunkManager(PCG::ChunkManager* chunkManager) { m_ChunkManager = chunkManager; }

        // ====================================================================
        // Manual stat updates (for external systems)
        // ====================================================================

        /**
         * @brief Set the update time for this frame
         * @param time Update time in milliseconds
         */
        void SetUpdateTime(float time) { m_UpdateTime = time; }

        /**
         * @brief Set the render time for this frame
         * @param time Render time in milliseconds
         */
        void SetRenderTime(float time) { m_RenderTime = time; }

        /**
         * @brief Set draw call count
         * @param count Number of draw calls
         */
        void SetDrawCalls(int count) { m_DrawCalls = count; }

        /**
         * @brief Set triangle count
         * @param count Number of triangles rendered
         */
        void SetTriangleCount(int count) { m_TriangleCount = count; }

        /**
         * @brief Set active chunk count
         * @param count Number of active terrain chunks
         */
        void SetActiveChunks(int count) { m_ActiveChunks = count; }

        /**
         * @brief Set pending chunk count
         * @param count Number of chunks pending generation
         */
        void SetPendingChunks(int count) { m_PendingChunks = count; }

        // ====================================================================
        // Configuration
        // ====================================================================

        /**
         * @brief Enable or disable the panel
         */
        void SetVisible(bool visible) { m_Visible = visible; }

        /**
         * @brief Check if panel is visible
         */
        bool IsVisible() const { return m_Visible; }

        /**
         * @brief Toggle panel visibility
         */
        void ToggleVisible() { m_Visible = !m_Visible; }

        /**
         * @brief Set whether to show the FPS graph
         */
        void SetShowGraph(bool show) { m_ShowGraph = show; }

        /**
         * @brief Set whether to show detailed memory stats
         */
        void SetShowMemoryDetails(bool show) { m_ShowMemoryDetails = show; }

        /**
         * @brief Set whether to show terrain stats
         */
        void SetShowTerrainStats(bool show) { m_ShowTerrainStats = show; }

    private:
        /**
         * @brief Update FPS history for graph display
         */
        void UpdateFPSHistory();

        /**
         * @brief Update memory usage statistics
         */
        void UpdateMemoryStats();

        /**
         * @brief Draw performance section
         */
        void DrawPerformanceSection();

        /**
         * @brief Draw memory section
         */
        void DrawMemorySection();

        /**
         * @brief Draw rendering section
         */
        void DrawRenderingSection();

        /**
         * @brief Draw terrain section
         */
        void DrawTerrainSection();

        /**
         * @brief Draw FPS graph
         */
        void DrawFPSGraph();

        /**
         * @brief Format a byte count as human-readable string
         */
        static const char* FormatBytes(size_t bytes);

    private:
        // References
        Engine* m_Engine = nullptr;
        Renderer* m_Renderer = nullptr;
        PCG::ChunkManager* m_ChunkManager = nullptr;

        // Visibility
        bool m_Visible = true;
        bool m_ShowGraph = true;
        bool m_ShowMemoryDetails = false;
        bool m_ShowTerrainStats = true;

        // Performance statistics
        float m_FPS = 0.0f;
        float m_FrameTime = 0.0f;       // ms
        float m_UpdateTime = 0.0f;       // ms
        float m_RenderTime = 0.0f;       // ms

        // Smoothed values
        float m_SmoothedFPS = 0.0f;
        float m_SmoothedFrameTime = 0.0f;

        // Memory statistics
        size_t m_TotalPhysicalMemory = 0;
        size_t m_AvailablePhysicalMemory = 0;
        size_t m_ProcessMemoryUsage = 0;
        size_t m_PeakMemoryUsage = 0;
        size_t m_VideoMemoryUsage = 0;

        // Rendering statistics
        int m_DrawCalls = 0;
        int m_TriangleCount = 0;
        int m_VertexCount = 0;

        // Terrain statistics
        int m_ActiveChunks = 0;
        int m_PendingChunks = 0;
        int m_VisibleChunks = 0;

        // FPS history for graph
        static constexpr size_t FPS_HISTORY_SIZE = 120;
        std::array<float, FPS_HISTORY_SIZE> m_FPSHistory;
        size_t m_FPSHistoryIndex = 0;
        float m_FPSMin = 0.0f;
        float m_FPSMax = 0.0f;
        float m_FPSAvg = 0.0f;

        // Timing
        float m_StatUpdateTimer = 0.0f;
        static constexpr float STAT_UPDATE_INTERVAL = 0.25f; // Update every 250ms
    };

} // namespace SM
