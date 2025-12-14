#pragma once

/**
 * @file DX12Core.h
 * @brief DirectX 12 Core functionality
 *
 * Handles device creation, command queues, swap chain, descriptor heaps,
 * and GPU synchronization with triple buffering support.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>
#include <string>

// Link DirectX libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace SM
{
    using Microsoft::WRL::ComPtr;

    // Forward declarations
    class CommandList;

    /**
     * @brief Number of frames to buffer (triple buffering)
     */
    constexpr uint32_t FRAME_BUFFER_COUNT = 3;

    /**
     * @brief Descriptor heap types supported
     */
    enum class DescriptorHeapType
    {
        RTV,        // Render Target View
        DSV,        // Depth Stencil View
        CBV_SRV_UAV // Constant Buffer, Shader Resource, Unordered Access Views
    };

    /**
     * @brief Descriptor handle wrapper (CPU and GPU addresses)
     */
    struct DescriptorHandle
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CPU = {};
        D3D12_GPU_DESCRIPTOR_HANDLE GPU = {};
        uint32_t HeapIndex = 0;

        bool IsValid() const { return CPU.ptr != 0; }
    };

    /**
     * @brief Descriptor heap wrapper with allocation tracking
     */
    class DescriptorHeap
    {
    public:
        DescriptorHeap() = default;
        ~DescriptorHeap() = default;

        /**
         * @brief Initialize the descriptor heap
         * @param device D3D12 device
         * @param type Heap type
         * @param numDescriptors Maximum number of descriptors
         * @param shaderVisible Whether the heap is shader visible
         * @return true if successful
         */
        bool Initialize(
            ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            uint32_t numDescriptors,
            bool shaderVisible = false
        );

        /**
         * @brief Allocate a descriptor from the heap
         * @return Descriptor handle
         */
        DescriptorHandle Allocate();

        /**
         * @brief Free a descriptor (simple implementation - marks as available)
         * @param handle Handle to free
         */
        void Free(const DescriptorHandle& handle);

        /**
         * @brief Get the native heap
         */
        ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }

        /**
         * @brief Get handle at specific index
         */
        DescriptorHandle GetHandle(uint32_t index) const;

        /**
         * @brief Get descriptor increment size
         */
        uint32_t GetDescriptorSize() const { return m_DescriptorSize; }

    private:
        ComPtr<ID3D12DescriptorHeap> m_Heap;
        D3D12_DESCRIPTOR_HEAP_TYPE m_Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        D3D12_CPU_DESCRIPTOR_HANDLE m_CPUStart = {};
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUStart = {};
        uint32_t m_DescriptorSize = 0;
        uint32_t m_NumDescriptors = 0;
        uint32_t m_CurrentIndex = 0;
        bool m_ShaderVisible = false;
    };

    /**
     * @brief Frame resources for each buffered frame
     */
    struct FrameContext
    {
        ComPtr<ID3D12CommandAllocator> CommandAllocator;
        ComPtr<ID3D12Resource> BackBuffer;
        DescriptorHandle RTVHandle;
        uint64_t FenceValue = 0;
    };

    /**
     * @brief DirectX 12 Core system
     *
     * Manages the fundamental DX12 resources including device, queues,
     * swap chain, and synchronization primitives.
     */
    class DX12Core
    {
    public:
        DX12Core() = default;
        ~DX12Core();

        // Prevent copying
        DX12Core(const DX12Core&) = delete;
        DX12Core& operator=(const DX12Core&) = delete;

        /**
         * @brief Initialize DirectX 12
         * @param hwnd Window handle
         * @param width Window width
         * @param height Window height
         * @param vsync Enable vertical sync
         * @return true if successful
         */
        bool Initialize(HWND hwnd, uint32_t width, uint32_t height, bool vsync = true);

        /**
         * @brief Shutdown and release all resources
         */
        void Shutdown();

        /**
         * @brief Resize the swap chain
         * @param width New width
         * @param height New height
         */
        void Resize(uint32_t width, uint32_t height);

        /**
         * @brief Begin a new frame
         * @return Current frame index
         */
        uint32_t BeginFrame();

        /**
         * @brief End the current frame and present
         */
        void EndFrame();

        /**
         * @brief Wait for GPU to finish all work
         */
        void WaitForGPU();

        /**
         * @brief Wait for a specific fence value
         * @param fenceValue Value to wait for
         */
        void WaitForFence(uint64_t fenceValue);

        /**
         * @brief Execute command lists on the direct queue
         * @param numLists Number of command lists
         * @param ppCommandLists Array of command lists
         */
        void ExecuteCommandLists(uint32_t numLists, ID3D12CommandList* const* ppCommandLists);

        /**
         * @brief Signal the fence on the direct queue
         * @return The signaled fence value
         */
        uint64_t Signal();

        // Accessors
        ID3D12Device* GetDevice() const { return m_Device.Get(); }
        ID3D12CommandQueue* GetDirectQueue() const { return m_DirectQueue.Get(); }
        ID3D12CommandQueue* GetCopyQueue() const { return m_CopyQueue.Get(); }
        IDXGISwapChain4* GetSwapChain() const { return m_SwapChain.Get(); }

        uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
        FrameContext& GetCurrentFrame() { return m_FrameContexts[m_CurrentFrameIndex]; }
        const FrameContext& GetCurrentFrame() const { return m_FrameContexts[m_CurrentFrameIndex]; }

        ID3D12Resource* GetCurrentBackBuffer() const { return m_FrameContexts[m_CurrentFrameIndex].BackBuffer.Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const { return m_FrameContexts[m_CurrentFrameIndex].RTVHandle.CPU; }

        ID3D12Resource* GetDepthBuffer() const { return m_DepthBuffer.Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_DSVHandle.CPU; }

        DescriptorHeap& GetRTVHeap() { return m_RTVHeap; }
        DescriptorHeap& GetDSVHeap() { return m_DSVHeap; }
        DescriptorHeap& GetCBVSRVUAVHeap() { return m_CBVSRVUAVHeap; }

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        float GetAspectRatio() const { return static_cast<float>(m_Width) / static_cast<float>(m_Height); }

        DXGI_FORMAT GetBackBufferFormat() const { return m_BackBufferFormat; }
        DXGI_FORMAT GetDepthFormat() const { return m_DepthFormat; }

        bool IsVSyncEnabled() const { return m_VSyncEnabled; }
        void SetVSync(bool enabled) { m_VSyncEnabled = enabled; }

        /**
         * @brief Check if tearing (variable refresh rate) is supported
         */
        bool IsTearingSupported() const { return m_TearingSupported; }

    private:
        /**
         * @brief Enable debug layer (Debug builds only)
         */
        void EnableDebugLayer();

        /**
         * @brief Create DXGI factory
         */
        bool CreateFactory();

        /**
         * @brief Select the best GPU adapter
         */
        bool SelectAdapter();

        /**
         * @brief Create the D3D12 device
         */
        bool CreateDevice();

        /**
         * @brief Create command queues
         */
        bool CreateCommandQueues();

        /**
         * @brief Create the swap chain
         */
        bool CreateSwapChain();

        /**
         * @brief Create descriptor heaps
         */
        bool CreateDescriptorHeaps();

        /**
         * @brief Create frame resources (command allocators, RTVs)
         */
        bool CreateFrameResources();

        /**
         * @brief Create the depth buffer
         */
        bool CreateDepthBuffer();

        /**
         * @brief Create synchronization objects
         */
        bool CreateSyncObjects();

        /**
         * @brief Move to the next frame
         */
        void MoveToNextFrame();

    private:
        // Window
        HWND m_Hwnd = nullptr;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        // DXGI
        ComPtr<IDXGIFactory6> m_Factory;
        ComPtr<IDXGIAdapter4> m_Adapter;

        // Device
        ComPtr<ID3D12Device> m_Device;

        // Command Queues
        ComPtr<ID3D12CommandQueue> m_DirectQueue;
        ComPtr<ID3D12CommandQueue> m_CopyQueue;

        // Swap Chain
        ComPtr<IDXGISwapChain4> m_SwapChain;
        DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        bool m_VSyncEnabled = true;
        bool m_TearingSupported = false;

        // Depth Buffer
        ComPtr<ID3D12Resource> m_DepthBuffer;
        DescriptorHandle m_DSVHandle;
        DXGI_FORMAT m_DepthFormat = DXGI_FORMAT_D32_FLOAT;

        // Descriptor Heaps
        DescriptorHeap m_RTVHeap;
        DescriptorHeap m_DSVHeap;
        DescriptorHeap m_CBVSRVUAVHeap;

        // Frame Resources
        std::array<FrameContext, FRAME_BUFFER_COUNT> m_FrameContexts;
        uint32_t m_CurrentFrameIndex = 0;

        // Synchronization
        ComPtr<ID3D12Fence> m_Fence;
        uint64_t m_FenceValue = 0;
        HANDLE m_FenceEvent = nullptr;

        // Debug
        ComPtr<ID3D12Debug1> m_DebugController;
    };

    /**
     * @brief Helper to check HRESULT and log errors
     * @param hr HRESULT to check
     * @param msg Error message
     * @return true if SUCCEEDED
     */
    bool CheckHResult(HRESULT hr, const char* msg);

    /**
     * @brief Get string representation of D3D12 feature level
     */
    const char* FeatureLevelToString(D3D_FEATURE_LEVEL level);

} // namespace SM
