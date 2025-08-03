#pragma once

#include <string>
#include <Windows.h>

namespace DllLoader::Util {
    const std::string GetLastErrorAsString( ) {

        // Get last error id
        DWORD errorMessageID = ::GetLastError( );
        if (errorMessageID == 0) {
            return std::string();
        }

        LPSTR messageBuffer = nullptr;

        // Load the message
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                     NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_PORTUGUESE), (LPSTR) &messageBuffer, 0, NULL);

        if (size == 0) {
            return std::string( );
        }

        // Convert it to std::string
        std::string message(messageBuffer, size);

        LocalFree(messageBuffer);

        return message;
    }
}