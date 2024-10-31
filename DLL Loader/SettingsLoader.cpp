#include "SettingsLoader.hpp"
#include "Util/system.hpp"


namespace DllLoader {
    SettingsLoader::SettingsLoader() {
    }

    const Settings* const SettingsLoader::getSettings( ) const {
        return this->defaultSettings;
    }
    const Settings& SettingsLoader::getSettings(const std::filesystem::path& key) const {
        return this->getSettings(key.string( ));
    }
    const Settings& SettingsLoader::getSettings(const std::string& key) const {
        return this->settingsMap.at(key);
    }

    const bool SettingsLoader::setDefault(const std::filesystem::path& configFile) {
        return this->setDefault(configFile.string( ));
    }
    const bool SettingsLoader::setDefault(const std::string& configFile) {
        if (!this->settingsMap.contains(configFile)) {
            return false;
        }

        this->defaultSettings = &this->settingsMap.at(configFile);

        return true;
    }
    const bool SettingsLoader::setDefault(const Settings& newSettings) {
        for (auto& settingsPair : this->settingsMap) {
            const Settings& settings = this->settingsMap.at(settingsPair.first);

            if (&settings == &newSettings) {
                this->defaultSettings = &settings;
            
                return true;
            }
        }

        return false;
    }

    const bool SettingsLoader::load(const std::filesystem::path& configFile) {
        spdlog::info("Loading config file '{}'.", configFile.string( ));

        // Parse toml
        try {
            auto config = toml::parse_file(configFile.string( ));

            // First arg is key, everything after are args to Settings constructor
            this->settingsMap.try_emplace(configFile.string(), this->loaderDir, config);

            spdlog::info("Successfully loaded settings!");
        } catch (const toml::parse_error& e) {
            spdlog::error("Error loading config file '{}': {}", configFile.string( ), e.description( ));

            return false;
        }

        if (!this->setDefault(configFile)) {
            spdlog::debug("Couldn't set default setting as config {}?", configFile.string( ));
        }

        return true;
    }


    const bool SettingsLoader::initialize(const char* configFile) {
        if (!Util::getModuleDirectory(this->loaderDir)) {
            return false;
        }

        // Check if given config exists
        fs::path path(configFile);
        if (!Util::checkPath<Util::FILE>(path)) {
            // Maybe it's a path relative to the executable, try that
            path = this->loaderDir / configFile;

            if (!Util::checkPath<Util::FILE>(path)) {
                spdlog::error("Given config file path is neither a file nor a symbolic link! '{}'", path.string( ));

                return false;
            }
        }

        if (!this->load(path)) {
            return false;
        }

        return true;
    }
}