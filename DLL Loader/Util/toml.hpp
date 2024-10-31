#pragma once

#include "../SettingsLoader.hpp"

#include "DllLoader/Settings.hpp"
#include "string_helper.hpp"
#include "system.hpp"

#include <spdlog/spdlog.h>

#include <unordered_map>


namespace DllLoader::Util {
    namespace level = spdlog::level;

    static const std::unordered_map<std::string, spdlog::level::level_enum> spdlogLevelMap = {
        { "Off", level::off },
        { "Critical", level::critical },
        { "Error", level::err },
        { "Warn", level::warn },
        { "Info", level::info },
        { "Debug", level::debug },
        { "Trace", level::trace },
        { "All", level::trace }
    };

    template<typename T>
    const T getConfig(const std::initializer_list<std::string>& list) {
        return SettingsLoader::getInstance( ).getSettings( )->getConfigReader( ).read_config_object<T>(list);
    }


    // Single option
    template<typename T>
    inline const bool loadOptionInto(T& var, const ConfigReader& reader, const std::initializer_list<std::string>& path, const T& defaultValue) {
        var = reader.read_config_option<T>(path).value_or(defaultValue);
        spdlog::trace("Read option '{}' from config, got '{}'", join('.', path), var);

        return true;
    }
    template<>
    inline const bool loadOptionInto(level::level_enum& var, const ConfigReader& reader, const std::initializer_list<std::string>& path, const level::level_enum& defaultValue) {
        const std::string logLevelStr = reader.read_config_option<std::string>(path).value_or("");

        if (!spdlogLevelMap.contains(logLevelStr)) {
            return false;
        }
    
        var = spdlogLevelMap.at(logLevelStr);
    
        return true;
    }
    template<PathType T = ALL, bool ignoreInvalid = true>
    inline const bool loadOptionInto(fs::path& var, const ConfigReader& reader, const std::initializer_list<std::string>& path, const fs::path& defaultValue = "") {
        const fs::path temp = reader.read_config_option<ignoreInvalid>(path).value_or(defaultValue);
        spdlog::trace("Read path '{}' from config, got '{}'", join('.', path), temp.string( ));

        if (ignoreInvalid && !checkPath<T>(temp)) {
            return false;
        }

        var = temp;

        return true;
    }



    // Array of options
#define LOAD_OPTIONS_INTO( T ) \
    var = reader.read_config_options<T>(path); \
    const std::string tab(4, ' '); \
    spdlog::trace("Read option '{}' from config, got ({}) [", join('.', path), var.size( )); \
    for (const fs::path& path : var) { \
        spdlog::trace("{}\"{}\",", tab, path.string( )); \
    } \
    spdlog::trace("]"); \
    return true; \

    template<typename T>
    inline const bool loadOptionsInto(std::vector<T>& var, const ConfigReader& reader, const std::initializer_list<std::string>& path) {
        LOAD_OPTIONS_INTO(T);
    }
    template<bool ignoreInvalid = true>
    inline const bool loadOptionsInto(std::vector<fs::path>& var, const ConfigReader& reader, const std::initializer_list<std::string>& path) {
        LOAD_OPTIONS_INTO(ignoreInvalid);
    }
}