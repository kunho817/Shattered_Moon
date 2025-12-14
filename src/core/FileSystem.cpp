#include "core/FileSystem.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

namespace SM
{
    // ============================================================================
    // File Reading
    // ============================================================================

    std::vector<uint8_t> FileSystem::ReadFile(const std::string& path)
    {
        std::vector<uint8_t> result;

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return result;
        }

        std::streamsize size = file.tellg();
        if (size <= 0)
        {
            return result;
        }

        file.seekg(0, std::ios::beg);
        result.resize(static_cast<size_t>(size));

        if (!file.read(reinterpret_cast<char*>(result.data()), size))
        {
            result.clear();
        }

        return result;
    }

    std::vector<uint8_t> FileSystem::ReadFile(const std::wstring& path)
    {
        std::vector<uint8_t> result;

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return result;
        }

        std::streamsize size = file.tellg();
        if (size <= 0)
        {
            return result;
        }

        file.seekg(0, std::ios::beg);
        result.resize(static_cast<size_t>(size));

        if (!file.read(reinterpret_cast<char*>(result.data()), size))
        {
            result.clear();
        }

        return result;
    }

    std::string FileSystem::ReadTextFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string FileSystem::ReadTextFile(const std::wstring& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool FileSystem::ReadFileWithError(
        const std::string& path,
        std::vector<uint8_t>& outData,
        std::string& outError)
    {
        outData.clear();
        outError.clear();

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            outError = "Failed to open file: " + path;
            return false;
        }

        std::streamsize size = file.tellg();
        if (size < 0)
        {
            outError = "Failed to get file size: " + path;
            return false;
        }

        file.seekg(0, std::ios::beg);
        outData.resize(static_cast<size_t>(size));

        if (size > 0 && !file.read(reinterpret_cast<char*>(outData.data()), size))
        {
            outError = "Failed to read file data: " + path;
            outData.clear();
            return false;
        }

        return true;
    }

    // ============================================================================
    // File Writing
    // ============================================================================

    bool FileSystem::WriteFile(const std::string& path, const std::vector<uint8_t>& data)
    {
        // Create parent directories if needed
        std::string dir = GetDirectory(path);
        if (!dir.empty() && !IsDirectory(dir))
        {
            CreateDirectory(dir);
        }

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        if (!data.empty())
        {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        return file.good();
    }

    bool FileSystem::WriteFile(const std::wstring& path, const std::vector<uint8_t>& data)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        if (!data.empty())
        {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        return file.good();
    }

    bool FileSystem::WriteTextFile(const std::string& path, const std::string& text)
    {
        // Create parent directories if needed
        std::string dir = GetDirectory(path);
        if (!dir.empty() && !IsDirectory(dir))
        {
            CreateDirectory(dir);
        }

        std::ofstream file(path);
        if (!file.is_open())
        {
            return false;
        }

        file << text;
        return file.good();
    }

    bool FileSystem::WriteTextFile(const std::wstring& path, const std::string& text)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            return false;
        }

        file << text;
        return file.good();
    }

    // ============================================================================
    // File Information
    // ============================================================================

    bool FileSystem::FileExists(const std::string& path)
    {
        return std::filesystem::exists(path);
    }

    bool FileSystem::FileExists(const std::wstring& path)
    {
        return std::filesystem::exists(path);
    }

    bool FileSystem::IsDirectory(const std::string& path)
    {
        return std::filesystem::is_directory(path);
    }

    bool FileSystem::IsDirectory(const std::wstring& path)
    {
        return std::filesystem::is_directory(path);
    }

    size_t FileSystem::GetFileSize(const std::string& path)
    {
        try
        {
            return static_cast<size_t>(std::filesystem::file_size(path));
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return 0;
        }
    }

    size_t FileSystem::GetFileSize(const std::wstring& path)
    {
        try
        {
            return static_cast<size_t>(std::filesystem::file_size(path));
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return 0;
        }
    }

    std::time_t FileSystem::GetLastWriteTime(const std::string& path)
    {
        try
        {
            auto fileTime = std::filesystem::last_write_time(path);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                fileTime - std::filesystem::file_time_type::clock::now() +
                std::chrono::system_clock::now()
            );
            return std::chrono::system_clock::to_time_t(sctp);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return 0;
        }
    }

    // ============================================================================
    // Path Operations
    // ============================================================================

    std::string FileSystem::GetExtension(const std::string& path)
    {
        std::filesystem::path p(path);
        return p.extension().string();
    }

    std::string FileSystem::GetExtension(const std::wstring& path)
    {
        std::filesystem::path p(path);
        return p.extension().string();
    }

    std::string FileSystem::GetExtensionNoDot(const std::string& path)
    {
        std::string ext = GetExtension(path);
        if (!ext.empty() && ext[0] == '.')
        {
            return ext.substr(1);
        }
        return ext;
    }

    std::string FileSystem::GetFilename(const std::string& path)
    {
        std::filesystem::path p(path);
        return p.filename().string();
    }

    std::string FileSystem::GetFilenameWithoutExtension(const std::string& path)
    {
        std::filesystem::path p(path);
        return p.stem().string();
    }

    std::string FileSystem::GetDirectory(const std::string& path)
    {
        std::filesystem::path p(path);
        return p.parent_path().string();
    }

    std::string FileSystem::CombinePath(const std::string& base, const std::string& relative)
    {
        std::filesystem::path p(base);
        p /= relative;
        return p.string();
    }

    std::string FileSystem::GetAbsolutePath(const std::string& path)
    {
        try
        {
            return std::filesystem::absolute(path).string();
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return path;
        }
    }

    std::string FileSystem::NormalizePath(const std::string& path)
    {
        std::string result = path;

        // Replace backslashes with forward slashes
        std::replace(result.begin(), result.end(), '\\', '/');

        // Remove trailing slash
        while (!result.empty() && result.back() == '/')
        {
            result.pop_back();
        }

        return result;
    }

    std::string FileSystem::GetRelativePath(const std::string& from, const std::string& to)
    {
        try
        {
            return std::filesystem::relative(to, from).string();
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return to;
        }
    }

    // ============================================================================
    // Directory Operations
    // ============================================================================

    bool FileSystem::CreateDirectory(const std::string& path)
    {
        try
        {
            return std::filesystem::create_directories(path);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    bool FileSystem::CreateDirectory(const std::wstring& path)
    {
        try
        {
            return std::filesystem::create_directories(path);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    bool FileSystem::DeleteFile(const std::string& path)
    {
        try
        {
            return std::filesystem::remove(path);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    bool FileSystem::DeleteDirectory(const std::string& path)
    {
        try
        {
            return std::filesystem::remove(path);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    bool FileSystem::DeleteDirectoryRecursive(const std::string& path)
    {
        try
        {
            std::filesystem::remove_all(path);
            return true;
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    bool FileSystem::CopyFile(
        const std::string& source,
        const std::string& destination,
        bool overwrite)
    {
        try
        {
            auto options = overwrite
                ? std::filesystem::copy_options::overwrite_existing
                : std::filesystem::copy_options::none;
            std::filesystem::copy(source, destination, options);
            return true;
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    bool FileSystem::MoveFile(const std::string& source, const std::string& destination)
    {
        try
        {
            std::filesystem::rename(source, destination);
            return true;
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    // ============================================================================
    // Directory Listing
    // ============================================================================

    std::vector<std::string> FileSystem::GetFilesInDirectory(
        const std::string& directory,
        const std::string& extension,
        bool recursive)
    {
        std::vector<std::string> files;

        try
        {
            auto iterator = recursive
                ? std::filesystem::recursive_directory_iterator(directory)
                : std::filesystem::recursive_directory_iterator(directory,
                    std::filesystem::directory_options::skip_permission_denied);

            if (!recursive)
            {
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    if (entry.is_regular_file())
                    {
                        if (extension.empty() || entry.path().extension() == extension)
                        {
                            files.push_back(entry.path().string());
                        }
                    }
                }
            }
            else
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
                {
                    if (entry.is_regular_file())
                    {
                        if (extension.empty() || entry.path().extension() == extension)
                        {
                            files.push_back(entry.path().string());
                        }
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // Return empty list on error
        }

        return files;
    }

    std::vector<std::string> FileSystem::GetDirectoriesInDirectory(
        const std::string& directory,
        bool recursive)
    {
        std::vector<std::string> directories;

        try
        {
            if (!recursive)
            {
                for (const auto& entry : std::filesystem::directory_iterator(directory))
                {
                    if (entry.is_directory())
                    {
                        directories.push_back(entry.path().string());
                    }
                }
            }
            else
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
                {
                    if (entry.is_directory())
                    {
                        directories.push_back(entry.path().string());
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // Return empty list on error
        }

        return directories;
    }

    // ============================================================================
    // Application Paths
    // ============================================================================

    std::string FileSystem::GetExecutableDirectory()
    {
#ifdef _WIN32
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::filesystem::path p(path);
        return p.parent_path().string();
#else
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        if (count != -1)
        {
            std::filesystem::path p(std::string(path, count));
            return p.parent_path().string();
        }
        return "";
#endif
    }

    std::string FileSystem::GetWorkingDirectory()
    {
        return std::filesystem::current_path().string();
    }

    bool FileSystem::SetWorkingDirectory(const std::string& path)
    {
        try
        {
            std::filesystem::current_path(path);
            return true;
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return false;
        }
    }

    std::string FileSystem::GetAssetsDirectory()
    {
        return CombinePath(GetExecutableDirectory(), "assets");
    }

    std::string FileSystem::GetShadersDirectory()
    {
        return CombinePath(GetExecutableDirectory(), "shaders");
    }

    // ============================================================================
    // String Conversion
    // ============================================================================

    std::string FileSystem::WStringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return "";

#ifdef _WIN32
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
            static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);

        std::string result(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
            static_cast<int>(wstr.size()), result.data(), size, nullptr, nullptr);

        return result;
#else
        std::mbstate_t state{};
        const wchar_t* src = wstr.data();
        size_t len = std::wcsrtombs(nullptr, &src, 0, &state);

        if (len == static_cast<size_t>(-1)) return "";

        std::string result(len, '\0');
        std::wcsrtombs(result.data(), &src, len, &state);
        return result;
#endif
    }

    std::wstring FileSystem::StringToWString(const std::string& str)
    {
        if (str.empty()) return L"";

#ifdef _WIN32
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
            static_cast<int>(str.size()), nullptr, 0);

        std::wstring result(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
            static_cast<int>(str.size()), result.data(), size);

        return result;
#else
        std::mbstate_t state{};
        const char* src = str.data();
        size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);

        if (len == static_cast<size_t>(-1)) return L"";

        std::wstring result(len, L'\0');
        std::mbsrtowcs(result.data(), &src, len, &state);
        return result;
#endif
    }

} // namespace SM
