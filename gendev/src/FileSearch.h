/**
 * @file FileSearch.h
 * @brief File search functionality for gendev ask command
 *
 * Provides keyword-based search across project source files
 * and documentation.
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace gendev
{
    // ============================================================================
    // SearchResult Structure
    // ============================================================================

    /**
     * @brief Result of a file search operation
     *
     * Contains the file path, line number, context, and relevance score.
     */
    struct SearchResult
    {
        /** Full path to the file */
        std::string FilePath;

        /** Line number where the match was found (1-indexed) */
        int LineNumber = 0;

        /** The matching line content */
        std::string MatchLine;

        /** Context lines around the match */
        std::string Context;

        /** Relevance score (0.0 to 1.0, higher is more relevant) */
        float Relevance = 0.0f;

        /** Number of keyword matches in this result */
        int MatchCount = 0;

        /** Comparison for sorting by relevance */
        bool operator<(const SearchResult& other) const
        {
            return Relevance > other.Relevance; // Higher relevance first
        }
    };

    // ============================================================================
    // FileSearch Class
    // ============================================================================

    /**
     * @brief File search engine for project files
     *
     * Searches through source files, headers, and documentation
     * to find relevant information based on keywords.
     */
    class FileSearch
    {
    public:
        /**
         * @brief Construct a file searcher
         * @param rootPath Root directory to search from
         */
        explicit FileSearch(const std::string& rootPath);

        /**
         * @brief Set the file extensions to search
         * @param extensions List of extensions (e.g., {".h", ".cpp", ".md"})
         */
        void SetExtensions(const std::vector<std::string>& extensions);

        /**
         * @brief Get the current search extensions
         */
        const std::vector<std::string>& GetExtensions() const { return m_Extensions; }

        /**
         * @brief Set maximum results to return
         */
        void SetMaxResults(size_t maxResults) { m_MaxResults = maxResults; }

        /**
         * @brief Set context lines to include around matches
         */
        void SetContextLines(int lines) { m_ContextLines = lines; }

        /**
         * @brief Search for a query string
         * @param query The search query
         * @return Vector of search results sorted by relevance
         */
        std::vector<SearchResult> Search(const std::string& query);

        /**
         * @brief Search for multiple keywords
         * @param keywords Keywords to search for
         * @return Vector of search results sorted by relevance
         */
        std::vector<SearchResult> SearchKeywords(const std::vector<std::string>& keywords);

        /**
         * @brief Add a directory to exclude from search
         */
        void AddExcludedDirectory(const std::string& dirName);

        /**
         * @brief Set file importance weights
         * @param pattern Glob pattern or file name
         * @param weight Weight multiplier for relevance
         */
        void SetFileWeight(const std::string& pattern, float weight);

    private:
        std::string m_RootPath;
        std::vector<std::string> m_Extensions = { ".h", ".hpp", ".cpp", ".md", ".txt" };
        std::vector<std::string> m_ExcludedDirs = { "build", ".git", "external", "_deps" };
        std::map<std::string, float> m_FileWeights;
        size_t m_MaxResults = 20;
        int m_ContextLines = 3;

        /**
         * @brief Get all searchable files in the root path
         * @return Vector of file paths
         */
        std::vector<std::filesystem::path> GetSearchableFiles();

        /**
         * @brief Calculate relevance score for content
         * @param content The content to evaluate
         * @param keywords Keywords to match
         * @param filePath File path (for weighting)
         * @return Relevance score
         */
        float CalculateRelevance(
            const std::string& content,
            const std::vector<std::string>& keywords,
            const std::filesystem::path& filePath);

        /**
         * @brief Search a single file
         * @param filePath Path to the file
         * @param keywords Keywords to search for
         * @param results Output vector to append results to
         */
        void SearchFile(
            const std::filesystem::path& filePath,
            const std::vector<std::string>& keywords,
            std::vector<SearchResult>& results);

        /**
         * @brief Get context lines around a match
         * @param lines All lines of the file
         * @param matchLine Index of the matching line (0-indexed)
         * @return Context string with line numbers
         */
        std::string GetContext(
            const std::vector<std::string>& lines,
            size_t matchLine);

        /**
         * @brief Check if a directory should be excluded
         */
        bool IsExcludedDirectory(const std::filesystem::path& path) const;

        /**
         * @brief Get file weight for relevance calculation
         */
        float GetFileWeight(const std::filesystem::path& filePath) const;

        /**
         * @brief Check if string contains keyword (case-insensitive)
         */
        bool ContainsKeyword(const std::string& text, const std::string& keyword) const;

        /**
         * @brief Convert string to lowercase
         */
        std::string ToLower(const std::string& str) const;
    };

    // ============================================================================
    // Keyword Extraction Utilities
    // ============================================================================

    /**
     * @brief Extract keywords from a natural language query
     * @param query The query string
     * @return Vector of keywords
     */
    std::vector<std::string> ExtractKeywords(const std::string& query);

} // namespace gendev
