/**
 * @file FileSearch.cpp
 * @brief Implementation of file search functionality
 */

#include "FileSearch.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <iomanip>
#include <map>

namespace gendev
{
    // ============================================================================
    // Stop Words for Keyword Extraction
    // ============================================================================

    static const std::set<std::string> g_StopWords = {
        "a", "an", "the", "is", "are", "was", "were", "be", "been", "being",
        "have", "has", "had", "do", "does", "did", "will", "would", "could",
        "should", "may", "might", "must", "shall", "can", "need", "dare",
        "ought", "used", "to", "of", "in", "for", "on", "with", "at", "by",
        "from", "up", "about", "into", "over", "after", "beneath", "under",
        "above", "and", "but", "or", "nor", "so", "yet", "both", "either",
        "neither", "not", "only", "own", "same", "than", "too", "very",
        "just", "also", "now", "how", "what", "which", "who", "whom",
        "this", "that", "these", "those", "am", "if", "then", "because",
        "as", "until", "while", "it", "its", "i", "my", "we", "our", "you",
        "your", "he", "him", "his", "she", "her", "they", "them", "their"
    };

    // Korean stop words (common particles and endings)
    static const std::set<std::string> g_KoreanStopWords = {
        "은", "는", "이", "가", "을", "를", "에", "의", "로", "으로",
        "와", "과", "도", "만", "에서", "까지", "부터", "처럼", "같이",
        "보다", "대로", "마다", "밖에", "조차", "마저", "나", "이나",
        "요", "다", "니다", "습니다", "어요", "아요", "었", "았", "겠"
    };

    // ============================================================================
    // Keyword Extraction
    // ============================================================================

    std::vector<std::string> ExtractKeywords(const std::string& query)
    {
        std::vector<std::string> keywords;
        std::string current;

        for (char c : query)
        {
            // Allow alphanumeric, underscore, and Korean characters
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' ||
                (static_cast<unsigned char>(c) >= 0x80))
            {
                current += c;
            }
            else if (!current.empty())
            {
                // Convert to lowercase for English
                std::string lower;
                for (char ch : current)
                {
                    if (static_cast<unsigned char>(ch) < 0x80)
                    {
                        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    }
                    else
                    {
                        lower += ch;
                    }
                }

                // Check if it's a stop word
                if (g_StopWords.find(lower) == g_StopWords.end() &&
                    g_KoreanStopWords.find(current) == g_KoreanStopWords.end())
                {
                    // Check minimum length (2 for English, any for Korean/mixed)
                    bool hasNonAscii = false;
                    for (char ch : current)
                    {
                        if (static_cast<unsigned char>(ch) >= 0x80)
                        {
                            hasNonAscii = true;
                            break;
                        }
                    }

                    if (hasNonAscii || current.length() >= 2)
                    {
                        keywords.push_back(current);
                    }
                }
                current.clear();
            }
        }

        // Don't forget the last word
        if (!current.empty())
        {
            std::string lower;
            for (char ch : current)
            {
                if (static_cast<unsigned char>(ch) < 0x80)
                {
                    lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                }
                else
                {
                    lower += ch;
                }
            }

            if (g_StopWords.find(lower) == g_StopWords.end())
            {
                bool hasNonAscii = false;
                for (char ch : current)
                {
                    if (static_cast<unsigned char>(ch) >= 0x80)
                    {
                        hasNonAscii = true;
                        break;
                    }
                }

                if (hasNonAscii || current.length() >= 2)
                {
                    keywords.push_back(current);
                }
            }
        }

        // Remove duplicates while preserving order
        std::vector<std::string> unique;
        std::set<std::string> seen;
        for (const auto& kw : keywords)
        {
            std::string lower;
            for (char ch : kw)
            {
                if (static_cast<unsigned char>(ch) < 0x80)
                {
                    lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                }
                else
                {
                    lower += ch;
                }
            }

            if (seen.find(lower) == seen.end())
            {
                seen.insert(lower);
                unique.push_back(kw);
            }
        }

        return unique;
    }

    // ============================================================================
    // FileSearch Implementation
    // ============================================================================

    FileSearch::FileSearch(const std::string& rootPath)
        : m_RootPath(rootPath)
    {
        // Set default file weights (higher = more important)
        m_FileWeights["Project.md"] = 2.0f;
        m_FileWeights["README.md"] = 1.5f;
        m_FileWeights["CLAUDE.md"] = 1.5f;
        m_FileWeights[".h"] = 1.2f;
        m_FileWeights[".hpp"] = 1.2f;
        m_FileWeights[".cpp"] = 1.0f;
        m_FileWeights[".md"] = 1.3f;
    }

    void FileSearch::SetExtensions(const std::vector<std::string>& extensions)
    {
        m_Extensions = extensions;
    }

    void FileSearch::AddExcludedDirectory(const std::string& dirName)
    {
        m_ExcludedDirs.push_back(dirName);
    }

    void FileSearch::SetFileWeight(const std::string& pattern, float weight)
    {
        m_FileWeights[pattern] = weight;
    }

    std::vector<SearchResult> FileSearch::Search(const std::string& query)
    {
        std::vector<std::string> keywords = ExtractKeywords(query);
        return SearchKeywords(keywords);
    }

    std::vector<SearchResult> FileSearch::SearchKeywords(const std::vector<std::string>& keywords)
    {
        std::vector<SearchResult> results;

        if (keywords.empty())
        {
            return results;
        }

        auto files = GetSearchableFiles();

        for (const auto& filePath : files)
        {
            SearchFile(filePath, keywords, results);
        }

        // Sort by relevance
        std::sort(results.begin(), results.end());

        // Limit results
        if (results.size() > m_MaxResults)
        {
            results.resize(m_MaxResults);
        }

        return results;
    }

    std::vector<std::filesystem::path> FileSearch::GetSearchableFiles()
    {
        std::vector<std::filesystem::path> files;

        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                m_RootPath,
                std::filesystem::directory_options::skip_permission_denied))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }

                // Check if in excluded directory
                if (IsExcludedDirectory(entry.path()))
                {
                    continue;
                }

                // Check extension
                std::string ext = entry.path().extension().string();
                bool validExt = false;
                for (const auto& allowedExt : m_Extensions)
                {
                    if (ext == allowedExt)
                    {
                        validExt = true;
                        break;
                    }
                }

                if (validExt)
                {
                    files.push_back(entry.path());
                }
            }
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // Ignore filesystem errors
        }

        return files;
    }

    bool FileSearch::IsExcludedDirectory(const std::filesystem::path& path) const
    {
        std::string pathStr = path.string();

        // Normalize path separators
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

        for (const auto& excluded : m_ExcludedDirs)
        {
            // Check if any path component matches the excluded directory
            std::string pattern = "/" + excluded + "/";
            if (pathStr.find(pattern) != std::string::npos)
            {
                return true;
            }

            // Also check if it starts with the excluded dir
            if (pathStr.find(excluded + "/") == 0)
            {
                return true;
            }
        }

        return false;
    }

    float FileSearch::GetFileWeight(const std::filesystem::path& filePath) const
    {
        std::string filename = filePath.filename().string();
        std::string ext = filePath.extension().string();

        // Check for exact filename match first
        auto it = m_FileWeights.find(filename);
        if (it != m_FileWeights.end())
        {
            return it->second;
        }

        // Check for extension match
        it = m_FileWeights.find(ext);
        if (it != m_FileWeights.end())
        {
            return it->second;
        }

        return 1.0f;
    }

    void FileSearch::SearchFile(
        const std::filesystem::path& filePath,
        const std::vector<std::string>& keywords,
        std::vector<SearchResult>& results)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }
        file.close();

        float fileWeight = GetFileWeight(filePath);

        for (size_t i = 0; i < lines.size(); ++i)
        {
            const std::string& currentLine = lines[i];
            int matchCount = 0;

            // Count keyword matches in this line
            for (const auto& keyword : keywords)
            {
                if (ContainsKeyword(currentLine, keyword))
                {
                    ++matchCount;
                }
            }

            if (matchCount > 0)
            {
                SearchResult result;
                result.FilePath = filePath.string();
                result.LineNumber = static_cast<int>(i + 1); // 1-indexed
                result.MatchLine = currentLine;
                result.Context = GetContext(lines, i);
                result.MatchCount = matchCount;

                // Calculate relevance
                // Base relevance from match count ratio
                float matchRatio = static_cast<float>(matchCount) / static_cast<float>(keywords.size());

                // Bonus for matching more keywords
                float multiMatchBonus = matchCount > 1 ? 0.2f * (matchCount - 1) : 0.0f;

                result.Relevance = (matchRatio + multiMatchBonus) * fileWeight;

                // Cap at 1.0
                result.Relevance = std::min(result.Relevance, 1.0f);

                results.push_back(result);
            }
        }
    }

    std::string FileSearch::GetContext(
        const std::vector<std::string>& lines,
        size_t matchLine)
    {
        std::ostringstream context;

        size_t startLine = matchLine > static_cast<size_t>(m_ContextLines)
            ? matchLine - m_ContextLines
            : 0;
        size_t endLine = std::min(matchLine + m_ContextLines + 1, lines.size());

        for (size_t i = startLine; i < endLine; ++i)
        {
            // Format: "123 | content"
            int lineNum = static_cast<int>(i + 1);
            context << (i == matchLine ? ">" : " ");
            context << std::setw(4) << lineNum << " | " << lines[i] << "\n";
        }

        return context.str();
    }

    float FileSearch::CalculateRelevance(
        const std::string& content,
        const std::vector<std::string>& keywords,
        const std::filesystem::path& filePath)
    {
        int matchCount = 0;
        for (const auto& keyword : keywords)
        {
            if (ContainsKeyword(content, keyword))
            {
                ++matchCount;
            }
        }

        if (matchCount == 0)
        {
            return 0.0f;
        }

        float baseRelevance = static_cast<float>(matchCount) / static_cast<float>(keywords.size());
        float fileWeight = GetFileWeight(filePath);

        return baseRelevance * fileWeight;
    }

    bool FileSearch::ContainsKeyword(const std::string& text, const std::string& keyword) const
    {
        std::string lowerText = ToLower(text);
        std::string lowerKeyword = ToLower(keyword);

        return lowerText.find(lowerKeyword) != std::string::npos;
    }

    std::string FileSearch::ToLower(const std::string& str) const
    {
        std::string result;
        result.reserve(str.size());

        for (char c : str)
        {
            if (static_cast<unsigned char>(c) < 0x80)
            {
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            else
            {
                result += c;
            }
        }

        return result;
    }

} // namespace gendev
