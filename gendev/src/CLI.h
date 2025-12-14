/**
 * @file CLI.h
 * @brief Command-line interface parser for gendev tool
 *
 * Parses command-line arguments into structured command data
 * for processing by various command handlers.
 */

#pragma once

#include <string>
#include <vector>
#include <map>

namespace gendev
{
    // ============================================================================
    // CommandArgs Structure
    // ============================================================================

    /**
     * @brief Parsed command-line arguments
     *
     * Contains the command name, positional arguments, and flag options.
     */
    struct CommandArgs
    {
        /** The primary command (e.g., "ask", "create") */
        std::string Command;

        /** Positional arguments after the command */
        std::vector<std::string> Arguments;

        /** Flag options (e.g., --update-cmake -> {"update-cmake", "true"}) */
        std::map<std::string, std::string> Flags;

        /** Check if a flag is present */
        bool HasFlag(const std::string& flag) const
        {
            return Flags.find(flag) != Flags.end();
        }

        /** Get flag value or default */
        std::string GetFlag(const std::string& flag, const std::string& defaultValue = "") const
        {
            auto it = Flags.find(flag);
            if (it != Flags.end())
            {
                return it->second;
            }
            return defaultValue;
        }
    };

    // ============================================================================
    // CLI Class
    // ============================================================================

    /**
     * @brief Command-line interface parser
     *
     * Handles parsing of command-line arguments and provides
     * help/version information.
     */
    class CLI
    {
    public:
        /**
         * @brief Construct CLI parser
         * @param argc Argument count from main()
         * @param argv Argument values from main()
         */
        CLI(int argc, char* argv[]);

        /**
         * @brief Parse command-line arguments
         * @return Parsed command arguments structure
         */
        CommandArgs Parse();

        /**
         * @brief Print help information to stdout
         */
        void PrintHelp() const;

        /**
         * @brief Print version information to stdout
         */
        void PrintVersion() const;

        /**
         * @brief Check if help was requested
         * @return true if --help or -h flag was present
         */
        bool IsHelpRequested() const { return m_HelpRequested; }

        /**
         * @brief Check if version was requested
         * @return true if --version or -v flag was present
         */
        bool IsVersionRequested() const { return m_VersionRequested; }

        /**
         * @brief Get the program name (argv[0])
         */
        const std::string& GetProgramName() const { return m_ProgramName; }

    private:
        int m_Argc;
        char** m_Argv;
        std::string m_ProgramName;
        bool m_HelpRequested = false;
        bool m_VersionRequested = false;

        /**
         * @brief Check if a string is a flag (starts with - or --)
         */
        bool IsFlag(const std::string& arg) const;

        /**
         * @brief Parse a flag argument
         * @param arg The flag string
         * @param flags Output map to store parsed flag
         */
        void ParseFlag(const std::string& arg, std::map<std::string, std::string>& flags);
    };

    // ============================================================================
    // Console Output Utilities
    // ============================================================================

    namespace Console
    {
        /** Print text in green (success) */
        void PrintSuccess(const std::string& message);

        /** Print text in red (error) */
        void PrintError(const std::string& message);

        /** Print text in yellow (warning) */
        void PrintWarning(const std::string& message);

        /** Print text in cyan (info) */
        void PrintInfo(const std::string& message);

        /** Print text in default color */
        void Print(const std::string& message);

        /** Enable or disable colored output */
        void SetColorEnabled(bool enabled);
    }

} // namespace gendev
