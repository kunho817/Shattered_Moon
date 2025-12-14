#include "renderer/DX12Core.h"

#include <iostream>
#include <vector>
#include <cassert>

namespace SM
{
    // ============================================================================
    // Helper Functions
    // ============================================================================

    bool CheckHResult(HRESULT hr, const char* msg)
    {
        if (FAILED(hr))
        {
            std::cerr << "[DX12] " << msg << " (HRESULT: 0x"
                      << std::hex << hr << std::dec << ")" << std::endl;
            return false;
        }
        return true;
    }

    const char* FeatureLevelToString(D3D_FEATURE_LEVEL level)
    {
        switch (level)
        {
            case D3D_FEATURE_LEVEL_12_2: return "12.2";
            case D3D_FEATURE_LEVEL_12_1: return "12.1";
            case D3D_FEATURE_LEVEL_12_0: return "12.0";
            case D3D_FEATURE_LEVEL_11_1: return "11.1";
            case D3D_FEATURE_LEVEL_11_0: return "11.0";
            default: return "Unknown";
        }
    }

    // ============================================================================
    // DescriptorHeap Implementation
    // ============================================================================

    bool DescriptorHeap::Initialize(
        ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t numDescriptors,
        bool shaderVisible)
    {
        m_Type = type;
        m_NumDescriptors = numDescriptors;
        m_ShaderVisible = shaderVisible;
        m_CurrentIndex = 0;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = type;
        desc.NumDescriptors = numDescriptors;
        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                   : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap));
        if (!CheckHResult(hr, "Failed to create descriptor heap"))
        {
            return false;
        }

        m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
        m_CPUStart = m_Heap->GetCPUDescriptorHandleForHeapStart();

        if (shaderVisible)
        {
            m_GPUStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
        }

        return true;
    }

    DescriptorHandle DescriptorHeap::Allocate()
    {
        assert(m_CurrentIndex < m_NumDescriptors && "Descriptor heap is full!");

        DescriptorHandle handle;
        handle.HeapIndex = m_CurrentIndex;
        handle.CPU.ptr = m_CPUStart.ptr + (m_CurrentIndex * m_DescriptorSize);

        if (m_ShaderVisible)
        {
            handle.GPU.ptr = m_GPUStart.ptr + (m_CurrentIndex * m_DescriptorSize);
        }

        m_CurrentIndex++;
        return handle;
    }

    void DescriptorHeap::Free(const DescriptorHandle& handle)
    {
        // Simple implementation - doesn't actually reclaim space
        // For production, implement a free list
        (void)handle;
    }

    DescriptorHandle DescriptorHeap::GetHandle(uint32_t index) const
    {
        assert(index < m_NumDescriptors);

        DescriptorHandle handle;
        handle.HeapIndex = index;
        handle.CPU.ptr = m_CPUStart.ptr + (index * m_DescriptorSize);

        if (m_ShaderVisible)
        {
            handle.GPU.ptr = m_GPUStart.ptr + (index * m_DescriptorSize);
        }

        return handle;
    }

    // ============================================================================
    // DX12Core Implementation
    // ============================================================================

    DX12Core::~DX12Core()
    {
        Shutdown();
    }

    bool DX12Core::Initialize(HWND hwnd, uint32_t width, uint32_t height, bool vsync)
    {
        m_Hwnd = hwnd;
        m_Width = width;
        m_Height = height;
        m_VSyncEnabled = vsync;

        std::cout << "[DX12] Initializing DirectX 12..." << std::endl;

        // Enable debug layer in debug builds
        EnableDebugLayer();

        // Create DXGI Factory
        if (!CreateFactory())
        {
            return false;
        }

        // Select best GPU adapter
        if (!SelectAdapter())
        {
            return false;
        }

        // Create D3D12 Device
        if (!CreateDevice())
        {
            return false;
        }

        // Create Command Queues
        if (!CreateCommandQueues())
        {
            return false;
        }

        // Create Descriptor Heaps
        if (!CreateDescriptorHeaps())
        {
            return false;
        }

        // Create Swap Chain
        if (!CreateSwapChain())
        {
            return false;
        }

        // Create Frame Resources
        if (!CreateFrameResources())
        {
            return false;
        }

        // Create Depth Buffer
        if (!CreateDepthBuffer())
        {
            return false;
        }

        // Create Sync Objects
        if (!CreateSyncObjects())
        {
            return false;
        }

        std::cout << "[DX12] DirectX 12 initialized successfully!" << std::endl;
        std::cout << "[DX12] Resolution: " << m_Width << "x" << m_Height << std::endl;
        std::cout << "[DX12] V-Sync: " << (m_VSyncEnabled ? "Enabled" : "Disabled") << std::endl;
        std::cout << "[DX12] Tearing Support: " << (m_TearingSupported ? "Yes" : "No") << std::endl;

        return true;
    }

    void DX12Core::Shutdown()
    {
        // Wait for GPU to finish all work
        WaitForGPU();

        // Close fence event
        if (m_FenceEvent)
        {
            CloseHandle(m_FenceEvent);
            m_FenceEvent = nullptr;
        }

        // Release resources in reverse order
        m_Fence.Reset();

        for (auto& frame : m_FrameContexts)
        {
            frame.CommandAllocator.Reset();
            frame.BackBuffer.Reset();
        }

        m_DepthBuffer.Reset();
        m_SwapChain.Reset();
        m_CopyQueue.Reset();
        m_DirectQueue.Reset();
        m_Device.Reset();
        m_Adapter.Reset();
        m_Factory.Reset();

#if defined(_DEBUG)
        // Report live objects
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
#endif

        std::cout << "[DX12] DirectX 12 shutdown complete." << std::endl;
    }

    void DX12Core::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        if (width == m_Width && height == m_Height)
        {
            return;
        }

        // Wait for GPU to finish
        WaitForGPU();

        // Release back buffer references
        for (auto& frame : m_FrameContexts)
        {
            frame.BackBuffer.Reset();
            frame.FenceValue = m_FenceValue;
        }

        // Release depth buffer
        m_DepthBuffer.Reset();

        // Resize swap chain
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        m_SwapChain->GetDesc1(&desc);

        HRESULT hr = m_SwapChain->ResizeBuffers(
            FRAME_BUFFER_COUNT,
            width,
            height,
            desc.Format,
            desc.Flags
        );

        if (!CheckHResult(hr, "Failed to resize swap chain"))
        {
            return;
        }

        m_Width = width;
        m_Height = height;
        m_CurrentFrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

        // Recreate back buffer views
        for (uint32_t i = 0; i < FRAME_BUFFER_COUNT; ++i)
        {
            hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_FrameContexts[i].BackBuffer));
            if (!CheckHResult(hr, "Failed to get back buffer"))
            {
                return;
            }

            m_Device->CreateRenderTargetView(
                m_FrameContexts[i].BackBuffer.Get(),
                nullptr,
                m_FrameContexts[i].RTVHandle.CPU
            );
        }

        // Recreate depth buffer
        CreateDepthBuffer();

        std::cout << "[DX12] Resized to " << width << "x" << height << std::endl;
    }

    uint32_t DX12Core::BeginFrame()
    {
        // Wait if this frame's resources are still in use
        auto& frame = m_FrameContexts[m_CurrentFrameIndex];
        WaitForFence(frame.FenceValue);

        // Reset command allocator
        frame.CommandAllocator->Reset();

        return m_CurrentFrameIndex;
    }

    void DX12Core::EndFrame()
    {
        // Present
        UINT syncInterval = m_VSyncEnabled ? 1 : 0;
        UINT presentFlags = (!m_VSyncEnabled && m_TearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;

        HRESULT hr = m_SwapChain->Present(syncInterval, presentFlags);
        if (!CheckHResult(hr, "Present failed"))
        {
            // Handle device removed
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                std::cerr << "[DX12] Device lost! Reason: 0x"
                          << std::hex << m_Device->GetDeviceRemovedReason()
                          << std::dec << std::endl;
            }
        }

        // Signal and move to next frame
        MoveToNextFrame();
    }

    void DX12Core::WaitForGPU()
    {
        // Signal and wait for all frames
        uint64_t fenceValue = Signal();
        WaitForFence(fenceValue);
    }

    void DX12Core::WaitForFence(uint64_t fenceValue)
    {
        if (m_Fence->GetCompletedValue() < fenceValue)
        {
            m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
            WaitForSingleObject(m_FenceEvent, INFINITE);
        }
    }

    void DX12Core::ExecuteCommandLists(uint32_t numLists, ID3D12CommandList* const* ppCommandLists)
    {
        m_DirectQueue->ExecuteCommandLists(numLists, ppCommandLists);
    }

    uint64_t DX12Core::Signal()
    {
        m_FenceValue++;
        m_DirectQueue->Signal(m_Fence.Get(), m_FenceValue);
        return m_FenceValue;
    }

    void DX12Core::EnableDebugLayer()
    {
#if defined(_DEBUG)
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable GPU-based validation (slower but catches more issues)
            if (SUCCEEDED(debugController.As(&m_DebugController)))
            {
                m_DebugController->SetEnableGPUBasedValidation(FALSE); // Too slow for regular use
                m_DebugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
            }

            std::cout << "[DX12] Debug layer enabled." << std::endl;
        }
        else
        {
            std::cerr << "[DX12] Warning: Failed to enable debug layer." << std::endl;
        }
#endif
    }

    bool DX12Core::CreateFactory()
    {
        UINT dxgiFlags = 0;

#if defined(_DEBUG)
        dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        HRESULT hr = CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&m_Factory));
        if (!CheckHResult(hr, "Failed to create DXGI factory"))
        {
            return false;
        }

        // Check for tearing support
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(m_Factory.As(&factory5)))
        {
            BOOL allowTearing = FALSE;
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            m_TearingSupported = SUCCEEDED(hr) && allowTearing;
        }

        return true;
    }

    bool DX12Core::SelectAdapter()
    {
        ComPtr<IDXGIAdapter1> adapter;
        SIZE_T maxVideoMemory = 0;

        std::cout << "[DX12] Enumerating GPU adapters..." << std::endl;

        for (UINT i = 0;
             m_Factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
             ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Skip software adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            // Check if adapter supports D3D12
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
            {
                // Convert wide string to narrow for output
                char adapterName[256];
                WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, adapterName, sizeof(adapterName), nullptr, nullptr);

                std::cout << "[DX12] Found adapter: " << adapterName
                          << " (VRAM: " << (desc.DedicatedVideoMemory / (1024 * 1024)) << " MB)" << std::endl;

                // Select adapter with most video memory
                if (desc.DedicatedVideoMemory > maxVideoMemory)
                {
                    maxVideoMemory = desc.DedicatedVideoMemory;
                    adapter.As(&m_Adapter);
                }
            }
        }

        if (!m_Adapter)
        {
            std::cerr << "[DX12] No suitable D3D12 adapter found!" << std::endl;
            return false;
        }

        DXGI_ADAPTER_DESC1 desc;
        m_Adapter->GetDesc1(&desc);
        char adapterName[256];
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, adapterName, sizeof(adapterName), nullptr, nullptr);
        std::cout << "[DX12] Selected adapter: " << adapterName << std::endl;

        return true;
    }

    bool DX12Core::CreateDevice()
    {
        // Try feature levels from highest to lowest
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        D3D_FEATURE_LEVEL selectedLevel = D3D_FEATURE_LEVEL_11_0;

        for (auto level : featureLevels)
        {
            HRESULT hr = D3D12CreateDevice(m_Adapter.Get(), level, IID_PPV_ARGS(&m_Device));
            if (SUCCEEDED(hr))
            {
                selectedLevel = level;
                break;
            }
        }

        if (!m_Device)
        {
            std::cerr << "[DX12] Failed to create D3D12 device!" << std::endl;
            return false;
        }

        std::cout << "[DX12] Created device with feature level " << FeatureLevelToString(selectedLevel) << std::endl;

#if defined(_DEBUG)
        // Configure debug message filtering
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(m_Device.As(&infoQueue)))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

            // Suppress some verbose messages
            D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
            D3D12_MESSAGE_ID denyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumSeverities = _countof(severities);
            filter.DenyList.pSeverityList = severities;
            filter.DenyList.NumIDs = _countof(denyIds);
            filter.DenyList.pIDList = denyIds;

            infoQueue->PushStorageFilter(&filter);
        }
#endif

        return true;
    }

    bool DX12Core::CreateCommandQueues()
    {
        // Direct command queue (for rendering)
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        HRESULT hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_DirectQueue));
        if (!CheckHResult(hr, "Failed to create direct command queue"))
        {
            return false;
        }

        // Copy command queue (for async uploads)
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CopyQueue));
        if (!CheckHResult(hr, "Failed to create copy command queue"))
        {
            return false;
        }

        std::cout << "[DX12] Created command queues (Direct + Copy)" << std::endl;
        return true;
    }

    bool DX12Core::CreateSwapChain()
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = m_BackBufferFormat;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = m_Factory->CreateSwapChainForHwnd(
            m_DirectQueue.Get(),
            m_Hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        );

        if (!CheckHResult(hr, "Failed to create swap chain"))
        {
            return false;
        }

        // Disable Alt+Enter fullscreen toggle (we handle it ourselves)
        m_Factory->MakeWindowAssociation(m_Hwnd, DXGI_MWA_NO_ALT_ENTER);

        hr = swapChain1.As(&m_SwapChain);
        if (!CheckHResult(hr, "Failed to get IDXGISwapChain4"))
        {
            return false;
        }

        m_CurrentFrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

        std::cout << "[DX12] Created swap chain (" << FRAME_BUFFER_COUNT << " buffers, "
                  << m_Width << "x" << m_Height << ")" << std::endl;

        return true;
    }

    bool DX12Core::CreateDescriptorHeaps()
    {
        // RTV heap for back buffers
        if (!m_RTVHeap.Initialize(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_COUNT + 16, false))
        {
            return false;
        }

        // DSV heap
        if (!m_DSVHeap.Initialize(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 4, false))
        {
            return false;
        }

        // CBV/SRV/UAV heap (shader visible)
        if (!m_CBVSRVUAVHeap.Initialize(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096, true))
        {
            return false;
        }

        std::cout << "[DX12] Created descriptor heaps (RTV, DSV, CBV/SRV/UAV)" << std::endl;
        return true;
    }

    bool DX12Core::CreateFrameResources()
    {
        for (uint32_t i = 0; i < FRAME_BUFFER_COUNT; ++i)
        {
            auto& frame = m_FrameContexts[i];

            // Get back buffer
            HRESULT hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&frame.BackBuffer));
            if (!CheckHResult(hr, "Failed to get back buffer"))
            {
                return false;
            }

            // Create RTV
            frame.RTVHandle = m_RTVHeap.Allocate();
            m_Device->CreateRenderTargetView(frame.BackBuffer.Get(), nullptr, frame.RTVHandle.CPU);

            // Create command allocator
            hr = m_Device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&frame.CommandAllocator)
            );

            if (!CheckHResult(hr, "Failed to create command allocator"))
            {
                return false;
            }

            frame.FenceValue = 0;
        }

        std::cout << "[DX12] Created frame resources (" << FRAME_BUFFER_COUNT << " frames)" << std::endl;
        return true;
    }

    bool DX12Core::CreateDepthBuffer()
    {
        D3D12_RESOURCE_DESC depthDesc = {};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Alignment = 0;
        depthDesc.Width = m_Width;
        depthDesc.Height = m_Height;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = m_DepthFormat;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = m_DepthFormat;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        HRESULT hr = m_Device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_DepthBuffer)
        );

        if (!CheckHResult(hr, "Failed to create depth buffer"))
        {
            return false;
        }

        // Create DSV
        m_DSVHandle = m_DSVHeap.Allocate();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = m_DepthFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        m_Device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsvDesc, m_DSVHandle.CPU);

        std::cout << "[DX12] Created depth buffer (" << m_Width << "x" << m_Height << ")" << std::endl;
        return true;
    }

    bool DX12Core::CreateSyncObjects()
    {
        HRESULT hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
        if (!CheckHResult(hr, "Failed to create fence"))
        {
            return false;
        }

        m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!m_FenceEvent)
        {
            std::cerr << "[DX12] Failed to create fence event!" << std::endl;
            return false;
        }

        m_FenceValue = 0;

        std::cout << "[DX12] Created synchronization objects" << std::endl;
        return true;
    }

    void DX12Core::MoveToNextFrame()
    {
        // Schedule signal for current frame
        uint64_t currentFenceValue = m_FenceValue;
        m_DirectQueue->Signal(m_Fence.Get(), currentFenceValue);
        m_FrameContexts[m_CurrentFrameIndex].FenceValue = currentFenceValue;
        m_FenceValue++;

        // Move to next frame
        m_CurrentFrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
    }

} // namespace SM
