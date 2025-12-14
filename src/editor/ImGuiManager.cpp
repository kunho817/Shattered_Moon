#include "editor/ImGuiManager.h"
#include "renderer/Renderer.h"
#include "renderer/DX12Core.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include <iostream>

namespace SM
{
    ImGuiManager& ImGuiManager::Get()
    {
        static ImGuiManager instance;
        return instance;
    }

    ImGuiManager::~ImGuiManager()
    {
        Shutdown();
    }

    bool ImGuiManager::Initialize(HWND hwnd, Renderer* renderer)
    {
        if (!renderer || !renderer->IsInitialized())
        {
            std::cerr << "[ImGuiManager] Renderer not initialized" << std::endl;
            return false;
        }

        DX12Core* core = renderer->GetCore();
        if (!core)
        {
            std::cerr << "[ImGuiManager] DX12Core not available" << std::endl;
            return false;
        }

        return Initialize(
            hwnd,
            core->GetDevice(),
            core->GetCBVSRVUAVHeap().GetHeap(),
            FRAME_BUFFER_COUNT,
            core->GetBackBufferFormat()
        );
    }

    bool ImGuiManager::Initialize(
        HWND hwnd,
        ID3D12Device* device,
        ID3D12DescriptorHeap* srvHeap,
        int numFrames,
        DXGI_FORMAT rtvFormat)
    {
        if (m_Initialized)
        {
            std::cout << "[ImGuiManager] Already initialized" << std::endl;
            return true;
        }

        if (!hwnd || !device || !srvHeap)
        {
            std::cerr << "[ImGuiManager] Invalid parameters" << std::endl;
            return false;
        }

        m_Hwnd = hwnd;
        m_Device = device;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable keyboard navigation
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable gamepad navigation
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable docking (requires imgui docking branch)
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable multi-viewport (requires imgui docking branch)

        // Setup default style
        SetupDefaultStyle();
        SetDarkTheme();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(hwnd))
        {
            std::cerr << "[ImGuiManager] Failed to initialize Win32 backend" << std::endl;
            ImGui::DestroyContext();
            return false;
        }

        // Get descriptor handle for ImGui font texture
        // We use index 0 of the CBV/SRV/UAV heap for ImGui
        // Note: Make sure this index is reserved in DX12Core
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
        m_FontTextureHeapIndex = 0; // Reserved for ImGui

        if (!ImGui_ImplDX12_Init(
            device,
            numFrames,
            rtvFormat,
            srvHeap,
            cpuHandle,
            gpuHandle))
        {
            std::cerr << "[ImGuiManager] Failed to initialize DX12 backend" << std::endl;
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        m_Initialized = true;
        std::cout << "[ImGuiManager] Initialized successfully" << std::endl;

        return true;
    }

    void ImGuiManager::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        m_Initialized = false;
        m_Hwnd = nullptr;
        m_Device = nullptr;

        std::cout << "[ImGuiManager] Shutdown complete" << std::endl;
    }

    void ImGuiManager::BeginFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Show demo window if enabled
        if (m_ShowDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);
        }
    }

    void ImGuiManager::EndFrame()
    {
        if (!m_Initialized)
        {
            return;
        }

        ImGui::EndFrame();
        ImGui::Render();
    }

    void ImGuiManager::Render(ID3D12GraphicsCommandList* cmdList)
    {
        if (!m_Initialized || !cmdList)
        {
            return;
        }

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
    }

    LRESULT ImGuiManager::ProcessWin32Message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!m_Initialized)
        {
            return 0;
        }

        // Forward to ImGui Win32 handler
        extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
    }

    bool ImGuiManager::WantsKeyboardInput() const
    {
        if (!m_Initialized)
        {
            return false;
        }

        return ImGui::GetIO().WantCaptureKeyboard;
    }

    bool ImGuiManager::WantsMouseInput() const
    {
        if (!m_Initialized)
        {
            return false;
        }

        return ImGui::GetIO().WantCaptureMouse;
    }

    void ImGuiManager::SetTheme(ImGuiTheme theme)
    {
        switch (theme)
        {
        case ImGuiTheme::Dark:
            SetDarkTheme();
            break;
        case ImGuiTheme::Light:
            SetLightTheme();
            break;
        case ImGuiTheme::Classic:
            SetClassicTheme();
            break;
        case ImGuiTheme::ShatteredMoon:
            SetShatteredMoonTheme();
            break;
        }
    }

    void ImGuiManager::SetDarkTheme()
    {
        ImGui::StyleColorsDark();
        m_CurrentTheme = ImGuiTheme::Dark;

        // Additional dark theme tweaks
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 0.55f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.30f, 0.40f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.30f, 0.45f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.15f, 1.00f);
    }

    void ImGuiManager::SetLightTheme()
    {
        ImGui::StyleColorsLight();
        m_CurrentTheme = ImGuiTheme::Light;

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
    }

    void ImGuiManager::SetClassicTheme()
    {
        ImGui::StyleColorsClassic();
        m_CurrentTheme = ImGuiTheme::Classic;
    }

    void ImGuiManager::SetShatteredMoonTheme()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Set rounding
        style.WindowRounding = 6.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.TabRounding = 4.0f;

        // Set padding
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.FramePadding = ImVec2(6.0f, 4.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

        // Custom Shattered Moon color scheme (space/moon theme)
        ImVec4* colors = style.Colors;

        // Background colors (deep space black/blue)
        colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.08f, 0.96f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.10f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.06f, 0.06f, 0.10f, 0.98f);

        // Border (subtle glow)
        colors[ImGuiCol_Border] = ImVec4(0.20f, 0.25f, 0.40f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // Frame backgrounds (dark blue-gray)
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.12f, 0.18f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.18f, 0.28f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.25f, 0.40f, 0.67f);

        // Title bar (moon silver/gray)
        colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.08f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.12f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.02f, 0.02f, 0.04f, 0.51f);

        // Menu bar
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.08f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.30f, 0.45f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.40f, 0.55f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.50f, 0.65f, 1.00f);

        // Buttons (accent blue/purple)
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.45f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.35f, 0.60f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.40f, 0.70f, 1.00f);

        // Header
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.45f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.35f, 0.60f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.40f, 0.70f, 1.00f);

        // Separator
        colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.30f, 0.45f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.45f, 0.70f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.50f, 0.55f, 0.80f, 1.00f);

        // Resize grip
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.35f, 0.60f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.45f, 0.70f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.55f, 0.80f, 0.95f);

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.15f, 0.25f, 0.86f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.35f, 0.60f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.30f, 0.50f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.10f, 0.15f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.18f, 0.30f, 1.00f);

        // Plot lines (cyan/green for data visualization)
        colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.80f, 0.90f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.50f, 0.90f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.30f, 0.70f, 0.50f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.80f, 0.60f, 1.00f);

        // Table
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.12f, 0.15f, 0.25f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.30f, 0.45f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

        // Text
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.35f, 0.60f, 0.35f);

        // Misc
        colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.50f, 0.80f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.60f, 0.90f, 1.00f);

        colors[ImGuiCol_NavHighlight] = ImVec4(0.40f, 0.50f, 0.80f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        m_CurrentTheme = ImGuiTheme::ShatteredMoon;
    }

    void ImGuiManager::SetFontScale(float scale)
    {
        m_FontScale = scale;
        if (m_Initialized)
        {
            ImGui::GetIO().FontGlobalScale = scale;
        }
    }

    void ImGuiManager::SetupDefaultStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // General rounding
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.TabRounding = 4.0f;

        // Padding and spacing
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(5.0f, 3.0f);
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.IndentSpacing = 20.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 10.0f;

        // Borders
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        // Alignment
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        // Anti-aliased rendering
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;
    }

} // namespace SM
