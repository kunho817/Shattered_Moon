/**
 * @file CreateCommand.cpp
 * @brief Implementation of create command handler
 */

#include "CreateCommand.h"
#include "CLI.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <regex>

namespace gendev
{
    // ============================================================================
    // Constants
    // ============================================================================

    static const char* CMAKE_MARKER = "# GENDEV_SOURCE_LIST";

    // ============================================================================
    // CreateCommand Implementation
    // ============================================================================

    CreateCommand::CreateCommand(const std::string& projectRoot)
        : m_ProjectRoot(projectRoot)
    {
        // Set default templates path
        m_TemplatesPath = (std::filesystem::path(projectRoot) / "gendev" / "templates").string();
        m_Generator.SetTemplatesPath(m_TemplatesPath);
    }

    void CreateCommand::SetTemplatesPath(const std::string& path)
    {
        m_TemplatesPath = path;
        m_Generator.SetTemplatesPath(path);
    }

    int CreateCommand::Execute(
        const std::vector<std::string>& args,
        const std::map<std::string, std::string>& flags)
    {
        if (args.empty())
        {
            PrintUsage();
            return 1;
        }

        // First argument is the type
        std::string type = args[0];
        std::transform(type.begin(), type.end(), type.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // Second argument is the name
        if (args.size() < 2)
        {
            Console::PrintError("Error: Missing name argument.");
            PrintUsage();
            return 1;
        }

        std::string name = Capitalize(args[1]);

        // Remaining arguments are member definitions
        std::vector<std::string> members;
        for (size_t i = 2; i < args.size(); ++i)
        {
            members.push_back(args[i]);
        }

        // Dispatch to appropriate handler
        if (type == "component")
        {
            return CreateComponent(name, members, flags);
        }
        else if (type == "system")
        {
            return CreateSystem(name, flags);
        }
        else if (type == "event")
        {
            return CreateEvent(name, members, flags);
        }
        else
        {
            Console::PrintError("Error: Unknown type '" + type + "'.");
            Console::Print("Valid types: component, system, event");
            return 1;
        }
    }

    int CreateCommand::CreateComponent(
        const std::string& name,
        const std::vector<std::string>& members,
        const std::map<std::string, std::string>& flags)
    {
        Console::PrintInfo("Creating component: " + name + "Component");

        // Parse member definitions
        std::vector<MemberDefinition> memberDefs = ParseMemberDefinitions(members);

        if (memberDefs.empty() && !members.empty())
        {
            Console::PrintWarning("Warning: No valid member definitions found.");
            Console::Print("Members should be in format: name:type (e.g., x:float)");
        }

        // Get output directory
        std::string outputDir = GetOutputDirectory("component");

        // Generate component
        if (!m_Generator.GenerateComponent(name, memberDefs, outputDir))
        {
            Console::PrintError("Error: " + m_Generator.GetLastError());
            return 1;
        }

        // Print success message
        for (const auto& file : m_Generator.GetGeneratedFiles())
        {
            Console::PrintSuccess("Created: " + file);
        }

        // Update CMakeLists.txt if requested
        if (flags.find("update-cmake") != flags.end())
        {
            if (UpdateCMakeLists(m_Generator.GetGeneratedFiles()))
            {
                Console::PrintSuccess("Updated CMakeLists.txt");
            }
            else
            {
                Console::PrintWarning("Warning: Could not update CMakeLists.txt");
                Console::Print("Add '# GENDEV_SOURCE_LIST' marker to your CMakeLists.txt");
            }
        }

        return 0;
    }

    int CreateCommand::CreateSystem(
        const std::string& name,
        const std::map<std::string, std::string>& flags)
    {
        Console::PrintInfo("Creating system: " + name + "System");

        // Get output directory
        std::string outputDir = GetOutputDirectory("system");

        // Generate system
        if (!m_Generator.GenerateSystem(name, outputDir))
        {
            Console::PrintError("Error: " + m_Generator.GetLastError());
            return 1;
        }

        // Print success message
        for (const auto& file : m_Generator.GetGeneratedFiles())
        {
            Console::PrintSuccess("Created: " + file);
        }

        // Update CMakeLists.txt if requested
        if (flags.find("update-cmake") != flags.end())
        {
            if (UpdateCMakeLists(m_Generator.GetGeneratedFiles()))
            {
                Console::PrintSuccess("Updated CMakeLists.txt");
            }
            else
            {
                Console::PrintWarning("Warning: Could not update CMakeLists.txt");
                Console::Print("Add '# GENDEV_SOURCE_LIST' marker to your CMakeLists.txt");
            }
        }

        return 0;
    }

    int CreateCommand::CreateEvent(
        const std::string& name,
        const std::vector<std::string>& members,
        const std::map<std::string, std::string>& flags)
    {
        Console::PrintInfo("Creating event: " + name + "Event");

        // Parse member definitions
        std::vector<MemberDefinition> memberDefs = ParseMemberDefinitions(members);

        if (memberDefs.empty() && !members.empty())
        {
            Console::PrintWarning("Warning: No valid member definitions found.");
            Console::Print("Members should be in format: name:type (e.g., entity1:int)");
        }

        // Get output directory
        std::string outputDir = GetOutputDirectory("event");

        // Generate event
        if (!m_Generator.GenerateEvent(name, memberDefs, outputDir))
        {
            Console::PrintError("Error: " + m_Generator.GetLastError());
            return 1;
        }

        // Print success message
        for (const auto& file : m_Generator.GetGeneratedFiles())
        {
            Console::PrintSuccess("Created: " + file);
        }

        // Update CMakeLists.txt if requested
        if (flags.find("update-cmake") != flags.end())
        {
            if (UpdateCMakeLists(m_Generator.GetGeneratedFiles()))
            {
                Console::PrintSuccess("Updated CMakeLists.txt");
            }
            else
            {
                Console::PrintWarning("Warning: Could not update CMakeLists.txt");
                Console::Print("Add '# GENDEV_SOURCE_LIST' marker to your CMakeLists.txt");
            }
        }

        return 0;
    }

    std::string CreateCommand::GetOutputDirectory(const std::string& type)
    {
        // Use custom output path if set
        if (!m_CustomOutputPath.empty())
        {
            return m_CustomOutputPath;
        }

        // Default paths relative to project root
        std::filesystem::path basePath = std::filesystem::path(m_ProjectRoot) / "src" / "ecs";

        if (type == "component")
        {
            return (basePath / "components").string();
        }
        else if (type == "system")
        {
            return (basePath / "systems").string();
        }
        else if (type == "event")
        {
            return (basePath / "events").string();
        }

        return basePath.string();
    }

    bool CreateCommand::UpdateCMakeLists(const std::vector<std::string>& sourceFiles)
    {
        if (sourceFiles.empty())
        {
            return true;
        }

        // Find CMakeLists.txt starting from the first source file's directory
        std::filesystem::path firstFile(sourceFiles[0]);
        std::string cmakeFile = FindCMakeListsFile(firstFile.parent_path().string());

        if (cmakeFile.empty())
        {
            return false;
        }

        return AddSourcesToCMake(cmakeFile, sourceFiles);
    }

    std::string CreateCommand::FindCMakeListsFile(const std::string& startPath)
    {
        std::filesystem::path current(startPath);
        std::filesystem::path root(m_ProjectRoot);

        // Traverse up until we find CMakeLists.txt or reach project root
        while (!current.empty())
        {
            std::filesystem::path cmakePath = current / "CMakeLists.txt";
            if (std::filesystem::exists(cmakePath))
            {
                return cmakePath.string();
            }

            // Check if we've reached or passed the project root
            try
            {
                if (std::filesystem::equivalent(current, root))
                {
                    // Check root CMakeLists.txt
                    cmakePath = root / "CMakeLists.txt";
                    if (std::filesystem::exists(cmakePath))
                    {
                        return cmakePath.string();
                    }
                    break;
                }
            }
            catch (const std::filesystem::filesystem_error&)
            {
                // Continue if comparison fails
            }

            // Move to parent directory
            std::filesystem::path parent = current.parent_path();
            if (parent == current)
            {
                break; // Reached filesystem root
            }
            current = parent;
        }

        return "";
    }

    bool CreateCommand::AddSourcesToCMake(
        const std::string& cmakeFile,
        const std::vector<std::string>& sourceFiles)
    {
        // Read existing content
        std::ifstream inFile(cmakeFile);
        if (!inFile.is_open())
        {
            return false;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(inFile, line))
        {
            lines.push_back(line);
        }
        inFile.close();

        // Find the marker
        int markerLine = -1;
        std::string markerIndent;

        for (size_t i = 0; i < lines.size(); ++i)
        {
            size_t markerPos = lines[i].find(CMAKE_MARKER);
            if (markerPos != std::string::npos)
            {
                markerLine = static_cast<int>(i);
                // Extract indentation
                markerIndent = lines[i].substr(0, markerPos);
                break;
            }
        }

        if (markerLine < 0)
        {
            return false;
        }

        // Get base path for relative path calculation
        std::filesystem::path cmakePath(cmakeFile);
        std::string basePath = cmakePath.parent_path().string();

        // Prepare new source lines
        std::vector<std::string> newSourceLines;
        for (const auto& sourceFile : sourceFiles)
        {
            // Only add .cpp files to CMakeLists.txt
            if (sourceFile.find(".cpp") != std::string::npos ||
                sourceFile.find(".h") != std::string::npos)
            {
                std::string relativePath = GetRelativePath(sourceFile, basePath);
                newSourceLines.push_back(markerIndent + relativePath);
            }
        }

        // Insert new lines after the marker
        lines.insert(lines.begin() + markerLine + 1, newSourceLines.begin(), newSourceLines.end());

        // Write back
        std::ofstream outFile(cmakeFile);
        if (!outFile.is_open())
        {
            return false;
        }

        for (const auto& l : lines)
        {
            outFile << l << "\n";
        }

        return true;
    }

    std::string CreateCommand::GetRelativePath(
        const std::string& absolutePath,
        const std::string& basePath)
    {
        try
        {
            std::filesystem::path absolute(absolutePath);
            std::filesystem::path base(basePath);

            auto relative = std::filesystem::relative(absolute, base);
            std::string result = relative.string();

            // Normalize path separators to forward slashes for CMake
            std::replace(result.begin(), result.end(), '\\', '/');

            return result;
        }
        catch (const std::filesystem::filesystem_error&)
        {
            return absolutePath;
        }
    }

    void CreateCommand::PrintUsage()
    {
        Console::Print("Usage: gendev create <type> <name> [members...]");
        Console::Print("");
        Console::Print("Types:");
        Console::Print("  component  Create an ECS component struct");
        Console::Print("  system     Create an ECS system class (.h and .cpp)");
        Console::Print("  event      Create an event struct");
        Console::Print("");
        Console::Print("Members:");
        Console::Print("  Specify members as name:type pairs");
        Console::Print("  Supported types: int, float, double, bool, string, vec2, vec3, entity");
        Console::Print("");
        Console::Print("Examples:");
        Console::Print("  gendev create component Velocity x:float y:float z:float");
        Console::Print("  gendev create system Movement");
        Console::Print("  gendev create event PlayerDeath playerID:int cause:string");
        Console::Print("");
        Console::Print("Options:");
        Console::Print("  --update-cmake  Add generated files to CMakeLists.txt");
        Console::Print("  --output=<path> Specify output directory");
    }

    std::string CreateCommand::Capitalize(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        std::string result = str;
        result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
        return result;
    }

} // namespace gendev
