/******************************************************************************
    Copyright (C) 2023 by xurei <xureilab@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "open_in_explorer.h"
#include "file_util.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <spawn.h>
#include <sys/types.h>
extern char **environ;
#endif

bool open_in_explorer(const char *path)
{
    if (!path || !*path) {
        return false;
    }
    auto npath = normalize_path(path);

    #ifdef _WIN32
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

        return reinterpret_cast<intptr_t>(result) > 32;

    #elif defined(__APPLE__)
        // `open` handles paths containing spaces correctly when passed
        // as a separate argv element, but system() doesn't.
        pid_t pid;
        char *const argv[] = {
            const_cast<char *>("open"),
            const_cast<char *>(npath.c_str()),
            nullptr,
        };

        return posix_spawnp(&pid, "open", nullptr, nullptr, argv, environ) == 0;

    #else
        pid_t pid;
        char *const argv[] = {
            const_cast<char *>("xdg-open"),
            const_cast<char *>(npath.c_str()),
            nullptr,
        };

        return posix_spawnp(&pid, "xdg-open", nullptr, nullptr, argv, environ) == 0;
    #endif
}
