#include "editor/EditorUI.h"
#include "ecs/World.h"
#include "gameplay/Camera.h"
#include "gameplay/CameraController.h"
#include "pcg/ChunkManager.h"
#include "renderer/Renderer.h"

#include <imgui.h>

namespace SM
{
    bool EditorUI::Initialize(HWND hwnd, Renderer* renderer)
    {
        if (m_Initialized)
        {
            return true;
        }

        // Initialize ImGui through the manager
        if (!ImGuiManager::Get().Initialize(hwnd, renderer))
        {
            return false;
        }

        // Apply custom theme
        ImGuiManager::Get().SetTheme(ImGuiTheme::ShatteredMoon);

        // Enable docking
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Log initialization
        ConsolePanel::Get().Log("Editor UI initialized", LogLevel::Info);

        m_Initialized = true;
        return true;
    }

    void EditorUI::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGuiManager::Get().Shutdown();
        m_Initialized = false;
    }

    void EditorUI::BeginFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGuiManager::Get().BeginFrame();
    }

    void EditorUI::Draw(
        World* world,
        GameCamera* camera,
        FPSCameraController* fpsController,
        OrbitCameraController* orbitController,
        PCG::ChunkManager* chunkManager)
    {
        if (!m_Initialized || !m_Visible)
        {
            return;
        }

        // Draw docking space first (covers the entire viewport)
        if (m_UseDocking)
        {
            DrawDockSpace();
        }

        // Draw main menu bar
        DrawMenuBar();

        // Draw panels
        m_StatsPanel.Draw(chunkManager);

        if (camera)
        {
            m_CameraPanel.Draw(camera, fpsController, orbitController);
        }

        m_PCGPanel.Draw(chunkManager);

        if (world)
        {
            m_ECSPanel.Draw(*world);
        }

        ConsolePanel::Get().Draw();

        // Show ImGui demo window if enabled
        if (m_ShowDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);
        }

        // Show about dialog if enabled
        if (m_ShowAboutDialog)
        {
            ShowAboutDialog();
        }
    }

    void EditorUI::EndFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGuiManager::Get().EndFrame();
    }

    void EditorUI::Render(ID3D12GraphicsCommandList* commandList)
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGuiManager::Get().Render(commandList);
    }

    bool EditorUI::WantCaptureMouse() const
    {
        if (!m_Initialized || !m_Visible)
        {
            return false;
        }

        return ImGui::GetIO().WantCaptureMouse;
    }

    bool EditorUI::WantCaptureKeyboard() const
    {
        if (!m_Initialized || !m_Visible)
        {
            return false;
        }

        return ImGui::GetIO().WantCaptureKeyboard;
    }

    void EditorUI::UpdateStats(float deltaTime, float updateTime, float renderTime)
    {
        m_StatsPanel.UpdateFrameTime(deltaTime, updateTime, renderTime);
    }

    void EditorUI::DrawMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            // File menu
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    ConsolePanel::Get().Log("New scene requested", LogLevel::Info);
                }

                if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
                {
                    ConsolePanel::Get().Log("Open scene requested", LogLevel::Info);
                }

                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                {
                    ConsolePanel::Get().Log("Save scene requested", LogLevel::Info);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    PostQuitMessage(0);
                }

                ImGui::EndMenu();
            }

            // Edit menu
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false))
                {
                    // TODO: Implement undo
                }

                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false))
                {
                    // TODO: Implement redo
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Preferences"))
                {
                    ConsolePanel::Get().Log("Preferences dialog requested", LogLevel::Info);
                }

                ImGui::EndMenu();
            }

            // View menu
            if (ImGui::BeginMenu("View"))
            {
                bool statsVisible = m_StatsPanel.IsVisible();
                if (ImGui::MenuItem("Stats Panel", "F3", &statsVisible))
                {
                    m_StatsPanel.SetVisible(statsVisible);
                }

                bool cameraVisible = m_CameraPanel.IsVisible();
                if (ImGui::MenuItem("Camera Panel", nullptr, &cameraVisible))
                {
                    m_CameraPanel.SetVisible(cameraVisible);
                }

                bool pcgVisible = m_PCGPanel.IsVisible();
                if (ImGui::MenuItem("PCG Panel", nullptr, &pcgVisible))
                {
                    m_PCGPanel.SetVisible(pcgVisible);
                }

                bool ecsVisible = m_ECSPanel.IsVisible();
                if (ImGui::MenuItem("ECS Inspector", nullptr, &ecsVisible))
                {
                    m_ECSPanel.SetVisible(ecsVisible);
                }

                bool consoleVisible = ConsolePanel::Get().IsVisible();
                if (ImGui::MenuItem("Console", "`", &consoleVisible))
                {
                    ConsolePanel::Get().SetVisible(consoleVisible);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Show All Panels"))
                {
                    m_StatsPanel.SetVisible(true);
                    m_CameraPanel.SetVisible(true);
                    m_PCGPanel.SetVisible(true);
                    m_ECSPanel.SetVisible(true);
                    ConsolePanel::Get().SetVisible(true);
                }

                if (ImGui::MenuItem("Hide All Panels"))
                {
                    m_StatsPanel.SetVisible(false);
                    m_CameraPanel.SetVisible(false);
                    m_PCGPanel.SetVisible(false);
                    m_ECSPanel.SetVisible(false);
                    ConsolePanel::Get().SetVisible(false);
                }

                ImGui::Separator();

                ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);

                ImGui::EndMenu();
            }

            // Theme menu
            if (ImGui::BeginMenu("Theme"))
            {
                if (ImGui::MenuItem("Shattered Moon"))
                {
                    ImGuiManager::Get().SetTheme(ImGuiManager::Theme::ShatteredMoon);
                }

                if (ImGui::MenuItem("Dark"))
                {
                    ImGuiManager::Get().SetTheme(ImGuiManager::Theme::Dark);
                }

                if (ImGui::MenuItem("Light"))
                {
                    ImGuiManager::Get().SetTheme(ImGuiManager::Theme::Light);
                }

                if (ImGui::MenuItem("Classic"))
                {
                    ImGuiManager::Get().SetTheme(ImGuiManager::Theme::Classic);
                }

                ImGui::EndMenu();
            }

            // Help menu
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                {
                    m_ShowAboutDialog = true;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Keyboard Shortcuts"))
                {
                    ConsolePanel::Get().Log("=== Keyboard Shortcuts ===", LogLevel::Info);
                    ConsolePanel::Get().Log("F1 - Toggle Editor UI", LogLevel::Info);
                    ConsolePanel::Get().Log("F2 - Toggle Camera Mode (FPS/Orbit)", LogLevel::Info);
                    ConsolePanel::Get().Log("F3 - Toggle Stats Panel", LogLevel::Info);
                    ConsolePanel::Get().Log("` - Toggle Console", LogLevel::Info);
                    ConsolePanel::Get().Log("WASD - Move Camera", LogLevel::Info);
                    ConsolePanel::Get().Log("Right Mouse - Rotate Camera", LogLevel::Info);
                }

                ImGui::EndMenu();
            }

            // Right-aligned FPS display
            float fps = 1.0f / ImGui::GetIO().DeltaTime;
            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", fps);

            float textWidth = ImGui::CalcTextSize(fpsText).x;
            ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - 20);
            ImGui::TextDisabled("%s", fpsText);

            ImGui::EndMainMenuBar();
        }
    }

    void EditorUI::DrawDockSpace()
    {
        // Create a full-screen dockspace
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Position below menu bar
        float menuBarHeight = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - menuBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus |
                                        ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        // Create the dockspace
        ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        // Set up initial docking layout
        if (!m_DockspaceInitialized)
        {
            m_DockspaceInitialized = true;

            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

            // Split the dockspace
            ImGuiID dockLeft, dockRight, dockBottom;
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.25f, &dockLeft, &dockRight);
            ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.3f, &dockBottom, &dockRight);

            // Dock windows
            ImGui::DockBuilderDockWindow("Stats", dockLeft);
            ImGui::DockBuilderDockWindow("Camera", dockLeft);
            ImGui::DockBuilderDockWindow("Terrain Generation (PCG)", dockLeft);
            ImGui::DockBuilderDockWindow("ECS Inspector", dockRight);
            ImGui::DockBuilderDockWindow("Console", dockBottom);

            ImGui::DockBuilderFinish(dockspaceId);
        }

        ImGui::End();
    }

    void EditorUI::ShowAboutDialog()
    {
        ImGui::OpenPopup("About Shattered Moon");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("About Shattered Moon", &m_ShowAboutDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Shattered Moon Engine");
            ImGui::Separator();

            ImGui::Text("Version: 0.1.0 (Development)");
            ImGui::Text("Build: DirectX 12");
            ImGui::Spacing();

            ImGui::Text("Features:");
            ImGui::BulletText("DirectX 12 Rendering");
            ImGui::BulletText("Entity Component System (ECS)");
            ImGui::BulletText("Procedural Content Generation (PCG)");
            ImGui::BulletText("Terrain with Chunk System");
            ImGui::BulletText("FBM Noise-based Heightmaps");
            ImGui::BulletText("Dear ImGui Editor");
            ImGui::Spacing();

            ImGui::Text("Libraries:");
            ImGui::BulletText("Dear ImGui %s", IMGUI_VERSION);
            ImGui::BulletText("DirectX 12");
            ImGui::Spacing();

            ImGui::Separator();

            if (ImGui::Button("Close", ImVec2(120, 0)))
            {
                m_ShowAboutDialog = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

} // namespace SM
