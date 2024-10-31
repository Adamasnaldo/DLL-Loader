#pragma once

#include <filesystem>
#include <Windows.h>

namespace DllLoader::Util {
    namespace fs = std::filesystem;

    enum PathType {
        FILE,
        DIR,
        ALL
    };

    template<PathType type>
    inline const bool checkPath(const fs::path& path) {
        switch (type) {
            case PathType::FILE: return fs::is_regular_file(path) || fs::is_symlink(path);
            case PathType::DIR:  return fs::is_directory(path);
            default:   return fs::exists(path);
        }
    }

    const BOOL getModuleDirectory(fs::path& moduleDir);

    const BOOL createProcessW(LPCWSTR lpApplicationName, STARTUPINFOW& startupInfo, PROCESS_INFORMATION& processInfo);
    const BOOL launchProcess(const fs::path& programPath, PROCESS_INFORMATION& processInfo);

    const DWORD getProcessID64(const fs::path& processPath);
}