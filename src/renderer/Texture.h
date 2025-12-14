#pragma once

/**
 * @file Texture.h
 * @brief Texture resource interface for DirectX 12
 *
 * Provides texture loading, creation, and management.
 * This is a basic interface for future expansion.
 */

#include "renderer/DX12Core.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>

namespace SM
{
    using Microsoft::WRL::ComPtr;

    // Forward declarations
    class DX12Core;
    class CommandList;

    /**
     * @brief Texture type enumeration
     */
    enum class TextureType
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube
    };

    /**
     * @brief Texture usage flags
     */
    enum class TextureUsage : uint32_t
    {
        ShaderResource = 0x01,      // Can be bound as SRV
        RenderTarget = 0x02,        // Can be bound as RTV
        DepthStencil = 0x04,        // Can be bound as DSV
        UnorderedAccess = 0x08      // Can be bound as UAV
    };

    inline TextureUsage operator|(TextureUsage a, TextureUsage b)
    {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline TextureUsage operator&(TextureUsage a, TextureUsage b)
    {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    /**
     * @brief Texture descriptor
     */
    struct TextureDesc
    {
        uint32_t Width = 1;
        uint32_t Height = 1;
        uint32_t DepthOrArraySize = 1;
        uint32_t MipLevels = 1;
        DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        TextureType Type = TextureType::Texture2D;
        TextureUsage Usage = TextureUsage::ShaderResource;
        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

    /**
     * @brief Texture resource class
     *
     * Manages a D3D12 texture resource with associated views.
     */
    class Texture
    {
    public:
        Texture() = default;
        ~Texture() = default;

        // Prevent copying
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        // Allow moving
        Texture(Texture&&) noexcept = default;
        Texture& operator=(Texture&&) noexcept = default;

        /**
         * @brief Create texture from descriptor
         * @param core DX12 core reference
         * @param desc Texture descriptor
         * @param debugName Optional debug name
         * @return true if successful
         */
        bool Create(DX12Core* core, const TextureDesc& desc, const char* debugName = nullptr);

        /**
         * @brief Create texture from file (future implementation)
         * @param core DX12 core reference
         * @param filename Path to texture file
         * @param generateMips Generate mip maps
         * @return true if successful
         */
        bool LoadFromFile(DX12Core* core, const std::string& filename, bool generateMips = true);

        /**
         * @brief Create a render target texture
         * @param core DX12 core reference
         * @param width Texture width
         * @param height Texture height
         * @param format Texture format
         * @param debugName Optional debug name
         * @return true if successful
         */
        bool CreateRenderTarget(
            DX12Core* core,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
            const char* debugName = nullptr
        );

        /**
         * @brief Create a depth stencil texture
         * @param core DX12 core reference
         * @param width Texture width
         * @param height Texture height
         * @param format Depth format
         * @param debugName Optional debug name
         * @return true if successful
         */
        bool CreateDepthStencil(
            DX12Core* core,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT,
            const char* debugName = nullptr
        );

        /**
         * @brief Resize the texture (for render targets)
         * @param width New width
         * @param height New height
         */
        void Resize(uint32_t width, uint32_t height);

        /**
         * @brief Get the D3D12 resource
         */
        ID3D12Resource* GetResource() const { return m_Resource.Get(); }

        /**
         * @brief Get SRV handle
         */
        const DescriptorHandle& GetSRV() const { return m_SRVHandle; }

        /**
         * @brief Get RTV handle (if render target)
         */
        const DescriptorHandle& GetRTV() const { return m_RTVHandle; }

        /**
         * @brief Get DSV handle (if depth stencil)
         */
        const DescriptorHandle& GetDSV() const { return m_DSVHandle; }

        /**
         * @brief Get UAV handle (if unordered access)
         */
        const DescriptorHandle& GetUAV() const { return m_UAVHandle; }

        /**
         * @brief Get texture descriptor
         */
        const TextureDesc& GetDesc() const { return m_Desc; }

        /**
         * @brief Get texture width
         */
        uint32_t GetWidth() const { return m_Desc.Width; }

        /**
         * @brief Get texture height
         */
        uint32_t GetHeight() const { return m_Desc.Height; }

        /**
         * @brief Get texture format
         */
        DXGI_FORMAT GetFormat() const { return m_Desc.Format; }

        /**
         * @brief Check if texture is valid
         */
        bool IsValid() const { return m_Resource != nullptr; }

        /**
         * @brief Check if texture is a render target
         */
        bool IsRenderTarget() const
        {
            return (static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::RenderTarget)) != 0;
        }

        /**
         * @brief Check if texture is a depth stencil
         */
        bool IsDepthStencil() const
        {
            return (static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::DepthStencil)) != 0;
        }

    private:
        /**
         * @brief Create resource views based on usage flags
         */
        void CreateViews();

        /**
         * @brief Get D3D12 resource flags from texture usage
         */
        D3D12_RESOURCE_FLAGS GetResourceFlags() const;

        /**
         * @brief Get initial resource state based on usage
         */
        D3D12_RESOURCE_STATES GetInitialState() const;

    private:
        DX12Core* m_Core = nullptr;
        ComPtr<ID3D12Resource> m_Resource;

        TextureDesc m_Desc;

        // Descriptor handles
        DescriptorHandle m_SRVHandle;
        DescriptorHandle m_RTVHandle;
        DescriptorHandle m_DSVHandle;
        DescriptorHandle m_UAVHandle;
    };

    /**
     * @brief Get DXGI format bytes per pixel
     * @param format DXGI format
     * @return Bytes per pixel (0 for compressed/unknown formats)
     */
    uint32_t GetFormatBytesPerPixel(DXGI_FORMAT format);

    /**
     * @brief Check if format is a depth format
     * @param format DXGI format
     * @return true if depth format
     */
    bool IsDepthFormat(DXGI_FORMAT format);

    /**
     * @brief Get the typeless version of a depth format for SRV binding
     * @param depthFormat Depth format
     * @return Typeless format
     */
    DXGI_FORMAT GetTypelessDepthFormat(DXGI_FORMAT depthFormat);

    /**
     * @brief Get the shader-readable format for a depth buffer
     * @param depthFormat Depth format
     * @return Shader-readable format
     */
    DXGI_FORMAT GetDepthSRVFormat(DXGI_FORMAT depthFormat);

} // namespace SM
