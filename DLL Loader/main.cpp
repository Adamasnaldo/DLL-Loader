// DLL Loader.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include "DllLoader.hpp"

#include <spdlog/spdlog.h>

constexpr auto DEFAULT_CONFIG_FILE = "config_default.toml";


int main(int argc, char* argv[]) {

    // Define temporary logger
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::info);

    if (argc > 2) {
        spdlog::error("Only one argument!");
        spdlog::error("Usage: \"{}\" <config_file>", argv[0]);

        return 1;
    }

    const char* const configFile = argc == 2 ? argv[1] : DEFAULT_CONFIG_FILE;

    spdlog::info("Starting with config file '{}'...", configFile);

    DllLoader::DllLoader loader;

    if (!loader.initialize(configFile)) {
        spdlog::error("Couldn't initialize loader, exiting...");

        return 1;
    }

    if (loader.run( ) != TRUE) {
        spdlog::error("Error while running loader, exiting...");

        return 1;
    }

    return 0;
}