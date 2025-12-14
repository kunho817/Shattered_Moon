#include "core/ResourceManager.h"
#include "core/FileSystem.h"
#include "core/AssetLoader.h"

#include <iostream>
#include <algorithm>

namespace SM
{
    // ============================================================================
    // Resource Type Helpers
    // ============================================================================

    ResourceType GetResourceTypeFromExtension(const std::string& extension)
    {
        // Convert to lowercase for comparison
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Texture formats
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
            ext == ".bmp" || ext == ".tga" || ext == ".dds" ||
            ext == ".hdr" || ext == ".ktx")
        {
            return ResourceType::Texture;
        }

        // Mesh formats
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" ||
            ext == ".glb" || ext == ".dae" || ext == ".3ds")
        {
            return ResourceType::Mesh;
        }

        // Shader formats
        if (ext == ".hlsl" || ext == ".glsl" || ext == ".vert" ||
            ext == ".frag" || ext == ".comp" || ext == ".cso" ||
            ext == ".spv")
        {
            return ResourceType::Shader;
        }

        // Material formats
        if (ext == ".mat" || ext == ".material")
        {
            return ResourceType::Material;
        }

        // Audio formats
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" ||
            ext == ".flac" || ext == ".aiff")
        {
            return ResourceType::Audio;
        }

        // Script formats
        if (ext == ".lua" || ext == ".js" || ext == ".py")
        {
            return ResourceType::Script;
        }

        // Font formats
        if (ext == ".ttf" || ext == ".otf" || ext == ".fnt")
        {
            return ResourceType::Font;
        }

        // Animation formats
        if (ext == ".anim" || ext == ".animation")
        {
            return ResourceType::Animation;
        }

        // Config formats
        if (ext == ".json" || ext == ".xml" || ext == ".yaml" ||
            ext == ".ini" || ext == ".cfg" || ext == ".toml")
        {
            return ResourceType::Config;
        }

        // Scene/Prefab formats
        if (ext == ".scene")
        {
            return ResourceType::Scene;
        }
        if (ext == ".prefab")
        {
            return ResourceType::Prefab;
        }

        return ResourceType::RawData;
    }

    // ============================================================================
    // Resource Manager Implementation
    // ============================================================================

    ResourceManager& ResourceManager::Get()
    {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager::~ResourceManager()
    {
        Shutdown();
    }

    bool ResourceManager::Initialize(uint32_t maxAsyncLoads)
    {
        if (m_IsInitialized)
        {
            return true;
        }

        m_MaxAsyncLoads = maxAsyncLoads;
        m_IsInitialized = true;

#if defined(_DEBUG)
        std::cout << "[ResourceManager] Initialized with max async loads: "
                  << maxAsyncLoads << std::endl;
#endif

        return true;
    }

    void ResourceManager::Shutdown()
    {
        if (!m_IsInitialized)
        {
            return;
        }

#if defined(_DEBUG)
        std::cout << "[ResourceManager] Shutting down..." << std::endl;
        DebugPrintResources();
#endif

        // Wait for all async loads to complete
        {
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            m_PendingLoads.clear();
        }

        // Unload all resources
        UnloadAll();

        m_IsInitialized = false;
    }

    void ResourceManager::Update()
    {
        if (!m_IsInitialized)
        {
            return;
        }

        // Process completed async loads
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto& [id, entry] : m_Resources)
        {
            if (entry->IsLoading && entry->LoadFuture.valid())
            {
                // Check if loading is complete (non-blocking)
                auto status = entry->LoadFuture.wait_for(std::chrono::milliseconds(0));
                if (status == std::future_status::ready)
                {
                    bool success = entry->LoadFuture.get();
                    entry->IsLoading = false;
                    entry->IsLoaded = success;
                    m_ActiveAsyncLoads--;

#if defined(_DEBUG)
                    if (success)
                    {
                        std::cout << "[ResourceManager] Async load complete: "
                                  << entry->Path << std::endl;
                    }
                    else
                    {
                        std::cout << "[ResourceManager] Async load failed: "
                                  << entry->Path << std::endl;
                    }
#endif
                }
            }
        }

        // Start pending loads if we have capacity
        if (m_ActiveAsyncLoads < m_MaxAsyncLoads)
        {
            std::lock_guard<std::mutex> asyncLock(m_AsyncMutex);

            while (!m_PendingLoads.empty() && m_ActiveAsyncLoads < m_MaxAsyncLoads)
            {
                ResourceLoadRequest request = std::move(m_PendingLoads.back());
                m_PendingLoads.pop_back();

                m_ActiveAsyncLoads++;
                ProcessAsyncLoad(std::move(request));
            }
        }

        // Hot reload check (if enabled)
        if (m_HotReloadEnabled)
        {
            static float hotReloadTimer = 0.0f;
            hotReloadTimer += 0.016f; // Assume ~60fps

            if (hotReloadTimer >= 1.0f) // Check once per second
            {
                hotReloadTimer = 0.0f;
                CheckForModifiedResources();
            }
        }
    }

    ResourceHandle ResourceManager::LoadResource(const std::string& path, ResourceType type)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // Check if already loaded
        auto it = m_PathToID.find(path);
        if (it != m_PathToID.end())
        {
            auto& entry = m_Resources[it->second];
            entry->RefCount++;

#if defined(_DEBUG)
            std::cout << "[ResourceManager] Resource already loaded (refcount="
                      << entry->RefCount.load() << "): " << path << std::endl;
#endif

            return entry->Handle;
        }

        // Determine type from extension if not specified
        if (type == ResourceType::Unknown)
        {
            type = GetResourceTypeFromExtension(FileSystem::GetExtension(path));
        }

        // Load the resource
        void* data = LoadResourceData(path, type);
        if (!data)
        {
#if defined(_DEBUG)
            std::cout << "[ResourceManager] Failed to load: " << path << std::endl;
#endif
            return ResourceHandle{};
        }

        // Create entry
        auto entry = std::make_unique<ResourceEntry>();
        entry->Handle.ID = GenerateResourceID();
        entry->Handle.Type = type;
        entry->Path = path;
        entry->Name = FileSystem::GetFilenameWithoutExtension(path);
        entry->Data = data;
        entry->RefCount = 1;
        entry->LastModified = static_cast<uint64_t>(FileSystem::GetLastWriteTime(path));
        entry->IsLoaded = true;

        ResourceHandle handle = entry->Handle;

        m_PathToID[path] = handle.ID;
        m_Resources[handle.ID] = std::move(entry);

#if defined(_DEBUG)
        std::cout << "[ResourceManager] Loaded: " << path
                  << " (ID=" << handle.ID << ", Type="
                  << ResourceTypeToString(type) << ")" << std::endl;
#endif

        return handle;
    }

    void ResourceManager::LoadResourceAsync(
        const std::string& path,
        ResourceType type,
        std::function<void(ResourceHandle)> callback)
    {
        // Check if already loaded
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_PathToID.find(path);
            if (it != m_PathToID.end())
            {
                auto& entry = m_Resources[it->second];
                entry->RefCount++;

                if (callback)
                {
                    callback(entry->Handle);
                }
                return;
            }
        }

        // Add to pending loads
        ResourceLoadRequest request;
        request.Path = path;
        request.Type = type;
        request.Callback = callback;

        {
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            m_PendingLoads.push_back(std::move(request));
        }
    }

    void* ResourceManager::GetResourceData(ResourceHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it == m_Resources.end())
        {
            return nullptr;
        }

        if (!it->second->IsLoaded)
        {
            return nullptr;
        }

        return it->second->Data;
    }

    ResourceHandle ResourceManager::GetHandle(const std::string& path) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_PathToID.find(path);
        if (it != m_PathToID.end())
        {
            auto resIt = m_Resources.find(it->second);
            if (resIt != m_Resources.end())
            {
                return resIt->second->Handle;
            }
        }

        return ResourceHandle{};
    }

    bool ResourceManager::IsLoaded(ResourceHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it == m_Resources.end())
        {
            return false;
        }

        return it->second->IsLoaded;
    }

    bool ResourceManager::IsLoading(ResourceHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it == m_Resources.end())
        {
            return false;
        }

        return it->second->IsLoading;
    }

    void ResourceManager::AddRef(ResourceHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it != m_Resources.end())
        {
            it->second->RefCount++;
        }
    }

    void ResourceManager::Release(ResourceHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it == m_Resources.end())
        {
            return;
        }

        if (it->second->RefCount > 0)
        {
            it->second->RefCount--;
        }

        // Unload if no more references
        if (it->second->RefCount == 0)
        {
#if defined(_DEBUG)
            std::cout << "[ResourceManager] Releasing: " << it->second->Path << std::endl;
#endif
            UnloadResourceData(*it->second);
            m_PathToID.erase(it->second->Path);
            m_Resources.erase(it);
        }
    }

    void ResourceManager::Unload(ResourceHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it == m_Resources.end())
        {
            return;
        }

#if defined(_DEBUG)
        std::cout << "[ResourceManager] Force unloading: " << it->second->Path << std::endl;
#endif

        UnloadResourceData(*it->second);
        m_PathToID.erase(it->second->Path);
        m_Resources.erase(it);
    }

    void ResourceManager::UnloadAllOfType(ResourceType type)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        std::vector<uint32_t> toRemove;

        for (auto& [id, entry] : m_Resources)
        {
            if (entry->Handle.Type == type)
            {
                UnloadResourceData(*entry);
                m_PathToID.erase(entry->Path);
                toRemove.push_back(id);
            }
        }

        for (uint32_t id : toRemove)
        {
            m_Resources.erase(id);
        }

#if defined(_DEBUG)
        std::cout << "[ResourceManager] Unloaded " << toRemove.size()
                  << " resources of type " << ResourceTypeToString(type) << std::endl;
#endif
    }

    void ResourceManager::UnloadAll()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto& [id, entry] : m_Resources)
        {
            UnloadResourceData(*entry);
        }

        m_Resources.clear();
        m_PathToID.clear();

#if defined(_DEBUG)
        std::cout << "[ResourceManager] All resources unloaded" << std::endl;
#endif
    }

    uint32_t ResourceManager::CheckForModifiedResources()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        uint32_t reloadCount = 0;

        for (auto& [id, entry] : m_Resources)
        {
            if (!entry->IsLoaded)
            {
                continue;
            }

            uint64_t currentModTime =
                static_cast<uint64_t>(FileSystem::GetLastWriteTime(entry->Path));

            if (currentModTime > entry->LastModified)
            {
#if defined(_DEBUG)
                std::cout << "[ResourceManager] Hot reloading: " << entry->Path << std::endl;
#endif

                // Reload the resource
                UnloadResourceData(*entry);

                void* newData = LoadResourceData(entry->Path, entry->Handle.Type);
                if (newData)
                {
                    entry->Data = newData;
                    entry->IsLoaded = true;
                    entry->LastModified = currentModTime;
                    reloadCount++;
                }
            }
        }

        return reloadCount;
    }

    bool ResourceManager::ReloadResource(ResourceHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it == m_Resources.end())
        {
            return false;
        }

        ResourceEntry& entry = *it->second;

        // Unload current data
        UnloadResourceData(entry);

        // Reload
        void* newData = LoadResourceData(entry.Path, entry.Handle.Type);
        if (newData)
        {
            entry.Data = newData;
            entry.IsLoaded = true;
            entry.LastModified =
                static_cast<uint64_t>(FileSystem::GetLastWriteTime(entry.Path));

#if defined(_DEBUG)
            std::cout << "[ResourceManager] Reloaded: " << entry.Path << std::endl;
#endif
            return true;
        }

        return false;
    }

    size_t ResourceManager::GetLoadedResourceCount() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        size_t count = 0;
        for (const auto& [id, entry] : m_Resources)
        {
            if (entry->IsLoaded)
            {
                count++;
            }
        }
        return count;
    }

    size_t ResourceManager::GetTotalMemoryUsage() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        size_t total = 0;
        for (const auto& [id, entry] : m_Resources)
        {
            if (entry->IsLoaded)
            {
                total += entry->DataSize;
            }
        }
        return total;
    }

    size_t ResourceManager::GetPendingLoadCount() const
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        return m_PendingLoads.size() + m_ActiveAsyncLoads.load();
    }

    bool ResourceManager::GetResourceInfo(
        ResourceHandle handle,
        std::string& outPath,
        ResourceType& outType,
        uint32_t& outRefCount) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        auto it = m_Resources.find(handle.ID);
        if (it == m_Resources.end())
        {
            return false;
        }

        outPath = it->second->Path;
        outType = it->second->Handle.Type;
        outRefCount = it->second->RefCount.load();
        return true;
    }

    void ResourceManager::DebugPrintResources() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        std::cout << "=== Loaded Resources ===" << std::endl;
        std::cout << "Total: " << m_Resources.size() << std::endl;

        for (const auto& [id, entry] : m_Resources)
        {
            std::cout << "  [" << id << "] " << entry->Path
                      << " (Type=" << ResourceTypeToString(entry->Handle.Type)
                      << ", RefCount=" << entry->RefCount.load()
                      << ", Loaded=" << (entry->IsLoaded ? "Yes" : "No")
                      << ")" << std::endl;
        }

        std::cout << "=========================" << std::endl;
    }

    uint32_t ResourceManager::GenerateResourceID()
    {
        return m_NextResourceID++;
    }

    void* ResourceManager::LoadResourceData(const std::string& path, ResourceType type)
    {
        // Find appropriate loader
        IAssetLoader* loader = AssetLoaderRegistry::Get().FindLoader(path);

        if (!loader)
        {
            // Use raw loader as fallback
            loader = AssetLoaderRegistry::Get().GetRawLoader();
        }

        // Allocate data container based on type
        // For now, we use RawAssetData for everything
        // Real implementation would have type-specific containers
        auto* data = new RawAssetData();

        if (!loader->Load(path, data))
        {
            delete data;
            return nullptr;
        }

        return data;
    }

    void ResourceManager::UnloadResourceData(ResourceEntry& entry)
    {
        if (!entry.Data)
        {
            return;
        }

        // For now, all data is RawAssetData
        // Real implementation would check type and use appropriate unloader
        IAssetLoader* loader = AssetLoaderRegistry::Get().FindLoader(entry.Path);
        if (!loader)
        {
            loader = AssetLoaderRegistry::Get().GetRawLoader();
        }

        loader->Unload(entry.Data);
        delete static_cast<RawAssetData*>(entry.Data);

        entry.Data = nullptr;
        entry.IsLoaded = false;
        entry.DataSize = 0;
    }

    void ResourceManager::ProcessAsyncLoad(ResourceLoadRequest request)
    {
        // Create entry for the resource
        ResourceHandle handle;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            // Double-check it wasn't loaded while we were waiting
            auto it = m_PathToID.find(request.Path);
            if (it != m_PathToID.end())
            {
                handle = m_Resources[it->second]->Handle;
                m_Resources[it->second]->RefCount++;
                m_ActiveAsyncLoads--;

                if (request.Callback)
                {
                    request.Callback(handle);
                }
                return;
            }

            // Determine type if not specified
            if (request.Type == ResourceType::Unknown)
            {
                request.Type = GetResourceTypeFromExtension(
                    FileSystem::GetExtension(request.Path)
                );
            }

            // Create entry
            auto entry = std::make_unique<ResourceEntry>();
            entry->Handle.ID = GenerateResourceID();
            entry->Handle.Type = request.Type;
            entry->Path = request.Path;
            entry->Name = FileSystem::GetFilenameWithoutExtension(request.Path);
            entry->RefCount = 1;
            entry->IsLoading = true;

            handle = entry->Handle;

            // Start async load
            entry->LoadFuture = std::async(std::launch::async,
                [this, path = request.Path, type = request.Type, id = handle.ID]() -> bool
                {
                    void* data = LoadResourceData(path, type);
                    if (data)
                    {
                        std::lock_guard<std::mutex> lock(m_Mutex);
                        auto it = m_Resources.find(id);
                        if (it != m_Resources.end())
                        {
                            it->second->Data = data;
                            it->second->LastModified = static_cast<uint64_t>(
                                FileSystem::GetLastWriteTime(path)
                            );
                        }
                        return true;
                    }
                    return false;
                }
            );

            m_PathToID[request.Path] = handle.ID;
            m_Resources[handle.ID] = std::move(entry);
        }

        // Store callback for later
        if (request.Callback)
        {
            // In a real implementation, we'd store this and call it when load completes
            // For simplicity, we'll call it immediately with the pending handle
            request.Callback(handle);
        }
    }

} // namespace SM
