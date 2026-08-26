//
// Created by olivier on 26/08/26.
//

#include "open_in_explorer.h"

bool open_in_explorer(const char *path)
{
    if (!path || !*path) {
        return false;
    }

    #ifdef _WIN32
        wchar_t *wpath = os_utf8_to_wcs_ptr(path);
        if (!wpath) {
            return false;
        }

        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            wpath,
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );

        bfree(wpath);

        return reinterpret_cast<intptr_t>(result) > 32;

    #elif defined(__APPLE__)
        // `open` handles paths containing spaces correctly when passed
        // as a separate argv element, but system() doesn't.
        pid_t pid;
        char *const argv[] = {
            const_cast<char *>("open"),
            const_cast<char *>(path),
            nullptr,
        };

        return posix_spawnp(&pid, "open", nullptr, nullptr, argv, environ) == 0;

    #else
        pid_t pid;
        char *const argv[] = {
            const_cast<char *>("xdg-open"),
            const_cast<char *>(path),
            nullptr,
        };

        return posix_spawnp(&pid, "xdg-open", nullptr, nullptr, argv, environ) == 0;
    #endif
}