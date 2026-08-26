//
// Created by olivier on 26/08/26.
//

#include "open_in_explorer.h"
#include <util/platform.h>

#include "file_util.h"

bool open_in_explorer(const char *path)
{
    if (!path || !*path) {
        return false;
    }

    #ifdef _WIN32
        auto npath = normalize_path(path);
        wchar_t wpath[512];
        size_t size = os_utf8_to_wcs(npath.c_str(), 0, wpath, sizeof(wpath));
        if (size <= 0) {
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
