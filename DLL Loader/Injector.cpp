#include "Injector.hpp"
#include "Util/toml.hpp"

#include <spdlog/spdlog.h>

#define ERR_RETURN( MSG, RET ) spdlog::error("{} ({})", MSG, GetLastError()); return RET;
#define ERR_RETURN_FALSE( MSG ) ERR_RETURN(MSG, FALSE);
#define ERR_FREE_RETURN_FALSE( H_PROCESS, LP_ADDRESS, DW_SIZE, DW_FREE_TYPE, MSG ) VirtualFreeEx(H_PROCESS, LP_ADDRESS, DW_SIZE, DW_FREE_TYPE); ERR_RETURN_FALSE(MSG);

#define LOAD_VECTOR( VAR, READER, PATH ) { \
    const std::string joinedPath = Util::join('.', std::vector<std::string>(PATH)); \
    if (!Util::loadOptionsInto(VAR, READER, PATH)) { \
        spdlog::error("Couldn't load config value for {}!", joinedPath); \
        return false; \
    } \
    spdlog::trace("Loaded {} paths for value {}!", VAR.size( ), joinedPath); \
}


namespace DllLoader {

    constexpr auto DLL_EXT = ".dll";

    Injector::Injector( ) {
        // Get kernel32.dll module handle for the LoadLibraryW
        this->kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (this->kernel32 == NULL) {
            ERR_RETURN("Couldn't get kernel32 module handle!", );
        }

        // Get LoadLibraryW address
        this->loadLibraryAddr = (LPVOID) GetProcAddress(kernel32, "LoadLibraryW");
        if (this->loadLibraryAddr == NULL) {
            ERR_RETURN("Couldn't get address of LoadLibraryW, is kernel32.dll broken?", );
        }

        this->config = Util::getConfig<InjectorConfig>({ "DllLoader", "Injector" });
    }

    template<typename T>
    inline void Injector::iterateDirectory(std::unordered_set<std::wstring>& dllPaths, const fs::path& path, const fs::directory_options& options) const {
        T iterator(path, options);
    
        for (const fs::directory_entry& entry : iterator) {
            // We only want regular files
            if (!Util::checkPath<Util::FILE>(entry.path())) {
                spdlog::trace("File '{}' isn't a file.", entry.path( ).string( ));

                continue;
            }
            if (entry.path( ).extension( ) != DLL_EXT) {
                spdlog::trace("Got file '{}', which has the wrong extension. Expected '{}', but got '{}'", entry.path( ).string( ), DLL_EXT, entry.path( ).extension( ).string());

                continue;
            }
    
            // If it's in the exclude files, don't add it
            if (this->config.excludeFilePaths.contains(entry.path( ))) {
                spdlog::warn("Found file '{}', but it's in ExcludeFilePaths, so it will be ignored", entry.path( ).string( ));
    
                continue;
            }
            if (this->config.excludeFileNames.contains(entry.path( ).filename( ).wstring( ))) {
                spdlog::warn("Found file '{}', but it's in ExcludeFileNames, so it will be ignored", entry.path( ).string( ));

                continue;
            }
    
            spdlog::info("Added dll '{}'", entry.path( ).string( ));
            dllPaths.insert(entry.path( ).wstring( ));
        }
    }
    template<DirectoryIteratorType dirIterType>
    inline void Injector::iterateDirectory(std::unordered_set<std::wstring>& dllPaths, const fs::path& path, const fs::directory_options& options) const {
        switch (dirIterType) {
            case RECURSIVE:
                this->iterateDirectory<fs::recursive_directory_iterator>(dllPaths, path, options);
                break;
            case NON_RECURSIVE:
            default:
                this->iterateDirectory<fs::directory_iterator>(dllPaths, path, options);
                break;
        }
    }

    const std::unordered_set<std::wstring> Injector::calculateDlls( ) const {
        std::unordered_set<std::wstring> dllPaths;

        // Search directories for dlls
        for (const fs::path& path : this->config.includeDirectories) {
            const bool recurse = this->config.recurseSubdirectories;

            spdlog::debug("Searching path '{}' {}...", path.string( ), recurse ? "recursively" : "non-recursively");

            if (!Util::checkPath<Util::DIR>(path)) {
                spdlog::warn("Path '{}' is not a dir! Skipping...", path.string( ));

                continue;
            }

            const fs::directory_options& options = fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied;

            if (recurse) {
                this->iterateDirectory<RECURSIVE>(dllPaths, path, options);
            } else {
                this->iterateDirectory<NON_RECURSIVE>(dllPaths, path, options);
            }
        }

        // Add the single modules
        spdlog::debug("Adding the separate ExternalModules...");
        for (const fs::path& path : this->config.externalModules) {
            if (!Util::checkPath<Util::FILE>(path)) {
                spdlog::warn("File '{}' in ExternalModules isn't a file!", path.string( ));

                continue;
            }
            // If in the future I want to restrict to only .dll it's here :D
            //if (path.extension( ) != DLL_EXT) {
            //    spdlog::trace("File '{}' in ExternalModules has the wrong extension! Expected '{}', but got '{}'", path.string( ), DLL_EXT, path.extension( ).string( ));
            //
            //    continue;
            //}

            spdlog::info("Added dll '{}'", path.string( ));
            dllPaths.insert(path.wstring( ));
        }

        return dllPaths;
    }


    BOOL Injector::inject(const HANDLE hProcess) const {
        using len_t = unsigned long long;

        std::vector<len_t> offsets = std::vector<len_t>(1, 0);  // List of offsets. Starts with 0
        len_t totalLen = 0;
        std::wstring buffer = L"";  // Buffer used to write all dlls

        const auto dllPaths = this->calculateDlls( );

        spdlog::info("| The following dlls will be injected:");
        for (const std::wstring& path : dllPaths) {
            spdlog::info("\\_ {}", Util::wideStringToString(path));

            // Number of chars + 1 ('\0'), multiplied by bytes for each wchar
            const len_t len = (path.length( ) + 1) * sizeof(WCHAR);

            totalLen += len;
            offsets.push_back(totalLen);

            buffer += path;
            buffer.append(1, '\0'); // Add '\0' to finish this string
        }

        std::wstring test = L"";
        for (const wchar_t c : buffer) {
            if (c == L'\0') test += L"\\0";
            else test += c;
        }
        spdlog::debug("Buffer to inject: {}", Util::wideStringToString(test));

        // Remove last offset, as it is no one's offset
        offsets.pop_back( );

        // Allocate region to insert dll path
        const LPVOID baseAddress = VirtualAllocEx(hProcess, NULL, totalLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (baseAddress == NULL) {
            ERR_RETURN_FALSE("Couldn't allocate region of pages!");
        }

        // Write dll path in allocated region
        if (WriteProcessMemory(hProcess, baseAddress, buffer.data( ), totalLen, NULL) == 0) {
            ERR_FREE_RETURN_FALSE(hProcess, baseAddress, 0, MEM_RELEASE, "Couldn't write dll path to region :(");
        }

        for (len_t offset : offsets) {
            // Calculate base address of current dll path
            const LPVOID lpParameter = static_cast<LPVOID>(static_cast<LPBYTE>(baseAddress) + offset);

            // Run LoadLibraryW
            HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE) this->loadLibraryAddr, lpParameter, 0, NULL);
            if (hThread == NULL) {
                ERR_FREE_RETURN_FALSE(hProcess, baseAddress, 0, MEM_RELEASE, "Couldn't create remote thread to load library!");
            }

            // Wait
            if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED) {
                ERR_FREE_RETURN_FALSE(hProcess, baseAddress, 0, MEM_RELEASE, "Wait failed lol.");
            }

            // Print exit code
            DWORD res = 0;
            if (GetExitCodeThread(hThread, &res) == 0) {
                ERR_FREE_RETURN_FALSE(hProcess, baseAddress, 0, MEM_RELEASE, "Couldn't get thread exit code, what?");
            }
            spdlog::info("Exit code: {}", res);

            // We don't need the handle anymore
            if (CloseHandle(hThread) == 0) {
                ERR_RETURN_FALSE("Couldn't close handle for LoadLibraryW thread.");
            }
        }

        // Free the region
        if (VirtualFreeEx(hProcess, baseAddress, 0, MEM_RELEASE) == 0) {
            ERR_RETURN_FALSE("Couldn't free the regions rip.");
        }


        return TRUE;
    }



    const bool InjectorConfig::from_toml(const ConfigReader& reader) {
        spdlog::debug("Loading config for InjectorConfig!");

        if (!Util::loadOptionInto<bool>(this->recurseSubdirectories, reader, { "RecurseSubdirectories" }, false)) {
            spdlog::error("Couldn't load config value for RecurseSubdirectories???");

            return false;
        }

        spdlog::trace("Got config value for 'RecurseSubdirectories': '{}'", this->recurseSubdirectories);

        LOAD_VECTOR(this->includeDirectories, reader, { "IncludeDirectories" });
        LOAD_VECTOR(this->externalModules, reader, { "ExternalModules" });

        // We need to store a vector to then convert into a set
        std::vector<fs::path> excludeFilePaths;
        if (!Util::loadOptionsInto<false>(excludeFilePaths, reader, { "ExcludeFilePaths" })) {
            spdlog::error("Couldn't load config value for ExcludeFilePaths!");

            return false;
        }
        this->excludeFilePaths = std::unordered_set<fs::path>(std::make_move_iterator(excludeFilePaths.begin( )),
                                                          std::make_move_iterator(excludeFilePaths.end( )));

        spdlog::trace("Loaded {} paths for value ExcludeFilePaths!", this->excludeFilePaths.size( ));

        std::vector<std::wstring> excludeFiles;
        if (!Util::loadOptionsInto(excludeFiles, reader, { "ExcludeFileNames" })) {
            spdlog::error("Couldn't load config value for ExcludeFileNames!");

            return false;
        }
        this->excludeFileNames = std::unordered_set<std::wstring>(std::make_move_iterator(excludeFiles.begin( )),
                                                          std::make_move_iterator(excludeFiles.end( )));

        spdlog::trace("Loaded {} paths for value ExcludeFileNames!", this->excludeFileNames.size( ));

        return true;
    }
}