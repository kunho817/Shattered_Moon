/**
 * @file main.cpp
 * @brief Shattered Moon - Application entry point
 *
 * Win32 application entry point that initializes and runs the engine.
 */

#include "core/Engine.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

/**
 * @brief Win32 application entry point
 *
 * @param hInstance Application instance handle
 * @param hPrevInstance Not used (legacy)
 * @param lpCmdLine Command line arguments
 * @param nCmdShow Window show state
 * @return Application exit code
 */
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // Suppress unused parameter warnings
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Enable memory leak detection in debug builds
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Configure the engine
    SM::EngineConfig config;
    config.windowTitle = L"Shattered Moon";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.vsync = true;
    config.fullscreen = false;

    // Get engine instance and initialize
    SM::Engine& engine = SM::Engine::Get();

    if (!engine.Initialize(config))
    {
        MessageBoxW(
            nullptr,
            L"Failed to initialize engine!",
            L"Shattered Moon - Error",
            MB_OK | MB_ICONERROR
        );
        return -1;
    }

    // Run the main game loop
    int exitCode = engine.Run();

    return exitCode;
}

/**
 * @brief Console application entry point (for debug builds)
 *
 * This allows running the application from a command prompt with console output.
 */
#if defined(_DEBUG)
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    return WinMain(GetModuleHandle(nullptr), nullptr, GetCommandLineA(), SW_SHOW);
}
#endif
