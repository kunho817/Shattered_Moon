#pragma once

/**
 * @file Engine.h
 * @brief Shattered Moon Engine - Main engine class
 *
 * Core engine class that manages the game loop, initialization,
 * and coordination of all engine subsystems.
 */

#include <memory>
#include <string>
#include <chrono>

namespace SM
{
    // Forward declarations
    class Window;
    class World;

    /**
     * @brief Engine configuration structure
     *
     * Contains all settings needed to initialize the engine.
     */
    struct EngineConfig
    {
        std::wstring windowTitle = L"Shattered Moon";
        int windowWidth = 1280;
        int windowHeight = 720;
        bool vsync = true;
        bool fullscreen = false;
    };

    /**
     * @brief Main engine class
     *
     * Singleton class that manages the entire engine lifecycle.
     * Handles initialization, main loop, and shutdown of all subsystems.
     */
    class Engine
    {
    public:
        /**
         * @brief Get the singleton instance of the engine
         * @return Reference to the engine instance
         */
        static Engine& Get();

        // Prevent copying
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        /**
         * @brief Initialize the engine with the given configuration
         * @param config Engine configuration settings
         * @return true if initialization succeeded, false otherwise
         */
        bool Initialize(const EngineConfig& config = EngineConfig{});

        /**
         * @brief Run the main game loop
         * @return Exit code (0 for success)
         */
        int Run();

        /**
         * @brief Request the engine to shutdown
         */
        void RequestShutdown();

        /**
         * @brief Check if the engine is running
         * @return true if the engine is currently running
         */
        bool IsRunning() const { return m_IsRunning; }

        /**
         * @brief Get the main window
         * @return Pointer to the main window, nullptr if not initialized
         */
        Window* GetWindow() const { return m_Window.get(); }

        /**
         * @brief Get the ECS world
         * @return Pointer to the ECS world, nullptr if not initialized
         */
        World* GetWorld() const { return m_World.get(); }

        /**
         * @brief Get the time elapsed since last frame (delta time)
         * @return Delta time in seconds
         */
        float GetDeltaTime() const { return m_DeltaTime; }

        /**
         * @brief Get the total time since engine started
         * @return Total time in seconds
         */
        float GetTotalTime() const { return m_TotalTime; }

        /**
         * @brief Get the current frames per second
         * @return Current FPS
         */
        float GetFPS() const { return m_FPS; }

    private:
        Engine() = default;
        ~Engine();

        /**
         * @brief Shutdown and cleanup all engine resources
         */
        void Shutdown();

        /**
         * @brief Update game logic
         * @param deltaTime Time since last frame in seconds
         */
        void Update(float deltaTime);

        /**
         * @brief Render the current frame
         */
        void Render();

        /**
         * @brief Calculate timing information (delta time, FPS)
         */
        void CalculateTiming();

        /**
         * @brief Initialize the ECS system and register components
         */
        void InitializeECS();

        /**
         * @brief Run ECS tests (debug builds only)
         */
        void TestECS();

    private:
        // Engine state
        bool m_IsRunning = false;
        bool m_IsInitialized = false;

        // Timing
        std::chrono::high_resolution_clock::time_point m_LastFrameTime;
        std::chrono::high_resolution_clock::time_point m_StartTime;
        float m_DeltaTime = 0.0f;
        float m_TotalTime = 0.0f;
        float m_FPS = 0.0f;
        float m_FPSAccumulator = 0.0f;
        int m_FrameCount = 0;

        // Core systems
        std::unique_ptr<Window> m_Window;
        std::unique_ptr<World> m_World;

        // Configuration
        EngineConfig m_Config;
    };

} // namespace SM
