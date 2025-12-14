#pragma once

/**
 * @file ResourceManager.h
 * @brief Shattered Moon Engine - Resource Management System
 *
 * Provides centralized management of game resources with:
 * - Reference counting for automatic resource lifetime management
 * - Asynchronous loading support using std::async
 * - Type-safe resource access through handles
 * - Hot-reloading capability for development
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <future>
#include <functional>
#include <atomic>

namespace SM
{
    // Forward declarations
    class IAssetLoader;

    // ============================================================================
    // Resource Types
    // ============================================================================

    /**
     * @brief Types of resources managed by the ResourceManager
     */
    enum class ResourceType : uint8_t
    {
        Unknown = 0,
        Texture,     // 2D textures, cubemaps, etc.
        Mesh,        // 3D geometry
        Shader,      // Vertex, pixel, compute shaders
        Material,    // Material definitions
        Audio,       // Sound effects and music
        Script,      // Lua or other script files
        Font,        // Font files
        Animation,   // Animation data
        Prefab,      // Prefab definitions
        Scene,       // Scene data
        Config,      // Configuration files
        RawData,     // Generic binary data

        Count        // Number of resource types
    };

    /**
     * @brief Convert resource type to string
     * @param type Resource type
     * @return String representation
     */
    inline const char* ResourceTypeToString(ResourceType type)
    {
        switch (type)
        {
            case ResourceType::Texture:   return "Texture";
            case ResourceType::Mesh:      return "Mesh";
            case ResourceType::Shader:    return "Shader";
            case ResourceType::Material:  return "Material";
            case ResourceType::Audio:     return "Audio";
            case ResourceType::Script:    return "Script";
            case ResourceType::Font:      return "Font";
            case ResourceType::Animation: return "Animation";
            case ResourceType::Prefab:    return "Prefab";
            case ResourceType::Scene:     return "Scene";
            case ResourceType::Config:    return "Config";
            case ResourceType::RawData:   return "RawData";
            default:                      return "Unknown";
        }
    }

    /**
     * @brief Get resource type from file extension
     * @param extension File extension (e.g., ".png")
     * @return Corresponding resource type
     */
    ResourceType GetResourceTypeFromExtension(const std::string& extension);

    // ============================================================================
    // Resource Handle
    // ============================================================================

    /**
     * @brief Handle to a managed resource
     *
     * Lightweight identifier for accessing resources through the ResourceManager.
     * Handles are invalidated when resources are unloaded.
     *
     * Example usage:
     * @code
     *   ResourceHandle texHandle = resourceManager.Load<Texture>("textures/player.png");
     *   if (texHandle.IsValid()) {
     *       Texture* tex = resourceManager.Get<Texture>(texHandle);
     *   }
     * @endcode
     */
    struct ResourceHandle
    {
        static constexpr uint32_t INVALID_ID = 0;

        uint32_t ID = INVALID_ID;           // Unique resource identifier
        ResourceType Type = ResourceType::Unknown;

        /**
         * @brief Check if the handle is valid
         * @return true if handle points to a valid resource
         */
        bool IsValid() const
        {
            return ID != INVALID_ID && Type != ResourceType::Unknown;
        }

        /**
         * @brief Reset handle to invalid state
         */
        void Invalidate()
        {
            ID = INVALID_ID;
            Type = ResourceType::Unknown;
        }

        bool operator==(const ResourceHandle& other) const
        {
            return ID == other.ID && Type == other.Type;
        }

        bool operator!=(const ResourceHandle& other) const
        {
            return !(*this == other);
        }
    };

    // Hash function for ResourceHandle
    struct ResourceHandleHash
    {
        size_t operator()(const ResourceHandle& handle) const
        {
            return std::hash<uint32_t>()(handle.ID) ^
                   (std::hash<uint8_t>()(static_cast<uint8_t>(handle.Type)) << 16);
        }
    };

    // ============================================================================
    // Resource Entry
    // ============================================================================

    /**
     * @brief Internal resource entry with metadata and data
     */
    struct ResourceEntry
    {
        ResourceHandle Handle;             // Resource handle
        std::string Path;                  // Source file path
        std::string Name;                  // Resource name
        void* Data = nullptr;              // Actual resource data
        size_t DataSize = 0;               // Size of data in bytes
        std::atomic<uint32_t> RefCount{0}; // Reference count
        uint64_t LastModified = 0;         // File modification time
        bool IsLoaded = false;             // Is data currently loaded
        bool IsLoading = false;            // Is currently being loaded async
        std::future<bool> LoadFuture;      // Future for async loading

        ResourceEntry() = default;

        // Non-copyable due to atomic
        ResourceEntry(const ResourceEntry&) = delete;
        ResourceEntry& operator=(const ResourceEntry&) = delete;

        // Movable
        ResourceEntry(ResourceEntry&& other) noexcept
            : Handle(other.Handle)
            , Path(std::move(other.Path))
            , Name(std::move(other.Name))
            , Data(other.Data)
            , DataSize(other.DataSize)
            , RefCount(other.RefCount.load())
            , LastModified(other.LastModified)
            , IsLoaded(other.IsLoaded)
            , IsLoading(other.IsLoading)
            , LoadFuture(std::move(other.LoadFuture))
        {
            other.Data = nullptr;
            other.IsLoaded = false;
        }
    };

    // ============================================================================
    // Load Request
    // ============================================================================

    /**
     * @brief Request structure for async loading
     */
    struct ResourceLoadRequest
    {
        std::string Path;
        ResourceType Type = ResourceType::Unknown;
        std::function<void(ResourceHandle)> Callback;
        bool HighPriority = false;
    };

    // ============================================================================
    // Resource Manager
    // ============================================================================

    /**
     * @brief Central manager for all game resources
     *
     * Provides:
     * - Reference counted resource management
     * - Synchronous and asynchronous loading
     * - Type-safe resource access
     * - Hot-reloading support for development
     *
     * Example usage:
     * @code
     *   auto& rm = ResourceManager::Get();
     *
     *   // Synchronous loading
     *   ResourceHandle tex = rm.Load<Texture>("textures/player.png");
     *   Texture* texData = rm.Get<Texture>(tex);
     *
     *   // Async loading
     *   rm.LoadAsync<Mesh>("models/enemy.obj", [](ResourceHandle h) {
     *       // Mesh is now loaded
     *   });
     *
     *   // Release when done
     *   rm.Release(tex);
     * @endcode
     */
    class ResourceManager
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the resource manager
         */
        static ResourceManager& Get();

        // Non-copyable
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        /**
         * @brief Initialize the resource manager
         * @param maxAsyncLoads Maximum concurrent async load operations
         * @return true if successful
         */
        bool Initialize(uint32_t maxAsyncLoads = 4);

        /**
         * @brief Shutdown and release all resources
         */
        void Shutdown();

        /**
         * @brief Update the resource manager (process async loads)
         * Called once per frame.
         */
        void Update();

        // ====================================================================
        // Resource Loading
        // ====================================================================

        /**
         * @brief Load a resource synchronously
         * @tparam T Resource type (for type deduction)
         * @param path Path to the resource file
         * @return Handle to the loaded resource
         */
        template<typename T>
        ResourceHandle Load(const std::string& path)
        {
            return LoadResource(path, GetResourceTypeForCppType<T>());
        }

        /**
         * @brief Load a resource by path and type
         * @param path Path to the resource file
         * @param type Resource type
         * @return Handle to the loaded resource
         */
        ResourceHandle LoadResource(const std::string& path, ResourceType type);

        /**
         * @brief Load a resource asynchronously
         * @tparam T Resource type
         * @param path Path to the resource file
         * @param callback Function to call when loading completes
         */
        template<typename T>
        void LoadAsync(const std::string& path,
                      std::function<void(ResourceHandle)> callback = nullptr)
        {
            LoadResourceAsync(path, GetResourceTypeForCppType<T>(), callback);
        }

        /**
         * @brief Load a resource asynchronously by type
         * @param path Path to the resource file
         * @param type Resource type
         * @param callback Function to call when loading completes
         */
        void LoadResourceAsync(
            const std::string& path,
            ResourceType type,
            std::function<void(ResourceHandle)> callback = nullptr
        );

        // ====================================================================
        // Resource Access
        // ====================================================================

        /**
         * @brief Get resource data by handle
         * @tparam T Expected data type
         * @param handle Resource handle
         * @return Pointer to resource data, nullptr if invalid
         */
        template<typename T>
        T* Get(ResourceHandle handle)
        {
            return static_cast<T*>(GetResourceData(handle));
        }

        /**
         * @brief Get resource data by handle (non-template)
         * @param handle Resource handle
         * @return Pointer to resource data, nullptr if invalid
         */
        void* GetResourceData(ResourceHandle handle);

        /**
         * @brief Get a resource handle by path
         * @param path Resource file path
         * @return Handle if resource is loaded, invalid handle otherwise
         */
        ResourceHandle GetHandle(const std::string& path) const;

        /**
         * @brief Check if a resource is currently loaded
         * @param handle Resource handle
         * @return true if resource is loaded and valid
         */
        bool IsLoaded(ResourceHandle handle) const;

        /**
         * @brief Check if a resource is currently loading asynchronously
         * @param handle Resource handle
         * @return true if resource is being loaded
         */
        bool IsLoading(ResourceHandle handle) const;

        // ====================================================================
        // Resource Lifetime
        // ====================================================================

        /**
         * @brief Add a reference to a resource
         * @param handle Resource handle
         */
        void AddRef(ResourceHandle handle);

        /**
         * @brief Release a reference to a resource
         * @param handle Resource handle
         * @note Resource is unloaded when ref count reaches 0
         */
        void Release(ResourceHandle handle);

        /**
         * @brief Unload a resource immediately (ignores ref count)
         * @param handle Resource handle
         */
        void Unload(ResourceHandle handle);

        /**
         * @brief Unload all resources of a specific type
         * @param type Resource type to unload
         */
        void UnloadAllOfType(ResourceType type);

        /**
         * @brief Unload all resources
         */
        void UnloadAll();

        // ====================================================================
        // Hot Reloading
        // ====================================================================

        /**
         * @brief Enable or disable hot reloading
         * @param enable true to enable
         */
        void SetHotReloadEnabled(bool enable) { m_HotReloadEnabled = enable; }

        /**
         * @brief Check if hot reloading is enabled
         * @return true if enabled
         */
        bool IsHotReloadEnabled() const { return m_HotReloadEnabled; }

        /**
         * @brief Check for modified resources and reload them
         * @return Number of resources reloaded
         */
        uint32_t CheckForModifiedResources();

        /**
         * @brief Force reload a specific resource
         * @param handle Resource handle
         * @return true if reload succeeded
         */
        bool ReloadResource(ResourceHandle handle);

        // ====================================================================
        // Statistics
        // ====================================================================

        /**
         * @brief Get the total number of loaded resources
         * @return Resource count
         */
        size_t GetLoadedResourceCount() const;

        /**
         * @brief Get the total memory used by loaded resources
         * @return Memory usage in bytes
         */
        size_t GetTotalMemoryUsage() const;

        /**
         * @brief Get the number of pending async loads
         * @return Pending load count
         */
        size_t GetPendingLoadCount() const;

        /**
         * @brief Get resource information by handle
         * @param handle Resource handle
         * @param outPath Output path string
         * @param outType Output type
         * @param outRefCount Output reference count
         * @return true if handle is valid
         */
        bool GetResourceInfo(
            ResourceHandle handle,
            std::string& outPath,
            ResourceType& outType,
            uint32_t& outRefCount
        ) const;

        /**
         * @brief Print all loaded resources (debug)
         */
        void DebugPrintResources() const;

    private:
        ResourceManager() = default;
        ~ResourceManager();

        /**
         * @brief Generate a unique resource ID
         * @return Unique ID
         */
        uint32_t GenerateResourceID();

        /**
         * @brief Internal load implementation
         * @param path File path
         * @param type Resource type
         * @return Loaded data pointer
         */
        void* LoadResourceData(const std::string& path, ResourceType type);

        /**
         * @brief Internal unload implementation
         * @param entry Resource entry to unload
         */
        void UnloadResourceData(ResourceEntry& entry);

        /**
         * @brief Process a single async load
         * @param request Load request
         */
        void ProcessAsyncLoad(ResourceLoadRequest request);

        /**
         * @brief Get resource type for C++ type (specialization needed)
         */
        template<typename T>
        static ResourceType GetResourceTypeForCppType()
        {
            // Default to RawData, specialize for specific types
            return ResourceType::RawData;
        }

    private:
        bool m_IsInitialized = false;
        bool m_HotReloadEnabled = false;
        uint32_t m_MaxAsyncLoads = 4;

        // Resource storage
        std::unordered_map<uint32_t, std::unique_ptr<ResourceEntry>> m_Resources;
        std::unordered_map<std::string, uint32_t> m_PathToID;  // Path -> Resource ID

        // ID generation
        std::atomic<uint32_t> m_NextResourceID{1};

        // Async loading
        std::vector<ResourceLoadRequest> m_PendingLoads;
        std::atomic<uint32_t> m_ActiveAsyncLoads{0};

        // Thread safety
        mutable std::mutex m_Mutex;
        mutable std::mutex m_AsyncMutex;
    };

    // ============================================================================
    // Scoped Resource Reference
    // ============================================================================

    /**
     * @brief RAII helper for automatic resource reference management
     *
     * Automatically adds a reference on construction and releases on destruction.
     *
     * Example usage:
     * @code
     *   {
     *       ScopedResource<Texture> tex = resourceManager.Load<Texture>("player.png");
     *       // Use tex...
     *   } // Automatically released here
     * @endcode
     */
    template<typename T>
    class ScopedResource
    {
    public:
        ScopedResource() = default;

        explicit ScopedResource(ResourceHandle handle)
            : m_Handle(handle)
        {
            if (m_Handle.IsValid())
            {
                ResourceManager::Get().AddRef(m_Handle);
            }
        }

        ~ScopedResource()
        {
            Release();
        }

        // Non-copyable
        ScopedResource(const ScopedResource&) = delete;
        ScopedResource& operator=(const ScopedResource&) = delete;

        // Movable
        ScopedResource(ScopedResource&& other) noexcept
            : m_Handle(other.m_Handle)
        {
            other.m_Handle.Invalidate();
        }

        ScopedResource& operator=(ScopedResource&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                m_Handle = other.m_Handle;
                other.m_Handle.Invalidate();
            }
            return *this;
        }

        /**
         * @brief Get the resource data
         * @return Pointer to resource data
         */
        T* Get() const
        {
            return ResourceManager::Get().Get<T>(m_Handle);
        }

        /**
         * @brief Get the resource handle
         * @return Resource handle
         */
        ResourceHandle GetHandle() const { return m_Handle; }

        /**
         * @brief Check if the resource is valid
         * @return true if valid
         */
        bool IsValid() const { return m_Handle.IsValid(); }

        /**
         * @brief Release the resource reference
         */
        void Release()
        {
            if (m_Handle.IsValid())
            {
                ResourceManager::Get().Release(m_Handle);
                m_Handle.Invalidate();
            }
        }

        // Convenience operators
        T* operator->() const { return Get(); }
        T& operator*() const { return *Get(); }
        explicit operator bool() const { return IsValid(); }

    private:
        ResourceHandle m_Handle;
    };

} // namespace SM
