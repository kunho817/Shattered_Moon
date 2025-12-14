#include "editor/ConsolePanel.h"

#include <imgui.h>
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace SM
{
    ConsolePanel& ConsolePanel::Get()
    {
        static ConsolePanel instance;
        return instance;
    }

    ConsolePanel::ConsolePanel()
    {
        // Reserve space for logs
        m_Logs.reserve(m_MaxEntries);

        // Add welcome message
        Log("Shattered Moon Console initialized", LogLevel::Info);
        Log("Type 'help' for available commands", LogLevel::Info);
    }

    void ConsolePanel::Draw()
    {
        if (!m_Visible)
        {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 520), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Console", &m_Visible, ImGuiWindowFlags_MenuBar))
        {
            // Menu bar
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Options"))
                {
                    ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

                    if (ImGui::BeginMenu("Log Levels"))
                    {
                        ImGui::Checkbox("Trace", &m_ShowTrace);
                        ImGui::Checkbox("Debug", &m_ShowDebug);
                        ImGui::Checkbox("Info", &m_ShowInfo);
                        ImGui::Checkbox("Warning", &m_ShowWarning);
                        ImGui::Checkbox("Error", &m_ShowError);
                        ImGui::Checkbox("Critical", &m_ShowCritical);
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Actions"))
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        Clear();
                    }
                    if (ImGui::MenuItem("Copy All"))
                    {
                        // Copy all logs to clipboard
                        std::string allLogs;
                        for (const auto& entry : m_Logs)
                        {
                            allLogs += "[" + FormatTimestamp(entry.Timestamp) + "] ";
                            allLogs += "[" + std::string(GetLogLevelString(entry.Level)) + "] ";
                            allLogs += entry.Message + "\n";
                        }
                        ImGui::SetClipboardText(allLogs.c_str());
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            // Filter input
            DrawFilters();

            ImGui::Separator();

            // Log entries
            DrawLogEntries();

            // Command input
            DrawCommandInput();
        }
        ImGui::End();
    }

    void ConsolePanel::Log(const std::string& message, LogLevel level)
    {
        std::lock_guard<std::mutex> lock(m_LogMutex);

        // Check for duplicate message (collapse repeated messages)
        if (!m_Logs.empty())
        {
            LogEntry& last = m_Logs.back();
            if (last.Message == message && last.Level == level)
            {
                last.Count++;
                last.Timestamp = std::chrono::system_clock::now();
                return;
            }
        }

        // Create new entry
        LogEntry entry;
        entry.Message = message;
        entry.Level = level;
        entry.Timestamp = std::chrono::system_clock::now();
        entry.Count = 1;

        m_Logs.push_back(entry);

        // Trim if over limit
        if (m_MaxEntries > 0 && m_Logs.size() > m_MaxEntries)
        {
            m_Logs.erase(m_Logs.begin(), m_Logs.begin() + (m_Logs.size() - m_MaxEntries));
        }

        // Auto-scroll on new message
        if (m_AutoScroll)
        {
            m_ScrollToBottom = true;
        }
    }

    void ConsolePanel::LogFormat(LogLevel level, const char* format, ...)
    {
        char buffer[1024];

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        Log(buffer, level);
    }

    void ConsolePanel::Clear()
    {
        std::lock_guard<std::mutex> lock(m_LogMutex);
        m_Logs.clear();
        Log("Console cleared", LogLevel::Info);
    }

    ImVec4 ConsolePanel::GetLogLevelColor(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
        case LogLevel::Debug:
            return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Light Blue
        case LogLevel::Info:
            return ImVec4(0.9f, 0.9f, 0.9f, 1.0f);  // White
        case LogLevel::Warning:
            return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);  // Yellow
        case LogLevel::Error:
            return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);  // Light Red
        case LogLevel::Critical:
            return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);  // Red
        default:
            return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    const char* ConsolePanel::GetLogLevelString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT";
        default:                 return "???";
        }
    }

    std::string ConsolePanel::FormatTimestamp(const std::chrono::system_clock::time_point& time)
    {
        auto timeT = std::chrono::system_clock::to_time_t(time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            time.time_since_epoch()) % 1000;

        std::tm tm;
        localtime_s(&tm, &timeT);

        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":"
            << std::setw(2) << tm.tm_sec << "."
            << std::setw(3) << ms.count();

        return oss.str();
    }

    void ConsolePanel::DrawFilters()
    {
        // Filter text input
        ImGui::PushItemWidth(200);
        if (ImGui::InputTextWithHint("##Filter", "Filter...", m_FilterBuffer, sizeof(m_FilterBuffer)))
        {
            m_FilterText = m_FilterBuffer;
            std::transform(m_FilterText.begin(), m_FilterText.end(), m_FilterText.begin(), ::tolower);
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();

        // Quick level filter buttons
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));

        if (ImGui::Button(m_ShowInfo ? "I" : "-"))
        {
            m_ShowInfo = !m_ShowInfo;
        }
        ImGui::SetItemTooltip("Toggle Info messages");
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LogLevel::Warning));
        if (ImGui::Button(m_ShowWarning ? "W" : "-"))
        {
            m_ShowWarning = !m_ShowWarning;
        }
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("Toggle Warning messages");
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(LogLevel::Error));
        if (ImGui::Button(m_ShowError ? "E" : "-"))
        {
            m_ShowError = !m_ShowError;
        }
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("Toggle Error messages");

        ImGui::PopStyleVar();

        ImGui::SameLine();
        ImGui::TextDisabled("| %zu entries", m_Logs.size());

        ImGui::SameLine(ImGui::GetWindowWidth() - 60);
        if (ImGui::Button("Clear"))
        {
            Clear();
        }
    }

    void ConsolePanel::DrawLogEntries()
    {
        // Reserve space for command input at the bottom
        float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

        ImGui::BeginChild("LogScrollRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

        // Use clipper for performance with many entries
        ImGuiListClipper clipper;

        // First, build filtered list
        std::vector<size_t> filteredIndices;
        for (size_t i = 0; i < m_Logs.size(); ++i)
        {
            const LogEntry& entry = m_Logs[i];

            // Level filter
            bool showLevel = false;
            switch (entry.Level)
            {
            case LogLevel::Trace:    showLevel = m_ShowTrace; break;
            case LogLevel::Debug:    showLevel = m_ShowDebug; break;
            case LogLevel::Info:     showLevel = m_ShowInfo; break;
            case LogLevel::Warning:  showLevel = m_ShowWarning; break;
            case LogLevel::Error:    showLevel = m_ShowError; break;
            case LogLevel::Critical: showLevel = m_ShowCritical; break;
            }

            if (!showLevel)
            {
                continue;
            }

            // Text filter
            if (!m_FilterText.empty())
            {
                std::string lowerMessage = entry.Message;
                std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);

                if (lowerMessage.find(m_FilterText) == std::string::npos)
                {
                    continue;
                }
            }

            filteredIndices.push_back(i);
        }

        // Draw filtered entries
        clipper.Begin(static_cast<int>(filteredIndices.size()));
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                const LogEntry& entry = m_Logs[filteredIndices[row]];

                ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(entry.Level));

                // Timestamp
                ImGui::TextDisabled("[%s]", FormatTimestamp(entry.Timestamp).c_str());
                ImGui::SameLine();

                // Level tag
                ImGui::Text("[%s]", GetLogLevelString(entry.Level));
                ImGui::SameLine();

                // Message
                if (entry.Count > 1)
                {
                    ImGui::Text("%s (x%d)", entry.Message.c_str(), entry.Count);
                }
                else
                {
                    ImGui::TextUnformatted(entry.Message.c_str());
                }

                ImGui::PopStyleColor();

                // Right-click context menu
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Copy"))
                    {
                        ImGui::SetClipboardText(entry.Message.c_str());
                    }
                    ImGui::EndPopup();
                }
            }
        }

        // Auto-scroll
        if (m_ScrollToBottom || (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        {
            ImGui::SetScrollHereY(1.0f);
        }
        m_ScrollToBottom = false;

        ImGui::EndChild();
    }

    void ConsolePanel::DrawCommandInput()
    {
        ImGui::Separator();

        // Command input field
        bool reclaimFocus = false;
        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue |
                                          ImGuiInputTextFlags_CallbackHistory;

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##CommandInput", m_InputBuffer, sizeof(m_InputBuffer), inputFlags,
            [](ImGuiInputTextCallbackData* data) -> int
            {
                ConsolePanel* console = static_cast<ConsolePanel*>(data->UserData);

                if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
                {
                    const int prevHistoryIndex = console->m_HistoryIndex;
                    if (data->EventKey == ImGuiKey_UpArrow)
                    {
                        if (console->m_HistoryIndex == -1)
                        {
                            console->m_HistoryIndex = static_cast<int>(console->m_CommandHistory.size()) - 1;
                        }
                        else if (console->m_HistoryIndex > 0)
                        {
                            console->m_HistoryIndex--;
                        }
                    }
                    else if (data->EventKey == ImGuiKey_DownArrow)
                    {
                        if (console->m_HistoryIndex != -1)
                        {
                            if (++console->m_HistoryIndex >= static_cast<int>(console->m_CommandHistory.size()))
                            {
                                console->m_HistoryIndex = -1;
                            }
                        }
                    }

                    if (prevHistoryIndex != console->m_HistoryIndex)
                    {
                        const char* historyStr = (console->m_HistoryIndex >= 0)
                            ? console->m_CommandHistory[console->m_HistoryIndex].c_str()
                            : "";
                        data->DeleteChars(0, data->BufTextLen);
                        data->InsertChars(0, historyStr);
                    }
                }

                return 0;
            }, this))
        {
            std::string command = m_InputBuffer;
            if (!command.empty())
            {
                ExecuteCommand(command);
                m_CommandHistory.push_back(command);
                m_HistoryIndex = -1;
            }
            m_InputBuffer[0] = '\0';
            reclaimFocus = true;
        }
        ImGui::PopItemWidth();

        // Focus management
        ImGui::SetItemDefaultFocus();
        if (reclaimFocus)
        {
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

    void ConsolePanel::ExecuteCommand(const std::string& command)
    {
        Log("> " + command, LogLevel::Trace);

        // Simple command parser
        std::string cmd = command;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (cmd == "help")
        {
            Log("Available commands:", LogLevel::Info);
            Log("  help    - Show this help message", LogLevel::Info);
            Log("  clear   - Clear the console", LogLevel::Info);
            Log("  version - Show engine version", LogLevel::Info);
            Log("  fps     - Show current FPS", LogLevel::Info);
        }
        else if (cmd == "clear")
        {
            Clear();
        }
        else if (cmd == "version")
        {
            Log("Shattered Moon Engine v0.1.0", LogLevel::Info);
        }
        else if (cmd == "fps")
        {
            // This would need to get FPS from engine
            Log("FPS display: Use Stats panel (F3)", LogLevel::Info);
        }
        else
        {
            Log("Unknown command: " + command, LogLevel::Warning);
            Log("Type 'help' for available commands", LogLevel::Info);
        }
    }

} // namespace SM
