// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <iostream>
#include <filesystem>
#include <fstream>

constexpr auto LOG_FILE_NAME = "teste.log";

template<typename ...Args>
void logAndPrint(std::ofstream& out, Args&& ...args) {
    (out << ... << args);
    (std::cout << ... << args);
}
template<typename ...Args>
void logAndPrintLn(std::ofstream& out, Args&& ...args) {
    logAndPrint(out, std::forward<Args>(args)..., "\n");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    std::ofstream file;
    const LPWSTR moduleFileName = new WCHAR[FILENAME_MAX];
    std::filesystem::path path;

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            GetModuleFileNameW(hModule, moduleFileName, MAX_PATH);

            path = std::filesystem::path(moduleFileName).parent_path() / LOG_FILE_NAME;
            file = std::ofstream(path, std::ios::out);

            logAndPrintLn(file, "Log file path: ", path.string());

            logAndPrintLn(file, R"(  ____)");
            logAndPrintLn(file, R"( |  _ \ _   _ _ __ ___  _ __ ___  _   _ )");
            logAndPrintLn(file, R"( | | | | | | | '_ ` _ \| '_ ` _ \| | | |)");
            logAndPrintLn(file, R"( | |_| | |_| | | | | | | | | | | | |_| |)");
            logAndPrintLn(file, R"( |____/ \__,_|_| |_| |_|_| |_| |_|\__, |)");
            logAndPrintLn(file, R"(    _                             |___/ )");
            logAndPrintLn(file, R"(   | |)");
            logAndPrintLn(file, R"(   | |)");
            logAndPrintLn(file, R"(   | |___)");
            logAndPrintLn(file, R"(   |_____|)");
            logAndPrintLn(file, R"(      _)");
            logAndPrintLn(file, R"(     | |)");
            logAndPrintLn(file, R"(     | |)");
            logAndPrintLn(file, R"(     | |___)");
            logAndPrintLn(file, R"(     |_____|)");
            logAndPrintLn(file, R"(  ___        _           _           _ _ _ _)");
            logAndPrintLn(file, R"( |_ _|_ __  (_) ___  ___| |_ ___  __| | | | |)");
            logAndPrintLn(file, R"(  | || '_ \ | |/ _ \/ __| __/ _ \/ _` | | | |)");
            logAndPrintLn(file, R"(  | || | | || |  __/ (__| ||  __/ (_| |_|_|_|)");
            logAndPrintLn(file, R"( |___|_| |_|/ |\___|\___|\__\___|\__,_(_|_|_))");
            logAndPrintLn(file, R"(          |__ /)");

            file.close( );

            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
