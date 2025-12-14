#include "renderer/GPUBuffer.h"

#include <iostream>
#include <cstring>
#include <cassert>

namespace SM
{
    // ============================================================================
    // GPUBuffer Implementation
    // ============================================================================

    bool GPUBuffer::Initialize(
        DX12Core* core,
        size_t size,
        GPUBufferUsage usage,
        const void* initialData)
    {
        assert(core != nullptr && "DX12Core cannot be null!");
        assert(size > 0 && "Buffer size must be greater than 0!");

        m_Core = core;
        m_Size = size;
        m_Usage = usage;

        ID3D12Device* device = core->GetDevice();

        D3D12_HEAP_PROPERTIES heapProps = {};
        D3D12_RESOURCE_STATES initialState;

        switch (usage)
        {
            case GPUBufferUsage::Upload:
                heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
                initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
                break;

            case GPUBufferUsage::Readback:
                heapProps.Type = D3D12_HEAP_TYPE_READBACK;
                initialState = D3D12_RESOURCE_STATE_COPY_DEST;
                break;

            case GPUBufferUsage::Default:
            default:
                heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
                initialState = D3D12_RESOURCE_STATE_COMMON;
                break;
        }

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            initialState,
            nullptr,
            IID_PPV_ARGS(&m_Resource)
        );

        if (!CheckHResult(hr, "Failed to create GPU buffer"))
        {
            return false;
        }

        // For Default usage, create upload buffer for staging
        if (usage == GPUBufferUsage::Default && initialData)
        {
            D3D12_HEAP_PROPERTIES uploadHeapProps = {};
            uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            hr = device->CreateCommittedResource(
                &uploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_UploadResource)
            );

            if (!CheckHResult(hr, "Failed to create upload buffer"))
            {
                return false;
            }

            // Copy data to upload buffer
            void* mapped = nullptr;
            D3D12_RANGE readRange = { 0, 0 };
            m_UploadResource->Map(0, &readRange, &mapped);
            std::memcpy(mapped, initialData, size);
            m_UploadResource->Unmap(0, nullptr);
        }
        else if (usage == GPUBufferUsage::Upload && initialData)
        {
            // Direct upload for Upload buffers
            Update(initialData, size, 0);
        }

        return true;
    }

    void GPUBuffer::Update(const void* data, size_t size, size_t offset)
    {
        assert(m_Usage == GPUBufferUsage::Upload && "Can only update Upload buffers directly!");
        assert(offset + size <= m_Size && "Update exceeds buffer size!");

        D3D12_RANGE readRange = { 0, 0 };
        void* mapped = nullptr;

        HRESULT hr = m_Resource->Map(0, &readRange, &mapped);
        if (SUCCEEDED(hr))
        {
            std::memcpy(static_cast<uint8_t*>(mapped) + offset, data, size);
            m_Resource->Unmap(0, nullptr);
        }
    }

    void* GPUBuffer::Map()
    {
        assert(m_Usage != GPUBufferUsage::Default && "Cannot map Default buffers!");

        if (m_MappedData)
        {
            return m_MappedData;
        }

        D3D12_RANGE readRange = { 0, 0 };
        if (m_Usage == GPUBufferUsage::Readback)
        {
            readRange.End = m_Size;
        }

        HRESULT hr = m_Resource->Map(0, &readRange, &m_MappedData);
        if (FAILED(hr))
        {
            m_MappedData = nullptr;
        }

        return m_MappedData;
    }

    void GPUBuffer::Unmap()
    {
        if (m_MappedData)
        {
            D3D12_RANGE writeRange = { 0, 0 };
            if (m_Usage == GPUBufferUsage::Upload)
            {
                writeRange.End = m_Size;
            }

            m_Resource->Unmap(0, &writeRange);
            m_MappedData = nullptr;
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS GPUBuffer::GetGPUAddress() const
    {
        return m_Resource ? m_Resource->GetGPUVirtualAddress() : 0;
    }

    // ============================================================================
    // VertexBuffer Implementation
    // ============================================================================

    bool VertexBuffer::Initialize(
        DX12Core* core,
        uint32_t vertexCount,
        uint32_t vertexStride,
        GPUBufferUsage usage,
        const void* initialData)
    {
        m_VertexCount = vertexCount;
        m_VertexStride = vertexStride;

        size_t bufferSize = static_cast<size_t>(vertexCount) * vertexStride;

        if (!GPUBuffer::Initialize(core, bufferSize, usage, initialData))
        {
            return false;
        }

        // Setup vertex buffer view
        m_View.BufferLocation = GetGPUAddress();
        m_View.SizeInBytes = static_cast<UINT>(bufferSize);
        m_View.StrideInBytes = vertexStride;

        return true;
    }

    // ============================================================================
    // IndexBuffer Implementation
    // ============================================================================

    bool IndexBuffer::Initialize(
        DX12Core* core,
        uint32_t indexCount,
        bool use32Bit,
        GPUBufferUsage usage,
        const void* initialData)
    {
        m_IndexCount = indexCount;
        m_Format = use32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

        size_t indexSize = use32Bit ? sizeof(uint32_t) : sizeof(uint16_t);
        size_t bufferSize = static_cast<size_t>(indexCount) * indexSize;

        if (!GPUBuffer::Initialize(core, bufferSize, usage, initialData))
        {
            return false;
        }

        // Setup index buffer view
        m_View.BufferLocation = GetGPUAddress();
        m_View.SizeInBytes = static_cast<UINT>(bufferSize);
        m_View.Format = m_Format;

        return true;
    }

    // ============================================================================
    // ConstantBuffer Implementation
    // ============================================================================

    bool ConstantBuffer::Initialize(
        DX12Core* core,
        size_t dataSize,
        const void* initialData)
    {
        m_DataSize = dataSize;
        m_AlignedSize = AlignSize(dataSize, CONSTANT_BUFFER_ALIGNMENT);

        // Constant buffers are always Upload for easy CPU updates
        if (!GPUBuffer::Initialize(core, m_AlignedSize, GPUBufferUsage::Upload, nullptr))
        {
            return false;
        }

        if (initialData)
        {
            Update(initialData, dataSize);
        }

        return true;
    }

    void ConstantBuffer::Update(const void* data, size_t size)
    {
        assert(size <= m_DataSize && "Update size exceeds constant buffer data size!");
        GPUBuffer::Update(data, size, 0);
    }

    // ============================================================================
    // UploadHeap Implementation
    // ============================================================================

    UploadHeap::~UploadHeap()
    {
        Shutdown();
    }

    bool UploadHeap::Initialize(DX12Core* core, size_t size)
    {
        assert(core != nullptr && "DX12Core cannot be null!");
        assert(size > 0 && "Upload heap size must be greater than 0!");

        m_Core = core;
        m_Size = size;

        ID3D12Device* device = core->GetDevice();

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_Resource)
        );

        if (!CheckHResult(hr, "Failed to create upload heap"))
        {
            return false;
        }

        // Persistently map the upload heap
        D3D12_RANGE readRange = { 0, 0 };
        hr = m_Resource->Map(0, &readRange, &m_MappedData);
        if (!CheckHResult(hr, "Failed to map upload heap"))
        {
            return false;
        }

        m_GPUBaseAddress = m_Resource->GetGPUVirtualAddress();
        m_CurrentOffset = 0;

        std::cout << "[DX12] Created upload heap (" << (size / (1024 * 1024)) << " MB)" << std::endl;
        return true;
    }

    void UploadHeap::Shutdown()
    {
        if (m_Resource && m_MappedData)
        {
            m_Resource->Unmap(0, nullptr);
            m_MappedData = nullptr;
        }

        m_Resource.Reset();
        m_Size = 0;
        m_CurrentOffset = 0;
    }

    D3D12_GPU_VIRTUAL_ADDRESS UploadHeap::Allocate(size_t size, size_t alignment)
    {
        // Align current offset
        size_t alignedOffset = AlignSize(m_CurrentOffset, alignment);

        // Check if allocation fits
        if (alignedOffset + size > m_Size)
        {
            std::cerr << "[DX12] Upload heap out of memory!" << std::endl;
            return 0;
        }

        D3D12_GPU_VIRTUAL_ADDRESS address = m_GPUBaseAddress + alignedOffset;
        m_CurrentOffset = alignedOffset + size;

        return address;
    }

    void* UploadHeap::GetCPUPointer(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) const
    {
        assert(gpuAddress >= m_GPUBaseAddress && "GPU address out of range!");
        assert(gpuAddress < m_GPUBaseAddress + m_Size && "GPU address out of range!");

        size_t offset = gpuAddress - m_GPUBaseAddress;
        return static_cast<uint8_t*>(m_MappedData) + offset;
    }

    void UploadHeap::Reset()
    {
        m_CurrentOffset = 0;
    }

} // namespace SM
