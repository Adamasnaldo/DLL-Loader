#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <filesystem>
#include <unordered_set>
#include <vector>

#include "DllLoader/Settings.hpp"


namespace DllLoader {
    namespace fs = std::filesystem;

    enum DirectoryIteratorType {
        RECURSIVE,
        NON_RECURSIVE
    };

    struct InjectorConfig {
        bool recurseSubdirectories = false;
        std::vector<fs::path> includeDirectories;
        std::unordered_set<std::wstring> excludeFileNames;
        std::unordered_set<fs::path> excludeFilePaths;
        std::vector<fs::path> externalModules;

        const bool from_toml(const ConfigReader& reader);
    };

    class Injector {
        private:
            HMODULE kernel32 = NULL;
            LPVOID loadLibraryAddr = NULL;

            InjectorConfig config;

            template<typename T>
            inline void iterateDirectory(std::unordered_set<std::wstring>& dllPaths, const fs::path& path, const fs::directory_options& options) const;
            template<DirectoryIteratorType>
            inline void iterateDirectory(std::unordered_set<std::wstring>& dllPaths, const fs::path& path, const fs::directory_options& options) const;

            const std::unordered_set<std::wstring> calculateDlls( ) const;

        public:
            Injector( );

            BOOL inject(const HANDLE hProcess) const;
    };
}