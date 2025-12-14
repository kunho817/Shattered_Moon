#pragma once

/**
 * @file ConsolePanel.h
 * @brief Console/Log panel for the editor
 *
 * Provides a console window for viewing log messages and executing commands.
 */

#include <string>
#include <vector>
#include <chrono>
#include <mutex>

namespace SM
{
    /**
     * @brief Log level enumeration for console messages
     */
    enum class LogLevel
    {
        Trace,      ///< Very detailed debug information
        Debug,      ///< Debug information
        Info,       ///< General information
        Warning,    ///< Warning messages
        Error,      ///< Error messages
        Critical    ///< Critical errors
    };

    /**
     * @brief Console/Log panel singleton
     *
     * Features:
     * - Log message display with filtering
     * - Color-coded log levels
     * - Auto-scroll option
     * - Command input (future)
     * - Clear and export functionality
     */
    class ConsolePanel
    {
    public:
        /**
         * @brief Get the singleton instance
         */
        static ConsolePanel& Get();

        // Prevent copying
        ConsolePanel(const ConsolePanel&) = delete;
        ConsolePanel& operator=(const ConsolePanel&) = delete;

        /**
         * @brief Draw the console panel using ImGui
         */
        void Draw();

        // ====================================================================
        // Logging
        // ====================================================================

        /**
         * @brief Log a message
         * @param message Message text
         * @param level Log level (default: Info)
         */
        void Log(const std::string& message, LogLevel level = LogLevel::Info);

        /**
         * @brief Log a formatted message
         * @param level Log level
         * @param format Printf-style format string
         * @param ... Format arguments
         */
        void LogFormat(LogLevel level, const char* format, ...);

        /**
         * @brief Log an info message
         */
        void LogInfo(const std::string& message) { Log(message, LogLevel::Info); }

        /**
         * @brief Log a debug message
         */
        void LogDebug(const std::string& message) { Log(message, LogLevel::Debug); }

        /**
         * @brief Log a warning message
         */
        void LogWarning(const std::string& message) { Log(message, LogLevel::Warning); }

        /**
         * @brief Log an error message
         */
        void LogError(const std::string& message) { Log(message, LogLevel::Error); }

        /**
         * @brief Clear all log messages
         */
        void Clear();

        /**
         * @brief Get the number of logged messages
         */
        size_t GetLogCount() const { return m_Logs.size(); }

        // ====================================================================
        // Filtering
        // ====================================================================

        /**
         * @brief Set the minimum log level to display
         * @param level Minimum level (lower levels will be hidden)
         */
        void SetMinLogLevel(LogLevel level) { m_MinLogLevel = level; }

        /**
         * @brief Get the minimum log level
         */
        LogLevel GetMinLogLevel() const { return m_MinLogLevel; }

        // ====================================================================
        // Configuration
        // ====================================================================

        /**
         * @brief Set auto-scroll behavior
         * @param enabled true to auto-scroll to new messages
         */
        void SetAutoScroll(bool enabled) { m_AutoScroll = enabled; }

        /**
         * @brief Get auto-scroll state
         */
        bool IsAutoScrollEnabled() const { return m_AutoScroll; }

        /**
         * @brief Set maximum number of log entries (oldest are removed)
         * @param maxEntries Maximum entries (0 = unlimited)
         */
        void SetMaxEntries(size_t maxEntries) { m_MaxEntries = maxEntries; }

        // ====================================================================
        // Visibility
        // ====================================================================

        /**
         * @brief Set panel visibility
         */
        void SetVisible(bool visible) { m_Visible = visible; }

        /**
         * @brief Get panel visibility
         */
        bool IsVisible() const { return m_Visible; }

        /**
         * @brief Toggle visibility
         */
        void ToggleVisible() { m_Visible = !m_Visible; }

    private:
        ConsolePanel();
        ~ConsolePanel() = default;

        /**
         * @brief Log entry structure
         */
        struct LogEntry
        {
            std::string Message;
            LogLevel Level;
            std::chrono::system_clock::time_point Timestamp;
            int Count; // For repeated messages
        };

        /**
         * @brief Get color for log level
         */
        static ImVec4 GetLogLevelColor(LogLevel level);

        /**
         * @brief Get string representation of log level
         */
        static const char* GetLogLevelString(LogLevel level);

        /**
         * @brief Format timestamp for display
         */
        static std::string FormatTimestamp(const std::chrono::system_clock::time_point& time);

        /**
         * @brief Draw filter controls
         */
        void DrawFilters();

        /**
         * @brief Draw log entries
         */
        void DrawLogEntries();

        /**
         * @brief Draw command input
         */
        void DrawCommandInput();

        /**
         * @brief Execute a console command
         */
        void ExecuteCommand(const std::string& command);

    private:
        std::vector<LogEntry> m_Logs;
        std::mutex m_LogMutex; // For thread-safe logging

        bool m_Visible = true;
        bool m_AutoScroll = true;
        bool m_ScrollToBottom = false;

        // Filtering
        LogLevel m_MinLogLevel = LogLevel::Trace;
        char m_FilterBuffer[128] = "";
        std::string m_FilterText;

        // Level filters
        bool m_ShowTrace = true;
        bool m_ShowDebug = true;
        bool m_ShowInfo = true;
        bool m_ShowWarning = true;
        bool m_ShowError = true;
        bool m_ShowCritical = true;

        // Command input
        char m_InputBuffer[256] = "";
        std::vector<std::string> m_CommandHistory;
        int m_HistoryIndex = -1;

        // Limits
        size_t m_MaxEntries = 1000;
    };

    // Convenience macros for logging
    #define SM_LOG_TRACE(msg)    SM::ConsolePanel::Get().Log(msg, SM::LogLevel::Trace)
    #define SM_LOG_DEBUG(msg)    SM::ConsolePanel::Get().Log(msg, SM::LogLevel::Debug)
    #define SM_LOG_INFO(msg)     SM::ConsolePanel::Get().Log(msg, SM::LogLevel::Info)
    #define SM_LOG_WARNING(msg)  SM::ConsolePanel::Get().Log(msg, SM::LogLevel::Warning)
    #define SM_LOG_ERROR(msg)    SM::ConsolePanel::Get().Log(msg, SM::LogLevel::Error)
    #define SM_LOG_CRITICAL(msg) SM::ConsolePanel::Get().Log(msg, SM::LogLevel::Critical)

} // namespace SM
