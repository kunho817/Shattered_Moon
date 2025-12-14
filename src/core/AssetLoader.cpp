#include "core/AssetLoader.h"
#include "core/FileSystem.h"

namespace SM
{
    // ============================================================================
    // Asset Metadata Implementation
    // ============================================================================

    bool AssetMetadata::HasBeenModified() const
    {
        if (Path.empty() || !IsLoaded)
        {
            return false;
        }

        std::time_t currentModTime = FileSystem::GetLastWriteTime(Path);
        return currentModTime > static_cast<std::time_t>(LastModified);
    }

    // ============================================================================
    // Raw Asset Loader Implementation
    // ============================================================================

    bool RawAssetLoader::Load(const std::string& path, void* outData)
    {
        if (!outData)
        {
            return false;
        }

        RawAssetData* rawData = static_cast<RawAssetData*>(outData);
        rawData->Clear();

        rawData->Data = FileSystem::ReadFile(path);
        if (rawData->Data.empty())
        {
            // Check if file exists but is empty vs. file not found
            if (!FileSystem::FileExists(path))
            {
                return false;
            }
            // Empty file is technically valid
        }

        rawData->SourcePath = path;
        rawData->OriginalSize = rawData->Data.size();
        return true;
    }

    void RawAssetLoader::Unload(void* data)
    {
        if (data)
        {
            RawAssetData* rawData = static_cast<RawAssetData*>(data);
            rawData->Clear();
        }
    }

    // ============================================================================
    // Asset Loader Registry Implementation
    // ============================================================================

    AssetLoaderRegistry& AssetLoaderRegistry::Get()
    {
        static AssetLoaderRegistry instance;
        return instance;
    }

    void AssetLoaderRegistry::RegisterLoader(std::unique_ptr<IAssetLoader> loader)
    {
        if (loader)
        {
            m_Loaders.push_back(std::move(loader));
        }
    }

    IAssetLoader* AssetLoaderRegistry::FindLoader(const std::string& path) const
    {
        for (const auto& loader : m_Loaders)
        {
            if (loader->CanLoad(path))
            {
                return loader.get();
            }
        }
        return nullptr;
    }

    void AssetLoaderRegistry::Clear()
    {
        m_Loaders.clear();
    }

} // namespace SM
