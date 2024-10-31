#pragma once

#include <DllLoader/Settings.hpp>
#include <filesystem>
#include <map>

#include <spdlog/spdlog.h>

namespace DllLoader {
    // Singleton pattern from:
    // https://stackoverflow.com/questions/1008019/how-do-you-implement-the-singleton-design-pattern
    class SettingsLoader {
    private:
        SettingsLoader( );

        std::map<std::string, Settings> settingsMap;
        const Settings* defaultSettings = nullptr;

        fs::path loaderDir;

    public:
        static SettingsLoader& getInstance( ) {
            static SettingsLoader instance;

            return instance;
        }

        // Returns default
        const Settings* const getSettings( ) const;
        const Settings& getSettings(const std::filesystem::path& key) const;
        const Settings& getSettings(const std::string& key) const;

        //SettingsLoader(const SettingsLoader&) = delete;
        void operator=(const SettingsLoader&) = delete;

        const bool setDefault(const std::filesystem::path& configFile);
        const bool setDefault(const std::string& configFile);
        const bool setDefault(const Settings& settings);

        const bool load(const std::filesystem::path& configFile);

        const bool initialize(const char* configFile);
    };
}