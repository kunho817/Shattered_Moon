#pragma once

/**
 * @file ImGuiManager.h
 * @brief Dear ImGui integration manager for DX12
 *
 * Singleton class that handles ImGui initialization, shutdown,
 * and frame management for DirectX 12 rendering.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <d3d12.h>
#include <cstdint>

namespace SM
{
    // Forward declarations
    class DX12Core;
    class Renderer;

    /**
     * @brief ImGui color theme enumeration
     */
    enum class ImGuiTheme
    {
        Dark,           ///< Dark theme (default)
        Light,          ///< Light theme
        Classic,        ///< Classic ImGui theme
        ShatteredMoon   ///< Custom Shattered Moon theme
    };

    /**
     * @brief Dear ImGui Manager Singleton
     *
     * Manages the lifecycle of Dear ImGui for DirectX 12 rendering.
     * Handles initialization, frame management, and input processing.
     *
     * Usage:
     * @code
     * // Initialize (usually in Engine::InitializeRenderer)
     * ImGuiManager::Get().Initialize(hwnd, renderer);
     *
     * // In render loop:
     * ImGuiManager::Get().BeginFrame();
     * // ... draw ImGui widgets ...
     * ImGuiManager::Get().EndFrame();
     * ImGuiManager::Get().Render(commandList);
     *
     * // Cleanup
     * ImGuiManager::Get().Shutdown();
     * @endcode
     */
    class ImGuiManager
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the ImGuiManager instance
         */
        static ImGuiManager& Get();

        // Prevent copying
        ImGuiManager(const ImGuiManager&) = delete;
        ImGuiManager& operator=(const ImGuiManager&) = delete;

        // ====================================================================
        // Lifecycle
        // ====================================================================

        /**
         * @brief Initialize ImGui with DX12 and Win32 backends
         * @param hwnd Window handle for input processing
         * @param renderer Renderer instance (provides DX12 resources)
         * @return true if initialization succeeded
         */
        bool Initialize(HWND hwnd, Renderer* renderer);

        /**
         * @brief Initialize ImGui with raw DX12 resources
         * @param hwnd Window handle for input processing
         * @param device DX12 device
         * @param srvHeap Shader resource view descriptor heap
         * @param numFrames Number of frame buffers (typically 2 or 3)
         * @param rtvFormat Render target view format
         * @return true if initialization succeeded
         */
        bool Initialize(
            HWND hwnd,
            ID3D12Device* device,
            ID3D12DescriptorHeap* srvHeap,
            int numFrames,
            DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM
        );

        /**
         * @brief Shutdown and cleanup ImGui resources
         */
        void Shutdown();

        /**
         * @brief Check if ImGui is initialized
         * @return true if initialized and ready to use
         */
        bool IsInitialized() const { return m_Initialized; }

        // ====================================================================
        // Frame Management
        // ====================================================================

        /**
         * @brief Begin a new ImGui frame
         *
         * Call this at the start of your UI rendering, after BeginFrame()
         * in your renderer.
         */
        void BeginFrame();

        /**
         * @brief End the ImGui frame
         *
         * Call this after all ImGui drawing is complete, before EndFrame()
         * in your renderer.
         */
        void EndFrame();

        /**
         * @brief Render ImGui draw data to the command list
         * @param cmdList Command list to record ImGui draw commands
         *
         * Call this after EndFrame() but before executing the command list.
         */
        void Render(ID3D12GraphicsCommandList* cmdList);

        // ====================================================================
        // Input Processing
        // ====================================================================

        /**
         * @brief Process Win32 message for ImGui input
         * @param hwnd Window handle
         * @param msg Message type
         * @param wParam WPARAM
         * @param lParam LPARAM
         * @return true if ImGui handled the message
         */
        LRESULT ProcessWin32Message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        /**
         * @brief Check if ImGui wants to capture keyboard input
         * @return true if ImGui wants keyboard focus
         */
        bool WantsKeyboardInput() const;

        /**
         * @brief Check if ImGui wants to capture mouse input
         * @return true if ImGui wants mouse focus
         */
        bool WantsMouseInput() const;

        // ====================================================================
        // Theme and Styling
        // ====================================================================

        /**
         * @brief Set the color theme
         * @param theme Theme to apply
         */
        void SetTheme(ImGuiTheme theme);

        /**
         * @brief Set dark color theme
         */
        void SetDarkTheme();

        /**
         * @brief Set light color theme
         */
        void SetLightTheme();

        /**
         * @brief Set classic ImGui theme
         */
        void SetClassicTheme();

        /**
         * @brief Set custom Shattered Moon theme
         */
        void SetShatteredMoonTheme();

        /**
         * @brief Get the current theme
         * @return Current theme
         */
        ImGuiTheme GetCurrentTheme() const { return m_CurrentTheme; }

        /**
         * @brief Set global font scale
         * @param scale Font scale multiplier (1.0 = default)
         */
        void SetFontScale(float scale);

        /**
         * @brief Get global font scale
         */
        float GetFontScale() const { return m_FontScale; }

        // ====================================================================
        // Utility
        // ====================================================================

        /**
         * @brief Enable or disable demo window (for debugging)
         * @param show true to show demo window
         */
        void ShowDemoWindow(bool show) { m_ShowDemoWindow = show; }

        /**
         * @brief Check if demo window is enabled
         */
        bool IsDemoWindowVisible() const { return m_ShowDemoWindow; }

        /**
         * @brief Get SRV descriptor heap index used by ImGui
         * @return Heap index for ImGui font texture
         */
        uint32_t GetFontTextureHeapIndex() const { return m_FontTextureHeapIndex; }

    private:
        ImGuiManager() = default;
        ~ImGuiManager();

        /**
         * @brief Setup default style
         */
        void SetupDefaultStyle();

        bool m_Initialized = false;
        ImGuiTheme m_CurrentTheme = ImGuiTheme::Dark;
        float m_FontScale = 1.0f;
        bool m_ShowDemoWindow = false;
        uint32_t m_FontTextureHeapIndex = 0;

        // Stored for cleanup
        HWND m_Hwnd = nullptr;
        ID3D12Device* m_Device = nullptr;
    };

} // namespace SM
