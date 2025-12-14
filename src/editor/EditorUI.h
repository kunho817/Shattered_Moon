#pragma once

/**
 * @file EditorUI.h
 * @brief Main editor UI container class
 *
 * Integrates all editor panels and provides the main menu bar
 * for the Shattered Moon engine editor.
 */

#include "editor/ImGuiManager.h"
#include "editor/StatsPanel.h"
#include "editor/CameraPanel.h"
#include "editor/PCGPanel.h"
#include "editor/ECSPanel.h"
#include "editor/ConsolePanel.h"

namespace SM
{
    // Forward declarations
    class World;
    class GameCamera;
    class FPSCameraController;
    class OrbitCameraController;

    namespace PCG
    {
        class ChunkManager;
    }

    /**
     * @brief Main editor UI container
     *
     * Manages all editor panels and provides:
     * - Menu bar with view toggles
     * - Panel layout management
     * - Input handling priority
     * - Editor visibility toggle (F1)
     */
    class EditorUI
    {
    public:
        EditorUI() = default;
        ~EditorUI() = default;

        // Prevent copying
        EditorUI(const EditorUI&) = delete;
        EditorUI& operator=(const EditorUI&) = delete;

        /**
         * @brief Initialize the editor UI
         * @param hwnd Window handle
         * @param renderer Renderer instance
         * @return true if initialization succeeded
         */
        bool Initialize(HWND hwnd, class Renderer* renderer);

        /**
         * @brief Shutdown the editor UI
         */
        void Shutdown();

        /**
         * @brief Begin a new editor frame
         */
        void BeginFrame();

        /**
         * @brief Draw all editor UI
         * @param world ECS world for entity inspection
         * @param camera Game camera for camera panel
         * @param fpsController FPS camera controller
         * @param orbitController Orbit camera controller
         * @param chunkManager Chunk manager for terrain panel
         */
        void Draw(
            World* world,
            GameCamera* camera,
            FPSCameraController* fpsController,
            OrbitCameraController* orbitController,
            PCG::ChunkManager* chunkManager);

        /**
         * @brief End the editor frame
         */
        void EndFrame();

        /**
         * @brief Render ImGui draw data
         * @param commandList Command list to record to
         */
        void Render(ID3D12GraphicsCommandList* commandList);

        // ====================================================================
        // Visibility
        // ====================================================================

        /**
         * @brief Check if the editor is visible
         */
        bool IsVisible() const { return m_Visible; }

        /**
         * @brief Set editor visibility
         */
        void SetVisible(bool visible) { m_Visible = visible; }

        /**
         * @brief Toggle editor visibility
         */
        void ToggleVisible() { m_Visible = !m_Visible; }

        // ====================================================================
        // Input
        // ====================================================================

        /**
         * @brief Check if editor wants to capture mouse input
         * @return true if ImGui wants mouse input
         */
        bool WantCaptureMouse() const;

        /**
         * @brief Check if editor wants to capture keyboard input
         * @return true if ImGui wants keyboard input
         */
        bool WantCaptureKeyboard() const;

        // ====================================================================
        // Panel Access
        // ====================================================================

        /**
         * @brief Get the stats panel
         */
        StatsPanel& GetStatsPanel() { return m_StatsPanel; }

        /**
         * @brief Get the camera panel
         */
        CameraPanel& GetCameraPanel() { return m_CameraPanel; }

        /**
         * @brief Get the PCG panel
         */
        PCGPanel& GetPCGPanel() { return m_PCGPanel; }

        /**
         * @brief Get the ECS panel
         */
        ECSPanel& GetECSPanel() { return m_ECSPanel; }

        /**
         * @brief Get the console panel (singleton)
         */
        ConsolePanel& GetConsolePanel() { return ConsolePanel::Get(); }

        // ====================================================================
        // Stats Update
        // ====================================================================

        /**
         * @brief Update frame statistics
         * @param deltaTime Frame delta time
         * @param updateTime Time spent in update
         * @param renderTime Time spent in rendering
         */
        void UpdateStats(float deltaTime, float updateTime, float renderTime);

    private:
        /**
         * @brief Draw the main menu bar
         */
        void DrawMenuBar();

        /**
         * @brief Draw docking space
         */
        void DrawDockSpace();

        /**
         * @brief Show about dialog
         */
        void ShowAboutDialog();

    private:
        bool m_Visible = true;
        bool m_Initialized = false;

        // Panels
        StatsPanel m_StatsPanel;
        CameraPanel m_CameraPanel;
        PCGPanel m_PCGPanel;
        ECSPanel m_ECSPanel;

        // Dialog states
        bool m_ShowAboutDialog = false;
        bool m_ShowDemoWindow = false;

        // Docking
        bool m_UseDocking = true;
        bool m_DockspaceInitialized = false;
    };

} // namespace SM
