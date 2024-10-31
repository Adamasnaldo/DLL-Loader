#include "DllLoader.hpp"

#include "SettingsLoader.hpp"

#include "Log/Logger.hpp"
#include "Util/system.hpp"
#include "Util/toml.hpp"

#include <TlHelp32.h>
#include <spdlog/spdlog.h>


namespace DllLoader {
    DllLoader::DllLoader( ) { };

    const bool DllLoader::initialize(const char* configFile) {
        spdlog::info("Initializing DllLoader with config file '{}'...", configFile);
        if (!SettingsLoader::getInstance( ).initialize(configFile)) {
            spdlog::error("Error initiaizing settings!");

            return false;
        }

        if (Log::setupLogger( ) == nullptr) {
            spdlog::error("Error setting up logger?");

            return false;
        }

        this->config = Util::getConfig<DllLoaderConfig>({ "DllLoader" });

        return true;
    };


    const BOOL DllLoader::injectIntoProcess(const HANDLE hProcess) const {
        const Injector injector;
        if (injector.inject(hProcess) != TRUE) {
            spdlog::error("Couldn't inject dlls into process!");
        }

        spdlog::info("Successfully injected into module!");
        spdlog::info("Waiting for process to exit...");

        // Wait for process to finish
        WaitForSingleObject(hProcess, INFINITE);

        spdlog::info("Process finished!");

        return TRUE;
    }


    const BOOL DllLoader::launchAndInject(const fs::path& programPath) const {
        PROCESS_INFORMATION processInfo;
        if (Util::launchProcess(programPath, processInfo) != TRUE) {
            spdlog::error("Couldn't launch process {}!", programPath.string( ));

            return FALSE;
        }

        spdlog::info("Launched new process '{}'!", programPath.string( ));

        // Inject
        if (this->injectIntoProcess(processInfo.hProcess) != TRUE) {
            spdlog::error("Couldn't inject into spawned process '{}' with id {}.", programPath.string( ), processInfo.dwProcessId);

            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);

            return FALSE;
        }

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

        return TRUE;
    }

    const BOOL DllLoader::injectIntoRunning(const DWORD processID) const {
        const HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, false, processID);

        if (this->injectIntoProcess(handle) != TRUE) {
            spdlog::error("Couldn't inject into already running process with id {}", processID);

            return FALSE;
        }

        CloseHandle(handle);

        return TRUE;
    }


    /*
     *
     * MAIN FUNCTION, THAT RUNS THE PROGRAM AND INJECTS THE DLLS
     *
     */
    const BOOL DllLoader::run( ) const {
        const fs::path& executablePath = this->config.executablePath;
        if (!Util::checkPath<Util::FILE>(executablePath)) {
            spdlog::error("Path from config '{}' isn't a file! Exiting...", executablePath.string( ));

            return FALSE;
        }

        DWORD processID = Util::getProcessID64(executablePath);
        spdlog::debug("Got process id: {}", processID);

        if (processID == 0) {
            // Not running
            if (this->launchAndInject(executablePath) != TRUE) {
                return FALSE;
            }
        } else if (this->injectIntoRunning(processID) != TRUE) {
            return FALSE;
        }

        return TRUE;
    }



    /*
     * CONFIG
     */

    const bool DllLoaderConfig::from_toml(const ConfigReader& reader) {
        spdlog::debug("Loading config for DllLoaderConfig!");

        if (!Util::loadOptionInto<Util::FILE, true>(this->executablePath, reader, { "ExecutablePath" })) {
            spdlog::error("Executable Path isn't a file! Value: '{}'", this->executablePath.string( ));

            return false;
        }

        spdlog::info("Got ExecutablePath from config! Value: '{}'", this->executablePath.string( ));

        return true;
    }
}