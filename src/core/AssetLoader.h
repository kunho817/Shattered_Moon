#pragma once

/**
 * @file AssetLoader.h
 * @brief Shattered Moon Engine - Asset Loader Interface
 *
 * Defines the interface for loading different types of game assets.
 * Each asset type (textures, meshes, shaders, etc.) has a specific
 * loader implementation.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace SM
{
    // Forward declarations
    struct ResourceHandle;

    // ============================================================================
    // Asset Load Result
    // ============================================================================

    /**
     * @brief Result status for asset loading operations
     */
    enum class AssetLoadResult
    {
        Success,           // Asset loaded successfully
        FileNotFound,      // File does not exist
        InvalidFormat,     // File format is invalid or unsupported
        CorruptedData,     // File data is corrupted
        OutOfMemory,       // Not enough memory to load asset
        IOError,           // General I/O error
        NotSupported,      // Asset type not supported
        Unknown            // Unknown error
    };

    /**
     * @brief Convert load result to string for logging
     * @param result The result to convert
     * @return Human-readable string
     */
    inline const char* AssetLoadResultToString(AssetLoadResult result)
    {
        switch (result)
        {
            case AssetLoadResult::Success:       return "Success";
            case AssetLoadResult::FileNotFound:  return "File Not Found";
            case AssetLoadResult::InvalidFormat: return "Invalid Format";
            case AssetLoadResult::CorruptedData: return "Corrupted Data";
            case AssetLoadResult::OutOfMemory:   return "Out of Memory";
            case AssetLoadResult::IOError:       return "I/O Error";
            case AssetLoadResult::NotSupported:  return "Not Supported";
            default:                             return "Unknown Error";
        }
    }

    // ============================================================================
    // Asset Loader Interface
    // ============================================================================

    /**
     * @brief Base interface for all asset loaders
     *
     * Asset loaders are responsible for loading specific types of assets
     * (textures, meshes, audio, etc.) from files into memory.
     *
     * Implementations should:
     * - Validate file format and integrity
     * - Handle loading from various file formats
     * - Provide async loading capabilities where appropriate
     * - Release resources properly when unloading
     *
     * Example usage:
     * @code
     *   class TextureLoader : public IAssetLoader {
     *       bool Load(const std::string& path, void* outData) override { ... }
     *       void Unload(void* data) override { ... }
     *   };
     * @endcode
     */
    class IAssetLoader
    {
    public:
        virtual ~IAssetLoader() = default;

        /**
         * @brief Load an asset from a file
         * @param path Path to the asset file
         * @param outData Pointer to receive loaded data (type depends on loader)
         * @return true if loading succeeded, false otherwise
         */
        virtual bool Load(const std::string& path, void* outData) = 0;

        /**
         * @brief Load an asset with detailed result
         * @param path Path to the asset file
         * @param outData Pointer to receive loaded data
         * @param outError Optional error message
         * @return Result status
         */
        virtual AssetLoadResult LoadWithResult(
            const std::string& path,
            void* outData,
            std::string* outError = nullptr)
        {
            // Default implementation uses simple Load
            if (Load(path, outData))
            {
                return AssetLoadResult::Success;
            }
            if (outError)
            {
                *outError = "Failed to load asset: " + path;
            }
            return AssetLoadResult::Unknown;
        }

        /**
         * @brief Unload/release asset data
         * @param data Pointer to asset data to release
         */
        virtual void Unload(void* data) = 0;

        /**
         * @brief Get the list of supported file extensions
         * @return Vector of extensions (e.g., ".png", ".jpg")
         */
        virtual std::vector<std::string> GetSupportedExtensions() const = 0;

        /**
         * @brief Check if this loader can handle a file
         * @param path Path to the file
         * @return true if the file can be loaded by this loader
         */
        virtual bool CanLoad(const std::string& path) const
        {
            auto extensions = GetSupportedExtensions();
            for (const auto& ext : extensions)
            {
                if (path.length() >= ext.length())
                {
                    if (path.compare(path.length() - ext.length(), ext.length(), ext) == 0)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        /**
         * @brief Check if async loading is supported
         * @return true if async loading is available
         */
        virtual bool SupportsAsyncLoading() const { return false; }

        /**
         * @brief Get estimated memory usage for an asset
         * @param path Path to the asset file
         * @return Estimated size in bytes, 0 if unknown
         */
        virtual size_t GetEstimatedMemoryUsage(const std::string& path) const
        {
            (void)path;
            return 0;
        }
    };

    // ============================================================================
    // Raw Data Container
    // ============================================================================

    /**
     * @brief Simple container for raw loaded data
     *
     * Used for assets that are just raw byte data without specific structure.
     */
    struct RawAssetData
    {
        std::vector<uint8_t> Data;    // Raw byte data
        std::string SourcePath;        // Original file path
        size_t OriginalSize = 0;       // Size when loaded

        bool IsValid() const { return !Data.empty(); }
        void Clear()
        {
            Data.clear();
            Data.shrink_to_fit();
            SourcePath.clear();
            OriginalSize = 0;
        }
    };

    // ============================================================================
    // Generic Raw Asset Loader
    // ============================================================================

    /**
     * @brief Simple loader that reads files as raw binary data
     *
     * Useful for loading custom file formats or when raw byte access is needed.
     */
    class RawAssetLoader : public IAssetLoader
    {
    public:
        bool Load(const std::string& path, void* outData) override;
        void Unload(void* data) override;

        std::vector<std::string> GetSupportedExtensions() const override
        {
            return {}; // Can load any file
        }

        bool CanLoad(const std::string& path) const override
        {
            (void)path;
            return true; // Can load any file as raw data
        }
    };

    // ============================================================================
    // Asset Metadata
    // ============================================================================

    /**
     * @brief Metadata for loaded assets
     *
     * Contains information about an asset that can be used for
     * caching, dependency tracking, and hot reloading.
     */
    struct AssetMetadata
    {
        std::string Path;              // File path
        std::string Name;              // Asset name (usually filename)
        uint64_t LastModified = 0;     // Last modification timestamp
        size_t FileSize = 0;           // File size in bytes
        size_t MemorySize = 0;         // Memory size when loaded
        uint32_t LoadCount = 0;        // Number of times loaded
        bool IsLoaded = false;         // Currently loaded in memory

        /**
         * @brief Check if the source file has been modified
         * @return true if file has been modified since loading
         */
        bool HasBeenModified() const;
    };

    // ============================================================================
    // Async Load Request
    // ============================================================================

    /**
     * @brief Status of an async load operation
     */
    enum class AsyncLoadStatus
    {
        Pending,    // Waiting to start
        Loading,    // Currently loading
        Completed,  // Successfully completed
        Failed,     // Failed to load
        Cancelled   // Loading was cancelled
    };

    /**
     * @brief Request for async asset loading
     */
    struct AsyncLoadRequest
    {
        std::string Path;                              // Asset path
        void* UserData = nullptr;                      // User-defined data
        AsyncLoadStatus Status = AsyncLoadStatus::Pending;
        AssetLoadResult Result = AssetLoadResult::Unknown;
        std::string ErrorMessage;                      // Error if failed
        float Progress = 0.0f;                         // 0.0 to 1.0

        bool IsComplete() const
        {
            return Status == AsyncLoadStatus::Completed ||
                   Status == AsyncLoadStatus::Failed ||
                   Status == AsyncLoadStatus::Cancelled;
        }
    };

    // ============================================================================
    // Asset Loader Registry
    // ============================================================================

    /**
     * @brief Registry for asset loaders
     *
     * Manages the mapping between file extensions and their loaders.
     * The ResourceManager uses this to find the appropriate loader for each asset.
     */
    class AssetLoaderRegistry
    {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the registry
         */
        static AssetLoaderRegistry& Get();

        // Non-copyable
        AssetLoaderRegistry(const AssetLoaderRegistry&) = delete;
        AssetLoaderRegistry& operator=(const AssetLoaderRegistry&) = delete;

        /**
         * @brief Register an asset loader for specific extensions
         * @param loader Loader instance (registry takes ownership)
         */
        void RegisterLoader(std::unique_ptr<IAssetLoader> loader);

        /**
         * @brief Find a loader for a file path
         * @param path File path
         * @return Pointer to loader, nullptr if none found
         */
        IAssetLoader* FindLoader(const std::string& path) const;

        /**
         * @brief Get the raw asset loader (always available)
         * @return Pointer to raw asset loader
         */
        RawAssetLoader* GetRawLoader() { return &m_RawLoader; }

        /**
         * @brief Clear all registered loaders
         */
        void Clear();

    private:
        AssetLoaderRegistry() = default;

        std::vector<std::unique_ptr<IAssetLoader>> m_Loaders;
        RawAssetLoader m_RawLoader;
    };

} // namespace SM
