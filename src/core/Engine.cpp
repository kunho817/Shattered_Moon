#include "core/Engine.h"
#include "core/Window.h"
#include "core/Memory.h"
#include "core/ResourceManager.h"
#include "core/FileSystem.h"
#include "ecs/ECS.h"
#include "renderer/Renderer.h"
#include "renderer/TerrainRenderer.h"
#include "pcg/PCG.h"

#include <iostream>
#include <iomanip>

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

        // Initialize Memory Management (first, as other systems may use it)
        if (!InitializeMemory())
        {
            std::cerr << "[Engine] Failed to initialize memory management" << std::endl;
            return false;
        }

        // Initialize Resource Management
        if (!InitializeResources())
        {
            std::cerr << "[Engine] Failed to initialize resource management" << std::endl;
            return false;
        }

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

        // Initialize DX12 Renderer
        if (!InitializeRenderer())
        {
            std::cerr << "[Engine] Failed to initialize DX12 renderer" << std::endl;
            return false;
        }

        // Initialize Terrain System
        if (!InitializeTerrain())
        {
            std::cerr << "[Engine] Failed to initialize terrain system" << std::endl;
            // Non-fatal: continue without terrain
            m_TerrainEnabled = false;
        }

        // Initialize timing
        m_StartTime = std::chrono::high_resolution_clock::now();
        m_LastFrameTime = m_StartTime;

        m_IsInitialized = true;
        m_IsRunning = true;

        // Test renderer (debug builds only)
        TestRenderer();

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

    MemoryManager& Engine::GetMemoryManager()
    {
        return MemoryManager::Get();
    }

    ResourceManager& Engine::GetResourceManager()
    {
        return ResourceManager::Get();
    }

    void Engine::Shutdown()
    {
        if (!m_IsInitialized)
        {
            return;
        }

        // Shutdown in reverse order of initialization

        // Shutdown Terrain system first
        if (m_TerrainRenderer)
        {
            m_TerrainRenderer->Shutdown();
            m_TerrainRenderer.reset();
        }

        if (m_ChunkManager)
        {
            m_ChunkManager->Shutdown();
            m_ChunkManager.reset();
        }

        // Shutdown Renderer (before window)
        if (m_Renderer)
        {
            m_Renderer->Shutdown();
            m_Renderer.reset();
        }

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

        // Shutdown Resource Management
        ResourceManager::Get().Shutdown();

        // Shutdown Memory Management (last, as other systems may use it)
        MemoryManager::Get().Shutdown();

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
        // Update Resource Manager (process async loads)
        ResourceManager::Get().Update();

        // Update ECS systems
        if (m_World)
        {
            m_World->Update(deltaTime);
        }

        // TODO: Update physics
        // TODO: Update audio
        // TODO: Update input

        // Clear frame stack at end of frame
        MemoryManager::Get().ClearFrameStack();
    }

    void Engine::Render()
    {
        if (!m_Renderer || !m_Renderer->IsInitialized())
        {
            return;
        }

        // Begin frame
        m_Renderer->BeginFrame();

        // Clear render target with sky blue background
        m_Renderer->Clear(0.4f, 0.6f, 0.9f, 1.0f);

        // Update per-frame constants
        m_Renderer->UpdateFrameConstants(m_TotalTime);

        // Set viewport
        m_Renderer->SetViewport(m_Renderer->GetWidth(), m_Renderer->GetHeight());

        // Render procedural terrain if enabled
        if (m_TerrainEnabled && m_ChunkManager && m_TerrainRenderer)
        {
            RenderTerrain();
        }
        else
        {
            // Fallback: render test scene with primitive meshes
            RenderTestScene();
        }

        // TODO: Render UI (ImGui)

        // End frame
        m_Renderer->EndFrame();

        // Present to screen
        m_Renderer->Present();
    }

    void Engine::RenderTestScene()
    {
        if (!m_Renderer)
        {
            return;
        }

        // Animate camera
        float angle = m_TotalTime * 0.3f;
        Camera& camera = m_Renderer->GetCamera();
        camera.Position = DirectX::XMFLOAT3(
            sinf(angle) * 8.0f,
            3.0f + sinf(m_TotalTime * 0.5f) * 1.0f,
            cosf(angle) * 8.0f
        );
        camera.Target = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_Renderer->SetCamera(camera);

        // Draw a rotating cube at center
        {
            DirectX::XMMATRIX world = DirectX::XMMatrixRotationY(m_TotalTime * 0.5f);
            MaterialData material = CreateColoredMaterial(0.2f, 0.4f, 0.8f);
            material.Metallic = 0.3f;
            material.Roughness = 0.5f;

            Mesh* cube = m_Renderer->GetPrimitiveMesh(0); // Cube
            if (cube)
            {
                m_Renderer->DrawMesh(*cube, material, world);
            }
        }

        // Draw a sphere
        {
            DirectX::XMMATRIX world =
                DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f) *
                DirectX::XMMatrixTranslation(3.0f, 0.0f, 0.0f);
            MaterialData material = CreateMetallicMaterial(0.8f, 0.2f, 0.2f, 0.8f, 0.2f);

            Mesh* sphere = m_Renderer->GetPrimitiveMesh(1); // Sphere
            if (sphere)
            {
                m_Renderer->DrawMesh(*sphere, material, world);
            }
        }

        // Draw a plane (ground)
        {
            DirectX::XMMATRIX world =
                DirectX::XMMatrixScaling(10.0f, 1.0f, 10.0f) *
                DirectX::XMMatrixTranslation(0.0f, -1.5f, 0.0f);
            MaterialData material = CreateColoredMaterial(0.3f, 0.3f, 0.3f);
            material.Roughness = 0.9f;

            Mesh* plane = m_Renderer->GetPrimitiveMesh(2); // Plane
            if (plane)
            {
                m_Renderer->DrawMesh(*plane, material, world);
            }
        }

        // Draw a cylinder
        {
            DirectX::XMMATRIX world =
                DirectX::XMMatrixRotationX(m_TotalTime * 0.3f) *
                DirectX::XMMatrixTranslation(-3.0f, 0.0f, 0.0f);
            MaterialData material = CreateColoredMaterial(0.2f, 0.7f, 0.3f);
            material.Metallic = 0.5f;
            material.Roughness = 0.4f;

            Mesh* cylinder = m_Renderer->GetPrimitiveMesh(3); // Cylinder
            if (cylinder)
            {
                m_Renderer->DrawMesh(*cylinder, material, world);
            }
        }

        // Draw a cone
        {
            DirectX::XMMATRIX world =
                DirectX::XMMatrixTranslation(0.0f, 0.0f, 3.0f);
            MaterialData material = CreateColoredMaterial(0.8f, 0.6f, 0.1f);
            material.Roughness = 0.6f;

            Mesh* cone = m_Renderer->GetPrimitiveMesh(4); // Cone
            if (cone)
            {
                m_Renderer->DrawMesh(*cone, material, world);
            }
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

        // Test memory and resource systems
        TestMemoryAndResources();

        // Test PCG system
        TestPCG();
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

    bool Engine::InitializeMemory()
    {
        return MemoryManager::Get().Initialize(
            m_Config.frameStackSize,
            m_Config.persistentStackSize
        );
    }

    bool Engine::InitializeResources()
    {
        auto& rm = ResourceManager::Get();

        if (!rm.Initialize(m_Config.maxAsyncResourceLoads))
        {
            return false;
        }

#if defined(_DEBUG)
        rm.SetHotReloadEnabled(m_Config.enableHotReload);
#else
        rm.SetHotReloadEnabled(false);
#endif

        return true;
    }

    void Engine::TestMemoryAndResources()
    {
#if defined(_DEBUG)
        std::cout << "\n=== Memory & Resource Test Start ===" << std::endl;

        // Test 1: Pool Allocator
        std::cout << "\n--- Pool Allocator Test ---" << std::endl;
        {
            PoolAllocator pool(64, 10);
            std::cout << "Created pool: " << pool.GetTotalBlockCount() << " blocks of "
                      << pool.GetBlockSize() << " bytes" << std::endl;

            void* ptr1 = pool.Allocate();
            void* ptr2 = pool.Allocate();
            void* ptr3 = pool.Allocate();

            std::cout << "Allocated 3 blocks, free: " << pool.GetFreeBlockCount() << std::endl;

            pool.Deallocate(ptr2);
            std::cout << "Deallocated 1 block, free: " << pool.GetFreeBlockCount() << std::endl;

            void* ptr4 = pool.Allocate();
            std::cout << "Allocated 1 block, reused slot: " << (ptr4 == ptr2 ? "yes" : "no") << std::endl;

            pool.Reset();
            std::cout << "Reset pool, free: " << pool.GetFreeBlockCount() << std::endl;
        }

        // Test 2: Stack Allocator
        std::cout << "\n--- Stack Allocator Test ---" << std::endl;
        {
            StackAllocator stack(1024);
            std::cout << "Created stack: " << stack.GetTotalSize() << " bytes" << std::endl;

            StackMarker marker1 = stack.GetMarker();

            void* ptr1 = stack.Push(100);
            std::cout << "Pushed 100 bytes, used: " << stack.GetUsedSize() << std::endl;

            void* ptr2 = stack.Push(200);
            std::cout << "Pushed 200 bytes, used: " << stack.GetUsedSize() << std::endl;

            StackMarker marker2 = stack.GetMarker();

            void* ptr3 = stack.Push(50);
            std::cout << "Pushed 50 bytes, used: " << stack.GetUsedSize() << std::endl;

            stack.Pop(marker2);
            std::cout << "Popped to marker2, used: " << stack.GetUsedSize() << std::endl;

            stack.Pop(marker1);
            std::cout << "Popped to marker1, used: " << stack.GetUsedSize() << std::endl;

            (void)ptr1; (void)ptr2; (void)ptr3; // Suppress warnings
        }

        // Test 3: Object Pool
        std::cout << "\n--- Object Pool Test ---" << std::endl;
        {
            struct TestObject
            {
                int Value;
                float Data[4];

                TestObject() : Value(0), Data{} {}
                TestObject(int v) : Value(v), Data{} {}
            };

            ObjectPool<TestObject> pool(5, true, 3);
            std::cout << "Created object pool, capacity: " << pool.GetTotalCapacity() << std::endl;

            std::vector<TestObject*> objects;
            for (int i = 0; i < 7; ++i)
            {
                TestObject* obj = pool.Acquire(i * 10);
                objects.push_back(obj);
            }

            std::cout << "Acquired 7 objects, active: " << pool.GetActiveCount()
                      << ", capacity: " << pool.GetTotalCapacity() << std::endl;

            pool.Release(objects[2]);
            pool.Release(objects[4]);
            std::cout << "Released 2 objects, active: " << pool.GetActiveCount() << std::endl;

            TestObject* reused = pool.Acquire(999);
            std::cout << "Acquired new object with value: " << reused->Value << std::endl;

            // Clean up
            for (auto* obj : objects)
            {
                if (obj != objects[2] && obj != objects[4])
                {
                    pool.Release(obj);
                }
            }
            pool.Release(reused);
        }

        // Test 4: Frame Stack (via MemoryManager)
        std::cout << "\n--- Frame Stack Test ---" << std::endl;
        {
            auto* frameStack = MemoryManager::Get().GetFrameStack();
            if (frameStack)
            {
                std::cout << "Frame stack size: " << frameStack->GetTotalSize() << " bytes" << std::endl;

                void* temp = frameStack->Push(1024);
                std::cout << "Allocated 1024 bytes from frame stack" << std::endl;
                (void)temp;

                // This would normally be cleared at end of frame
                frameStack->Clear();
            }
        }

        // Test 5: File System
        std::cout << "\n--- File System Test ---" << std::endl;
        {
            std::string exeDir = FileSystem::GetExecutableDirectory();
            std::cout << "Executable directory: " << exeDir << std::endl;

            std::string assetsDir = FileSystem::GetAssetsDirectory();
            std::cout << "Assets directory: " << assetsDir << std::endl;

            std::string testPath = "test/path/file.png";
            std::cout << "Extension of '" << testPath << "': " << FileSystem::GetExtension(testPath) << std::endl;
            std::cout << "Filename: " << FileSystem::GetFilename(testPath) << std::endl;
            std::cout << "Directory: " << FileSystem::GetDirectory(testPath) << std::endl;
        }

        // Test 6: Resource Manager
        std::cout << "\n--- Resource Manager Test ---" << std::endl;
        {
            auto& rm = ResourceManager::Get();
            std::cout << "Resource manager initialized" << std::endl;
            std::cout << "Loaded resources: " << rm.GetLoadedResourceCount() << std::endl;
            std::cout << "Hot reload enabled: " << (rm.IsHotReloadEnabled() ? "yes" : "no") << std::endl;

            // Test resource type detection
            std::cout << "Type for .png: " << ResourceTypeToString(GetResourceTypeFromExtension(".png")) << std::endl;
            std::cout << "Type for .obj: " << ResourceTypeToString(GetResourceTypeFromExtension(".obj")) << std::endl;
            std::cout << "Type for .hlsl: " << ResourceTypeToString(GetResourceTypeFromExtension(".hlsl")) << std::endl;
            std::cout << "Type for .wav: " << ResourceTypeToString(GetResourceTypeFromExtension(".wav")) << std::endl;
        }

        std::cout << "\n=== Memory & Resource Test Complete ===" << std::endl;
#endif
    }

    bool Engine::InitializeRenderer()
    {
        if (!m_Window)
        {
            std::cerr << "[Engine] Cannot initialize renderer: Window not created" << std::endl;
            return false;
        }

        m_Renderer = std::make_unique<Renderer>();

        bool result = m_Renderer->Initialize(
            m_Window->GetHandle(),
            static_cast<uint32_t>(m_Config.windowWidth),
            static_cast<uint32_t>(m_Config.windowHeight),
            m_Config.vsync
        );

        if (result)
        {
            std::cout << "[Engine] DX12 Renderer initialized successfully" << std::endl;
            std::cout << "[Engine] Render target: " << m_Renderer->GetWidth()
                      << "x" << m_Renderer->GetHeight() << std::endl;
        }

        return result;
    }

    void Engine::OnWindowResize(uint32_t width, uint32_t height)
    {
        if (m_Renderer && m_Renderer->IsInitialized())
        {
            m_Renderer->Resize(width, height);
            std::cout << "[Engine] Window resized to " << width << "x" << height << std::endl;
        }
    }

    void Engine::TestRenderer()
    {
#if defined(_DEBUG)
        if (!m_Renderer || !m_Renderer->IsInitialized())
        {
            return;
        }

        std::cout << "\n=== Renderer Test Start ===" << std::endl;

        std::cout << "Renderer initialized: " << (m_Renderer->IsInitialized() ? "yes" : "no") << std::endl;
        std::cout << "Render target size: " << m_Renderer->GetWidth() << "x" << m_Renderer->GetHeight() << std::endl;
        std::cout << "Aspect ratio: " << m_Renderer->GetAspectRatio() << std::endl;

        // Test primitive mesh access
        std::cout << "\nPrimitive meshes available:" << std::endl;
        const char* meshNames[] = { "Cube", "Sphere", "Plane", "Cylinder", "Cone", "Quad" };
        for (uint32_t i = 0; i < 6; ++i)
        {
            Mesh* mesh = m_Renderer->GetPrimitiveMesh(i);
            if (mesh && mesh->IsValid())
            {
                std::cout << "  " << meshNames[i] << ": "
                          << mesh->GetVertexCount() << " vertices, "
                          << mesh->GetIndexCount() << " indices" << std::endl;
            }
        }

        // Test camera
        const Camera& camera = m_Renderer->GetCamera();
        std::cout << "\nCamera position: ("
                  << camera.Position.x << ", "
                  << camera.Position.y << ", "
                  << camera.Position.z << ")" << std::endl;
        std::cout << "Camera FOV: " << DirectX::XMConvertToDegrees(camera.FieldOfView) << " degrees" << std::endl;

        std::cout << "\n=== Renderer Test Complete ===" << std::endl;
#endif
    }

    void Engine::TestPCG()
    {
#if defined(_DEBUG)
        std::cout << "\n=== PCG (Procedural Content Generation) Test Start ===" << std::endl;

        // Test 1: Basic Noise Algorithms
        std::cout << "\n--- Noise Algorithm Test ---" << std::endl;
        {
            const uint32_t testSeed = 12345;

            // Perlin Noise
            PCG::PerlinNoise perlin(testSeed);
            float perlinValue = perlin.Sample(1.5f, 2.5f);
            std::cout << "Perlin Noise at (1.5, 2.5): " << std::fixed << std::setprecision(4) << perlinValue << std::endl;

            // Simplex Noise
            PCG::SimplexNoise simplex(testSeed);
            float simplexValue = simplex.Sample(1.5f, 2.5f);
            std::cout << "Simplex Noise at (1.5, 2.5): " << std::fixed << std::setprecision(4) << simplexValue << std::endl;

            // Worley Noise
            PCG::WorleyNoise worley(testSeed);
            float worleyValue = worley.Sample(1.5f, 2.5f);
            std::cout << "Worley Noise at (1.5, 2.5): " << std::fixed << std::setprecision(4) << worleyValue << std::endl;

            // Value Noise
            PCG::ValueNoise value(testSeed);
            float valueNoise = value.Sample(1.5f, 2.5f);
            std::cout << "Value Noise at (1.5, 2.5): " << std::fixed << std::setprecision(4) << valueNoise << std::endl;

            // Seed reproducibility test
            PCG::PerlinNoise perlin2(testSeed);
            float perlinValue2 = perlin2.Sample(1.5f, 2.5f);
            std::cout << "Seed reproducibility test: " << (std::abs(perlinValue - perlinValue2) < 0.0001f ? "PASSED" : "FAILED") << std::endl;
        }

        // Test 2: FBM (Fractal Brownian Motion)
        std::cout << "\n--- FBM Test ---" << std::endl;
        {
            PCG::PerlinNoise baseNoise(42);
            PCG::FBM fbm(&baseNoise);

            PCG::FBMSettings settings;
            settings.Octaves = 6;
            settings.Frequency = 0.1f;
            settings.Lacunarity = 2.0f;
            settings.Persistence = 0.5f;

            // Standard FBM
            float fbmValue = fbm.Sample(5.0f, 5.0f, settings);
            std::cout << "FBM at (5.0, 5.0): " << std::fixed << std::setprecision(4) << fbmValue << std::endl;

            // Ridged FBM
            float ridgedValue = fbm.Ridged(5.0f, 5.0f, settings);
            std::cout << "Ridged FBM at (5.0, 5.0): " << std::fixed << std::setprecision(4) << ridgedValue << std::endl;

            // Turbulence
            float turbulenceValue = fbm.Turbulence(5.0f, 5.0f, settings);
            std::cout << "Turbulence at (5.0, 5.0): " << std::fixed << std::setprecision(4) << turbulenceValue << std::endl;

            // Billow
            float billowValue = fbm.Billow(5.0f, 5.0f, settings);
            std::cout << "Billow at (5.0, 5.0): " << std::fixed << std::setprecision(4) << billowValue << std::endl;

            // Domain Warped
            float warpedValue = fbm.WarpedSample(5.0f, 5.0f, settings, 0.5f);
            std::cout << "Domain Warped at (5.0, 5.0): " << std::fixed << std::setprecision(4) << warpedValue << std::endl;
        }

        // Test 3: Heightmap Generation
        std::cout << "\n--- Heightmap Generation Test ---" << std::endl;
        {
            PCG::HeightmapSettings settings;
            settings.Seed = 54321;
            settings.Width = 64;
            settings.Height = 64;
            settings.Noise = PCG::FBMSettings::Terrain();
            settings.MinHeight = 0.0f;
            settings.MaxHeight = 100.0f;
            settings.ApplyFalloffMap = true;

            PCG::HeightmapGenerator generator;
            std::vector<float> heightmap = generator.Generate(settings);

            // Calculate statistics
            float minH = heightmap[0], maxH = heightmap[0], avgH = 0.0f;
            for (float h : heightmap)
            {
                minH = std::min(minH, h);
                maxH = std::max(maxH, h);
                avgH += h;
            }
            avgH /= static_cast<float>(heightmap.size());

            std::cout << "Generated heightmap: " << settings.Width << "x" << settings.Height << std::endl;
            std::cout << "Height range: [" << std::fixed << std::setprecision(2)
                      << minH << ", " << maxH << "]" << std::endl;
            std::cout << "Average height: " << avgH << std::endl;

            // ASCII visualization of a small section (8x8 from center)
            std::cout << "\nHeightmap preview (8x8 center region):" << std::endl;
            const char* shades = " .:-=+*#%@";
            int startX = settings.Width / 2 - 4;
            int startY = settings.Height / 2 - 4;

            for (int y = startY; y < startY + 8; ++y)
            {
                std::cout << "  ";
                for (int x = startX; x < startX + 8; ++x)
                {
                    float h = heightmap[y * settings.Width + x];
                    float normalized = (h - minH) / (maxH - minH + 0.001f);
                    int shadeIdx = static_cast<int>(normalized * 9.0f);
                    shadeIdx = std::clamp(shadeIdx, 0, 9);
                    std::cout << shades[shadeIdx] << shades[shadeIdx];
                }
                std::cout << std::endl;
            }

            // Test erosion
            std::cout << "\nApplying hydraulic erosion (1000 iterations)..." << std::endl;
            PCG::ErosionSettings erosionSettings;
            erosionSettings.Iterations = 1000;
            generator.ApplyErosion(heightmap, settings.Width, settings.Height, erosionSettings);

            // Recalculate stats after erosion
            minH = heightmap[0]; maxH = heightmap[0]; avgH = 0.0f;
            for (float h : heightmap)
            {
                minH = std::min(minH, h);
                maxH = std::max(maxH, h);
                avgH += h;
            }
            avgH /= static_cast<float>(heightmap.size());
            std::cout << "After erosion - Height range: [" << std::fixed << std::setprecision(2)
                      << minH << ", " << maxH << "], Avg: " << avgH << std::endl;
        }

        // Test 4: Biome Classification
        std::cout << "\n--- Biome System Test ---" << std::endl;
        {
            PCG::BiomeMap biomeMap;
            std::cout << "Water level: " << biomeMap.GetWaterLevel() << std::endl;

            // Test biome classification at different heights
            float testHeights[] = {0.1f, 0.25f, 0.35f, 0.5f, 0.7f, 0.9f};
            std::cout << "Biome classification by height:" << std::endl;
            for (float h : testHeights)
            {
                PCG::BiomeType biome = biomeMap.GetBiome(h);
                std::cout << "  Height " << std::fixed << std::setprecision(2)
                          << h << " -> " << PCG::BiomeTypeToString(biome) << std::endl;
            }

            // Test biome with moisture
            std::cout << "\nBiome classification with moisture:" << std::endl;
            struct TestCase { float height; float moisture; };
            TestCase testCases[] = {
                {0.4f, 0.1f},  // Dry lowland
                {0.4f, 0.5f},  // Medium moisture
                {0.4f, 0.9f},  // Wet lowland
                {0.7f, 0.5f},  // Highland
            };

            for (const auto& tc : testCases)
            {
                PCG::BiomeType biome = biomeMap.GetBiome(tc.height, tc.moisture);
                float r, g, b, a;
                biomeMap.GetBiomeColor(tc.height, tc.moisture, 0.5f, r, g, b, a);

                std::cout << "  Height=" << std::fixed << std::setprecision(2) << tc.height
                          << ", Moisture=" << tc.moisture
                          << " -> " << PCG::BiomeTypeToString(biome)
                          << " (Color: " << std::setprecision(2) << r << "," << g << "," << b << ")"
                          << std::endl;
            }

            // Test biome data properties
            const PCG::BiomeData& forestData = biomeMap.GetBiomeData(PCG::BiomeType::Forest);
            std::cout << "\nForest biome properties:" << std::endl;
            std::cout << "  Name: " << forestData.Name << std::endl;
            std::cout << "  Height range: [" << forestData.MinHeight << ", " << forestData.MaxHeight << "]" << std::endl;
            std::cout << "  Movement speed: " << forestData.MovementSpeed << std::endl;
            std::cout << "  Resource density: " << forestData.ResourceDensity << std::endl;
        }

        // Test 5: Noise Factory
        std::cout << "\n--- Noise Factory Test ---" << std::endl;
        {
            auto perlin = PCG::NoiseFactory::Create(PCG::NoiseFactory::NoiseType::Perlin, 100);
            auto simplex = PCG::NoiseFactory::Create(PCG::NoiseFactory::NoiseType::Simplex, 100);
            auto worley = PCG::NoiseFactory::Create(PCG::NoiseFactory::NoiseType::Worley, 100);

            std::cout << "Created noise generators via factory:" << std::endl;
            std::cout << "  Perlin seed: " << perlin->GetSeed() << std::endl;
            std::cout << "  Simplex seed: " << simplex->GetSeed() << std::endl;
            std::cout << "  Worley seed: " << worley->GetSeed() << std::endl;
        }

        // Test 6: FBM Presets
        std::cout << "\n--- FBM Presets Test ---" << std::endl;
        {
            auto terrainFBM = PCG::FBMPresets::CreateTerrainFBM(777);
            auto cloudFBM = PCG::FBMPresets::CreateCloudFBM(888);
            auto detailFBM = PCG::FBMPresets::CreateDetailFBM(999);

            PCG::FBMSettings settings = PCG::FBMSettings::Terrain();

            float terrainValue = terrainFBM->Sample(10.0f, 10.0f, settings);
            float cloudValue = cloudFBM->Sample(10.0f, 10.0f, PCG::FBMSettings::Clouds());
            float detailValue = detailFBM->Sample(10.0f, 10.0f, settings);

            std::cout << "FBM preset samples at (10.0, 10.0):" << std::endl;
            std::cout << "  Terrain: " << std::fixed << std::setprecision(4) << terrainValue << std::endl;
            std::cout << "  Cloud: " << cloudValue << std::endl;
            std::cout << "  Detail: " << detailValue << std::endl;
        }

        std::cout << "\n=== PCG Test Complete ===" << std::endl;
#endif
    }

    // ============================================================================
    // Terrain System
    // ============================================================================

    bool Engine::InitializeTerrain()
    {
        if (!m_Renderer || !m_Renderer->IsInitialized())
        {
            std::cerr << "[Engine] Cannot initialize terrain: Renderer not ready" << std::endl;
            return false;
        }

        std::cout << "[Engine] Initializing terrain system..." << std::endl;

        // Create terrain renderer
        m_TerrainRenderer = std::make_unique<PCG::TerrainRenderer>();
        if (!m_TerrainRenderer->Initialize(*m_Renderer))
        {
            std::cerr << "[Engine] Failed to initialize terrain renderer" << std::endl;
            return false;
        }

        // Configure terrain renderer
        m_TerrainRenderer->SetHeightScale(50.0f);
        m_TerrainRenderer->SetTextureScale(10.0f);
        m_TerrainRenderer->SetFog(100.0f, 400.0f, DirectX::XMFLOAT4(0.6f, 0.75f, 0.9f, 1.0f));

        // Create chunk manager
        m_ChunkManager = std::make_unique<PCG::ChunkManager>();

        // Configure chunk manager
        PCG::ChunkManagerConfig chunkConfig;
        chunkConfig.ViewDistance = 200.0f;
        chunkConfig.UnloadDistance = 250.0f;
        chunkConfig.MaxChunksPerFrame = 2;
        chunkConfig.MaxMeshBuildsPerFrame = 4;

        // Configure terrain generation settings
        chunkConfig.TerrainSettings.Seed = 42;
        chunkConfig.TerrainSettings.Width = PCG::Chunk::SIZE;
        chunkConfig.TerrainSettings.Height = PCG::Chunk::SIZE;
        chunkConfig.TerrainSettings.MinHeight = 0.0f;
        chunkConfig.TerrainSettings.MaxHeight = 50.0f;
        chunkConfig.TerrainSettings.Noise = PCG::FBMSettings::Terrain();
        chunkConfig.TerrainSettings.Noise.Frequency = 0.01f;
        chunkConfig.TerrainSettings.ApplyDomainWarp = true;
        chunkConfig.TerrainSettings.WarpStrength = 0.2f;

        if (!m_ChunkManager->Initialize(m_Renderer->GetCore(), chunkConfig))
        {
            std::cerr << "[Engine] Failed to initialize chunk manager" << std::endl;
            return false;
        }

        // Force load initial chunks around origin
        m_ChunkManager->ForceLoadAround(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), 100.0f);

        std::cout << "[Engine] Terrain system initialized successfully!" << std::endl;
        std::cout << "[Engine] Initial chunks loaded: " << m_ChunkManager->GetLoadedChunkCount() << std::endl;

        return true;
    }

    void Engine::UpdateTerrain(const DirectX::XMFLOAT3& cameraPosition)
    {
        if (!m_ChunkManager)
        {
            return;
        }

        m_ChunkManager->Update(cameraPosition);
    }

    void Engine::RenderTerrain()
    {
        if (!m_TerrainRenderer || !m_ChunkManager || !m_Renderer)
        {
            return;
        }

        // Animate camera to fly over terrain
        float angle = m_TotalTime * 0.1f;
        float radius = 80.0f;
        float height = 30.0f + std::sin(m_TotalTime * 0.3f) * 10.0f;

        Camera& camera = m_Renderer->GetCamera();
        camera.Position = DirectX::XMFLOAT3(
            std::cos(angle) * radius,
            height,
            std::sin(angle) * radius
        );
        camera.Target = DirectX::XMFLOAT3(0.0f, 10.0f, 0.0f);
        camera.FarPlane = 500.0f;
        m_Renderer->SetCamera(camera);

        // Update terrain chunks based on camera position
        UpdateTerrain(camera.Position);

        // Calculate view-projection matrix
        DirectX::XMMATRIX view = camera.GetViewMatrix();
        DirectX::XMMATRIX proj = camera.GetProjectionMatrix(m_Renderer->GetAspectRatio());
        DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

        // Render terrain
        m_TerrainRenderer->RenderTerrain(*m_ChunkManager, viewProj, camera.Position);

#if defined(_DEBUG)
        // Debug output (every 60 frames)
        static int frameCounter = 0;
        if (++frameCounter >= 60)
        {
            frameCounter = 0;
            // Uncomment to see terrain stats:
            // std::cout << "[Terrain] Chunks: " << m_ChunkManager->GetLoadedChunkCount()
            //           << ", Pending: " << m_ChunkManager->GetPendingCount()
            //           << ", Rendered: " << m_TerrainRenderer->GetRenderedChunkCount()
            //           << ", Triangles: " << m_TerrainRenderer->GetRenderedTriangleCount()
            //           << std::endl;
        }
#endif
    }

} // namespace SM
