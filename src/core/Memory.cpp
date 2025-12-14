#include "core/Memory.h"

#include <cstring>
#include <iostream>

namespace SM
{
    // ============================================================================
    // Memory Statistics (Debug)
    // ============================================================================

#if defined(_DEBUG)
    MemoryStats& GetMemoryStats()
    {
        return MemoryManager::Get().GetStats();
    }
#endif

    // ============================================================================
    // Pool Allocator Implementation
    // ============================================================================

    PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount, size_t alignment)
        : m_BlockSize(blockSize)
        , m_BlockCount(blockCount)
        , m_Alignment(alignment)
    {
        // Ensure block size is at least large enough to hold a FreeBlock pointer
        // and properly aligned
        size_t minBlockSize = sizeof(FreeBlock);
        if (m_BlockSize < minBlockSize)
        {
            m_BlockSize = minBlockSize;
        }
        m_BlockSize = AlignUp(m_BlockSize, m_Alignment);

        // Allocate aligned memory
        size_t totalSize = m_BlockSize * m_BlockCount;
        m_Memory = static_cast<uint8_t*>(
            ::operator new(totalSize, std::align_val_t{ m_Alignment })
        );

        InitializeFreeList();

#if defined(_DEBUG)
        GetMemoryStats().RecordAllocation(totalSize);
#endif
    }

    PoolAllocator::~PoolAllocator()
    {
        if (m_Memory)
        {
#if defined(_DEBUG)
            GetMemoryStats().RecordDeallocation(m_BlockSize * m_BlockCount);
#endif
            ::operator delete(m_Memory, std::align_val_t{ m_Alignment });
            m_Memory = nullptr;
        }
    }

    PoolAllocator::PoolAllocator(PoolAllocator&& other) noexcept
        : m_Memory(other.m_Memory)
        , m_FreeList(other.m_FreeList)
        , m_BlockSize(other.m_BlockSize)
        , m_BlockCount(other.m_BlockCount)
        , m_FreeCount(other.m_FreeCount)
        , m_Alignment(other.m_Alignment)
    {
        other.m_Memory = nullptr;
        other.m_FreeList = nullptr;
        other.m_BlockCount = 0;
        other.m_FreeCount = 0;
    }

    PoolAllocator& PoolAllocator::operator=(PoolAllocator&& other) noexcept
    {
        if (this != &other)
        {
            if (m_Memory)
            {
                ::operator delete(m_Memory, std::align_val_t{ m_Alignment });
            }

            m_Memory = other.m_Memory;
            m_FreeList = other.m_FreeList;
            m_BlockSize = other.m_BlockSize;
            m_BlockCount = other.m_BlockCount;
            m_FreeCount = other.m_FreeCount;
            m_Alignment = other.m_Alignment;

            other.m_Memory = nullptr;
            other.m_FreeList = nullptr;
            other.m_BlockCount = 0;
            other.m_FreeCount = 0;
        }
        return *this;
    }

    void PoolAllocator::InitializeFreeList()
    {
        m_FreeList = nullptr;
        m_FreeCount = 0;

        // Build free list from end to start so first allocation returns first block
        for (size_t i = m_BlockCount; i > 0; --i)
        {
            FreeBlock* block = reinterpret_cast<FreeBlock*>(m_Memory + (i - 1) * m_BlockSize);
            block->Next = m_FreeList;
            m_FreeList = block;
            m_FreeCount++;
        }
    }

    void* PoolAllocator::Allocate()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (!m_FreeList)
        {
            return nullptr; // Pool exhausted
        }

        // Pop from free list
        FreeBlock* block = m_FreeList;
        m_FreeList = block->Next;
        m_FreeCount--;

#if defined(_DEBUG)
        GetMemoryStats().PoolAllocations++;
#endif

        return block;
    }

    void PoolAllocator::Deallocate(void* ptr)
    {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(m_Mutex);

        assert(Contains(ptr) && "Pointer does not belong to this pool");

        // Push to free list
        FreeBlock* block = static_cast<FreeBlock*>(ptr);
        block->Next = m_FreeList;
        m_FreeList = block;
        m_FreeCount++;
    }

    void PoolAllocator::Reset()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        InitializeFreeList();
    }

    size_t PoolAllocator::GetFreeBlockCount() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_FreeCount;
    }

    bool PoolAllocator::Contains(void* ptr) const
    {
        if (!ptr || !m_Memory) return false;

        uint8_t* p = static_cast<uint8_t*>(ptr);
        uint8_t* start = m_Memory;
        uint8_t* end = m_Memory + m_BlockSize * m_BlockCount;

        return p >= start && p < end;
    }

    // ============================================================================
    // Stack Allocator Implementation
    // ============================================================================

    StackAllocator::StackAllocator(size_t totalSize, size_t alignment)
        : m_TotalSize(totalSize)
        , m_DefaultAlignment(alignment)
    {
        m_Memory = static_cast<uint8_t*>(
            ::operator new(totalSize, std::align_val_t{ alignment })
        );

#if defined(_DEBUG)
        GetMemoryStats().RecordAllocation(totalSize);
#endif
    }

    StackAllocator::~StackAllocator()
    {
        if (m_Memory)
        {
#if defined(_DEBUG)
            GetMemoryStats().RecordDeallocation(m_TotalSize);
#endif
            ::operator delete(m_Memory, std::align_val_t{ m_DefaultAlignment });
            m_Memory = nullptr;
        }
    }

    StackAllocator::StackAllocator(StackAllocator&& other) noexcept
        : m_Memory(other.m_Memory)
        , m_TotalSize(other.m_TotalSize)
        , m_Offset(other.m_Offset)
        , m_DefaultAlignment(other.m_DefaultAlignment)
    {
        other.m_Memory = nullptr;
        other.m_TotalSize = 0;
        other.m_Offset = 0;
    }

    StackAllocator& StackAllocator::operator=(StackAllocator&& other) noexcept
    {
        if (this != &other)
        {
            if (m_Memory)
            {
                ::operator delete(m_Memory, std::align_val_t{ m_DefaultAlignment });
            }

            m_Memory = other.m_Memory;
            m_TotalSize = other.m_TotalSize;
            m_Offset = other.m_Offset;
            m_DefaultAlignment = other.m_DefaultAlignment;

            other.m_Memory = nullptr;
            other.m_TotalSize = 0;
            other.m_Offset = 0;
        }
        return *this;
    }

    void* StackAllocator::Push(size_t size, size_t alignment)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (alignment == 0)
        {
            alignment = m_DefaultAlignment;
        }

        // Calculate aligned offset
        size_t alignedOffset = AlignUp(m_Offset, alignment);
        size_t newOffset = alignedOffset + size;

        if (newOffset > m_TotalSize)
        {
            return nullptr; // Stack overflow
        }

        void* ptr = m_Memory + alignedOffset;
        m_Offset = newOffset;

#if defined(_DEBUG)
        GetMemoryStats().StackAllocations++;
#endif

        return ptr;
    }

    void* StackAllocator::PushZeroed(size_t size, size_t alignment)
    {
        void* ptr = Push(size, alignment);
        if (ptr)
        {
            std::memset(ptr, 0, size);
        }
        return ptr;
    }

    void StackAllocator::Pop(StackMarker marker)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        assert(marker <= m_Offset && "Invalid marker - trying to pop beyond current offset");
        m_Offset = marker;
    }

    void StackAllocator::Clear()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Offset = 0;
    }

    // ============================================================================
    // Memory Manager Implementation
    // ============================================================================

    MemoryManager& MemoryManager::Get()
    {
        static MemoryManager instance;
        return instance;
    }

    MemoryManager::~MemoryManager()
    {
        Shutdown();
    }

    bool MemoryManager::Initialize(size_t frameStackSize, size_t persistentStackSize)
    {
        if (m_IsInitialized)
        {
            return true;
        }

        m_FrameStack = std::make_unique<StackAllocator>(frameStackSize);
        m_PersistentStack = std::make_unique<StackAllocator>(persistentStackSize);

        m_IsInitialized = true;

#if defined(_DEBUG)
        std::cout << "[MemoryManager] Initialized with:" << std::endl;
        std::cout << "  Frame Stack: " << (frameStackSize / 1024) << " KB" << std::endl;
        std::cout << "  Persistent Stack: " << (persistentStackSize / 1024) << " KB" << std::endl;
#endif

        return true;
    }

    void MemoryManager::Shutdown()
    {
        if (!m_IsInitialized)
        {
            return;
        }

#if defined(_DEBUG)
        PrintStats();
#endif

        m_FrameStack.reset();
        m_PersistentStack.reset();
        m_IsInitialized = false;
    }

    void MemoryManager::ClearFrameStack()
    {
        if (m_FrameStack)
        {
            m_FrameStack->Clear();
        }
    }

#if defined(_DEBUG)
    void MemoryManager::PrintStats() const
    {
        std::cout << "=== Memory Statistics ===" << std::endl;
        std::cout << "Total Allocations: " << m_Stats.TotalAllocations << std::endl;
        std::cout << "Total Deallocations: " << m_Stats.TotalDeallocations << std::endl;
        std::cout << "Current Allocated: " << m_Stats.CurrentAllocatedBytes << " bytes" << std::endl;
        std::cout << "Peak Allocated: " << m_Stats.PeakAllocatedBytes << " bytes" << std::endl;
        std::cout << "Pool Allocations: " << m_Stats.PoolAllocations << std::endl;
        std::cout << "Stack Allocations: " << m_Stats.StackAllocations << std::endl;
        std::cout << "Object Pool Allocations: " << m_Stats.ObjectPoolAllocations << std::endl;

        if (m_FrameStack)
        {
            std::cout << "Frame Stack Used: " << m_FrameStack->GetUsedSize()
                      << " / " << m_FrameStack->GetTotalSize() << " bytes" << std::endl;
        }

        if (m_PersistentStack)
        {
            std::cout << "Persistent Stack Used: " << m_PersistentStack->GetUsedSize()
                      << " / " << m_PersistentStack->GetTotalSize() << " bytes" << std::endl;
        }

        std::cout << "=========================" << std::endl;
    }
#endif

} // namespace SM
