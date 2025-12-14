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
#include <DirectXMath.h>

namespace PCG
{
    class ChunkManager;
    class TerrainRenderer;
}

namespace SM
{
    // Forward declarations
    class Window;
    class World;
    class MemoryManager;
    class ResourceManager;
    class Renderer;

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

        // Memory configuration
        size_t frameStackSize = 4 * 1024 * 1024;       // 4MB per-frame allocations
        size_t persistentStackSize = 16 * 1024 * 1024; // 16MB persistent allocations

        // Resource configuration
        uint32_t maxAsyncResourceLoads = 4;
        bool enableHotReload = true; // Only in debug builds
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
         * @brief Get the memory manager
         * @return Reference to the memory manager
         */
        MemoryManager& GetMemoryManager();

        /**
         * @brief Get the resource manager
         * @return Reference to the resource manager
         */
        ResourceManager& GetResourceManager();

        /**
         * @brief Get the renderer
         * @return Pointer to the renderer, nullptr if not initialized
         */
        Renderer* GetRenderer() const { return m_Renderer.get(); }

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
         * @brief Render test scene with primitive meshes
         */
        void RenderTestScene();

        /**
         * @brief Calculate timing information (delta time, FPS)
         */
        void CalculateTiming();

        /**
         * @brief Initialize the memory management system
         * @return true if successful
         */
        bool InitializeMemory();

        /**
         * @brief Initialize the resource management system
         * @return true if successful
         */
        bool InitializeResources();

        /**
         * @brief Initialize the ECS system and register components
         */
        void InitializeECS();

        /**
         * @brief Run ECS tests (debug builds only)
         */
        void TestECS();

        /**
         * @brief Test memory and resource systems (debug builds only)
         */
        void TestMemoryAndResources();

        /**
         * @brief Initialize the DX12 renderer
         * @return true if successful
         */
        bool InitializeRenderer();

        /**
         * @brief Handle window resize event
         * @param width New width
         * @param height New height
         */
        void OnWindowResize(uint32_t width, uint32_t height);

        /**
         * @brief Test renderer with primitive meshes (debug builds only)
         */
        void TestRenderer();

        /**
         * @brief Test PCG noise and heightmap generation (debug builds only)
         */
        void TestPCG();

        /**
         * @brief Initialize terrain system
         * @return true if successful
         */
        bool InitializeTerrain();

        /**
         * @brief Update terrain system (chunk loading/unloading)
         * @param cameraPosition Current camera position
         */
        void UpdateTerrain(const DirectX::XMFLOAT3& cameraPosition);

        /**
         * @brief Render procedural terrain
         */
        void RenderTerrain();

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
        std::unique_ptr<Renderer> m_Renderer;

        // Terrain systems
        std::unique_ptr<PCG::ChunkManager> m_ChunkManager;
        std::unique_ptr<PCG::TerrainRenderer> m_TerrainRenderer;
        bool m_TerrainEnabled = true;

        // Configuration
        EngineConfig m_Config;
    };

} // namespace SM
