#pragma once

#include <DllLoader/Settings.hpp>
#include <filesystem>
#include <spdlog/sinks/basic_file_sink.h>


namespace DllLoader::Log {
    namespace fs = std::filesystem;

    struct LoggerConfig {
        bool enabled = true;
        spdlog::level::level_enum logLevel = spdlog::level::debug;
        spdlog::level::level_enum flushOn = spdlog::level::info;

        const bool from_toml(const ConfigReader& reader);
    };

    struct LoggerConsoleConfig {
        bool enabled = false;

        const bool from_toml(const ConfigReader& reader);
    };

    struct LoggerDiskConfig {
        bool enabled = true;
        bool appendLog = false;
        fs::path logFile;

        const bool from_toml(const ConfigReader& reader);
    };

    const spdlog::sink_ptr createFileSink(const fs::path& filePath, const bool append);
    const spdlog::sink_ptr createConsoleSink( );

    const std::shared_ptr<spdlog::logger> setupLogger( );
    static void setupConsoleLogger(std::shared_ptr<spdlog::logger> logger);
    static void setupDiskLogger(std::shared_ptr<spdlog::logger> logger);
}