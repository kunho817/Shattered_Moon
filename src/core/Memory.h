#pragma once

/**
 * @file Memory.h
 * @brief Shattered Moon Engine - Memory Management System
 *
 * Provides custom allocators for efficient memory management:
 * - PoolAllocator: Fixed-size block allocation with free list
 * - StackAllocator: LIFO allocation for frame-based temporary memory
 * - ObjectPool<T>: Type-safe object pooling with automatic expansion
 */

#include <cstdint>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>
#include <cassert>
#include <new>

namespace SM
{
    // ============================================================================
    // Memory Constants and Utilities
    // ============================================================================

    /**
     * @brief Default memory alignment (16 bytes for SIMD compatibility)
     */
    constexpr size_t DEFAULT_ALIGNMENT = 16;

    /**
     * @brief Memory page size (4KB)
     */
    constexpr size_t PAGE_SIZE = 4096;

    /**
     * @brief Align a size up to the given alignment
     * @param size Size to align
     * @param alignment Alignment requirement (must be power of 2)
     * @return Aligned size
     */
    constexpr size_t AlignUp(size_t size, size_t alignment)
    {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    /**
     * @brief Align a pointer up to the given alignment
     * @param ptr Pointer to align
     * @param alignment Alignment requirement (must be power of 2)
     * @return Aligned pointer
     */
    inline void* AlignPointer(void* ptr, size_t alignment)
    {
        return reinterpret_cast<void*>(
            AlignUp(reinterpret_cast<uintptr_t>(ptr), alignment)
        );
    }

    // ============================================================================
    // Memory Statistics (Debug)
    // ============================================================================

#if defined(_DEBUG)
    /**
     * @brief Memory tracking statistics for debugging
     */
    struct MemoryStats
    {
        size_t TotalAllocations = 0;
        size_t TotalDeallocations = 0;
        size_t CurrentAllocatedBytes = 0;
        size_t PeakAllocatedBytes = 0;
        size_t PoolAllocations = 0;
        size_t StackAllocations = 0;
        size_t ObjectPoolAllocations = 0;

        void RecordAllocation(size_t bytes)
        {
            TotalAllocations++;
            CurrentAllocatedBytes += bytes;
            if (CurrentAllocatedBytes > PeakAllocatedBytes)
            {
                PeakAllocatedBytes = CurrentAllocatedBytes;
            }
        }

        void RecordDeallocation(size_t bytes)
        {
            TotalDeallocations++;
            CurrentAllocatedBytes -= bytes;
        }

        void Reset()
        {
            TotalAllocations = 0;
            TotalDeallocations = 0;
            CurrentAllocatedBytes = 0;
            PeakAllocatedBytes = 0;
            PoolAllocations = 0;
            StackAllocations = 0;
            ObjectPoolAllocations = 0;
        }
    };

    /**
     * @brief Get global memory statistics
     * @return Reference to global memory stats
     */
    MemoryStats& GetMemoryStats();
#endif

    // ============================================================================
    // Pool Allocator
    // ============================================================================

    /**
     * @brief Fixed-size block allocator using a free list
     *
     * Efficient for allocating many objects of the same size.
     * Uses a free list for O(1) allocation and deallocation.
     *
     * Thread-safe when compiled with mutex enabled.
     *
     * Example usage:
     * @code
     *   PoolAllocator pool(sizeof(MyObject), 100);
     *   void* ptr = pool.Allocate();
     *   // Use ptr...
     *   pool.Deallocate(ptr);
     * @endcode
     */
    class PoolAllocator
    {
    public:
        /**
         * @brief Construct a pool allocator
         * @param blockSize Size of each block (will be aligned to minimum block size)
         * @param blockCount Number of blocks to pre-allocate
         * @param alignment Memory alignment (default: DEFAULT_ALIGNMENT)
         */
        PoolAllocator(size_t blockSize, size_t blockCount, size_t alignment = DEFAULT_ALIGNMENT);

        /**
         * @brief Destructor - releases all memory
         */
        ~PoolAllocator();

        // Non-copyable
        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;

        // Movable
        PoolAllocator(PoolAllocator&& other) noexcept;
        PoolAllocator& operator=(PoolAllocator&& other) noexcept;

        /**
         * @brief Allocate a single block from the pool
         * @return Pointer to allocated block, nullptr if pool is exhausted
         */
        void* Allocate();

        /**
         * @brief Deallocate a block back to the pool
         * @param ptr Pointer to block (must have been allocated from this pool)
         */
        void Deallocate(void* ptr);

        /**
         * @brief Reset the pool to initial state (all blocks free)
         * @note Does NOT call destructors - use only if objects are trivially destructible
         */
        void Reset();

        /**
         * @brief Get the number of free blocks
         * @return Number of available blocks
         */
        size_t GetFreeBlockCount() const;

        /**
         * @brief Get the total number of blocks
         * @return Total block count
         */
        size_t GetTotalBlockCount() const { return m_BlockCount; }

        /**
         * @brief Get the size of each block
         * @return Block size in bytes
         */
        size_t GetBlockSize() const { return m_BlockSize; }

        /**
         * @brief Check if a pointer was allocated from this pool
         * @param ptr Pointer to check
         * @return true if ptr belongs to this pool
         */
        bool Contains(void* ptr) const;

    private:
        /**
         * @brief Free list node (stored at the beginning of each free block)
         */
        struct FreeBlock
        {
            FreeBlock* Next;
        };

        void InitializeFreeList();

        uint8_t* m_Memory = nullptr;       // Raw memory buffer
        FreeBlock* m_FreeList = nullptr;   // Head of free list
        size_t m_BlockSize = 0;            // Size of each block
        size_t m_BlockCount = 0;           // Total number of blocks
        size_t m_FreeCount = 0;            // Number of free blocks
        size_t m_Alignment = DEFAULT_ALIGNMENT;

        mutable std::mutex m_Mutex;        // Thread safety
    };

    // ============================================================================
    // Stack Allocator
    // ============================================================================

    /**
     * @brief Marker type for stack allocator
     *
     * Used to mark positions for rolling back allocations.
     */
    using StackMarker = size_t;

    /**
     * @brief LIFO (Last-In-First-Out) stack allocator
     *
     * Efficient for frame-based temporary allocations where all allocations
     * in a frame are released together at the end.
     *
     * Example usage:
     * @code
     *   StackAllocator stack(1024 * 1024); // 1MB
     *
     *   // Each frame:
     *   StackMarker frameStart = stack.GetMarker();
     *   void* temp1 = stack.Push(sizeof(Matrix4));
     *   void* temp2 = stack.Push(sizeof(Vector3) * 100);
     *   // Use allocations...
     *   stack.Pop(frameStart); // Release all frame allocations
     * @endcode
     */
    class StackAllocator
    {
    public:
        /**
         * @brief Construct a stack allocator
         * @param totalSize Total size of the stack in bytes
         * @param alignment Default alignment for allocations
         */
        explicit StackAllocator(size_t totalSize, size_t alignment = DEFAULT_ALIGNMENT);

        /**
         * @brief Destructor - releases all memory
         */
        ~StackAllocator();

        // Non-copyable
        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;

        // Movable
        StackAllocator(StackAllocator&& other) noexcept;
        StackAllocator& operator=(StackAllocator&& other) noexcept;

        /**
         * @brief Allocate memory from the stack
         * @param size Size in bytes to allocate
         * @param alignment Alignment requirement (default: use allocator's default)
         * @return Pointer to allocated memory, nullptr if stack overflow
         */
        void* Push(size_t size, size_t alignment = 0);

        /**
         * @brief Allocate and zero-initialize memory
         * @param size Size in bytes to allocate
         * @param alignment Alignment requirement
         * @return Pointer to zero-initialized memory, nullptr if stack overflow
         */
        void* PushZeroed(size_t size, size_t alignment = 0);

        /**
         * @brief Allocate memory for a specific type
         * @tparam T Type to allocate
         * @param count Number of elements (default: 1)
         * @return Pointer to allocated memory
         */
        template<typename T>
        T* PushType(size_t count = 1)
        {
            return static_cast<T*>(Push(sizeof(T) * count, alignof(T)));
        }

        /**
         * @brief Roll back to a previous marker position
         * @param marker Marker obtained from GetMarker()
         */
        void Pop(StackMarker marker);

        /**
         * @brief Clear the entire stack (reset to empty)
         */
        void Clear();

        /**
         * @brief Get the current stack position as a marker
         * @return Current marker that can be used for Pop()
         */
        StackMarker GetMarker() const { return m_Offset; }

        /**
         * @brief Get the number of bytes currently allocated
         * @return Used bytes
         */
        size_t GetUsedSize() const { return m_Offset; }

        /**
         * @brief Get the total capacity of the stack
         * @return Total size in bytes
         */
        size_t GetTotalSize() const { return m_TotalSize; }

        /**
         * @brief Get the number of bytes remaining
         * @return Free bytes
         */
        size_t GetFreeSize() const { return m_TotalSize - m_Offset; }

    private:
        uint8_t* m_Memory = nullptr;       // Raw memory buffer
        size_t m_TotalSize = 0;            // Total size of the stack
        size_t m_Offset = 0;               // Current top of stack
        size_t m_DefaultAlignment = DEFAULT_ALIGNMENT;

        mutable std::mutex m_Mutex;        // Thread safety
    };

    // ============================================================================
    // Object Pool
    // ============================================================================

    /**
     * @brief Type-safe object pool with automatic expansion
     *
     * Provides efficient reuse of objects of a specific type.
     * Objects are constructed when acquired and can be reset when released.
     *
     * @tparam T Type of objects to pool
     *
     * Example usage:
     * @code
     *   ObjectPool<Particle> pool(100);
     *
     *   Particle* p = pool.Acquire();
     *   p->Initialize(...);
     *   // Use particle...
     *   pool.Release(p);
     * @endcode
     */
    template<typename T>
    class ObjectPool
    {
    public:
        /**
         * @brief Construct an object pool
         * @param initialCapacity Initial number of objects to pre-allocate
         * @param autoExpand Whether to automatically expand when exhausted
         * @param expansionSize Number of objects to add when expanding
         */
        explicit ObjectPool(
            size_t initialCapacity = 64,
            bool autoExpand = true,
            size_t expansionSize = 32
        )
            : m_AutoExpand(autoExpand)
            , m_ExpansionSize(expansionSize)
        {
            Expand(initialCapacity);
        }

        /**
         * @brief Destructor - destroys all objects and releases memory
         */
        ~ObjectPool()
        {
            Clear();
            for (auto* chunk : m_Chunks)
            {
                ::operator delete(chunk, std::align_val_t{ alignof(T) });
            }
        }

        // Non-copyable
        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;

        // Movable
        ObjectPool(ObjectPool&& other) noexcept
            : m_FreeList(std::move(other.m_FreeList))
            , m_Chunks(std::move(other.m_Chunks))
            , m_ActiveCount(other.m_ActiveCount)
            , m_TotalCapacity(other.m_TotalCapacity)
            , m_AutoExpand(other.m_AutoExpand)
            , m_ExpansionSize(other.m_ExpansionSize)
        {
            other.m_ActiveCount = 0;
            other.m_TotalCapacity = 0;
        }

        ObjectPool& operator=(ObjectPool&& other) noexcept
        {
            if (this != &other)
            {
                Clear();
                for (auto* chunk : m_Chunks)
                {
                    ::operator delete(chunk, std::align_val_t{ alignof(T) });
                }

                m_FreeList = std::move(other.m_FreeList);
                m_Chunks = std::move(other.m_Chunks);
                m_ActiveCount = other.m_ActiveCount;
                m_TotalCapacity = other.m_TotalCapacity;
                m_AutoExpand = other.m_AutoExpand;
                m_ExpansionSize = other.m_ExpansionSize;

                other.m_ActiveCount = 0;
                other.m_TotalCapacity = 0;
            }
            return *this;
        }

        /**
         * @brief Acquire an object from the pool
         * @tparam Args Constructor argument types
         * @param args Arguments to forward to object constructor
         * @return Pointer to acquired object, nullptr if pool exhausted and !autoExpand
         */
        template<typename... Args>
        T* Acquire(Args&&... args)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            if (m_FreeList.empty())
            {
                if (m_AutoExpand)
                {
                    Expand(m_ExpansionSize);
                }
                else
                {
                    return nullptr;
                }
            }

            T* obj = m_FreeList.back();
            m_FreeList.pop_back();
            m_ActiveCount++;

#if defined(_DEBUG)
            GetMemoryStats().ObjectPoolAllocations++;
#endif

            // Construct object in-place
            new (obj) T(std::forward<Args>(args)...);
            return obj;
        }

        /**
         * @brief Release an object back to the pool
         * @param obj Pointer to object (must have been acquired from this pool)
         */
        void Release(T* obj)
        {
            if (!obj) return;

            std::lock_guard<std::mutex> lock(m_Mutex);

            // Call destructor
            obj->~T();

            m_FreeList.push_back(obj);
            m_ActiveCount--;
        }

        /**
         * @brief Clear all objects (calls destructors on active objects)
         * @note After clearing, all objects are available for reuse
         */
        void Clear()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            // Reset free list with all objects
            m_FreeList.clear();
            m_FreeList.reserve(m_TotalCapacity);

            for (auto* chunk : m_Chunks)
            {
                for (size_t i = 0; i < m_ChunkSizes[&chunk - m_Chunks.data()]; ++i)
                {
                    m_FreeList.push_back(&chunk[i]);
                }
            }

            m_ActiveCount = 0;
        }

        /**
         * @brief Get the number of active (acquired) objects
         * @return Active object count
         */
        size_t GetActiveCount() const { return m_ActiveCount; }

        /**
         * @brief Get the number of free (available) objects
         * @return Free object count
         */
        size_t GetFreeCount() const
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return m_FreeList.size();
        }

        /**
         * @brief Get the total capacity of the pool
         * @return Total object count
         */
        size_t GetTotalCapacity() const { return m_TotalCapacity; }

        /**
         * @brief Check if auto-expansion is enabled
         * @return true if pool will expand when exhausted
         */
        bool IsAutoExpandEnabled() const { return m_AutoExpand; }

        /**
         * @brief Enable or disable auto-expansion
         * @param enable Whether to enable auto-expansion
         */
        void SetAutoExpand(bool enable) { m_AutoExpand = enable; }

    private:
        /**
         * @brief Expand the pool by adding more objects
         * @param count Number of objects to add
         */
        void Expand(size_t count)
        {
            // Allocate new chunk with proper alignment
            T* chunk = static_cast<T*>(
                ::operator new(sizeof(T) * count, std::align_val_t{ alignof(T) })
            );

            m_Chunks.push_back(chunk);
            m_ChunkSizes.push_back(count);

            // Add all new objects to free list
            m_FreeList.reserve(m_FreeList.size() + count);
            for (size_t i = 0; i < count; ++i)
            {
                m_FreeList.push_back(&chunk[i]);
            }

            m_TotalCapacity += count;

#if defined(_DEBUG)
            GetMemoryStats().RecordAllocation(sizeof(T) * count);
#endif
        }

        std::vector<T*> m_FreeList;              // Free object pointers
        std::vector<T*> m_Chunks;                // Allocated memory chunks
        std::vector<size_t> m_ChunkSizes;        // Size of each chunk
        size_t m_ActiveCount = 0;                // Currently acquired objects
        size_t m_TotalCapacity = 0;              // Total objects in pool
        bool m_AutoExpand = true;                // Auto-expand when exhausted
        size_t m_ExpansionSize = 32;             // Objects to add when expanding

        mutable std::mutex m_Mutex;              // Thread safety
    };

    // ============================================================================
    // Scoped Stack Allocation Helper
    // ============================================================================

    /**
     * @brief RAII helper for automatic stack allocation cleanup
     *
     * Automatically rolls back the stack allocator when going out of scope.
     *
     * Example usage:
     * @code
     *   StackAllocator& stack = GetFrameStack();
     *   {
     *       ScopedStackAllocation scope(stack);
     *       void* temp = stack.Push(1024);
     *       // Use temp...
     *   } // Automatically rolled back here
     * @endcode
     */
    class ScopedStackAllocation
    {
    public:
        explicit ScopedStackAllocation(StackAllocator& allocator)
            : m_Allocator(allocator)
            , m_Marker(allocator.GetMarker())
        {
        }

        ~ScopedStackAllocation()
        {
            m_Allocator.Pop(m_Marker);
        }

        // Non-copyable, non-movable
        ScopedStackAllocation(const ScopedStackAllocation&) = delete;
        ScopedStackAllocation& operator=(const ScopedStackAllocation&) = delete;

    private:
        StackAllocator& m_Allocator;
        StackMarker m_Marker;
    };

    // ============================================================================
    // Global Memory Manager
    // ============================================================================

    /**
     * @brief Central memory manager for engine-wide allocations
     *
     * Provides access to shared allocators and memory statistics.
     */
    class MemoryManager
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the memory manager
         */
        static MemoryManager& Get();

        // Non-copyable
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;

        /**
         * @brief Initialize the memory manager with specified sizes
         * @param frameStackSize Size of per-frame stack allocator
         * @param persistentStackSize Size of persistent stack allocator
         * @return true if initialization succeeded
         */
        bool Initialize(size_t frameStackSize = 4 * 1024 * 1024,
                       size_t persistentStackSize = 16 * 1024 * 1024);

        /**
         * @brief Shutdown and release all memory
         */
        void Shutdown();

        /**
         * @brief Get the frame stack allocator (cleared each frame)
         * @return Pointer to frame stack allocator
         */
        StackAllocator* GetFrameStack() { return m_FrameStack.get(); }

        /**
         * @brief Get the persistent stack allocator
         * @return Pointer to persistent stack allocator
         */
        StackAllocator* GetPersistentStack() { return m_PersistentStack.get(); }

        /**
         * @brief Clear the frame stack (call at end of each frame)
         */
        void ClearFrameStack();

        /**
         * @brief Check if the memory manager is initialized
         * @return true if initialized
         */
        bool IsInitialized() const { return m_IsInitialized; }

#if defined(_DEBUG)
        /**
         * @brief Get memory statistics
         * @return Reference to memory statistics
         */
        MemoryStats& GetStats() { return m_Stats; }

        /**
         * @brief Print memory statistics to debug output
         */
        void PrintStats() const;
#endif

    private:
        MemoryManager() = default;
        ~MemoryManager();

        bool m_IsInitialized = false;
        std::unique_ptr<StackAllocator> m_FrameStack;
        std::unique_ptr<StackAllocator> m_PersistentStack;

#if defined(_DEBUG)
        MemoryStats m_Stats;
#endif
    };

} // namespace SM
