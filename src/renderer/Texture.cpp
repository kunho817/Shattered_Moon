#include "renderer/Texture.h"

#include <iostream>
#include <cassert>

namespace SM
{
    bool Texture::Create(DX12Core* core, const TextureDesc& desc, const char* debugName)
    {
        assert(core != nullptr && "DX12Core cannot be null!");
        m_Core = core;
        m_Desc = desc;

        ID3D12Device* device = core->GetDevice();

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Alignment = 0;
        resourceDesc.Width = desc.Width;
        resourceDesc.Height = desc.Height;
        resourceDesc.MipLevels = desc.MipLevels;
        resourceDesc.Format = desc.Format;
        resourceDesc.SampleDesc.Count = desc.SampleCount;
        resourceDesc.SampleDesc.Quality = desc.SampleQuality;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = GetResourceFlags();

        switch (desc.Type)
        {
            case TextureType::Texture1D:
                resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
                resourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.DepthOrArraySize);
                break;

            case TextureType::Texture2D:
            case TextureType::TextureCube:
                resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                resourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.DepthOrArraySize);
                break;

            case TextureType::Texture3D:
                resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
                resourceDesc.DepthOrArraySize = static_cast<UINT16>(desc.DepthOrArraySize);
                break;
        }

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_CLEAR_VALUE* clearValue = nullptr;
        D3D12_CLEAR_VALUE clearValueData = {};

        // Set clear value for render targets and depth stencils
        if ((static_cast<uint32_t>(desc.Usage) & static_cast<uint32_t>(TextureUsage::RenderTarget)) != 0)
        {
            clearValueData.Format = desc.Format;
            clearValueData.Color[0] = 0.0f;
            clearValueData.Color[1] = 0.0f;
            clearValueData.Color[2] = 0.0f;
            clearValueData.Color[3] = 1.0f;
            clearValue = &clearValueData;
        }
        else if ((static_cast<uint32_t>(desc.Usage) & static_cast<uint32_t>(TextureUsage::DepthStencil)) != 0)
        {
            clearValueData.Format = desc.Format;
            clearValueData.DepthStencil.Depth = 1.0f;
            clearValueData.DepthStencil.Stencil = 0;
            clearValue = &clearValueData;
        }

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            GetInitialState(),
            clearValue,
            IID_PPV_ARGS(&m_Resource)
        );

        if (!CheckHResult(hr, "Failed to create texture"))
        {
            return false;
        }

        // Set debug name
        if (debugName)
        {
            wchar_t wname[256];
            MultiByteToWideChar(CP_UTF8, 0, debugName, -1, wname, 256);
            m_Resource->SetName(wname);
        }

        // Create views
        CreateViews();

        return true;
    }

    bool Texture::LoadFromFile(DX12Core* core, const std::string& filename, bool generateMips)
    {
        (void)core;
        (void)filename;
        (void)generateMips;

        // TODO: Implement texture loading from file
        // This would use WIC or stb_image to load the texture data
        // and upload it to the GPU
        std::cerr << "[DX12] Texture::LoadFromFile not yet implemented!" << std::endl;
        return false;
    }

    bool Texture::CreateRenderTarget(
        DX12Core* core,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        const char* debugName)
    {
        TextureDesc desc;
        desc.Width = width;
        desc.Height = height;
        desc.Format = format;
        desc.Type = TextureType::Texture2D;
        desc.Usage = TextureUsage::ShaderResource | TextureUsage::RenderTarget;
        desc.MipLevels = 1;

        return Create(core, desc, debugName);
    }

    bool Texture::CreateDepthStencil(
        DX12Core* core,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        const char* debugName)
    {
        TextureDesc desc;
        desc.Width = width;
        desc.Height = height;
        desc.Format = format;
        desc.Type = TextureType::Texture2D;
        desc.Usage = TextureUsage::DepthStencil;
        desc.MipLevels = 1;

        return Create(core, desc, debugName);
    }

    void Texture::Resize(uint32_t width, uint32_t height)
    {
        if (width == m_Desc.Width && height == m_Desc.Height)
        {
            return;
        }

        // Store descriptor and recreate
        TextureDesc desc = m_Desc;
        desc.Width = width;
        desc.Height = height;

        m_Resource.Reset();
        Create(m_Core, desc, nullptr);
    }

    void Texture::CreateViews()
    {
        if (!m_Resource || !m_Core)
        {
            return;
        }

        ID3D12Device* device = m_Core->GetDevice();

        // Create SRV if shader resource
        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::ShaderResource)) != 0)
        {
            m_SRVHandle = m_Core->GetCBVSRVUAVHeap().Allocate();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            // Use typeless format for depth textures
            if (IsDepthFormat(m_Desc.Format))
            {
                srvDesc.Format = GetDepthSRVFormat(m_Desc.Format);
            }
            else
            {
                srvDesc.Format = m_Desc.Format;
            }

            switch (m_Desc.Type)
            {
                case TextureType::Texture1D:
                    if (m_Desc.DepthOrArraySize > 1)
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                        srvDesc.Texture1DArray.MipLevels = m_Desc.MipLevels;
                        srvDesc.Texture1DArray.ArraySize = m_Desc.DepthOrArraySize;
                    }
                    else
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                        srvDesc.Texture1D.MipLevels = m_Desc.MipLevels;
                    }
                    break;

                case TextureType::Texture2D:
                    if (m_Desc.SampleCount > 1)
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    }
                    else if (m_Desc.DepthOrArraySize > 1)
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        srvDesc.Texture2DArray.MipLevels = m_Desc.MipLevels;
                        srvDesc.Texture2DArray.ArraySize = m_Desc.DepthOrArraySize;
                    }
                    else
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Texture2D.MipLevels = m_Desc.MipLevels;
                    }
                    break;

                case TextureType::Texture3D:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                    srvDesc.Texture3D.MipLevels = m_Desc.MipLevels;
                    break;

                case TextureType::TextureCube:
                    if (m_Desc.DepthOrArraySize > 6)
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                        srvDesc.TextureCubeArray.MipLevels = m_Desc.MipLevels;
                        srvDesc.TextureCubeArray.NumCubes = m_Desc.DepthOrArraySize / 6;
                    }
                    else
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                        srvDesc.TextureCube.MipLevels = m_Desc.MipLevels;
                    }
                    break;
            }

            device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_SRVHandle.CPU);
        }

        // Create RTV if render target
        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::RenderTarget)) != 0)
        {
            m_RTVHandle = m_Core->GetRTVHeap().Allocate();

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = m_Desc.Format;

            if (m_Desc.SampleCount > 1)
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;
            }

            device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, m_RTVHandle.CPU);
        }

        // Create DSV if depth stencil
        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::DepthStencil)) != 0)
        {
            m_DSVHandle = m_Core->GetDSVHeap().Allocate();

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = m_Desc.Format;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            if (m_Desc.SampleCount > 1)
            {
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;
            }

            device->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, m_DSVHandle.CPU);
        }

        // Create UAV if unordered access
        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::UnorderedAccess)) != 0)
        {
            m_UAVHandle = m_Core->GetCBVSRVUAVHeap().Allocate();

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = m_Desc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;

            device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, m_UAVHandle.CPU);
        }
    }

    D3D12_RESOURCE_FLAGS Texture::GetResourceFlags() const
    {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::RenderTarget)) != 0)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::DepthStencil)) != 0)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            // Depth buffers without SRV usage should deny shader resource
            if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::ShaderResource)) == 0)
            {
                flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }
        }

        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::UnorderedAccess)) != 0)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        return flags;
    }

    D3D12_RESOURCE_STATES Texture::GetInitialState() const
    {
        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::DepthStencil)) != 0)
        {
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }

        if ((static_cast<uint32_t>(m_Desc.Usage) & static_cast<uint32_t>(TextureUsage::RenderTarget)) != 0)
        {
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        return D3D12_RESOURCE_STATE_COMMON;
    }

    // ============================================================================
    // Helper Functions
    // ============================================================================

    uint32_t GetFormatBytesPerPixel(DXGI_FORMAT format)
    {
        switch (format)
        {
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
            case DXGI_FORMAT_R32G32B32A32_UINT:
            case DXGI_FORMAT_R32G32B32A32_SINT:
                return 16;

            case DXGI_FORMAT_R32G32B32_FLOAT:
            case DXGI_FORMAT_R32G32B32_UINT:
            case DXGI_FORMAT_R32G32B32_SINT:
                return 12;

            case DXGI_FORMAT_R16G16B16A16_FLOAT:
            case DXGI_FORMAT_R16G16B16A16_UNORM:
            case DXGI_FORMAT_R16G16B16A16_UINT:
            case DXGI_FORMAT_R16G16B16A16_SNORM:
            case DXGI_FORMAT_R16G16B16A16_SINT:
            case DXGI_FORMAT_R32G32_FLOAT:
            case DXGI_FORMAT_R32G32_UINT:
            case DXGI_FORMAT_R32G32_SINT:
                return 8;

            case DXGI_FORMAT_R10G10B10A2_UNORM:
            case DXGI_FORMAT_R10G10B10A2_UINT:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_UINT:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
            case DXGI_FORMAT_R8G8B8A8_SINT:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_R16G16_FLOAT:
            case DXGI_FORMAT_R16G16_UNORM:
            case DXGI_FORMAT_R16G16_UINT:
            case DXGI_FORMAT_R16G16_SNORM:
            case DXGI_FORMAT_R16G16_SINT:
            case DXGI_FORMAT_R32_FLOAT:
            case DXGI_FORMAT_R32_UINT:
            case DXGI_FORMAT_R32_SINT:
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
                return 4;

            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8_UINT:
            case DXGI_FORMAT_R8G8_SNORM:
            case DXGI_FORMAT_R8G8_SINT:
            case DXGI_FORMAT_R16_FLOAT:
            case DXGI_FORMAT_R16_UNORM:
            case DXGI_FORMAT_R16_UINT:
            case DXGI_FORMAT_R16_SNORM:
            case DXGI_FORMAT_R16_SINT:
            case DXGI_FORMAT_D16_UNORM:
                return 2;

            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8_UINT:
            case DXGI_FORMAT_R8_SNORM:
            case DXGI_FORMAT_R8_SINT:
            case DXGI_FORMAT_A8_UNORM:
                return 1;

            default:
                return 0; // Unknown or compressed format
        }
    }

    bool IsDepthFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_D16_UNORM:
                return true;

            default:
                return false;
        }
    }

    DXGI_FORMAT GetTypelessDepthFormat(DXGI_FORMAT depthFormat)
    {
        switch (depthFormat)
        {
            case DXGI_FORMAT_D32_FLOAT:
                return DXGI_FORMAT_R32_TYPELESS;

            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
                return DXGI_FORMAT_R32G8X24_TYPELESS;

            case DXGI_FORMAT_D24_UNORM_S8_UINT:
                return DXGI_FORMAT_R24G8_TYPELESS;

            case DXGI_FORMAT_D16_UNORM:
                return DXGI_FORMAT_R16_TYPELESS;

            default:
                return depthFormat;
        }
    }

    DXGI_FORMAT GetDepthSRVFormat(DXGI_FORMAT depthFormat)
    {
        switch (depthFormat)
        {
            case DXGI_FORMAT_D32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;

            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
                return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

            case DXGI_FORMAT_D24_UNORM_S8_UINT:
                return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

            case DXGI_FORMAT_D16_UNORM:
                return DXGI_FORMAT_R16_UNORM;

            default:
                return depthFormat;
        }
    }

} // namespace SM
