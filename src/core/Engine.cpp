#include "core/Engine.h"
#include "core/Window.h"
#include "ecs/ECS.h"

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

        // Initialize ECS World
        m_World = std::make_unique<World>();
        InitializeECS();

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

        // Shutdown ECS
        if (m_World)
        {
            m_World->ShutdownSystems();
            m_World.reset();
        }

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
        // Update ECS systems
        if (m_World)
        {
            m_World->Update(deltaTime);
        }

        // TODO: Update physics
        // TODO: Update audio
        // TODO: Update input
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

    void Engine::InitializeECS()
    {
        if (!m_World)
        {
            return;
        }

        // Register basic component types
        m_World->RegisterComponent<TransformComponent>();
        m_World->RegisterComponent<MeshComponent>();
        m_World->RegisterComponent<MaterialComponent>();
        m_World->RegisterComponent<TagComponent>();
        m_World->RegisterComponent<VelocityComponent>();
        m_World->RegisterComponent<CameraComponent>();
        m_World->RegisterComponent<LightComponent>();

        // Initialize systems
        m_World->InitializeSystems();

        // ECS Test: Create some test entities
        TestECS();
    }

    void Engine::TestECS()
    {
        if (!m_World)
        {
            return;
        }

#if defined(_DEBUG)
        std::cout << "=== ECS Test Start ===" << std::endl;

        // Test 1: Create entity
        EntityID entity1 = m_World->CreateEntity("TestCube");
        std::cout << "Created entity: " << entity1 << std::endl;

        // Test 2: Add components
        m_World->AddComponent(entity1, TransformComponent{
            Vector3(0.0f, 0.0f, 5.0f),    // Position
            Quaternion::Identity(),        // Rotation
            Vector3::One()                 // Scale
        });

        m_World->AddComponent(entity1, MeshComponent{ PrimitiveMesh::Cube });
        m_World->AddComponent(entity1, MaterialComponent{ Color::Blue() });

        std::cout << "Added Transform, Mesh, Material components" << std::endl;

        // Test 3: Query component
        TransformComponent& transform = m_World->GetComponent<TransformComponent>(entity1);
        std::cout << "Entity position: ("
                  << transform.Position.x << ", "
                  << transform.Position.y << ", "
                  << transform.Position.z << ")" << std::endl;

        // Test 4: HasComponent check
        bool hasTransform = m_World->HasComponent<TransformComponent>(entity1);
        bool hasCamera = m_World->HasComponent<CameraComponent>(entity1);
        std::cout << "Has Transform: " << (hasTransform ? "true" : "false") << std::endl;
        std::cout << "Has Camera: " << (hasCamera ? "true" : "false") << std::endl;

        // Test 5: Create more entities
        EntityID entity2 = m_World->CreateEntity("TestSphere");
        m_World->AddComponent(entity2, TransformComponent{ Vector3(3.0f, 0.0f, 5.0f) });
        m_World->AddComponent(entity2, MeshComponent{ PrimitiveMesh::Sphere });
        m_World->AddComponent(entity2, MaterialComponent{ Color::Red() });

        EntityID entity3 = m_World->CreateEntity("MainCamera");
        m_World->AddComponent(entity3, TransformComponent{ Vector3(0.0f, 2.0f, -5.0f) });
        m_World->AddComponent(entity3, CameraComponent{});

        std::cout << "Total entities: " << m_World->GetEntityCount() << std::endl;

        // Test 6: ForEach query
        std::cout << "Entities with Transform and Mesh:" << std::endl;
        m_World->ForEach<TransformComponent, MeshComponent>(
            [](EntityID entity, TransformComponent& t, MeshComponent& m) {
                std::cout << "  Entity " << entity << " - MeshID: " << m.MeshId
                          << " at (" << t.Position.x << ", " << t.Position.y << ", " << t.Position.z << ")"
                          << std::endl;
            }
        );

        // Test 7: Find entity by name
        EntityID found = m_World->FindEntityByName("TestSphere");
        std::cout << "Found 'TestSphere' at entity ID: " << found << std::endl;

        // Test 8: Remove component
        m_World->RemoveComponent<MaterialComponent>(entity1);
        bool hasMaterial = m_World->HasComponent<MaterialComponent>(entity1);
        std::cout << "After removal, entity1 has Material: " << (hasMaterial ? "true" : "false") << std::endl;

        // Test 9: IsAlive check
        std::cout << "Entity1 is alive: " << (m_World->IsAlive(entity1) ? "true" : "false") << std::endl;

        // Test 10: Destroy entity
        m_World->DestroyEntity(entity2);
        std::cout << "After destroying entity2, total entities: " << m_World->GetEntityCount() << std::endl;
        std::cout << "Entity2 is alive: " << (m_World->IsAlive(entity2) ? "true" : "false") << std::endl;

        std::cout << "=== ECS Test Complete ===" << std::endl;
#endif
    }

} // namespace SM
