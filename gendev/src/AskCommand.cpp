/**
 * @file AskCommand.cpp
 * @brief Implementation of ask command handler
 */

#include "AskCommand.h"
#include "CLI.h"

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <set>
#include <map>

namespace gendev
{
    AskCommand::AskCommand(const std::string& projectRoot)
        : m_ProjectRoot(projectRoot)
    {
    }

    int AskCommand::Execute(const std::vector<std::string>& args)
    {
        // Check if query is provided
        if (args.empty())
        {
            Console::PrintError("Error: No query provided.");
            Console::Print("Usage: gendev ask \"your question here\"");
            return 1;
        }

        // Combine all arguments into a single query
        std::string query;
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (i > 0) query += " ";
            query += args[i];
        }

        // Extract keywords for display
        std::vector<std::string> keywords = ExtractKeywords(query);

        if (keywords.empty())
        {
            Console::PrintError("Error: No searchable keywords found in query.");
            return 1;
        }

        // Display search info
        std::ostringstream searchInfo;
        searchInfo << "Searching for: ";
        for (size_t i = 0; i < keywords.size(); ++i)
        {
            if (i > 0) searchInfo << ", ";
            searchInfo << "\"" << keywords[i] << "\"";
        }
        Console::PrintInfo(searchInfo.str());
        Console::Print("");

        // Perform search
        FileSearch searcher(m_ProjectRoot);
        searcher.SetMaxResults(m_MaxResults);
        searcher.SetContextLines(m_ContextLines);

        std::vector<SearchResult> results = searcher.Search(query);

        if (results.empty())
        {
            Console::PrintWarning("No results found for: " + query);
            Console::Print("Try using different keywords or check if the project files exist.");
            return 0;
        }

        // Print results
        PrintResults(results, query);

        return 0;
    }

    void AskCommand::PrintResults(
        const std::vector<SearchResult>& results,
        const std::string& query)
    {
        // Group results by file
        auto grouped = GroupResultsByFile(results);

        std::ostringstream summary;
        summary << "Found relevant information in " << grouped.size() << " file(s):";
        Console::PrintSuccess(summary.str());
        Console::Print("");

        int fileIndex = 1;
        for (const auto& [filePath, fileResults] : grouped)
        {
            // Print file header
            std::ostringstream header;
            header << "[" << fileIndex << "] " << GetRelativePath(filePath);
            Console::PrintInfo(header.str());

            // Sort results within this file by line number
            std::vector<SearchResult> sortedResults = fileResults;
            std::sort(sortedResults.begin(), sortedResults.end(),
                [](const SearchResult& a, const SearchResult& b) {
                    return a.LineNumber < b.LineNumber;
                });

            // Print each result's context (avoiding overlapping contexts)
            std::set<int> printedLines;
            for (const auto& result : sortedResults)
            {
                // Skip if we've already printed lines close to this one
                bool tooClose = false;
                for (int printed : printedLines)
                {
                    if (std::abs(result.LineNumber - printed) <= m_ContextLines)
                    {
                        tooClose = true;
                        break;
                    }
                }

                if (tooClose)
                {
                    continue;
                }

                printedLines.insert(result.LineNumber);

                Console::Print("...");
                Console::Print(result.Context);
            }

            Console::Print("");
            ++fileIndex;
        }
    }

    std::map<std::string, std::vector<SearchResult>> AskCommand::GroupResultsByFile(
        const std::vector<SearchResult>& results)
    {
        std::map<std::string, std::vector<SearchResult>> grouped;

        for (const auto& result : results)
        {
            grouped[result.FilePath].push_back(result);
        }

        return grouped;
    }

    std::string AskCommand::GetRelativePath(const std::string& fullPath)
    {
        try
        {
            std::filesystem::path full(fullPath);
            std::filesystem::path root(m_ProjectRoot);

            // Make both paths canonical for proper comparison
            if (std::filesystem::exists(full) && std::filesystem::exists(root))
            {
                full = std::filesystem::canonical(full);
                root = std::filesystem::canonical(root);
            }

            // Try to get relative path
            auto rel = std::filesystem::relative(full, root);
            return rel.string();
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // Return original path if relative path calculation fails
            return fullPath;
        }
    }

} // namespace gendev
