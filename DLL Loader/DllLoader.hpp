#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <filesystem>
#include <vector>

#include "DllLoader/Settings.hpp"
#include "Injector.hpp"

namespace DllLoader {
    namespace fs = std::filesystem;

    struct DllLoaderConfig {
        fs::path executablePath;

        const bool from_toml(const ConfigReader& reader);
    };

    class DllLoader {
    private:
        DllLoaderConfig config;

        const BOOL injectIntoProcess(const HANDLE hProcess) const;

        const BOOL launchAndInject(const fs::path& programPath) const;
        const BOOL injectIntoRunning(const DWORD processID) const;

    public:
        DllLoader();

        const bool initialize(const char* configFile);

        const BOOL run( ) const;
    };
}