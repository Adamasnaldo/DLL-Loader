#include "Logger.hpp"

#include "../SettingsLoader.hpp"
#include "../Util/string_helper.hpp"
#include "../Util/toml.hpp"

#include <iostream>

#define LOAD_BOOL( VAR, READER, PATH, DEFAULT ) { \
    const std::string joinedPath = Util::join('.', std::vector<std::string>(PATH)); \
    if (!Util::loadOptionInto<bool>(VAR, READER, PATH, DEFAULT)) { \
        spdlog::error("Couldn't load config value for {}???", joinedPath); \
        return false; \
    } \
    spdlog::trace("Got config value for '{}': '{}'", joinedPath, VAR); \
}
#define LOAD_LEVEL( VAR, READER, PATH, DEFAULT ) { \
    const std::string joinedPath = Util::join('.', std::vector<std::string>(PATH)); \
    if (!Util::loadOptionInto(VAR, READER, PATH, DEFAULT)) { \
        spdlog::error("Couldn't load config value for {}???", joinedPath); \
        return false; \
    } \
    spdlog::trace("Got config value for '{}': '{}'", joinedPath, static_cast<int>(VAR)); \
}
#define REDIRECT_STREAM( RES, STREAM, HANDLE, MODE, OLD ) \
    RES = freopen_s(&STREAM, HANDLE, MODE, OLD); \
    if (RES != 0) { \
        spdlog::error("Error redirecting {} to console! ({})", #OLD, RES); \
        return nullptr; \
    }

#define CREATE_HANDLE( NEW_HANDLE, FILE_NAME ) \
    const HANDLE NEW_HANDLE = CreateFileW(FILE_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); \
    if (NEW_HANDLE == INVALID_HANDLE_VALUE) { \
        spdlog::error("Error creating handle for wide {}. Error: ({})", #FILE_NAME, GetLastError( )); \
        return nullptr; \
    }

#define SET_HANDLE( OLD_HANDLE, NEW_HANDLE ) \
    if (SetStdHandle(OLD_HANDLE, NEW_HANDLE) == 0) { \
        spdlog::error("Error replacing handle {} with {}. Error: ({})", #OLD_HANDLE, #NEW_HANDLE, GetLastError( )); \
        return nullptr; \
    }


namespace DllLoader::Log {
    constexpr auto LOGGER_NAME = "DllLoader";

    const spdlog::sink_ptr createFileSink(const fs::path& filePath, const bool append) {
        return std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath.string(), !append);
    }

    const spdlog::sink_ptr createConsoleSink() {
        // Free previous console
        if (FreeConsole( ) == 0) {
            DWORD err = GetLastError( );
            std::cerr << "OLA" << err << std::endl;
            spdlog::error("Error freeing console! ({})", err);

            return nullptr;
        }
        // Alloc new console
        if (AllocConsole( ) == 0) {
            DWORD err = GetLastError( );
            spdlog::error("Error allocating console! ({})", err);

            return nullptr;
        }

        // Redirect stdin/stdout to console
        FILE* stream;
        errno_t res;

        REDIRECT_STREAM(res, stream, "CONOUT$", "w", stdout);
        std::cout.clear( );
        std::clog.clear( );

        REDIRECT_STREAM(res, stream, "CONOUT$", "w", stderr);
        std::cerr.clear( );

        //REDIRECT_STREAM(res, stream, "CONIN$", "r", stdin);
        //std::cin.clear( );

        // Same but for wide
        CREATE_HANDLE(hConOut, L"CONOUT$");
        //CREATE_HANDLE(hConIn, L"CONIN$");
        SET_HANDLE(STD_OUTPUT_HANDLE, hConOut);
        std::wcout.clear( );
        std::wclog.clear( );
        SET_HANDLE(STD_ERROR_HANDLE, hConOut);
        std::wcerr.clear( );
        //SET_HANDLE(STD_INPUT_HANDLE, hConIn);
        //std::wcin.clear( );

        return std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>( );
    }


    const std::shared_ptr<spdlog::logger> setupLogger( ) {
        std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(LOGGER_NAME);

        // If config is enabled it will change this level
        logger->set_level(spdlog::level::off);

        LoggerConfig config = Util::getConfig<LoggerConfig>({ "Logging" });

        if (config.enabled) {
            logger->set_level(config.logLevel);
            logger->flush_on(config.flushOn);
            setupConsoleLogger(logger);
            setupDiskLogger(logger);
        }

        spdlog::set_default_logger(logger);

        return logger;
    }

    static void setupConsoleLogger(std::shared_ptr<spdlog::logger> logger) {
        LoggerConsoleConfig config = Util::getConfig<LoggerConsoleConfig>({ "Logging", "Console" });

        if (!config.enabled) {
            return;
        }

        logger->sinks( ).push_back(createConsoleSink( ));
    }
    static void setupDiskLogger(std::shared_ptr<spdlog::logger> logger) {
        LoggerDiskConfig config = Util::getConfig<LoggerDiskConfig>({ "Logging", "Disk" });

        if (!config.enabled) {
            return;
        }

        logger->sinks( ).push_back(createFileSink(config.logFile, config.appendLog));
    }


    /*
     *  CONFIG
     */

    const bool LoggerConfig::from_toml(const ConfigReader& reader) {
        spdlog::debug("Loading config for LoggerConfig!");

        LOAD_BOOL(this->enabled, reader, { "Enabled" }, true);

        LOAD_LEVEL(this->logLevel, reader, { "LogLevel" }, spdlog::level::debug);
        LOAD_LEVEL(this->flushOn, reader, { "FlushOn" }, spdlog::level::info);

        return true;
    }
    const bool LoggerConsoleConfig::from_toml(const ConfigReader& reader) {
        spdlog::debug("Loading config for LoggerConsoleConfig!");

        LOAD_BOOL(this->enabled, reader, { "Enabled" }, false);

        return true;
    }
    const bool LoggerDiskConfig::from_toml(const ConfigReader& reader) {
        spdlog::debug("Loading config for LoggerDiskConfig!");

        LOAD_BOOL(this->enabled, reader, { "Enabled" }, true);
        LOAD_BOOL(this->appendLog, reader, { "AppendLog" }, false);

        if (!Util::loadOptionInto<Util::FILE, false>(this->logFile, reader, { "LogFile" }, "dll_loader.log")) {
            spdlog::error("LogFile isn't a file!");

            return false;
        }

        spdlog::trace("Got log file from config: '{}'", this->logFile.string());

        return true;
    }
}