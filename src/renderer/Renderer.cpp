#include "renderer/Renderer.h"

#include <iostream>
#include <cstring>

namespace SM
{
    Renderer::~Renderer()
    {
        Shutdown();
    }

    bool Renderer::Initialize(HWND hwnd, uint32_t width, uint32_t height, bool vsync)
    {
        std::cout << "[Renderer] Initializing DirectX 12 renderer..." << std::endl;

        // Initialize DX12 core
        if (!m_Core.Initialize(hwnd, width, height, vsync))
        {
            std::cerr << "[Renderer] Failed to initialize DX12 core!" << std::endl;
            return false;
        }

        // Initialize command list
        if (!m_CommandList.Initialize(&m_Core, CommandListType::Direct))
        {
            std::cerr << "[Renderer] Failed to initialize command list!" << std::endl;
            return false;
        }

        // Create shaders
        if (!CreateShaders())
        {
            std::cerr << "[Renderer] Failed to create shaders!" << std::endl;
            return false;
        }

        // Create root signature
        if (!CreateRootSignature())
        {
            std::cerr << "[Renderer] Failed to create root signature!" << std::endl;
            return false;
        }

        // Create pipeline states
        if (!CreatePipelineStates())
        {
            std::cerr << "[Renderer] Failed to create pipeline states!" << std::endl;
            return false;
        }

        // Create constant buffers
        if (!CreateConstantBuffers())
        {
            std::cerr << "[Renderer] Failed to create constant buffers!" << std::endl;
            return false;
        }

        // Create primitive meshes
        if (!CreatePrimitiveMeshes())
        {
            std::cerr << "[Renderer] Failed to create primitive meshes!" << std::endl;
            return false;
        }

        // Create default textures
        if (!CreateDefaultTextures())
        {
            std::cerr << "[Renderer] Failed to create default textures!" << std::endl;
            return false;
        }

        // Initialize default material
        m_DefaultMaterial = CreateDefaultMaterial();

        // Set default camera position
        m_Camera.Position = DirectX::XMFLOAT3(0.0f, 2.0f, -5.0f);
        m_Camera.Target = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

        m_Initialized = true;

        std::cout << "[Renderer] DirectX 12 renderer initialized successfully!" << std::endl;
        return true;
    }

    void Renderer::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        // Wait for GPU to finish
        m_Core.WaitForGPU();

        // Release resources
        m_WhiteTexture.reset();
        m_CubeMesh.reset();
        m_SphereMesh.reset();
        m_PlaneMesh.reset();
        m_CylinderMesh.reset();
        m_ConeMesh.reset();
        m_QuadMesh.reset();

        m_UploadHeap.reset();
        m_MaterialCB.reset();
        m_PerObjectCB.reset();
        m_PerFrameCB.reset();

        // Shutdown DX12 core
        m_Core.Shutdown();

        m_Initialized = false;
        std::cout << "[Renderer] Renderer shutdown complete." << std::endl;
    }

    void Renderer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_Core.Resize(width, height);
    }

    void Renderer::BeginFrame()
    {
        if (m_FrameStarted)
        {
            return;
        }

        // Begin frame in core (wait for previous frame to finish)
        m_Core.BeginFrame();

        // Get command allocator for this frame
        auto& frame = m_Core.GetCurrentFrame();

        // Reset and begin command list
        m_CommandList.Begin(frame.CommandAllocator.Get());

        // Transition back buffer to render target
        m_CommandList.TransitionBarrier(
            m_Core.GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        m_CommandList.FlushBarriers();

        // Set render target
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_Core.GetCurrentRTV();
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_Core.GetDSV();
        m_CommandList.SetRenderTargets(1, &rtv, &dsv);

        // Set viewport and scissor
        SetViewport(m_Core.GetWidth(), m_Core.GetHeight());

        // Set pipeline state
        m_CommandList.SetPipelineState(m_OpaquePSO.GetNative());
        m_CommandList.SetGraphicsRootSignature(m_RootSignature.GetNative());

        // Set descriptor heaps
        ID3D12DescriptorHeap* heaps[] = { m_Core.GetCBVSRVUAVHeap().GetHeap() };
        m_CommandList.SetDescriptorHeaps(1, heaps);

        // Set primitive topology
        m_CommandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Reset upload heap for this frame
        if (m_UploadHeap)
        {
            m_UploadHeap->Reset();
        }

        m_FrameStarted = true;
    }

    void Renderer::EndFrame()
    {
        if (!m_FrameStarted)
        {
            return;
        }

        // Transition back buffer to present
        m_CommandList.TransitionBarrier(
            m_Core.GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );
        m_CommandList.FlushBarriers();

        // Close and execute command list
        m_CommandList.End();
        m_CommandList.Execute();

        m_FrameStarted = false;
    }

    void Renderer::Present()
    {
        m_Core.EndFrame();
    }

    void Renderer::Clear(float r, float g, float b, float a)
    {
        float clearColor[4] = { r, g, b, a };

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_Core.GetCurrentRTV();
        m_CommandList.ClearRenderTarget(rtv, clearColor);

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_Core.GetDSV();
        m_CommandList.ClearDepthStencil(dsv, 1.0f, 0);
    }

    void Renderer::SetViewport(uint32_t width, uint32_t height)
    {
        m_CommandList.SetViewport(width, height);
        m_CommandList.SetScissorRect(width, height);
    }

    void Renderer::SetCamera(const Camera& camera)
    {
        m_Camera = camera;
    }

    void Renderer::UpdateFrameConstants(float totalTime)
    {
        // Calculate matrices
        DirectX::XMMATRIX view = m_Camera.GetViewMatrix();
        DirectX::XMMATRIX proj = m_Camera.GetProjectionMatrix(m_Core.GetAspectRatio());
        DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

        // Store in frame data
        DirectX::XMStoreFloat4x4(&m_FrameData.View, DirectX::XMMatrixTranspose(view));
        DirectX::XMStoreFloat4x4(&m_FrameData.Projection, DirectX::XMMatrixTranspose(proj));
        DirectX::XMStoreFloat4x4(&m_FrameData.ViewProjection, DirectX::XMMatrixTranspose(viewProj));
        m_FrameData.CameraPosition = m_Camera.Position;
        m_FrameData.Time = totalTime;

        // Update constant buffer
        m_PerFrameCB->Update(&m_FrameData, sizeof(PerFrameData));

        // Bind to root signature slot 0
        m_CommandList.SetGraphicsRootConstantBufferView(0, m_PerFrameCB->GetGPUAddress());
    }

    void Renderer::DrawMesh(
        const Mesh& mesh,
        const MaterialData& material,
        const DirectX::XMMATRIX& worldMatrix)
    {
        if (!mesh.IsValid())
        {
            return;
        }

        // Update per-object constant buffer
        PerObjectData objectData;
        DirectX::XMStoreFloat4x4(&objectData.World, DirectX::XMMatrixTranspose(worldMatrix));

        DirectX::XMMATRIX worldInvTranspose = DirectX::XMMatrixTranspose(
            DirectX::XMMatrixInverse(nullptr, worldMatrix)
        );
        DirectX::XMStoreFloat4x4(&objectData.WorldInverseTranspose, DirectX::XMMatrixTranspose(worldInvTranspose));

        m_PerObjectCB->Update(&objectData, sizeof(PerObjectData));

        // Update material constant buffer
        m_MaterialCB->Update(&material, sizeof(MaterialData));

        // Bind constant buffers
        m_CommandList.SetGraphicsRootConstantBufferView(1, m_PerObjectCB->GetGPUAddress());
        m_CommandList.SetGraphicsRootConstantBufferView(2, m_MaterialCB->GetGPUAddress());

        // Bind default texture
        if (m_WhiteTexture && m_WhiteTexture->GetSRV().IsValid())
        {
            m_CommandList.SetGraphicsRootDescriptorTable(3, m_WhiteTexture->GetSRV().GPU);
        }

        // Set vertex and index buffers
        const D3D12_VERTEX_BUFFER_VIEW& vbv = mesh.GetVertexBufferView();
        m_CommandList.SetVertexBuffers(0, 1, &vbv);

        if (mesh.HasIndices())
        {
            const D3D12_INDEX_BUFFER_VIEW& ibv = mesh.GetIndexBufferView();
            m_CommandList.SetIndexBuffer(&ibv);

            // Draw indexed
            m_CommandList.DrawIndexed(mesh.GetIndexCount());
        }
        else
        {
            // Draw non-indexed
            m_CommandList.Draw(mesh.GetVertexCount());
        }
    }

    void Renderer::DrawMesh(const Mesh& mesh, const DirectX::XMMATRIX& worldMatrix)
    {
        DrawMesh(mesh, m_DefaultMaterial, worldMatrix);
    }

    Mesh* Renderer::GetPrimitiveMesh(uint32_t type)
    {
        switch (type)
        {
            case 1: return m_CubeMesh.get();      // PrimitiveMesh::Cube
            case 2: return m_SphereMesh.get();    // PrimitiveMesh::Sphere
            case 3: return m_PlaneMesh.get();     // PrimitiveMesh::Plane
            case 4: return m_CylinderMesh.get();  // PrimitiveMesh::Cylinder
            case 5: return m_ConeMesh.get();      // PrimitiveMesh::Cone
            case 6: return m_QuadMesh.get();      // PrimitiveMesh::Quad
            default: return nullptr;
        }
    }

    bool Renderer::CreateShaders()
    {
        std::cout << "[Renderer] Compiling shaders..." << std::endl;

        // Compile vertex shader
        if (!CompileShaderFromFile(L"shaders/BasicVertex.hlsl", "main", "vs_5_1", m_VertexShader))
        {
            std::cerr << "[Renderer] Failed to compile vertex shader!" << std::endl;
            return false;
        }

        // Compile pixel shader
        if (!CompileShaderFromFile(L"shaders/BasicPixel.hlsl", "main", "ps_5_1", m_PixelShader))
        {
            std::cerr << "[Renderer] Failed to compile pixel shader!" << std::endl;
            return false;
        }

        std::cout << "[Renderer] Shaders compiled successfully." << std::endl;
        return true;
    }

    bool Renderer::CreateRootSignature()
    {
        std::cout << "[Renderer] Creating root signature..." << std::endl;

        if (!CreateBasicRootSignature(&m_Core, m_RootSignature))
        {
            return false;
        }

        std::cout << "[Renderer] Root signature created." << std::endl;
        return true;
    }

    bool Renderer::CreatePipelineStates()
    {
        std::cout << "[Renderer] Creating pipeline states..." << std::endl;

        // Create opaque PSO
        if (!CreateBasicPipelineState(&m_Core, m_RootSignature.GetNative(),
                                       m_VertexShader, m_PixelShader, m_OpaquePSO))
        {
            return false;
        }

        // Create wireframe PSO
        m_WireframePSO
            .Begin()
            .SetRootSignature(m_RootSignature.GetNative())
            .SetVertexShader(m_VertexShader)
            .SetPixelShader(m_PixelShader)
            .SetStandardInputLayout()
            .SetRasterizer(FillMode::Wireframe, CullMode::None)
            .SetBlendMode(BlendMode::Opaque)
            .SetDepthStencil(true, true, DepthFunc::Less)
            .SetRenderTargetFormat(m_Core.GetBackBufferFormat())
            .SetDepthStencilFormat(m_Core.GetDepthFormat())
            .Build(&m_Core);

        std::cout << "[Renderer] Pipeline states created." << std::endl;
        return true;
    }

    bool Renderer::CreateConstantBuffers()
    {
        std::cout << "[Renderer] Creating constant buffers..." << std::endl;

        m_PerFrameCB = std::make_unique<ConstantBuffer>();
        if (!m_PerFrameCB->Initialize(&m_Core, sizeof(PerFrameData)))
        {
            return false;
        }

        m_PerObjectCB = std::make_unique<ConstantBuffer>();
        if (!m_PerObjectCB->Initialize(&m_Core, sizeof(PerObjectData)))
        {
            return false;
        }

        m_MaterialCB = std::make_unique<ConstantBuffer>();
        if (!m_MaterialCB->Initialize(&m_Core, sizeof(MaterialData)))
        {
            return false;
        }

        // Create upload heap for dynamic allocations (2MB)
        m_UploadHeap = std::make_unique<UploadHeap>();
        if (!m_UploadHeap->Initialize(&m_Core, 2 * 1024 * 1024))
        {
            return false;
        }

        std::cout << "[Renderer] Constant buffers created." << std::endl;
        return true;
    }

    bool Renderer::CreatePrimitiveMeshes()
    {
        std::cout << "[Renderer] Creating primitive meshes..." << std::endl;

        // Cube
        m_CubeMesh = std::make_unique<Mesh>();
        MeshData cubeData = MeshGenerator::CreateCube(1.0f);
        if (!m_CubeMesh->Create(&m_Core, cubeData))
        {
            return false;
        }

        // Sphere
        m_SphereMesh = std::make_unique<Mesh>();
        MeshData sphereData = MeshGenerator::CreateSphere(0.5f, 32, 16);
        if (!m_SphereMesh->Create(&m_Core, sphereData))
        {
            return false;
        }

        // Plane
        m_PlaneMesh = std::make_unique<Mesh>();
        MeshData planeData = MeshGenerator::CreatePlane(10.0f, 10.0f, 10, 10);
        if (!m_PlaneMesh->Create(&m_Core, planeData))
        {
            return false;
        }

        // Cylinder
        m_CylinderMesh = std::make_unique<Mesh>();
        MeshData cylinderData = MeshGenerator::CreateCylinder(0.5f, 1.0f, 32, true);
        if (!m_CylinderMesh->Create(&m_Core, cylinderData))
        {
            return false;
        }

        // Cone
        m_ConeMesh = std::make_unique<Mesh>();
        MeshData coneData = MeshGenerator::CreateCone(0.5f, 1.0f, 32, true);
        if (!m_ConeMesh->Create(&m_Core, coneData))
        {
            return false;
        }

        // Quad
        m_QuadMesh = std::make_unique<Mesh>();
        MeshData quadData = MeshGenerator::CreateQuad(1.0f, 1.0f);
        if (!m_QuadMesh->Create(&m_Core, quadData))
        {
            return false;
        }

        std::cout << "[Renderer] Primitive meshes created." << std::endl;
        return true;
    }

    bool Renderer::CreateDefaultTextures()
    {
        std::cout << "[Renderer] Creating default textures..." << std::endl;

        // Create a simple 1x1 white texture
        m_WhiteTexture = std::make_unique<Texture>();

        TextureDesc desc;
        desc.Width = 1;
        desc.Height = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Type = TextureType::Texture2D;
        desc.Usage = TextureUsage::ShaderResource;
        desc.MipLevels = 1;

        if (!m_WhiteTexture->Create(&m_Core, desc, "WhiteTexture"))
        {
            std::cerr << "[Renderer] Failed to create white texture!" << std::endl;
            return false;
        }

        // TODO: Upload white pixel data to texture
        // For now, the texture will be uninitialized but bound

        std::cout << "[Renderer] Default textures created." << std::endl;
        return true;
    }

    // ============================================================================
    // Helper Functions
    // ============================================================================

    MaterialData CreateDefaultMaterial()
    {
        MaterialData material;
        material.BaseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        material.EmissiveColor = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
        material.Metallic = 0.0f;
        material.Roughness = 0.5f;
        material.AmbientOcclusion = 1.0f;
        material.EmissiveIntensity = 0.0f;
        material.UVScale = DirectX::XMFLOAT2(1.0f, 1.0f);
        material.UVOffset = DirectX::XMFLOAT2(0.0f, 0.0f);
        return material;
    }

    MaterialData CreateColoredMaterial(float r, float g, float b, float a)
    {
        MaterialData material = CreateDefaultMaterial();
        material.BaseColor = DirectX::XMFLOAT4(r, g, b, a);
        return material;
    }

    MaterialData CreateMetallicMaterial(float r, float g, float b, float metallic, float roughness)
    {
        MaterialData material = CreateDefaultMaterial();
        material.BaseColor = DirectX::XMFLOAT4(r, g, b, 1.0f);
        material.Metallic = metallic;
        material.Roughness = roughness;
        return material;
    }

} // namespace SM
