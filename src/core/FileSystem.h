#pragma once

/**
 * @file FileSystem.h
 * @brief Shattered Moon Engine - File System Utilities
 *
 * Provides cross-platform file system operations for loading
 * and managing game assets and configuration files.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace SM
{
    /**
     * @brief File system utility functions
     *
     * Static utility class providing common file operations
     * used throughout the engine for asset loading and configuration.
     */
    class FileSystem
    {
    public:
        // ====================================================================
        // File Reading
        // ====================================================================

        /**
         * @brief Read entire file as binary data
         * @param path Path to the file (absolute or relative)
         * @return Vector of bytes, empty if file could not be read
         */
        static std::vector<uint8_t> ReadFile(const std::string& path);

        /**
         * @brief Read entire file as binary data (wide string path)
         * @param path Path to the file (absolute or relative)
         * @return Vector of bytes, empty if file could not be read
         */
        static std::vector<uint8_t> ReadFile(const std::wstring& path);

        /**
         * @brief Read entire file as text string
         * @param path Path to the file
         * @return File contents as string, empty if file could not be read
         */
        static std::string ReadTextFile(const std::string& path);

        /**
         * @brief Read entire file as text string (wide string path)
         * @param path Path to the file
         * @return File contents as string, empty if file could not be read
         */
        static std::string ReadTextFile(const std::wstring& path);

        /**
         * @brief Read file with error reporting
         * @param path Path to the file
         * @param outData Output vector for file data
         * @param outError Output string for error message
         * @return true if successful, false otherwise
         */
        static bool ReadFileWithError(
            const std::string& path,
            std::vector<uint8_t>& outData,
            std::string& outError
        );

        // ====================================================================
        // File Writing
        // ====================================================================

        /**
         * @brief Write binary data to file
         * @param path Path to the file
         * @param data Data to write
         * @return true if successful, false otherwise
         */
        static bool WriteFile(const std::string& path, const std::vector<uint8_t>& data);

        /**
         * @brief Write binary data to file (wide string path)
         * @param path Path to the file
         * @param data Data to write
         * @return true if successful, false otherwise
         */
        static bool WriteFile(const std::wstring& path, const std::vector<uint8_t>& data);

        /**
         * @brief Write text to file
         * @param path Path to the file
         * @param text Text to write
         * @return true if successful, false otherwise
         */
        static bool WriteTextFile(const std::string& path, const std::string& text);

        /**
         * @brief Write text to file (wide string path)
         * @param path Path to the file
         * @param text Text to write
         * @return true if successful, false otherwise
         */
        static bool WriteTextFile(const std::wstring& path, const std::string& text);

        // ====================================================================
        // File Information
        // ====================================================================

        /**
         * @brief Check if a file exists
         * @param path Path to check
         * @return true if file exists, false otherwise
         */
        static bool FileExists(const std::string& path);

        /**
         * @brief Check if a file exists (wide string path)
         * @param path Path to check
         * @return true if file exists, false otherwise
         */
        static bool FileExists(const std::wstring& path);

        /**
         * @brief Check if a path is a directory
         * @param path Path to check
         * @return true if path is a directory, false otherwise
         */
        static bool IsDirectory(const std::string& path);

        /**
         * @brief Check if a path is a directory (wide string path)
         * @param path Path to check
         * @return true if path is a directory, false otherwise
         */
        static bool IsDirectory(const std::wstring& path);

        /**
         * @brief Get the size of a file in bytes
         * @param path Path to the file
         * @return File size in bytes, 0 if file doesn't exist
         */
        static size_t GetFileSize(const std::string& path);

        /**
         * @brief Get the size of a file in bytes (wide string path)
         * @param path Path to the file
         * @return File size in bytes, 0 if file doesn't exist
         */
        static size_t GetFileSize(const std::wstring& path);

        /**
         * @brief Get the last modification time of a file
         * @param path Path to the file
         * @return Modification time as time_t, 0 if file doesn't exist
         */
        static std::time_t GetLastWriteTime(const std::string& path);

        // ====================================================================
        // Path Operations
        // ====================================================================

        /**
         * @brief Get the file extension (including dot)
         * @param path File path
         * @return Extension (e.g., ".png"), empty if no extension
         */
        static std::string GetExtension(const std::string& path);

        /**
         * @brief Get the file extension (including dot, wide string)
         * @param path File path
         * @return Extension as narrow string
         */
        static std::string GetExtension(const std::wstring& path);

        /**
         * @brief Get the file extension without dot
         * @param path File path
         * @return Extension without dot (e.g., "png")
         */
        static std::string GetExtensionNoDot(const std::string& path);

        /**
         * @brief Get the filename from a path (including extension)
         * @param path Full path
         * @return Filename component
         */
        static std::string GetFilename(const std::string& path);

        /**
         * @brief Get the filename without extension
         * @param path Full path
         * @return Filename without extension
         */
        static std::string GetFilenameWithoutExtension(const std::string& path);

        /**
         * @brief Get the directory part of a path
         * @param path Full path
         * @return Directory path
         */
        static std::string GetDirectory(const std::string& path);

        /**
         * @brief Combine two path components
         * @param base Base path
         * @param relative Relative path to append
         * @return Combined path
         */
        static std::string CombinePath(const std::string& base, const std::string& relative);

        /**
         * @brief Get the absolute path from a relative path
         * @param path Relative or absolute path
         * @return Absolute path
         */
        static std::string GetAbsolutePath(const std::string& path);

        /**
         * @brief Normalize path separators to forward slashes
         * @param path Path to normalize
         * @return Normalized path
         */
        static std::string NormalizePath(const std::string& path);

        /**
         * @brief Get relative path from one path to another
         * @param from Base path
         * @param to Target path
         * @return Relative path from 'from' to 'to'
         */
        static std::string GetRelativePath(const std::string& from, const std::string& to);

        // ====================================================================
        // Directory Operations
        // ====================================================================

        /**
         * @brief Create a directory (and parent directories if needed)
         * @param path Directory path to create
         * @return true if directory was created or already exists
         */
        static bool CreateDirectory(const std::string& path);

        /**
         * @brief Create a directory (and parent directories if needed, wide string)
         * @param path Directory path to create
         * @return true if directory was created or already exists
         */
        static bool CreateDirectory(const std::wstring& path);

        /**
         * @brief Delete a file
         * @param path Path to the file
         * @return true if file was deleted
         */
        static bool DeleteFile(const std::string& path);

        /**
         * @brief Delete a directory (must be empty)
         * @param path Path to the directory
         * @return true if directory was deleted
         */
        static bool DeleteDirectory(const std::string& path);

        /**
         * @brief Delete a directory and all its contents recursively
         * @param path Path to the directory
         * @return true if directory was deleted
         */
        static bool DeleteDirectoryRecursive(const std::string& path);

        /**
         * @brief Copy a file
         * @param source Source file path
         * @param destination Destination file path
         * @param overwrite Whether to overwrite existing file
         * @return true if file was copied
         */
        static bool CopyFile(
            const std::string& source,
            const std::string& destination,
            bool overwrite = false
        );

        /**
         * @brief Move/rename a file
         * @param source Source file path
         * @param destination Destination file path
         * @return true if file was moved
         */
        static bool MoveFile(const std::string& source, const std::string& destination);

        // ====================================================================
        // Directory Listing
        // ====================================================================

        /**
         * @brief Get all files in a directory
         * @param directory Directory to search
         * @param extension Optional extension filter (e.g., ".png")
         * @param recursive Whether to search subdirectories
         * @return Vector of file paths
         */
        static std::vector<std::string> GetFilesInDirectory(
            const std::string& directory,
            const std::string& extension = "",
            bool recursive = false
        );

        /**
         * @brief Get all subdirectories in a directory
         * @param directory Directory to search
         * @param recursive Whether to search subdirectories
         * @return Vector of directory paths
         */
        static std::vector<std::string> GetDirectoriesInDirectory(
            const std::string& directory,
            bool recursive = false
        );

        // ====================================================================
        // Application Paths
        // ====================================================================

        /**
         * @brief Get the executable's directory
         * @return Directory containing the executable
         */
        static std::string GetExecutableDirectory();

        /**
         * @brief Get the current working directory
         * @return Current working directory
         */
        static std::string GetWorkingDirectory();

        /**
         * @brief Set the current working directory
         * @param path New working directory
         * @return true if successful
         */
        static bool SetWorkingDirectory(const std::string& path);

        /**
         * @brief Get the assets directory
         * @return Path to assets directory (relative to executable)
         */
        static std::string GetAssetsDirectory();

        /**
         * @brief Get the shaders directory
         * @return Path to shaders directory (relative to executable)
         */
        static std::string GetShadersDirectory();

        // ====================================================================
        // String Conversion
        // ====================================================================

        /**
         * @brief Convert wide string to narrow string (UTF-8)
         * @param wstr Wide string
         * @return Narrow string
         */
        static std::string WStringToString(const std::wstring& wstr);

        /**
         * @brief Convert narrow string (UTF-8) to wide string
         * @param str Narrow string
         * @return Wide string
         */
        static std::wstring StringToWString(const std::string& str);

    private:
        FileSystem() = delete; // Static class
    };

} // namespace SM
