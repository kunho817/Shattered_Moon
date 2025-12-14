#include "editor/StatsPanel.h"
#include "core/Engine.h"
#include "renderer/Renderer.h"
#include "pcg/ChunkManager.h"

#include <imgui.h>
#include <algorithm>
#include <numeric>
#include <cstdio>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif

namespace SM
{
    StatsPanel::StatsPanel()
    {
        // Initialize FPS history to zero
        m_FPSHistory.fill(0.0f);

        // Get initial memory stats
        UpdateMemoryStats();
    }

    void StatsPanel::Update(float deltaTime)
    {
        // Calculate FPS and frame time
        m_FrameTime = deltaTime * 1000.0f; // Convert to ms
        m_FPS = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;

        // Apply smoothing (exponential moving average)
        const float smoothingFactor = 0.1f;
        m_SmoothedFPS = m_SmoothedFPS * (1.0f - smoothingFactor) + m_FPS * smoothingFactor;
        m_SmoothedFrameTime = m_SmoothedFrameTime * (1.0f - smoothingFactor) + m_FrameTime * smoothingFactor;

        // Update FPS history
        UpdateFPSHistory();

        // Update memory stats periodically
        m_StatUpdateTimer += deltaTime;
        if (m_StatUpdateTimer >= STAT_UPDATE_INTERVAL)
        {
            m_StatUpdateTimer = 0.0f;
            UpdateMemoryStats();

            // Update terrain stats from chunk manager
            if (m_ChunkManager)
            {
                m_ActiveChunks = static_cast<int>(m_ChunkManager->GetLoadedChunkCount());
                m_PendingChunks = static_cast<int>(m_ChunkManager->GetPendingCount());
                m_VisibleChunks = static_cast<int>(m_ChunkManager->GetVisibleChunks().size());
            }
        }
    }

    void StatsPanel::UpdateFrameTime(float deltaTime, float updateTime, float renderTime)
    {
        m_UpdateTime = updateTime;
        m_RenderTime = renderTime;
        Update(deltaTime);
    }

    void StatsPanel::Draw()
    {
        Draw(nullptr);
    }

    void StatsPanel::Draw(PCG::ChunkManager* chunkManager)
    {
        if (!m_Visible)
        {
            return;
        }

        // Update chunk manager reference if provided
        if (chunkManager)
        {
            m_ChunkManager = chunkManager;
        }

        ImGui::SetNextWindowSize(ImVec2(300, 350), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("Stats", &m_Visible, flags))
        {
            DrawPerformanceSection();

            if (m_ShowGraph)
            {
                DrawFPSGraph();
            }

            DrawRenderingSection();

            if (m_ShowTerrainStats)
            {
                DrawTerrainSection();
            }

            if (m_ShowMemoryDetails)
            {
                DrawMemorySection();
            }
            else
            {
                // Simple memory display
                ImGui::Separator();
                ImGui::Text("Memory: %s", FormatBytes(m_ProcessMemoryUsage));
            }
        }
        ImGui::End();
    }

    void StatsPanel::UpdateFPSHistory()
    {
        m_FPSHistory[m_FPSHistoryIndex] = m_FPS;
        m_FPSHistoryIndex = (m_FPSHistoryIndex + 1) % FPS_HISTORY_SIZE;

        // Calculate min/max/avg
        m_FPSMin = *std::min_element(m_FPSHistory.begin(), m_FPSHistory.end());
        m_FPSMax = *std::max_element(m_FPSHistory.begin(), m_FPSHistory.end());
        m_FPSAvg = std::accumulate(m_FPSHistory.begin(), m_FPSHistory.end(), 0.0f) / FPS_HISTORY_SIZE;
    }

    void StatsPanel::UpdateMemoryStats()
    {
#ifdef _WIN32
        // Get system memory info
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus))
        {
            m_TotalPhysicalMemory = static_cast<size_t>(memStatus.ullTotalPhys);
            m_AvailablePhysicalMemory = static_cast<size_t>(memStatus.ullAvailPhys);
        }

        // Get process memory info
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
        {
            m_ProcessMemoryUsage = pmc.WorkingSetSize;
            m_PeakMemoryUsage = pmc.PeakWorkingSetSize;
        }
#endif
    }

    void StatsPanel::DrawPerformanceSection()
    {
        if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // FPS display with color coding
            ImVec4 fpsColor;
            if (m_SmoothedFPS >= 60.0f)
            {
                fpsColor = ImVec4(0.2f, 0.9f, 0.2f, 1.0f); // Green
            }
            else if (m_SmoothedFPS >= 30.0f)
            {
                fpsColor = ImVec4(0.9f, 0.9f, 0.2f, 1.0f); // Yellow
            }
            else
            {
                fpsColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f); // Red
            }

            ImGui::TextColored(fpsColor, "FPS: %.1f", m_SmoothedFPS);
            ImGui::SameLine();
            ImGui::TextDisabled("(%.2f ms)", m_SmoothedFrameTime);

            // Frame time breakdown
            ImGui::Text("Frame Time: %.2f ms", m_SmoothedFrameTime);
            if (m_UpdateTime > 0.0f || m_RenderTime > 0.0f)
            {
                ImGui::Indent();
                ImGui::Text("Update: %.2f ms", m_UpdateTime);
                ImGui::Text("Render: %.2f ms", m_RenderTime);
                ImGui::Unindent();
            }

            // FPS stats
            ImGui::Text("Min: %.0f | Max: %.0f | Avg: %.0f", m_FPSMin, m_FPSMax, m_FPSAvg);
        }
    }

    void StatsPanel::DrawMemorySection()
    {
        if (ImGui::CollapsingHeader("Memory"))
        {
            ImGui::Text("Process Memory: %s", FormatBytes(m_ProcessMemoryUsage));
            ImGui::Text("Peak Memory: %s", FormatBytes(m_PeakMemoryUsage));

            // Memory usage bar
            if (m_TotalPhysicalMemory > 0)
            {
                float usedPercent = static_cast<float>(m_TotalPhysicalMemory - m_AvailablePhysicalMemory)
                                  / static_cast<float>(m_TotalPhysicalMemory);
                ImGui::Text("System Memory:");
                ImGui::ProgressBar(usedPercent, ImVec2(-1, 0),
                    FormatBytes(m_TotalPhysicalMemory - m_AvailablePhysicalMemory));
                ImGui::Text("Available: %s / %s",
                    FormatBytes(m_AvailablePhysicalMemory),
                    FormatBytes(m_TotalPhysicalMemory));
            }
        }
    }

    void StatsPanel::DrawRenderingSection()
    {
        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Draw Calls: %d", m_DrawCalls);
            ImGui::Text("Triangles: %d", m_TriangleCount);

            if (m_VertexCount > 0)
            {
                ImGui::Text("Vertices: %d", m_VertexCount);
            }

            if (m_Renderer)
            {
                ImGui::Text("Resolution: %dx%d", m_Renderer->GetWidth(), m_Renderer->GetHeight());
                ImGui::Text("Aspect Ratio: %.2f", m_Renderer->GetAspectRatio());
            }
        }
    }

    void StatsPanel::DrawTerrainSection()
    {
        if (ImGui::CollapsingHeader("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Active Chunks: %d", m_ActiveChunks);
            ImGui::Text("Visible Chunks: %d", m_VisibleChunks);
            ImGui::Text("Pending Generation: %d", m_PendingChunks);

            if (m_ChunkManager)
            {
                const auto& config = m_ChunkManager->GetConfig();
                ImGui::Text("View Distance: %.0f", config.ViewDistance);
            }
        }
    }

    void StatsPanel::DrawFPSGraph()
    {
        ImGui::Separator();

        // Prepare data for plotting (reorder to show recent on right)
        std::array<float, FPS_HISTORY_SIZE> orderedHistory;
        for (size_t i = 0; i < FPS_HISTORY_SIZE; ++i)
        {
            size_t srcIndex = (m_FPSHistoryIndex + i) % FPS_HISTORY_SIZE;
            orderedHistory[i] = m_FPSHistory[srcIndex];
        }

        // Plot the graph
        char overlay[32];
        snprintf(overlay, sizeof(overlay), "%.1f FPS", m_SmoothedFPS);

        ImGui::PlotLines("##FPSGraph",
            orderedHistory.data(),
            static_cast<int>(FPS_HISTORY_SIZE),
            0,
            overlay,
            0.0f,
            std::max(m_FPSMax * 1.2f, 120.0f),
            ImVec2(-1, 60));
    }

    const char* StatsPanel::FormatBytes(size_t bytes)
    {
        static char buffer[32];

        if (bytes >= 1024ULL * 1024ULL * 1024ULL)
        {
            snprintf(buffer, sizeof(buffer), "%.2f GB",
                static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
        }
        else if (bytes >= 1024ULL * 1024ULL)
        {
            snprintf(buffer, sizeof(buffer), "%.2f MB",
                static_cast<double>(bytes) / (1024.0 * 1024.0));
        }
        else if (bytes >= 1024ULL)
        {
            snprintf(buffer, sizeof(buffer), "%.2f KB",
                static_cast<double>(bytes) / 1024.0);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%zu B", bytes);
        }

        return buffer;
    }

} // namespace SM
