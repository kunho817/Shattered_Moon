#include "core/Engine.h"
#include "core/Window.h"

#include <iostream>

namespace SM
{
    Engine& Engine::Get()
    {
        static Engine instance;
        return instance;
    }

    Engine::~Engine()
    {
        Shutdown();
    }

    bool Engine::Initialize(const EngineConfig& config)
    {
        if (m_IsInitialized)
        {
            return true;
        }

        m_Config = config;

        // Initialize Window
        m_Window = std::make_unique<Window>();

        WindowConfig windowConfig;
        windowConfig.title = config.windowTitle;
        windowConfig.width = config.windowWidth;
        windowConfig.height = config.windowHeight;
        windowConfig.fullscreen = config.fullscreen;

        if (!m_Window->Initialize(windowConfig))
        {
            return false;
        }

        // Initialize timing
        m_StartTime = std::chrono::high_resolution_clock::now();
        m_LastFrameTime = m_StartTime;

        m_IsInitialized = true;
        m_IsRunning = true;

        return true;
    }

    int Engine::Run()
    {
        if (!m_IsInitialized)
        {
            return -1;
        }

        // Main game loop
        while (m_IsRunning)
        {
            // Process window messages
            if (!m_Window->ProcessMessages())
            {
                m_IsRunning = false;
                break;
            }

            // Calculate timing
            CalculateTiming();

            // Update game logic
            Update(m_DeltaTime);

            // Render frame
            Render();
        }

        Shutdown();
        return 0;
    }

    void Engine::RequestShutdown()
    {
        m_IsRunning = false;
    }

    void Engine::Shutdown()
    {
        if (!m_IsInitialized)
        {
            return;
        }

        // Shutdown in reverse order of initialization

        // Shutdown window
        if (m_Window)
        {
            m_Window->Shutdown();
            m_Window.reset();
        }

        m_IsInitialized = false;
        m_IsRunning = false;
    }

    void Engine::CalculateTiming()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();

        // Calculate delta time
        std::chrono::duration<float> deltaTime = currentTime - m_LastFrameTime;
        m_DeltaTime = deltaTime.count();
        m_LastFrameTime = currentTime;

        // Calculate total time
        std::chrono::duration<float> totalTime = currentTime - m_StartTime;
        m_TotalTime = totalTime.count();

        // Calculate FPS (update once per second)
        m_FrameCount++;
        m_FPSAccumulator += m_DeltaTime;

        if (m_FPSAccumulator >= 1.0f)
        {
            m_FPS = static_cast<float>(m_FrameCount) / m_FPSAccumulator;
            m_FrameCount = 0;
            m_FPSAccumulator = 0.0f;

            // Update window title with FPS (optional, for debugging)
            if (m_Window)
            {
                std::wstring title = m_Config.windowTitle + L" - FPS: " + std::to_wstring(static_cast<int>(m_FPS));
                m_Window->SetTitle(title);
            }
        }
    }

    void Engine::Update(float deltaTime)
    {
        // TODO: Update ECS systems
        // TODO: Update physics
        // TODO: Update audio
        // TODO: Update input

        (void)deltaTime; // Suppress unused parameter warning for now
    }

    void Engine::Render()
    {
        // TODO: Begin frame (DX12)
        // TODO: Render world
        // TODO: Render UI (ImGui)
        // TODO: End frame (DX12)

        // For now, just clear the window with a color
        if (m_Window)
        {
            m_Window->Clear(0.1f, 0.1f, 0.2f); // Dark blue background
        }
    }

} // namespace SM
