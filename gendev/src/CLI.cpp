/**
 * @file CLI.cpp
 * @brief Implementation of command-line interface parser
 */

#include "CLI.h"

#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace gendev
{
    // ============================================================================
    // Version Information
    // ============================================================================

    static constexpr const char* GENDEV_VERSION = "0.1.0";
    static constexpr const char* GENDEV_NAME = "gendev";

    // ============================================================================
    // Console Implementation
    // ============================================================================

    namespace Console
    {
        static bool g_ColorEnabled = true;

#ifdef _WIN32
        // Windows console color codes
        enum class Color : WORD
        {
            Default = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
            Red = FOREGROUND_RED | FOREGROUND_INTENSITY,
            Green = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            Yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
            Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        };

        static void SetConsoleColor(Color color)
        {
            if (!g_ColorEnabled)
            {
                return;
            }

            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hConsole != INVALID_HANDLE_VALUE)
            {
                SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
            }
        }

        static void ResetConsoleColor()
        {
            SetConsoleColor(Color::Default);
        }
#else
        // ANSI color codes for Unix-like systems
        static void SetConsoleColor(const char* colorCode)
        {
            if (g_ColorEnabled)
            {
                std::cout << colorCode;
            }
        }

        static void ResetConsoleColor()
        {
            if (g_ColorEnabled)
            {
                std::cout << "\033[0m";
            }
        }
#endif

        void PrintSuccess(const std::string& message)
        {
#ifdef _WIN32
            SetConsoleColor(Color::Green);
#else
            SetConsoleColor("\033[32m");
#endif
            std::cout << message;
            ResetConsoleColor();
            std::cout << std::endl;
        }

        void PrintError(const std::string& message)
        {
#ifdef _WIN32
            SetConsoleColor(Color::Red);
#else
            SetConsoleColor("\033[31m");
#endif
            std::cerr << message;
            ResetConsoleColor();
            std::cerr << std::endl;
        }

        void PrintWarning(const std::string& message)
        {
#ifdef _WIN32
            SetConsoleColor(Color::Yellow);
#else
            SetConsoleColor("\033[33m");
#endif
            std::cout << message;
            ResetConsoleColor();
            std::cout << std::endl;
        }

        void PrintInfo(const std::string& message)
        {
#ifdef _WIN32
            SetConsoleColor(Color::Cyan);
#else
            SetConsoleColor("\033[36m");
#endif
            std::cout << message;
            ResetConsoleColor();
            std::cout << std::endl;
        }

        void Print(const std::string& message)
        {
            std::cout << message << std::endl;
        }

        void SetColorEnabled(bool enabled)
        {
            g_ColorEnabled = enabled;
        }

    } // namespace Console

    // ============================================================================
    // CLI Implementation
    // ============================================================================

    CLI::CLI(int argc, char* argv[])
        : m_Argc(argc)
        , m_Argv(argv)
    {
        if (argc > 0 && argv[0] != nullptr)
        {
            m_ProgramName = argv[0];

            // Extract just the filename from the path
            size_t lastSlash = m_ProgramName.find_last_of("/\\");
            if (lastSlash != std::string::npos)
            {
                m_ProgramName = m_ProgramName.substr(lastSlash + 1);
            }

            // Remove .exe extension on Windows
            size_t extPos = m_ProgramName.rfind(".exe");
            if (extPos != std::string::npos && extPos == m_ProgramName.length() - 4)
            {
                m_ProgramName = m_ProgramName.substr(0, extPos);
            }
        }
        else
        {
            m_ProgramName = GENDEV_NAME;
        }
    }

    CommandArgs CLI::Parse()
    {
        CommandArgs args;

        for (int i = 1; i < m_Argc; ++i)
        {
            std::string arg = m_Argv[i];

            // Check for help/version flags first
            if (arg == "--help" || arg == "-h")
            {
                m_HelpRequested = true;
                continue;
            }
            if (arg == "--version" || arg == "-v")
            {
                m_VersionRequested = true;
                continue;
            }

            // Parse flags
            if (IsFlag(arg))
            {
                ParseFlag(arg, args.Flags);
            }
            // First non-flag argument is the command
            else if (args.Command.empty())
            {
                args.Command = arg;
            }
            // Subsequent non-flag arguments are positional arguments
            else
            {
                args.Arguments.push_back(arg);
            }
        }

        return args;
    }

    bool CLI::IsFlag(const std::string& arg) const
    {
        return !arg.empty() && arg[0] == '-';
    }

    void CLI::ParseFlag(const std::string& arg, std::map<std::string, std::string>& flags)
    {
        std::string flagName;
        std::string flagValue = "true";

        // Handle --flag=value format
        size_t equalsPos = arg.find('=');
        if (equalsPos != std::string::npos)
        {
            flagName = arg.substr(0, equalsPos);
            flagValue = arg.substr(equalsPos + 1);
        }
        else
        {
            flagName = arg;
        }

        // Remove leading dashes
        while (!flagName.empty() && flagName[0] == '-')
        {
            flagName = flagName.substr(1);
        }

        if (!flagName.empty())
        {
            flags[flagName] = flagValue;
        }
    }

    void CLI::PrintHelp() const
    {
        std::cout << "Usage: " << m_ProgramName << " <command> [arguments] [options]\n\n";

        std::cout << "Commands:\n";
        std::cout << "  ask <query>              Search project files for relevant information\n";
        std::cout << "  create <type> <name>     Generate code from templates\n\n";

        std::cout << "Create Types:\n";
        std::cout << "  component <Name> [member:type ...]  Create ECS component\n";
        std::cout << "  system <Name>                       Create ECS system\n";
        std::cout << "  event <Name> [member:type ...]      Create event structure\n\n";

        std::cout << "Options:\n";
        std::cout << "  --update-cmake           Add generated files to CMakeLists.txt\n";
        std::cout << "  --output=<path>          Specify output directory\n";
        std::cout << "  --no-color               Disable colored output\n";
        std::cout << "  -h, --help               Show this help message\n";
        std::cout << "  -v, --version            Show version information\n\n";

        std::cout << "Examples:\n";
        std::cout << "  " << m_ProgramName << " create component Velocity x:float y:float z:float\n";
        std::cout << "  " << m_ProgramName << " create system Movement\n";
        std::cout << "  " << m_ProgramName << " create event PlayerDeath playerID:int\n";
        std::cout << "  " << m_ProgramName << " ask \"How does the ECS system work?\"\n";
        std::cout << "  " << m_ProgramName << " ask TransformComponent\n";
    }

    void CLI::PrintVersion() const
    {
        std::cout << GENDEV_NAME << " version " << GENDEV_VERSION << "\n";
        std::cout << "Shattered Moon Engine Development Tool\n";
    }

} // namespace gendev
