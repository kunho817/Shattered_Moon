/**
 * @file CodeGenerator.h
 * @brief Code generation utilities for gendev
 *
 * Handles loading templates and generating source files
 * for ECS components, systems, and events.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <utility>

namespace gendev
{
    // ============================================================================
    // MemberDefinition Structure
    // ============================================================================

    /**
     * @brief Represents a member variable definition
     *
     * Used for components and events to define their data members.
     */
    struct MemberDefinition
    {
        std::string Name;
        std::string Type;

        MemberDefinition() = default;
        MemberDefinition(const std::string& name, const std::string& type)
            : Name(name), Type(type) {}
    };

    // ============================================================================
    // CodeGenerator Class
    // ============================================================================

    /**
     * @brief Code generation engine
     *
     * Loads template files and generates source code for
     * ECS components, systems, and events.
     */
    class CodeGenerator
    {
    public:
        /**
         * @brief Construct the code generator
         * @param templatesPath Path to the templates directory
         */
        explicit CodeGenerator(const std::string& templatesPath = "");

        /**
         * @brief Set the templates directory path
         */
        void SetTemplatesPath(const std::string& path) { m_TemplatesPath = path; }

        /**
         * @brief Get the templates directory path
         */
        const std::string& GetTemplatesPath() const { return m_TemplatesPath; }

        /**
         * @brief Generate a component header file
         * @param name Component name (without "Component" suffix)
         * @param members Member variables for the component
         * @param outputPath Output directory path
         * @return true if generation succeeded
         */
        bool GenerateComponent(
            const std::string& name,
            const std::vector<MemberDefinition>& members,
            const std::string& outputPath);

        /**
         * @brief Generate system header and source files
         * @param name System name (without "System" suffix)
         * @param outputPath Output directory path
         * @return true if generation succeeded
         */
        bool GenerateSystem(
            const std::string& name,
            const std::string& outputPath);

        /**
         * @brief Generate an event header file
         * @param name Event name (without "Event" suffix)
         * @param members Data members for the event
         * @param outputPath Output directory path
         * @return true if generation succeeded
         */
        bool GenerateEvent(
            const std::string& name,
            const std::vector<MemberDefinition>& members,
            const std::string& outputPath);

        /**
         * @brief Get the list of generated files from the last operation
         * @return Vector of generated file paths
         */
        const std::vector<std::string>& GetGeneratedFiles() const { return m_GeneratedFiles; }

        /**
         * @brief Get the last error message
         */
        const std::string& GetLastError() const { return m_LastError; }

    private:
        std::string m_TemplatesPath;
        std::vector<std::string> m_GeneratedFiles;
        std::string m_LastError;

        /**
         * @brief Load a template file
         * @param templateName Name of the template (e.g., "Component.h.template")
         * @return Template content, or empty string on failure
         */
        std::string LoadTemplate(const std::string& templateName);

        /**
         * @brief Apply variable substitutions to a template
         * @param templateContent The template string
         * @param variables Map of variable names to values
         * @return Processed template string
         */
        std::string ApplyTemplate(
            const std::string& templateContent,
            const std::map<std::string, std::string>& variables);

        /**
         * @brief Generate member declarations from definitions
         * @param members The member definitions
         * @param indent Indentation string
         * @return Formatted member declarations
         */
        std::string GenerateMemberDeclarations(
            const std::vector<MemberDefinition>& members,
            const std::string& indent = "    ");

        /**
         * @brief Generate constructor parameters from definitions
         * @param members The member definitions
         * @return Formatted constructor parameter list
         */
        std::string GenerateConstructorParams(
            const std::vector<MemberDefinition>& members);

        /**
         * @brief Generate constructor initializer list
         * @param members The member definitions
         * @return Formatted initializer list
         */
        std::string GenerateInitializerList(
            const std::vector<MemberDefinition>& members);

        /**
         * @brief Write content to a file
         * @param filePath Full path to the output file
         * @param content Content to write
         * @return true if write succeeded
         */
        bool WriteFile(
            const std::string& filePath,
            const std::string& content);

        /**
         * @brief Ensure a directory exists
         * @param path Directory path
         * @return true if directory exists or was created
         */
        bool EnsureDirectoryExists(const std::string& path);

        /**
         * @brief Convert type string from CLI format to C++ format
         * @param type Type string (e.g., "float", "int", "string")
         * @return C++ type string
         */
        std::string NormalizeType(const std::string& type);

        /**
         * @brief Get the default template for a given type
         * @param templateName Name of the template
         * @return Default template content
         */
        std::string GetDefaultTemplate(const std::string& templateName);
    };

    // ============================================================================
    // Helper Functions
    // ============================================================================

    /**
     * @brief Parse member definitions from command-line arguments
     * @param args Arguments in "name:type" format
     * @return Vector of MemberDefinition
     */
    std::vector<MemberDefinition> ParseMemberDefinitions(
        const std::vector<std::string>& args);

} // namespace gendev
