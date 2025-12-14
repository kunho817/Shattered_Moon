#pragma once

/**
 * @file GPUBuffer.h
 * @brief GPU Buffer resources for DirectX 12
 *
 * Provides Vertex, Index, and Constant buffer abstractions
 * with upload heap management.
 */

#include "renderer/DX12Core.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>

namespace SM
{
    using Microsoft::WRL::ComPtr;

    // Forward declarations
    class DX12Core;
    class CommandList;

    /**
     * @brief GPU Buffer type
     */
    enum class GPUBufferType
    {
        Vertex,
        Index,
        Constant,
        Structured,
        Raw
    };

    /**
     * @brief GPU buffer usage flags
     */
    enum class GPUBufferUsage
    {
        Default,    // GPU-only, requires upload via copy queue
        Upload,     // CPU-writable, GPU-readable (for dynamic data)
        Readback    // GPU-writable, CPU-readable (for queries)
    };

    /**
     * @brief Base GPU Buffer class
     *
     * Manages a D3D12 buffer resource with optional upload heap.
     */
    class GPUBuffer
    {
    public:
        GPUBuffer() = default;
        virtual ~GPUBuffer() = default;

        // Prevent copying
        GPUBuffer(const GPUBuffer&) = delete;
        GPUBuffer& operator=(const GPUBuffer&) = delete;

        // Allow moving
        GPUBuffer(GPUBuffer&&) noexcept = default;
        GPUBuffer& operator=(GPUBuffer&&) noexcept = default;

        /**
         * @brief Initialize the buffer
         * @param core DX12 core reference
         * @param size Buffer size in bytes
         * @param usage Buffer usage
         * @param initialData Initial data to upload (optional)
         * @return true if successful
         */
        bool Initialize(
            DX12Core* core,
            size_t size,
            GPUBufferUsage usage = GPUBufferUsage::Default,
            const void* initialData = nullptr
        );

        /**
         * @brief Update buffer data (for Upload buffers only)
         * @param data Source data
         * @param size Data size in bytes
         * @param offset Offset in buffer
         */
        void Update(const void* data, size_t size, size_t offset = 0);

        /**
         * @brief Map the buffer for CPU access (Upload/Readback only)
         * @return Pointer to mapped memory
         */
        void* Map();

        /**
         * @brief Unmap the buffer
         */
        void Unmap();

        /**
         * @brief Get buffer GPU virtual address
         */
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;

        /**
         * @brief Get the D3D12 resource
         */
        ID3D12Resource* GetResource() const { return m_Resource.Get(); }

        /**
         * @brief Get the upload resource (for staging)
         */
        ID3D12Resource* GetUploadResource() const { return m_UploadResource.Get(); }

        /**
         * @brief Get buffer size
         */
        size_t GetSize() const { return m_Size; }

        /**
         * @brief Get buffer usage
         */
        GPUBufferUsage GetUsage() const { return m_Usage; }

        /**
         * @brief Check if buffer is valid
         */
        bool IsValid() const { return m_Resource != nullptr; }

    protected:
        DX12Core* m_Core = nullptr;
        ComPtr<ID3D12Resource> m_Resource;
        ComPtr<ID3D12Resource> m_UploadResource;  // For Default usage staging

        size_t m_Size = 0;
        GPUBufferUsage m_Usage = GPUBufferUsage::Default;
        void* m_MappedData = nullptr;
    };

    /**
     * @brief Vertex Buffer
     *
     * Specialized buffer for vertex data with view accessor.
     */
    class VertexBuffer : public GPUBuffer
    {
    public:
        VertexBuffer() = default;
        ~VertexBuffer() override = default;

        /**
         * @brief Initialize vertex buffer
         * @param core DX12 core reference
         * @param vertexCount Number of vertices
         * @param vertexStride Size of each vertex in bytes
         * @param usage Buffer usage
         * @param initialData Initial vertex data
         * @return true if successful
         */
        bool Initialize(
            DX12Core* core,
            uint32_t vertexCount,
            uint32_t vertexStride,
            GPUBufferUsage usage = GPUBufferUsage::Default,
            const void* initialData = nullptr
        );

        /**
         * @brief Get vertex buffer view
         */
        const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_View; }

        /**
         * @brief Get vertex count
         */
        uint32_t GetVertexCount() const { return m_VertexCount; }

        /**
         * @brief Get vertex stride
         */
        uint32_t GetVertexStride() const { return m_VertexStride; }

    private:
        D3D12_VERTEX_BUFFER_VIEW m_View = {};
        uint32_t m_VertexCount = 0;
        uint32_t m_VertexStride = 0;
    };

    /**
     * @brief Index Buffer
     *
     * Specialized buffer for index data with view accessor.
     */
    class IndexBuffer : public GPUBuffer
    {
    public:
        IndexBuffer() = default;
        ~IndexBuffer() override = default;

        /**
         * @brief Initialize index buffer
         * @param core DX12 core reference
         * @param indexCount Number of indices
         * @param use32Bit Use 32-bit indices (default) or 16-bit
         * @param usage Buffer usage
         * @param initialData Initial index data
         * @return true if successful
         */
        bool Initialize(
            DX12Core* core,
            uint32_t indexCount,
            bool use32Bit = true,
            GPUBufferUsage usage = GPUBufferUsage::Default,
            const void* initialData = nullptr
        );

        /**
         * @brief Get index buffer view
         */
        const D3D12_INDEX_BUFFER_VIEW& GetView() const { return m_View; }

        /**
         * @brief Get index count
         */
        uint32_t GetIndexCount() const { return m_IndexCount; }

        /**
         * @brief Check if using 32-bit indices
         */
        bool Is32Bit() const { return m_Format == DXGI_FORMAT_R32_UINT; }

    private:
        D3D12_INDEX_BUFFER_VIEW m_View = {};
        uint32_t m_IndexCount = 0;
        DXGI_FORMAT m_Format = DXGI_FORMAT_R32_UINT;
    };

    /**
     * @brief Constant Buffer
     *
     * Specialized buffer for constant/uniform data with 256-byte alignment.
     */
    class ConstantBuffer : public GPUBuffer
    {
    public:
        ConstantBuffer() = default;
        ~ConstantBuffer() override = default;

        /**
         * @brief Initialize constant buffer
         * @param core DX12 core reference
         * @param dataSize Size of constant data (will be aligned to 256 bytes)
         * @param initialData Initial constant data
         * @return true if successful
         */
        bool Initialize(
            DX12Core* core,
            size_t dataSize,
            const void* initialData = nullptr
        );

        /**
         * @brief Update constant buffer data
         * @param data Source data
         * @param size Data size
         */
        void Update(const void* data, size_t size);

        /**
         * @brief Get aligned size (256-byte aligned)
         */
        size_t GetAlignedSize() const { return m_AlignedSize; }

        /**
         * @brief Get original data size (before alignment)
         */
        size_t GetDataSize() const { return m_DataSize; }

    private:
        size_t m_DataSize = 0;
        size_t m_AlignedSize = 0;
    };

    /**
     * @brief Upload Heap Manager
     *
     * Ring buffer for efficient dynamic upload allocations.
     * Used for per-frame constant buffer updates.
     */
    class UploadHeap
    {
    public:
        UploadHeap() = default;
        ~UploadHeap();

        /**
         * @brief Initialize upload heap
         * @param core DX12 core reference
         * @param size Heap size in bytes
         * @return true if successful
         */
        bool Initialize(DX12Core* core, size_t size);

        /**
         * @brief Shutdown and release resources
         */
        void Shutdown();

        /**
         * @brief Allocate from upload heap
         * @param size Allocation size
         * @param alignment Required alignment (default 256 for CBV)
         * @return GPU address of allocation, or 0 if failed
         */
        D3D12_GPU_VIRTUAL_ADDRESS Allocate(size_t size, size_t alignment = 256);

        /**
         * @brief Get CPU pointer for current allocation
         * @param gpuAddress GPU address returned from Allocate
         * @return CPU pointer
         */
        void* GetCPUPointer(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) const;

        /**
         * @brief Reset allocator (call at start of frame)
         */
        void Reset();

        /**
         * @brief Get underlying resource
         */
        ID3D12Resource* GetResource() const { return m_Resource.Get(); }

        /**
         * @brief Get total size
         */
        size_t GetSize() const { return m_Size; }

        /**
         * @brief Get used size
         */
        size_t GetUsedSize() const { return m_CurrentOffset; }

    private:
        DX12Core* m_Core = nullptr;
        ComPtr<ID3D12Resource> m_Resource;

        void* m_MappedData = nullptr;
        size_t m_Size = 0;
        size_t m_CurrentOffset = 0;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUBaseAddress = 0;
    };

    /**
     * @brief Align size to specified alignment
     * @param size Original size
     * @param alignment Alignment requirement
     * @return Aligned size
     */
    inline size_t AlignSize(size_t size, size_t alignment)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    /**
     * @brief Constant buffer alignment requirement (256 bytes)
     */
    constexpr size_t CONSTANT_BUFFER_ALIGNMENT = 256;

} // namespace SM
