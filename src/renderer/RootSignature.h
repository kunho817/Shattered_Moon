#pragma once

/**
 * @file RootSignature.h
 * @brief DirectX 12 Root Signature builder
 *
 * Provides a fluent interface for building root signatures
 * with root parameters and static samplers.
 */

#include "renderer/DX12Core.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace SM
{
    using Microsoft::WRL::ComPtr;

    // Forward declarations
    class DX12Core;

    /**
     * @brief Root signature flags wrapper
     */
    enum class RootSignatureFlags : uint32_t
    {
        None = D3D12_ROOT_SIGNATURE_FLAG_NONE,
        AllowInputAssemblerInputLayout = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
        DenyVertexShaderRootAccess = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
        DenyHullShaderRootAccess = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
        DenyDomainShaderRootAccess = D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
        DenyGeometryShaderRootAccess = D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
        DenyPixelShaderRootAccess = D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
        AllowStreamOutput = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT,
        LocalRootSignature = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE
    };

    inline RootSignatureFlags operator|(RootSignatureFlags a, RootSignatureFlags b)
    {
        return static_cast<RootSignatureFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    /**
     * @brief Shader visibility for root parameters
     */
    enum class ShaderVisibility
    {
        All = D3D12_SHADER_VISIBILITY_ALL,
        Vertex = D3D12_SHADER_VISIBILITY_VERTEX,
        Hull = D3D12_SHADER_VISIBILITY_HULL,
        Domain = D3D12_SHADER_VISIBILITY_DOMAIN,
        Geometry = D3D12_SHADER_VISIBILITY_GEOMETRY,
        Pixel = D3D12_SHADER_VISIBILITY_PIXEL
    };

    /**
     * @brief Root parameter type
     */
    enum class RootParameterType
    {
        Constants,          // 32-bit root constants
        CBV,               // Constant buffer view (inline)
        SRV,               // Shader resource view (inline)
        UAV,               // Unordered access view (inline)
        DescriptorTable    // Descriptor table
    };

    /**
     * @brief Descriptor range for descriptor tables
     */
    struct DescriptorRange
    {
        D3D12_DESCRIPTOR_RANGE_TYPE Type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        uint32_t NumDescriptors = 1;
        uint32_t BaseShaderRegister = 0;
        uint32_t RegisterSpace = 0;
        uint32_t OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    };

    /**
     * @brief Static sampler configuration
     */
    struct StaticSamplerConfig
    {
        D3D12_FILTER Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        D3D12_TEXTURE_ADDRESS_MODE AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        D3D12_TEXTURE_ADDRESS_MODE AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        D3D12_TEXTURE_ADDRESS_MODE AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        float MipLODBias = 0.0f;
        uint32_t MaxAnisotropy = 16;
        D3D12_COMPARISON_FUNC ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        D3D12_STATIC_BORDER_COLOR BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        float MinLOD = 0.0f;
        float MaxLOD = D3D12_FLOAT32_MAX;
        uint32_t ShaderRegister = 0;
        uint32_t RegisterSpace = 0;
        ShaderVisibility Visibility = ShaderVisibility::All;
    };

    /**
     * @brief Root Signature builder and wrapper
     *
     * Provides a fluent interface for building D3D12 root signatures.
     */
    class RootSignature
    {
    public:
        RootSignature() = default;
        ~RootSignature() = default;

        // Prevent copying
        RootSignature(const RootSignature&) = delete;
        RootSignature& operator=(const RootSignature&) = delete;

        // Allow moving
        RootSignature(RootSignature&&) noexcept = default;
        RootSignature& operator=(RootSignature&&) noexcept = default;

        /**
         * @brief Begin building a new root signature
         * @param flags Root signature flags
         * @return Reference to this builder
         */
        RootSignature& Begin(RootSignatureFlags flags = RootSignatureFlags::AllowInputAssemblerInputLayout);

        /**
         * @brief Add 32-bit root constants
         * @param num32BitValues Number of 32-bit values
         * @param shaderRegister Register (b#)
         * @param registerSpace Register space
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddConstants(
            uint32_t num32BitValues,
            uint32_t shaderRegister,
            uint32_t registerSpace = 0,
            ShaderVisibility visibility = ShaderVisibility::All
        );

        /**
         * @brief Add constant buffer view (inline)
         * @param shaderRegister Register (b#)
         * @param registerSpace Register space
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddCBV(
            uint32_t shaderRegister,
            uint32_t registerSpace = 0,
            ShaderVisibility visibility = ShaderVisibility::All
        );

        /**
         * @brief Add shader resource view (inline)
         * @param shaderRegister Register (t#)
         * @param registerSpace Register space
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddSRV(
            uint32_t shaderRegister,
            uint32_t registerSpace = 0,
            ShaderVisibility visibility = ShaderVisibility::All
        );

        /**
         * @brief Add unordered access view (inline)
         * @param shaderRegister Register (u#)
         * @param registerSpace Register space
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddUAV(
            uint32_t shaderRegister,
            uint32_t registerSpace = 0,
            ShaderVisibility visibility = ShaderVisibility::All
        );

        /**
         * @brief Add descriptor table
         * @param ranges Descriptor ranges
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddDescriptorTable(
            const std::vector<DescriptorRange>& ranges,
            ShaderVisibility visibility = ShaderVisibility::All
        );

        /**
         * @brief Add SRV descriptor table (convenience)
         * @param numDescriptors Number of SRVs
         * @param baseRegister Base register (t#)
         * @param registerSpace Register space
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddSRVTable(
            uint32_t numDescriptors,
            uint32_t baseRegister = 0,
            uint32_t registerSpace = 0,
            ShaderVisibility visibility = ShaderVisibility::Pixel
        );

        /**
         * @brief Add CBV descriptor table (convenience)
         * @param numDescriptors Number of CBVs
         * @param baseRegister Base register (b#)
         * @param registerSpace Register space
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddCBVTable(
            uint32_t numDescriptors,
            uint32_t baseRegister = 0,
            uint32_t registerSpace = 0,
            ShaderVisibility visibility = ShaderVisibility::All
        );

        /**
         * @brief Add static sampler
         * @param config Sampler configuration
         * @return Reference to this builder
         */
        RootSignature& AddStaticSampler(const StaticSamplerConfig& config);

        /**
         * @brief Add default linear sampler
         * @param shaderRegister Register (s#)
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddLinearSampler(
            uint32_t shaderRegister = 0,
            ShaderVisibility visibility = ShaderVisibility::Pixel
        );

        /**
         * @brief Add default point sampler
         * @param shaderRegister Register (s#)
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddPointSampler(
            uint32_t shaderRegister = 0,
            ShaderVisibility visibility = ShaderVisibility::Pixel
        );

        /**
         * @brief Add anisotropic sampler
         * @param shaderRegister Register (s#)
         * @param maxAnisotropy Maximum anisotropy level
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddAnisotropicSampler(
            uint32_t shaderRegister = 0,
            uint32_t maxAnisotropy = 16,
            ShaderVisibility visibility = ShaderVisibility::Pixel
        );

        /**
         * @brief Add shadow comparison sampler
         * @param shaderRegister Register (s#)
         * @param visibility Shader visibility
         * @return Reference to this builder
         */
        RootSignature& AddShadowSampler(
            uint32_t shaderRegister = 0,
            ShaderVisibility visibility = ShaderVisibility::Pixel
        );

        /**
         * @brief Build and finalize the root signature
         * @param core DX12 core reference
         * @return true if successful
         */
        bool Build(DX12Core* core);

        /**
         * @brief Get the native root signature
         */
        ID3D12RootSignature* GetNative() const { return m_RootSignature.Get(); }

        /**
         * @brief Check if root signature is valid
         */
        bool IsValid() const { return m_RootSignature != nullptr; }

        /**
         * @brief Get the number of root parameters
         */
        uint32_t GetParameterCount() const { return static_cast<uint32_t>(m_Parameters.size()); }

    private:
        ComPtr<ID3D12RootSignature> m_RootSignature;

        D3D12_ROOT_SIGNATURE_FLAGS m_Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        std::vector<D3D12_ROOT_PARAMETER1> m_Parameters;
        std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers;

        // Storage for descriptor ranges (must persist until Build)
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> m_DescriptorRanges;
    };

    /**
     * @brief Create a basic root signature for simple rendering
     * @param core DX12 core reference
     * @param rootSignature Output root signature
     * @return true if successful
     */
    bool CreateBasicRootSignature(DX12Core* core, RootSignature& rootSignature);

} // namespace SM
