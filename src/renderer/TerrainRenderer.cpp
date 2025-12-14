#include "renderer/TerrainRenderer.h"
#include "renderer/Renderer.h"
#include "pcg/Chunk.h"
#include "pcg/ChunkManager.h"

#include <iostream>

namespace PCG
{
    TerrainRenderer::TerrainRenderer()
    {
        // Initialize default light direction (normalized)
        float len = std::sqrt(0.5f * 0.5f + 0.8f * 0.8f + 0.3f * 0.3f);
        m_LightDirection = DirectX::XMFLOAT3(0.5f / len, -0.8f / len, 0.3f / len);
    }

    TerrainRenderer::~TerrainRenderer()
    {
        Shutdown();
    }

    // ============================================================================
    // Initialization
    // ============================================================================

    bool TerrainRenderer::Initialize(SM::Renderer& renderer)
    {
        if (m_Initialized)
        {
            return true;
        }

        m_Renderer = &renderer;
        m_Core = renderer.GetCore();

        if (!m_Core)
        {
            std::cerr << "[TerrainRenderer] Cannot initialize: DX12Core is null" << std::endl;
            return false;
        }

        std::cout << "[TerrainRenderer] Initializing terrain renderer..." << std::endl;

        // Create shaders
        if (!CreateShaders())
        {
            std::cerr << "[TerrainRenderer] Failed to create terrain shaders!" << std::endl;
            return false;
        }

        // Create root signature
        if (!CreateRootSignature())
        {
            std::cerr << "[TerrainRenderer] Failed to create root signature!" << std::endl;
            return false;
        }

        // Create pipeline states
        if (!CreatePipelineState())
        {
            std::cerr << "[TerrainRenderer] Failed to create pipeline state!" << std::endl;
            return false;
        }

        // Create constant buffers
        if (!CreateConstantBuffers())
        {
            std::cerr << "[TerrainRenderer] Failed to create constant buffers!" << std::endl;
            return false;
        }

        m_Initialized = true;
        std::cout << "[TerrainRenderer] Terrain renderer initialized successfully!" << std::endl;

        return true;
    }

    void TerrainRenderer::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        m_PerFrameCB.reset();
        m_PerChunkCB.reset();

        m_Initialized = false;
        m_Renderer = nullptr;
        m_Core = nullptr;

        std::cout << "[TerrainRenderer] Shutdown complete" << std::endl;
    }

    // ============================================================================
    // Configuration
    // ============================================================================

    void TerrainRenderer::SetHeightScale(float scale)
    {
        m_Config.HeightScale = scale;
    }

    void TerrainRenderer::SetTextureScale(float scale)
    {
        m_Config.TextureScale = scale;
    }

    void TerrainRenderer::SetFog(float start, float end, const DirectX::XMFLOAT4& color)
    {
        m_Config.FogStart = start;
        m_Config.FogEnd = end;
        m_Config.FogColor = color;
    }

    void TerrainRenderer::SetFogEnabled(bool enabled)
    {
        m_Config.EnableFog = enabled;
    }

    void TerrainRenderer::SetWireframe(bool enabled)
    {
        m_Config.EnableWireframe = enabled;
    }

    // ============================================================================
    // Rendering
    // ============================================================================

    void TerrainRenderer::BeginTerrainPass()
    {
        if (!m_Initialized || !m_Renderer)
        {
            return;
        }

        // Get the command list from the renderer
        ID3D12GraphicsCommandList* cmdList = m_Renderer->GetCommandList();
        if (!cmdList)
        {
            return;
        }

        // Set pipeline state
        if (m_Config.EnableWireframe)
        {
            cmdList->SetPipelineState(m_WireframePSO.GetNative());
        }
        else
        {
            cmdList->SetPipelineState(m_TerrainPSO.GetNative());
        }

        // Set root signature
        cmdList->SetGraphicsRootSignature(m_RootSignature.GetNative());

        // Set primitive topology
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    void TerrainRenderer::EndTerrainPass()
    {
        // Currently nothing special to do
    }

    void TerrainRenderer::UpdateFrameConstants(const DirectX::XMMATRIX& viewProjection,
                                                const DirectX::XMFLOAT3& cameraPosition,
                                                float time)
    {
        // Store view-projection matrix (transposed for HLSL)
        DirectX::XMStoreFloat4x4(&m_FrameData.ViewProjection, DirectX::XMMatrixTranspose(viewProjection));

        m_FrameData.CameraPosition = cameraPosition;
        m_FrameData.HeightScale = m_Config.HeightScale;
        m_FrameData.LightDirection = m_LightDirection;
        m_FrameData.Time = time;
        m_FrameData.FogColor = m_Config.FogColor;
        m_FrameData.FogStart = m_Config.EnableFog ? m_Config.FogStart : 999999.0f;
        m_FrameData.FogEnd = m_Config.EnableFog ? m_Config.FogEnd : 999999.0f;

        // Update constant buffer
        if (m_PerFrameCB)
        {
            m_PerFrameCB->Update(&m_FrameData, sizeof(TerrainPerFrameData));
        }
    }

    void TerrainRenderer::RenderChunk(const Chunk& chunk, const DirectX::XMMATRIX& viewProjection)
    {
        if (!m_Initialized || !chunk.HasMesh())
        {
            return;
        }

        ID3D12GraphicsCommandList* cmdList = m_Renderer->GetCommandList();
        if (!cmdList)
        {
            return;
        }

        // Update per-chunk constants
        TerrainPerChunkData chunkData;

        // Identity world matrix (chunks are already in world space)
        DirectX::XMStoreFloat4x4(&chunkData.World, DirectX::XMMatrixIdentity());

        DirectX::XMFLOAT3 worldPos = chunk.GetWorldPosition();
        chunkData.ChunkOffset = DirectX::XMFLOAT2(worldPos.x, worldPos.z);
        chunkData.TextureScale = m_Config.TextureScale;
        chunkData.MinHeight = chunk.GetMinHeight();
        chunkData.MaxHeight = chunk.GetMaxHeight();

        m_PerChunkCB->Update(&chunkData, sizeof(TerrainPerChunkData));

        // Bind constant buffers
        cmdList->SetGraphicsRootConstantBufferView(0, m_PerFrameCB->GetGPUAddress());
        cmdList->SetGraphicsRootConstantBufferView(1, m_PerChunkCB->GetGPUAddress());

        // Get mesh and render
        const SM::Mesh& mesh = chunk.GetMesh();

        // Set vertex buffer
        D3D12_VERTEX_BUFFER_VIEW vbv = mesh.GetVertexBufferView();
        cmdList->IASetVertexBuffers(0, 1, &vbv);

        // Set index buffer and draw
        if (mesh.HasIndices())
        {
            D3D12_INDEX_BUFFER_VIEW ibv = mesh.GetIndexBufferView();
            cmdList->IASetIndexBuffer(&ibv);
            cmdList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);

            // Update statistics
            m_RenderedTriangleCount += mesh.GetIndexCount() / 3;
        }
        else
        {
            cmdList->DrawInstanced(mesh.GetVertexCount(), 1, 0, 0);
            m_RenderedTriangleCount += mesh.GetVertexCount() / 3;
        }

        m_RenderedChunkCount++;
    }

    void TerrainRenderer::RenderTerrain(ChunkManager& chunkManager,
                                         const DirectX::XMMATRIX& viewProjection,
                                         const DirectX::XMFLOAT3& cameraPosition)
    {
        if (!m_Initialized)
        {
            return;
        }

        // Reset statistics
        ResetStats();

        // Update frame constants
        UpdateFrameConstants(viewProjection, cameraPosition, 0.0f);

        // Begin terrain pass
        BeginTerrainPass();

        // Render all visible chunks
        const auto& visibleChunks = chunkManager.GetVisibleChunks();
        for (Chunk* chunk : visibleChunks)
        {
            if (chunk && chunk->HasMesh())
            {
                RenderChunk(*chunk, viewProjection);
            }
        }

        // End terrain pass
        EndTerrainPass();
    }

    void TerrainRenderer::ResetStats()
    {
        m_RenderedChunkCount = 0;
        m_RenderedTriangleCount = 0;
    }

    // ============================================================================
    // Private Methods
    // ============================================================================

    bool TerrainRenderer::CreateShaders()
    {
        std::cout << "[TerrainRenderer] Compiling terrain shaders..." << std::endl;

        // Compile terrain vertex shader
        if (!SM::CompileShaderFromFile(L"shaders/TerrainVertex.hlsl", "main", "vs_5_1", m_VertexShader))
        {
            std::cerr << "[TerrainRenderer] Failed to compile terrain vertex shader!" << std::endl;
            return false;
        }

        // Compile terrain pixel shader
        if (!SM::CompileShaderFromFile(L"shaders/TerrainPixel.hlsl", "main", "ps_5_1", m_PixelShader))
        {
            std::cerr << "[TerrainRenderer] Failed to compile terrain pixel shader!" << std::endl;
            return false;
        }

        std::cout << "[TerrainRenderer] Terrain shaders compiled successfully." << std::endl;
        return true;
    }

    bool TerrainRenderer::CreateRootSignature()
    {
        std::cout << "[TerrainRenderer] Creating terrain root signature..." << std::endl;

        // Terrain root signature:
        // 0: CBV - Per-frame constants (b0)
        // 1: CBV - Per-chunk constants (b1)

        D3D12_ROOT_PARAMETER rootParams[2] = {};

        // Per-frame CBV at register b0
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].Descriptor.RegisterSpace = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // Per-chunk CBV at register b1
        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[1].Descriptor.ShaderRegister = 1;
        rootParams[1].Descriptor.RegisterSpace = 0;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
        rsDesc.NumParameters = 2;
        rsDesc.pParameters = rootParams;
        rsDesc.NumStaticSamplers = 0;
        rsDesc.pStaticSamplers = nullptr;
        rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        if (!m_RootSignature.Create(m_Core, rsDesc))
        {
            std::cerr << "[TerrainRenderer] Failed to create root signature!" << std::endl;
            return false;
        }

        std::cout << "[TerrainRenderer] Terrain root signature created." << std::endl;
        return true;
    }

    bool TerrainRenderer::CreatePipelineState()
    {
        std::cout << "[TerrainRenderer] Creating terrain pipeline states..." << std::endl;

        // Create solid fill PSO
        m_TerrainPSO
            .Begin()
            .SetRootSignature(m_RootSignature.GetNative())
            .SetVertexShader(m_VertexShader)
            .SetPixelShader(m_PixelShader)
            .SetStandardInputLayout()
            .SetRasterizer(SM::FillMode::Solid, SM::CullMode::Back)
            .SetBlendMode(SM::BlendMode::Opaque)
            .SetDepthStencil(true, true, SM::DepthFunc::Less)
            .SetRenderTargetFormat(m_Core->GetBackBufferFormat())
            .SetDepthStencilFormat(m_Core->GetDepthFormat())
            .Build(m_Core);

        if (!m_TerrainPSO.GetNative())
        {
            std::cerr << "[TerrainRenderer] Failed to create solid PSO!" << std::endl;
            return false;
        }

        // Create wireframe PSO
        m_WireframePSO
            .Begin()
            .SetRootSignature(m_RootSignature.GetNative())
            .SetVertexShader(m_VertexShader)
            .SetPixelShader(m_PixelShader)
            .SetStandardInputLayout()
            .SetRasterizer(SM::FillMode::Wireframe, SM::CullMode::None)
            .SetBlendMode(SM::BlendMode::Opaque)
            .SetDepthStencil(true, true, SM::DepthFunc::Less)
            .SetRenderTargetFormat(m_Core->GetBackBufferFormat())
            .SetDepthStencilFormat(m_Core->GetDepthFormat())
            .Build(m_Core);

        if (!m_WireframePSO.GetNative())
        {
            std::cerr << "[TerrainRenderer] Failed to create wireframe PSO!" << std::endl;
            return false;
        }

        std::cout << "[TerrainRenderer] Terrain pipeline states created." << std::endl;
        return true;
    }

    bool TerrainRenderer::CreateConstantBuffers()
    {
        std::cout << "[TerrainRenderer] Creating terrain constant buffers..." << std::endl;

        // Per-frame constant buffer
        m_PerFrameCB = std::make_unique<SM::ConstantBuffer>();
        if (!m_PerFrameCB->Initialize(m_Core, sizeof(TerrainPerFrameData)))
        {
            std::cerr << "[TerrainRenderer] Failed to create per-frame CB!" << std::endl;
            return false;
        }

        // Per-chunk constant buffer
        m_PerChunkCB = std::make_unique<SM::ConstantBuffer>();
        if (!m_PerChunkCB->Initialize(m_Core, sizeof(TerrainPerChunkData)))
        {
            std::cerr << "[TerrainRenderer] Failed to create per-chunk CB!" << std::endl;
            return false;
        }

        std::cout << "[TerrainRenderer] Terrain constant buffers created." << std::endl;
        return true;
    }

} // namespace PCG
