/**
 * @file main.cpp
 * @brief Entry point for gendev CLI tool
 *
 * gendev is a development assistant tool for the Shattered Moon engine.
 * It provides code generation and project search capabilities.
 *
 * Usage:
 *   gendev ask <query>                              Search project files
 *   gendev create component <name> [members...]     Create ECS component
 *   gendev create system <name>                     Create ECS system
 *   gendev create event <name> [members...]         Create event struct
 */

#include "CLI.h"
#include "AskCommand.h"
#include "CreateCommand.h"

#include <iostream>
#include <filesystem>

/**
 * @brief Get the project root directory
 *
 * Searches for indicators of the project root (CMakeLists.txt, .git, etc.)
 * starting from the current working directory.
 */
std::string FindProjectRoot()
{
    std::filesystem::path current = std::filesystem::current_path();

    // Look for project root indicators
    const std::vector<std::string> indicators = {
        "CMakeLists.txt",
        "Project.md",
        ".git"
    };

    // Check each parent directory
    while (!current.empty())
    {
        for (const auto& indicator : indicators)
        {
            std::filesystem::path checkPath = current / indicator;
            if (std::filesystem::exists(checkPath))
            {
                return current.string();
            }
        }

        // Move to parent directory
        std::filesystem::path parent = current.parent_path();
        if (parent == current)
        {
            break; // Reached filesystem root
        }
        current = parent;
    }

    // Fallback to current directory
    return std::filesystem::current_path().string();
}

int main(int argc, char* argv[])
{
    // Set console output to UTF-8 on Windows
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Parse command line
    gendev::CLI cli(argc, argv);
    gendev::CommandArgs args = cli.Parse();

    // Handle help/version
    if (cli.IsHelpRequested())
    {
        cli.PrintHelp();
        return 0;
    }

    if (cli.IsVersionRequested())
    {
        cli.PrintVersion();
        return 0;
    }

    // Check for no-color flag
    if (args.HasFlag("no-color"))
    {
        gendev::Console::SetColorEnabled(false);
    }

    // Require a command
    if (args.Command.empty())
    {
        gendev::Console::PrintError("Error: No command specified.");
        gendev::Console::Print("");
        cli.PrintHelp();
        return 1;
    }

    // Find project root
    std::string projectRoot = FindProjectRoot();

    // Dispatch to command handler
    if (args.Command == "ask")
    {
        gendev::AskCommand askCmd(projectRoot);

        // Apply options
        if (args.HasFlag("max-results"))
        {
            try
            {
                size_t maxResults = std::stoul(args.GetFlag("max-results"));
                askCmd.SetMaxResults(maxResults);
            }
            catch (...)
            {
                gendev::Console::PrintWarning("Warning: Invalid max-results value, using default.");
            }
        }

        if (args.HasFlag("context"))
        {
            try
            {
                int contextLines = std::stoi(args.GetFlag("context"));
                askCmd.SetContextLines(contextLines);
            }
            catch (...)
            {
                gendev::Console::PrintWarning("Warning: Invalid context value, using default.");
            }
        }

        return askCmd.Execute(args.Arguments);
    }
    else if (args.Command == "create")
    {
        gendev::CreateCommand createCmd(projectRoot);

        // Apply options
        if (args.HasFlag("output"))
        {
            createCmd.SetOutputPath(args.GetFlag("output"));
        }

        if (args.HasFlag("templates"))
        {
            createCmd.SetTemplatesPath(args.GetFlag("templates"));
        }

        return createCmd.Execute(args.Arguments, args.Flags);
    }
    else
    {
        gendev::Console::PrintError("Error: Unknown command '" + args.Command + "'.");
        gendev::Console::Print("");
        gendev::Console::Print("Available commands:");
        gendev::Console::Print("  ask     Search project files for information");
        gendev::Console::Print("  create  Generate code from templates");
        gendev::Console::Print("");
        gendev::Console::Print("Use 'gendev --help' for more information.");
        return 1;
    }
}
