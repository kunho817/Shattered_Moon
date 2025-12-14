#include "renderer/PipelineState.h"
#include "renderer/RootSignature.h"

#include <iostream>
#include <fstream>
#include <cassert>

namespace SM
{
    // ============================================================================
    // Shader Compilation Functions
    // ============================================================================

    bool CompileShaderFromFile(
        const std::wstring& filename,
        const char* entryPoint,
        const char* target,
        ShaderBytecode& bytecode)
    {
        UINT compileFlags = 0;

#if defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ComPtr<ID3DBlob> error;
        HRESULT hr = D3DCompileFromFile(
            filename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            target,
            compileFlags,
            0,
            &bytecode.Blob,
            &error
        );

        if (FAILED(hr))
        {
            if (error)
            {
                std::cerr << "[DX12] Shader compilation failed: "
                          << static_cast<const char*>(error->GetBufferPointer()) << std::endl;
            }
            else
            {
                std::cerr << "[DX12] Failed to open shader file!" << std::endl;
            }
            return false;
        }

        return true;
    }

    bool CompileShaderFromString(
        const std::string& source,
        const char* entryPoint,
        const char* target,
        ShaderBytecode& bytecode)
    {
        UINT compileFlags = 0;

#if defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ComPtr<ID3DBlob> error;
        HRESULT hr = D3DCompile(
            source.c_str(),
            source.length(),
            nullptr,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            target,
            compileFlags,
            0,
            &bytecode.Blob,
            &error
        );

        if (FAILED(hr))
        {
            if (error)
            {
                std::cerr << "[DX12] Shader compilation failed: "
                          << static_cast<const char*>(error->GetBufferPointer()) << std::endl;
            }
            return false;
        }

        return true;
    }

    bool LoadCompiledShader(const std::wstring& filename, ShaderBytecode& bytecode)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            std::cerr << "[DX12] Failed to open compiled shader file!" << std::endl;
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        HRESULT hr = D3DCreateBlob(static_cast<SIZE_T>(size), &bytecode.Blob);
        if (FAILED(hr))
        {
            return false;
        }

        if (!file.read(static_cast<char*>(bytecode.Blob->GetBufferPointer()), size))
        {
            bytecode.Blob.Reset();
            return false;
        }

        return true;
    }

    // ============================================================================
    // GraphicsPipelineState Implementation
    // ============================================================================

    GraphicsPipelineState& GraphicsPipelineState::Begin()
    {
        m_PipelineState.Reset();
        m_InputElements.clear();
        m_SemanticNames.clear();

        // Initialize with defaults
        m_Desc = {};

        // Default rasterizer state
        m_Desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        m_Desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        m_Desc.RasterizerState.FrontCounterClockwise = FALSE;
        m_Desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        m_Desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        m_Desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        m_Desc.RasterizerState.DepthClipEnable = TRUE;
        m_Desc.RasterizerState.MultisampleEnable = FALSE;
        m_Desc.RasterizerState.AntialiasedLineEnable = FALSE;
        m_Desc.RasterizerState.ForcedSampleCount = 0;
        m_Desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Default blend state (opaque)
        m_Desc.BlendState.AlphaToCoverageEnable = FALSE;
        m_Desc.BlendState.IndependentBlendEnable = FALSE;
        for (uint32_t i = 0; i < 8; ++i)
        {
            m_Desc.BlendState.RenderTarget[i].BlendEnable = FALSE;
            m_Desc.BlendState.RenderTarget[i].LogicOpEnable = FALSE;
            m_Desc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
            m_Desc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
            m_Desc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
            m_Desc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
            m_Desc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
            m_Desc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            m_Desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
            m_Desc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        // Default depth stencil state
        m_Desc.DepthStencilState.DepthEnable = TRUE;
        m_Desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        m_Desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        m_Desc.DepthStencilState.StencilEnable = FALSE;
        m_Desc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        m_Desc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        m_Desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        m_Desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        m_Desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        m_Desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        m_Desc.DepthStencilState.BackFace = m_Desc.DepthStencilState.FrontFace;

        // Default other settings
        m_Desc.SampleMask = UINT_MAX;
        m_Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        m_Desc.NumRenderTargets = 1;
        m_Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        m_Desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        m_Desc.SampleDesc.Count = 1;
        m_Desc.SampleDesc.Quality = 0;

        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetRootSignature(ID3D12RootSignature* rootSignature)
    {
        m_Desc.pRootSignature = rootSignature;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetRootSignature(const RootSignature& rootSignature)
    {
        return SetRootSignature(rootSignature.GetNative());
    }

    GraphicsPipelineState& GraphicsPipelineState::SetVertexShader(const ShaderBytecode& bytecode)
    {
        m_Desc.VS = bytecode.GetBytecode();
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetPixelShader(const ShaderBytecode& bytecode)
    {
        m_Desc.PS = bytecode.GetBytecode();
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetGeometryShader(const ShaderBytecode& bytecode)
    {
        m_Desc.GS = bytecode.GetBytecode();
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetHullShader(const ShaderBytecode& bytecode)
    {
        m_Desc.HS = bytecode.GetBytecode();
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetDomainShader(const ShaderBytecode& bytecode)
    {
        m_Desc.DS = bytecode.GetBytecode();
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::AddInputElement(
        const char* semanticName,
        uint32_t semanticIndex,
        DXGI_FORMAT format,
        uint32_t inputSlot,
        uint32_t alignedByteOffset,
        D3D12_INPUT_CLASSIFICATION inputSlotClass,
        uint32_t instanceDataStepRate)
    {
        // Store semantic name (keep string alive)
        m_SemanticNames.push_back(semanticName);

        D3D12_INPUT_ELEMENT_DESC element = {};
        element.SemanticName = m_SemanticNames.back().c_str();
        element.SemanticIndex = semanticIndex;
        element.Format = format;
        element.InputSlot = inputSlot;
        element.AlignedByteOffset = alignedByteOffset;
        element.InputSlotClass = inputSlotClass;
        element.InstanceDataStepRate = instanceDataStepRate;

        m_InputElements.push_back(element);
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetStandardInputLayout()
    {
        m_InputElements.clear();
        m_SemanticNames.clear();

        // Position (float3)
        AddInputElement(SemanticName::Position, 0, DXGI_FORMAT_R32G32B32_FLOAT);
        // Normal (float3)
        AddInputElement(SemanticName::Normal, 0, DXGI_FORMAT_R32G32B32_FLOAT);
        // TexCoord (float2)
        AddInputElement(SemanticName::TexCoord, 0, DXGI_FORMAT_R32G32_FLOAT);
        // Color (float4)
        AddInputElement(SemanticName::Color, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);

        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetPositionOnlyInputLayout()
    {
        m_InputElements.clear();
        m_SemanticNames.clear();

        AddInputElement(SemanticName::Position, 0, DXGI_FORMAT_R32G32B32_FLOAT);

        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetRasterizer(
        FillMode fillMode,
        CullMode cullMode,
        bool frontCounterClockwise)
    {
        m_Desc.RasterizerState.FillMode = static_cast<D3D12_FILL_MODE>(fillMode);
        m_Desc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(cullMode);
        m_Desc.RasterizerState.FrontCounterClockwise = frontCounterClockwise ? TRUE : FALSE;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetDepthClip(bool enable)
    {
        m_Desc.RasterizerState.DepthClipEnable = enable ? TRUE : FALSE;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetDepthBias(int32_t depthBias, float depthBiasClamp, float slopeScaledDepthBias)
    {
        m_Desc.RasterizerState.DepthBias = depthBias;
        m_Desc.RasterizerState.DepthBiasClamp = depthBiasClamp;
        m_Desc.RasterizerState.SlopeScaledDepthBias = slopeScaledDepthBias;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetBlendMode(BlendMode mode, uint32_t rtIndex)
    {
        auto& rt = m_Desc.BlendState.RenderTarget[rtIndex];

        switch (mode)
        {
            case BlendMode::Opaque:
                rt.BlendEnable = FALSE;
                break;

            case BlendMode::AlphaBlend:
                rt.BlendEnable = TRUE;
                rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
                rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                rt.BlendOp = D3D12_BLEND_OP_ADD;
                rt.SrcBlendAlpha = D3D12_BLEND_ONE;
                rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                break;

            case BlendMode::Additive:
                rt.BlendEnable = TRUE;
                rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
                rt.DestBlend = D3D12_BLEND_ONE;
                rt.BlendOp = D3D12_BLEND_OP_ADD;
                rt.SrcBlendAlpha = D3D12_BLEND_ONE;
                rt.DestBlendAlpha = D3D12_BLEND_ONE;
                rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                break;

            case BlendMode::Multiply:
                rt.BlendEnable = TRUE;
                rt.SrcBlend = D3D12_BLEND_ZERO;
                rt.DestBlend = D3D12_BLEND_SRC_COLOR;
                rt.BlendOp = D3D12_BLEND_OP_ADD;
                rt.SrcBlendAlpha = D3D12_BLEND_ONE;
                rt.DestBlendAlpha = D3D12_BLEND_ZERO;
                rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                break;

            case BlendMode::PremultipliedAlpha:
                rt.BlendEnable = TRUE;
                rt.SrcBlend = D3D12_BLEND_ONE;
                rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                rt.BlendOp = D3D12_BLEND_OP_ADD;
                rt.SrcBlendAlpha = D3D12_BLEND_ONE;
                rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                break;
        }

        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetBlendState(
        uint32_t rtIndex,
        bool blendEnable,
        D3D12_BLEND srcBlend,
        D3D12_BLEND destBlend,
        D3D12_BLEND_OP blendOp,
        D3D12_BLEND srcBlendAlpha,
        D3D12_BLEND destBlendAlpha,
        D3D12_BLEND_OP blendOpAlpha,
        uint8_t writeMask)
    {
        auto& rt = m_Desc.BlendState.RenderTarget[rtIndex];
        rt.BlendEnable = blendEnable ? TRUE : FALSE;
        rt.SrcBlend = srcBlend;
        rt.DestBlend = destBlend;
        rt.BlendOp = blendOp;
        rt.SrcBlendAlpha = srcBlendAlpha;
        rt.DestBlendAlpha = destBlendAlpha;
        rt.BlendOpAlpha = blendOpAlpha;
        rt.RenderTargetWriteMask = writeMask;

        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetDepthStencil(
        bool depthEnable,
        bool depthWrite,
        DepthFunc depthFunc)
    {
        m_Desc.DepthStencilState.DepthEnable = depthEnable ? TRUE : FALSE;
        m_Desc.DepthStencilState.DepthWriteMask = depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        m_Desc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(depthFunc);
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::DisableDepth()
    {
        m_Desc.DepthStencilState.DepthEnable = FALSE;
        m_Desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        m_Desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
    {
        m_Desc.PrimitiveTopologyType = type;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetRenderTargetFormats(
        uint32_t numRenderTargets,
        const DXGI_FORMAT* formats)
    {
        m_Desc.NumRenderTargets = numRenderTargets;
        for (uint32_t i = 0; i < numRenderTargets; ++i)
        {
            m_Desc.RTVFormats[i] = formats[i];
        }
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetRenderTargetFormat(DXGI_FORMAT format)
    {
        m_Desc.NumRenderTargets = 1;
        m_Desc.RTVFormats[0] = format;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetDepthStencilFormat(DXGI_FORMAT format)
    {
        m_Desc.DSVFormat = format;
        return *this;
    }

    GraphicsPipelineState& GraphicsPipelineState::SetSampleDesc(uint32_t count, uint32_t quality)
    {
        m_Desc.SampleDesc.Count = count;
        m_Desc.SampleDesc.Quality = quality;
        return *this;
    }

    bool GraphicsPipelineState::Build(DX12Core* core)
    {
        assert(core != nullptr && "DX12Core cannot be null!");

        // Set input layout
        m_Desc.InputLayout.NumElements = static_cast<UINT>(m_InputElements.size());
        m_Desc.InputLayout.pInputElementDescs = m_InputElements.empty() ? nullptr : m_InputElements.data();

        ID3D12Device* device = core->GetDevice();

        HRESULT hr = device->CreateGraphicsPipelineState(&m_Desc, IID_PPV_ARGS(&m_PipelineState));

        if (!CheckHResult(hr, "Failed to create graphics pipeline state"))
        {
            return false;
        }

        return true;
    }

    bool CreateBasicPipelineState(
        DX12Core* core,
        ID3D12RootSignature* rootSignature,
        const ShaderBytecode& vs,
        const ShaderBytecode& ps,
        GraphicsPipelineState& pso)
    {
        return pso
            .Begin()
            .SetRootSignature(rootSignature)
            .SetVertexShader(vs)
            .SetPixelShader(ps)
            .SetStandardInputLayout()
            .SetRasterizer(FillMode::Solid, CullMode::Back)
            .SetBlendMode(BlendMode::Opaque)
            .SetDepthStencil(true, true, DepthFunc::Less)
            .SetRenderTargetFormat(core->GetBackBufferFormat())
            .SetDepthStencilFormat(core->GetDepthFormat())
            .Build(core);
    }

} // namespace SM
