#include "system.hpp"
#include "string_helper.hpp"

#include <TlHelp32.h>
#include <spdlog/spdlog.h>

namespace DllLoader::Util {
    const BOOL getModuleDirectory(fs::path& moduleDir) {
        WCHAR filename[MAX_PATH];

        if (GetModuleFileNameW(NULL, filename, MAX_PATH) == 0) {
            spdlog::error("Error getting loader's exe path.");

            return FALSE;
        }

        CHAR buffer[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, filename, -1, buffer, sizeof(buffer), NULL, NULL);
        spdlog::trace("Found loader's path at '{}', getting parent.", buffer);

        moduleDir = fs::path(filename).parent_path( );
        if (!checkPath<DIR>(moduleDir)) {
            // Idk if it gets here, will leave a message anyways
            spdlog::error("Path isn't a directory! Got '{}'", moduleDir.string( ));

            return FALSE;
        }

        spdlog::debug("Found loader's dir: '{}'", moduleDir.string( ));

        return TRUE;
    }

    const BOOL createProcessW(LPCWSTR lpApplicationName, STARTUPINFOW& startupInfo, PROCESS_INFORMATION& processInfo) {

        // Reset Info
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        ZeroMemory(&processInfo, sizeof(processInfo));

        if (!CreateProcessW(lpApplicationName,
                            NULL,            // No command line (use module name)
                            NULL,            // Process handle not inheritable
                            NULL,            // Thread handle not inheritable
                            FALSE,           // Set handle inheritance to FALSE
                            0,               // No creation flags
                            NULL,            // Use parent's environment block
                            NULL,            // Use parent's starting directory
                            &startupInfo,    // Pointer to STARTUPINFO structure
                            &processInfo)    // Pointer to PROCESS_INFORMATION structure
            ) {
            DWORD err = GetLastError( );
            spdlog::error("CreateProcess failed ({}).", err);

            return FALSE;
        }

        return TRUE;
    }

    const BOOL launchProcess(const fs::path& programPath, PROCESS_INFORMATION& processInfo) {
        STARTUPINFO startupInfo;

        const std::wstring lpApplicationName = programPath.wstring( );

        {
            DWORD lpBinaryType;
            if (GetBinaryTypeW(lpApplicationName.c_str( ), &lpBinaryType) == 0) {
                spdlog::error("Given executable path '{}' isn't executable! ({})", programPath.string( ), GetLastError( ));

                return FALSE;
            }

            spdlog::trace("Executable '{}' is of type {}", programPath.string( ), lpBinaryType);
        }

        if (!createProcessW(lpApplicationName.c_str( ), startupInfo, processInfo)) {
            spdlog::error("Couldn't create process for '{}', exiting...", programPath.string( ));

            return FALSE;
        }

        return TRUE;
    }


    static const BOOL compareProcessFullPath(const fs::path& processPath, const DWORD th32ProcessID) {
        MODULEENTRY32 moduleInfo{ };
        moduleInfo.dwSize = sizeof(moduleInfo);
        const HANDLE modulesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, th32ProcessID);

        if (Module32First(modulesSnapshot, &moduleInfo) != TRUE) {
            return FALSE;
        }

        const std::wstring nativePath = processPath.lexically_normal( ).native( );

        spdlog::trace("Full path is '{}', comparing to '{}'", wideStringToString(moduleInfo.szExePath), wideStringToString(nativePath.c_str( )));

        return _wcsicmp(nativePath.c_str( ), moduleInfo.szExePath) == 0;
    }

    const DWORD getProcessID64(const fs::path& processPath) {
        PROCESSENTRY32 processInfo{ };
        processInfo.dwSize = sizeof(processInfo);

        const HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(processesSnapshot, &processInfo) != TRUE) {
            spdlog::error("Error getting first module lol");

            return 0;
        }

        const fs::path& normalProcessPath = processPath.lexically_normal( );

        do {
            // Check file name
            if (_wcsicmp(normalProcessPath.filename( ).wstring( ).c_str( ), processInfo.szExeFile) != 0) {
                continue;
            }

            spdlog::debug("Found process '{}' by name! Checking full path...", normalProcessPath.filename( ).string( ));

            // Check full path
            if (compareProcessFullPath(normalProcessPath, processInfo.th32ProcessID) == FALSE) {
                spdlog::debug("Nope!");

                continue;
            }

            spdlog::debug("Found process {}, checking if it's what we want.", normalProcessPath.string( ));

            const HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processInfo.th32ProcessID);

            BOOL iswow64 = FALSE;
            //https://stackoverflow.com/questions/14184137/how-can-i-determine-whether-a-process-is-32-or-64-bit
            //If IsWow64Process() reports true, the process is 32-bit running on a 64-bit OS
            //So we want it to return false (32 bit on 32 bit os, or 64 bit on 64 bit OS, since we build x64 the first condition will never satisfy since they can't run this exe)

            if (hProcess == NULL) {
                spdlog::warn("Error on OpenProcess to check bitness");

                continue;
            }

            if (IsWow64Process(hProcess, &iswow64) == TRUE) {
                if (iswow64 == FALSE) {
                    CloseHandle(hProcess);
                    CloseHandle(processesSnapshot);

                    spdlog::debug("Found our process! id is {}", processInfo.th32ProcessID);

                    return processInfo.th32ProcessID;
                }
            } else {
                spdlog::error("IsWow64Process failed with error {}", GetLastError( ));
            }

            CloseHandle(hProcess);

        } while (Process32Next(processesSnapshot, &processInfo) == TRUE);

        CloseHandle(processesSnapshot);

        spdlog::warn("Didn't find any process that matches '{}'!", processPath.string( ));

        return 0;
    }
}