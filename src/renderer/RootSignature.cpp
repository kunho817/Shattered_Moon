#include "renderer/RootSignature.h"

#include <iostream>
#include <cassert>

namespace SM
{
    RootSignature& RootSignature::Begin(RootSignatureFlags flags)
    {
        // Reset state
        m_RootSignature.Reset();
        m_Parameters.clear();
        m_StaticSamplers.clear();
        m_DescriptorRanges.clear();

        m_Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(flags);

        return *this;
    }

    RootSignature& RootSignature::AddConstants(
        uint32_t num32BitValues,
        uint32_t shaderRegister,
        uint32_t registerSpace,
        ShaderVisibility visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(visibility);
        param.Constants.Num32BitValues = num32BitValues;
        param.Constants.ShaderRegister = shaderRegister;
        param.Constants.RegisterSpace = registerSpace;

        m_Parameters.push_back(param);
        return *this;
    }

    RootSignature& RootSignature::AddCBV(
        uint32_t shaderRegister,
        uint32_t registerSpace,
        ShaderVisibility visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(visibility);
        param.Descriptor.ShaderRegister = shaderRegister;
        param.Descriptor.RegisterSpace = registerSpace;
        param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

        m_Parameters.push_back(param);
        return *this;
    }

    RootSignature& RootSignature::AddSRV(
        uint32_t shaderRegister,
        uint32_t registerSpace,
        ShaderVisibility visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(visibility);
        param.Descriptor.ShaderRegister = shaderRegister;
        param.Descriptor.RegisterSpace = registerSpace;
        param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

        m_Parameters.push_back(param);
        return *this;
    }

    RootSignature& RootSignature::AddUAV(
        uint32_t shaderRegister,
        uint32_t registerSpace,
        ShaderVisibility visibility)
    {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(visibility);
        param.Descriptor.ShaderRegister = shaderRegister;
        param.Descriptor.RegisterSpace = registerSpace;
        param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

        m_Parameters.push_back(param);
        return *this;
    }

    RootSignature& RootSignature::AddDescriptorTable(
        const std::vector<DescriptorRange>& ranges,
        ShaderVisibility visibility)
    {
        // Store ranges
        std::vector<D3D12_DESCRIPTOR_RANGE1> d3dRanges;
        d3dRanges.reserve(ranges.size());

        for (const auto& range : ranges)
        {
            D3D12_DESCRIPTOR_RANGE1 d3dRange = {};
            d3dRange.RangeType = range.Type;
            d3dRange.NumDescriptors = range.NumDescriptors;
            d3dRange.BaseShaderRegister = range.BaseShaderRegister;
            d3dRange.RegisterSpace = range.RegisterSpace;
            d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            d3dRange.OffsetInDescriptorsFromTableStart = range.OffsetInDescriptorsFromTableStart;

            d3dRanges.push_back(d3dRange);
        }

        m_DescriptorRanges.push_back(std::move(d3dRanges));

        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(visibility);
        param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(m_DescriptorRanges.back().size());
        param.DescriptorTable.pDescriptorRanges = m_DescriptorRanges.back().data();

        m_Parameters.push_back(param);
        return *this;
    }

    RootSignature& RootSignature::AddSRVTable(
        uint32_t numDescriptors,
        uint32_t baseRegister,
        uint32_t registerSpace,
        ShaderVisibility visibility)
    {
        DescriptorRange range;
        range.Type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = numDescriptors;
        range.BaseShaderRegister = baseRegister;
        range.RegisterSpace = registerSpace;

        return AddDescriptorTable({ range }, visibility);
    }

    RootSignature& RootSignature::AddCBVTable(
        uint32_t numDescriptors,
        uint32_t baseRegister,
        uint32_t registerSpace,
        ShaderVisibility visibility)
    {
        DescriptorRange range;
        range.Type = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        range.NumDescriptors = numDescriptors;
        range.BaseShaderRegister = baseRegister;
        range.RegisterSpace = registerSpace;

        return AddDescriptorTable({ range }, visibility);
    }

    RootSignature& RootSignature::AddStaticSampler(const StaticSamplerConfig& config)
    {
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = config.Filter;
        sampler.AddressU = config.AddressU;
        sampler.AddressV = config.AddressV;
        sampler.AddressW = config.AddressW;
        sampler.MipLODBias = config.MipLODBias;
        sampler.MaxAnisotropy = config.MaxAnisotropy;
        sampler.ComparisonFunc = config.ComparisonFunc;
        sampler.BorderColor = config.BorderColor;
        sampler.MinLOD = config.MinLOD;
        sampler.MaxLOD = config.MaxLOD;
        sampler.ShaderRegister = config.ShaderRegister;
        sampler.RegisterSpace = config.RegisterSpace;
        sampler.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(config.Visibility);

        m_StaticSamplers.push_back(sampler);
        return *this;
    }

    RootSignature& RootSignature::AddLinearSampler(uint32_t shaderRegister, ShaderVisibility visibility)
    {
        StaticSamplerConfig config;
        config.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        config.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.ShaderRegister = shaderRegister;
        config.Visibility = visibility;

        return AddStaticSampler(config);
    }

    RootSignature& RootSignature::AddPointSampler(uint32_t shaderRegister, ShaderVisibility visibility)
    {
        StaticSamplerConfig config;
        config.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        config.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.ShaderRegister = shaderRegister;
        config.Visibility = visibility;

        return AddStaticSampler(config);
    }

    RootSignature& RootSignature::AddAnisotropicSampler(uint32_t shaderRegister, uint32_t maxAnisotropy, ShaderVisibility visibility)
    {
        StaticSamplerConfig config;
        config.Filter = D3D12_FILTER_ANISOTROPIC;
        config.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        config.MaxAnisotropy = maxAnisotropy;
        config.ShaderRegister = shaderRegister;
        config.Visibility = visibility;

        return AddStaticSampler(config);
    }

    RootSignature& RootSignature::AddShadowSampler(uint32_t shaderRegister, ShaderVisibility visibility)
    {
        StaticSamplerConfig config;
        config.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        config.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        config.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        config.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        config.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        config.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        config.ShaderRegister = shaderRegister;
        config.Visibility = visibility;

        return AddStaticSampler(config);
    }

    bool RootSignature::Build(DX12Core* core)
    {
        assert(core != nullptr && "DX12Core cannot be null!");

        ID3D12Device* device = core->GetDevice();

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        desc.Desc_1_1.NumParameters = static_cast<UINT>(m_Parameters.size());
        desc.Desc_1_1.pParameters = m_Parameters.empty() ? nullptr : m_Parameters.data();
        desc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(m_StaticSamplers.size());
        desc.Desc_1_1.pStaticSamplers = m_StaticSamplers.empty() ? nullptr : m_StaticSamplers.data();
        desc.Desc_1_1.Flags = m_Flags;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &signature, &error);

        if (FAILED(hr))
        {
            if (error)
            {
                std::cerr << "[DX12] Root signature serialization failed: "
                          << static_cast<const char*>(error->GetBufferPointer()) << std::endl;
            }
            return false;
        }

        hr = device->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&m_RootSignature)
        );

        if (!CheckHResult(hr, "Failed to create root signature"))
        {
            return false;
        }

        return true;
    }

    bool CreateBasicRootSignature(DX12Core* core, RootSignature& rootSignature)
    {
        return rootSignature
            .Begin(RootSignatureFlags::AllowInputAssemblerInputLayout)
            // b0 - Per-frame constants (View, Projection)
            .AddCBV(0, 0, ShaderVisibility::Vertex)
            // b1 - Per-object constants (World matrix)
            .AddCBV(1, 0, ShaderVisibility::Vertex)
            // b2 - Material constants
            .AddCBV(2, 0, ShaderVisibility::Pixel)
            // t0 - Texture (optional)
            .AddSRVTable(1, 0, 0, ShaderVisibility::Pixel)
            // s0 - Linear sampler
            .AddLinearSampler(0, ShaderVisibility::Pixel)
            .Build(core);
    }

} // namespace SM
