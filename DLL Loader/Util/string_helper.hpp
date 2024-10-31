#pragma once

#include <spdlog/spdlog.h>

namespace DllLoader::Util {
    // VS for some reason doesn't have this lol
    inline std::string operator+(const std::string& lhs, const fs::path& rhs) {
        return lhs + rhs.string( );
    }
    inline std::string& operator+=(std::string& lhs, const fs::path& rhs) {
        lhs = lhs + rhs;
    
        return lhs;
    }

    template<typename T>
    const std::string join(const std::string& sep, const T& elements) {
        std::string res = "";
        auto it = elements.begin( );

        // Add the first element without a separator
        if (it != elements.end( )) {
            res += *it;
            ++it;
        }

        while (it != elements.end( )) {
            res += sep;
            res += *it;
            ++it;
        }

        return res;
    }
    template<typename T>
    const std::string join(const char sep, const T& elements) {
        return join(std::string(1, sep), elements);
    }


    inline const std::string wideStringToString(std::wstring wstr) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str( ), -1, NULL, 0, NULL, NULL);
        LPSTR str = new CHAR[size_needed];
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str( ), -1, str, size_needed, NULL, NULL);

        return std::string(str);
    }
}