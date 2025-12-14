#include "renderer/CommandList.h"

#include <iostream>
#include <cassert>

namespace SM
{
    bool CommandList::Initialize(DX12Core* core, CommandListType type)
    {
        assert(core != nullptr && "DX12Core cannot be null!");
        m_Core = core;
        m_Type = type;

        ID3D12Device* device = core->GetDevice();

        D3D12_COMMAND_LIST_TYPE d3dType;
        switch (type)
        {
            case CommandListType::Direct:
                d3dType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;
            case CommandListType::Compute:
                d3dType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                break;
            case CommandListType::Copy:
                d3dType = D3D12_COMMAND_LIST_TYPE_COPY;
                break;
            default:
                d3dType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;
        }

        // Create owned allocator
        HRESULT hr = device->CreateCommandAllocator(d3dType, IID_PPV_ARGS(&m_OwnedAllocator));
        if (!CheckHResult(hr, "Failed to create command allocator"))
        {
            return false;
        }

        // Create command list (initially closed)
        hr = device->CreateCommandList(
            0,
            d3dType,
            m_OwnedAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&m_CommandList)
        );

        if (!CheckHResult(hr, "Failed to create command list"))
        {
            return false;
        }

        // Close immediately - we'll reset when Begin is called
        m_CommandList->Close();
        m_IsRecording = false;

        return true;
    }

    bool CommandList::Begin(ID3D12CommandAllocator* allocator)
    {
        assert(!m_IsRecording && "Command list is already recording!");

        ID3D12CommandAllocator* useAllocator = allocator ? allocator : m_OwnedAllocator.Get();

        HRESULT hr = useAllocator->Reset();
        if (!CheckHResult(hr, "Failed to reset command allocator"))
        {
            return false;
        }

        hr = m_CommandList->Reset(useAllocator, nullptr);
        if (!CheckHResult(hr, "Failed to reset command list"))
        {
            return false;
        }

        m_IsRecording = true;
        m_NumBarriers = 0;

        return true;
    }

    bool CommandList::End()
    {
        assert(m_IsRecording && "Command list is not recording!");

        // Flush any pending barriers
        FlushBarriers();

        HRESULT hr = m_CommandList->Close();
        if (!CheckHResult(hr, "Failed to close command list"))
        {
            return false;
        }

        m_IsRecording = false;
        return true;
    }

    bool CommandList::Reset(ID3D12CommandAllocator* allocator, ID3D12PipelineState* pso)
    {
        HRESULT hr = m_CommandList->Reset(allocator, pso);
        if (!CheckHResult(hr, "Failed to reset command list"))
        {
            return false;
        }

        m_IsRecording = true;
        m_NumBarriers = 0;
        return true;
    }

    void CommandList::Execute()
    {
        assert(!m_IsRecording && "Cannot execute while recording!");

        ID3D12CommandList* lists[] = { m_CommandList.Get() };
        m_Core->ExecuteCommandLists(1, lists);
    }

    void CommandList::TransitionBarrier(
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after)
    {
        if (before == after)
        {
            return;
        }

        if (m_NumBarriers >= MAX_BARRIERS)
        {
            FlushBarriers();
        }

        D3D12_RESOURCE_BARRIER& barrier = m_Barriers[m_NumBarriers++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    }

    void CommandList::UAVBarrier(ID3D12Resource* resource)
    {
        if (m_NumBarriers >= MAX_BARRIERS)
        {
            FlushBarriers();
        }

        D3D12_RESOURCE_BARRIER& barrier = m_Barriers[m_NumBarriers++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = resource;
    }

    void CommandList::AliasingBarrier(ID3D12Resource* before, ID3D12Resource* after)
    {
        if (m_NumBarriers >= MAX_BARRIERS)
        {
            FlushBarriers();
        }

        D3D12_RESOURCE_BARRIER& barrier = m_Barriers[m_NumBarriers++];
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Aliasing.pResourceBefore = before;
        barrier.Aliasing.pResourceAfter = after;
    }

    void CommandList::FlushBarriers()
    {
        if (m_NumBarriers > 0)
        {
            m_CommandList->ResourceBarrier(m_NumBarriers, m_Barriers);
            m_NumBarriers = 0;
        }
    }

    void CommandList::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* color)
    {
        m_CommandList->ClearRenderTargetView(rtv, color, 0, nullptr);
    }

    void CommandList::ClearDepthStencil(
        D3D12_CPU_DESCRIPTOR_HANDLE dsv,
        float depth,
        uint8_t stencil,
        D3D12_CLEAR_FLAGS flags)
    {
        m_CommandList->ClearDepthStencilView(dsv, flags, depth, stencil, 0, nullptr);
    }

    void CommandList::SetRenderTargets(
        uint32_t numRTVs,
        const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsv)
    {
        m_CommandList->OMSetRenderTargets(numRTVs, rtvs, FALSE, dsv);
    }

    void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = x;
        viewport.TopLeftY = y;
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = minDepth;
        viewport.MaxDepth = maxDepth;

        m_CommandList->RSSetViewports(1, &viewport);
    }

    void CommandList::SetViewport(uint32_t width, uint32_t height)
    {
        SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    }

    void CommandList::SetScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        D3D12_RECT rect = {};
        rect.left = left;
        rect.top = top;
        rect.right = right;
        rect.bottom = bottom;

        m_CommandList->RSSetScissorRects(1, &rect);
    }

    void CommandList::SetScissorRect(uint32_t width, uint32_t height)
    {
        SetScissorRect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
    }

    void CommandList::SetPipelineState(ID3D12PipelineState* pso)
    {
        m_CommandList->SetPipelineState(pso);
    }

    void CommandList::SetGraphicsRootSignature(ID3D12RootSignature* rootSignature)
    {
        m_CommandList->SetGraphicsRootSignature(rootSignature);
    }

    void CommandList::SetComputeRootSignature(ID3D12RootSignature* rootSignature)
    {
        m_CommandList->SetComputeRootSignature(rootSignature);
    }

    void CommandList::SetDescriptorHeaps(uint32_t numHeaps, ID3D12DescriptorHeap* const* heaps)
    {
        m_CommandList->SetDescriptorHeaps(numHeaps, heaps);
    }

    void CommandList::SetGraphicsRoot32BitConstants(uint32_t rootIndex, uint32_t num32BitValues, const void* data, uint32_t destOffset)
    {
        m_CommandList->SetGraphicsRoot32BitConstants(rootIndex, num32BitValues, data, destOffset);
    }

    void CommandList::SetGraphicsRootConstantBufferView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation)
    {
        m_CommandList->SetGraphicsRootConstantBufferView(rootIndex, bufferLocation);
    }

    void CommandList::SetGraphicsRootShaderResourceView(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation)
    {
        m_CommandList->SetGraphicsRootShaderResourceView(rootIndex, bufferLocation);
    }

    void CommandList::SetGraphicsRootDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor)
    {
        m_CommandList->SetGraphicsRootDescriptorTable(rootIndex, baseDescriptor);
    }

    void CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
    {
        m_CommandList->IASetPrimitiveTopology(topology);
    }

    void CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t numViews, const D3D12_VERTEX_BUFFER_VIEW* views)
    {
        m_CommandList->IASetVertexBuffers(startSlot, numViews, views);
    }

    void CommandList::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view)
    {
        m_CommandList->IASetIndexBuffer(view);
    }

    void CommandList::DrawIndexed(
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t startIndex,
        int32_t baseVertex,
        uint32_t startInstance)
    {
        FlushBarriers();
        m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
    }

    void CommandList::Draw(
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t startVertex,
        uint32_t startInstance)
    {
        FlushBarriers();
        m_CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
    }

    void CommandList::CopyBufferRegion(
        ID3D12Resource* dst,
        uint64_t dstOffset,
        ID3D12Resource* src,
        uint64_t srcOffset,
        uint64_t numBytes)
    {
        m_CommandList->CopyBufferRegion(dst, dstOffset, src, srcOffset, numBytes);
    }

    void CommandList::CopyTextureRegion(
        const D3D12_TEXTURE_COPY_LOCATION* dst,
        uint32_t dstX,
        uint32_t dstY,
        uint32_t dstZ,
        const D3D12_TEXTURE_COPY_LOCATION* src,
        const D3D12_BOX* srcBox)
    {
        m_CommandList->CopyTextureRegion(dst, dstX, dstY, dstZ, src, srcBox);
    }

    void CommandList::CopyResource(ID3D12Resource* dst, ID3D12Resource* src)
    {
        m_CommandList->CopyResource(dst, src);
    }

} // namespace SM
