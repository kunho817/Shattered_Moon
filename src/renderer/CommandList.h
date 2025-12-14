#pragma once

/**
 * @file CommandList.h
 * @brief DirectX 12 Command List wrapper
 *
 * Provides a convenient wrapper around ID3D12GraphicsCommandList
 * with Begin/End pattern and common rendering operations.
 */

#include "renderer/DX12Core.h"

#include <d3d12.h>
#include <wrl/client.h>

namespace SM
{
    using Microsoft::WRL::ComPtr;

    // Forward declarations
    class DX12Core;
    class GPUBuffer;
    class RootSignature;
    class PipelineState;

    /**
     * @brief Command list type
     */
    enum class CommandListType
    {
        Direct,     // Graphics and compute
        Compute,    // Compute only
        Copy        // Copy operations only
    };

    /**
     * @brief Wrapper for D3D12 Graphics Command List
     *
     * Simplifies command list usage with Begin/End pattern
     * and provides common rendering operations.
     */
    class CommandList
    {
    public:
        CommandList() = default;
        ~CommandList() = default;

        // Prevent copying
        CommandList(const CommandList&) = delete;
        CommandList& operator=(const CommandList&) = delete;

        // Allow moving
        CommandList(CommandList&&) noexcept = default;
        CommandList& operator=(CommandList&&) noexcept = default;

        /**
         * @brief Initialize the command list
         * @param core DX12 core reference
         * @param type Command list type
         * @return true if successful
         */
        bool Initialize(DX12Core* core, CommandListType type = CommandListType::Direct);

        /**
         * @brief Begin recording commands
         * @param allocator Command allocator to use (nullptr = use internal)
         * @return true if successful
         */
        bool Begin(ID3D12CommandAllocator* allocator = nullptr);

        /**
         * @brief End recording and close the command list
         * @return true if successful
         */
        bool End();

        /**
         * @brief Reset the command list with a new allocator
         * @param allocator Command allocator
         * @param pso Initial pipeline state (optional)
         * @return true if successful
         */
        bool Reset(ID3D12CommandAllocator* allocator, ID3D12PipelineState* pso = nullptr);

        /**
         * @brief Execute the command list
         */
        void Execute();

        // Resource Barriers
        /**
         * @brief Transition resource state
         */
        void TransitionBarrier(
            ID3D12Resource* resource,
            D3D12_RESOURCE_STATES before,
            D3D12_RESOURCE_STATES after
        );

        /**
         * @brief UAV barrier
         */
        void UAVBarrier(ID3D12Resource* resource);

        /**
         * @brief Aliasing barrier
         */
        void AliasingBarrier(ID3D12Resource* before, ID3D12Resource* after);

        /**
         * @brief Flush pending barriers
         */
        void FlushBarriers();

        // Render Target Operations
        /**
         * @brief Clear render target
         */
        void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* color);

        /**
         * @brief Clear depth stencil
         */
        void ClearDepthStencil(
            D3D12_CPU_DESCRIPTOR_HANDLE dsv,
            float depth = 1.0f,
            uint8_t stencil = 0,
            D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH
        );

        /**
         * @brief Set render targets
         */
        void SetRenderTargets(
            uint32_t numRTVs,
            const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
            const D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr
        );

        // Viewport and Scissor
        /**
         * @brief Set viewport
         */
        void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

        /**
         * @brief Set viewport (full screen)
         */
        void SetViewport(uint32_t width, uint32_t height);

        /**
         * @brief Set scissor rect
         */
        void SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

        /**
         * @brief Set scissor rect (full screen)
         */
        void SetScissorRect(uint32_t width, uint32_t height);

        // Pipeline State
        /**
         * @brief Set pipeline state object
         */
        void SetPipelineState(ID3D12PipelineState* pso);

        /**
         * @brief Set graphics root signature
         */
        void SetGraphicsRootSignature(ID3D12RootSignature* rootSignature);

        /**
         * @brief Set compute root signature
         */
        void SetComputeRootSignature(ID3D12RootSignature* rootSignature);

        // Descriptor Heaps
        /**
         * @brief Set descriptor heaps
         */
        void SetDescriptorHeaps(uint32_t numHeaps, ID3D12DescriptorHeap* const* heaps);

        // Root Parameters
        /**
         * @brief Set graphics root 32-bit constants
         */
        void SetGraphicsRoot32BitConstants(uint32_t rootIndex, uint32_t num32BitValues, const void* data, uint32_t destOffset = 0);

        /**
         * @brief Set graphics root constant buffer view
         */
        void SetGraphicsRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);

        /**
         * @brief Set graphics root shader resource view
         */
        void SetGraphicsRootShaderResourceView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation);

        /**
         * @brief Set graphics root descriptor table
         */
        void SetGraphicsRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);

        // Drawing
        /**
         * @brief Set primitive topology
         */
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

        /**
         * @brief Set vertex buffers
         */
        void SetVertexBuffers(uint32_t startSlot, uint32_t numViews, const D3D12_VERTEX_BUFFER_VIEW* views);

        /**
         * @brief Set index buffer
         */
        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view);

        /**
         * @brief Draw indexed primitives
         */
        void DrawIndexed(
            uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t startIndex = 0,
            int32_t baseVertex = 0,
            uint32_t startInstance = 0
        );

        /**
         * @brief Draw non-indexed primitives
         */
        void Draw(
            uint32_t vertexCount,
            uint32_t instanceCount = 1,
            uint32_t startVertex = 0,
            uint32_t startInstance = 0
        );

        // Copy Operations
        /**
         * @brief Copy buffer region
         */
        void CopyBufferRegion(
            ID3D12Resource* dst,
            uint64_t dstOffset,
            ID3D12Resource* src,
            uint64_t srcOffset,
            uint64_t numBytes
        );

        /**
         * @brief Copy texture region
         */
        void CopyTextureRegion(
            const D3D12_TEXTURE_COPY_LOCATION* dst,
            uint32_t dstX,
            uint32_t dstY,
            uint32_t dstZ,
            const D3D12_TEXTURE_COPY_LOCATION* src,
            const D3D12_BOX* srcBox = nullptr
        );

        /**
         * @brief Copy entire resource
         */
        void CopyResource(ID3D12Resource* dst, ID3D12Resource* src);

        // Accessors
        /**
         * @brief Get native command list
         */
        ID3D12GraphicsCommandList* GetNative() const { return m_CommandList.Get(); }

        /**
         * @brief Check if recording
         */
        bool IsRecording() const { return m_IsRecording; }

        /**
         * @brief Get command list type
         */
        CommandListType GetType() const { return m_Type; }

    private:
        DX12Core* m_Core = nullptr;
        ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        ComPtr<ID3D12CommandAllocator> m_OwnedAllocator;  // For standalone usage

        CommandListType m_Type = CommandListType::Direct;
        bool m_IsRecording = false;

        // Barrier batching
        static constexpr uint32_t MAX_BARRIERS = 16;
        D3D12_RESOURCE_BARRIER m_Barriers[MAX_BARRIERS] = {};
        uint32_t m_NumBarriers = 0;
    };

} // namespace SM
