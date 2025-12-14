/**
 * @file AskCommand.h
 * @brief Ask command handler for gendev
 *
 * Handles the "gendev ask <query>" command by searching
 * project files for relevant information.
 */

#pragma once

#include "FileSearch.h"

#include <string>
#include <vector>

namespace gendev
{
    // ============================================================================
    // AskCommand Class
    // ============================================================================

    /**
     * @brief Handler for the "ask" command
     *
     * Searches through project files to find information
     * relevant to the user's query.
     */
    class AskCommand
    {
    public:
        /**
         * @brief Construct the ask command handler
         * @param projectRoot Root directory of the project
         */
        explicit AskCommand(const std::string& projectRoot);

        /**
         * @brief Execute the ask command
         * @param args Command arguments (query string)
         * @return Exit code (0 = success, non-zero = error)
         */
        int Execute(const std::vector<std::string>& args);

        /**
         * @brief Set the maximum number of results to display
         */
        void SetMaxResults(size_t maxResults) { m_MaxResults = maxResults; }

        /**
         * @brief Set the number of context lines to show
         */
        void SetContextLines(int lines) { m_ContextLines = lines; }

    private:
        std::string m_ProjectRoot;
        size_t m_MaxResults = 10;
        int m_ContextLines = 3;

        /**
         * @brief Print search results to console
         * @param results The search results to display
         * @param query The original query (for display)
         */
        void PrintResults(
            const std::vector<SearchResult>& results,
            const std::string& query);

        /**
         * @brief Group results by file for better readability
         * @param results Ungrouped results
         * @return Results grouped by file path
         */
        std::map<std::string, std::vector<SearchResult>> GroupResultsByFile(
            const std::vector<SearchResult>& results);

        /**
         * @brief Get relative path from project root
         */
        std::string GetRelativePath(const std::string& fullPath);
    };

} // namespace gendev
