/**
 * @file CreateCommand.h
 * @brief Create command handler for gendev
 *
 * Handles the "gendev create <type> <name>" command for
 * generating ECS components, systems, and events.
 */

#pragma once

#include "CodeGenerator.h"

#include <string>
#include <vector>
#include <map>

namespace gendev
{
    // ============================================================================
    // CreateCommand Class
    // ============================================================================

    /**
     * @brief Handler for the "create" command
     *
     * Generates code files for ECS components, systems, and events
     * based on command-line arguments.
     */
    class CreateCommand
    {
    public:
        /**
         * @brief Construct the create command handler
         * @param projectRoot Root directory of the project
         */
        explicit CreateCommand(const std::string& projectRoot);

        /**
         * @brief Execute the create command
         * @param args Command arguments (type, name, members...)
         * @param flags Command flags (e.g., --update-cmake)
         * @return Exit code (0 = success, non-zero = error)
         */
        int Execute(
            const std::vector<std::string>& args,
            const std::map<std::string, std::string>& flags);

        /**
         * @brief Set the templates directory path
         */
        void SetTemplatesPath(const std::string& path);

        /**
         * @brief Set custom output directory
         */
        void SetOutputPath(const std::string& path) { m_CustomOutputPath = path; }

    private:
        std::string m_ProjectRoot;
        std::string m_TemplatesPath;
        std::string m_CustomOutputPath;
        CodeGenerator m_Generator;

        /**
         * @brief Create a component
         * @param name Component name
         * @param members Member definitions (name:type pairs)
         * @param flags Command flags
         * @return Exit code
         */
        int CreateComponent(
            const std::string& name,
            const std::vector<std::string>& members,
            const std::map<std::string, std::string>& flags);

        /**
         * @brief Create a system
         * @param name System name
         * @param flags Command flags
         * @return Exit code
         */
        int CreateSystem(
            const std::string& name,
            const std::map<std::string, std::string>& flags);

        /**
         * @brief Create an event
         * @param name Event name
         * @param members Member definitions (name:type pairs)
         * @param flags Command flags
         * @return Exit code
         */
        int CreateEvent(
            const std::string& name,
            const std::vector<std::string>& members,
            const std::map<std::string, std::string>& flags);

        /**
         * @brief Get output directory for a given type
         * @param type The type (component, system, event)
         * @return Output directory path
         */
        std::string GetOutputDirectory(const std::string& type);

        /**
         * @brief Update CMakeLists.txt with new source files
         * @param sourceFiles List of source file paths
         * @return true if successful
         */
        bool UpdateCMakeLists(const std::vector<std::string>& sourceFiles);

        /**
         * @brief Find the CMakeLists.txt file to update
         * @param startPath Directory to start searching from
         * @return Path to CMakeLists.txt, or empty string if not found
         */
        std::string FindCMakeListsFile(const std::string& startPath);

        /**
         * @brief Add source files to CMakeLists.txt at the marker position
         * @param cmakeFile Path to CMakeLists.txt
         * @param sourceFiles List of source file paths
         * @return true if successful
         */
        bool AddSourcesToCMake(
            const std::string& cmakeFile,
            const std::vector<std::string>& sourceFiles);

        /**
         * @brief Convert absolute path to relative path from CMakeLists.txt location
         */
        std::string GetRelativePath(
            const std::string& absolutePath,
            const std::string& basePath);

        /**
         * @brief Print usage information for the create command
         */
        void PrintUsage();

        /**
         * @brief Capitalize the first letter of a string
         */
        std::string Capitalize(const std::string& str);
    };

} // namespace gendev
