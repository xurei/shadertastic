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

#ifndef SHADERTASTIC_OBS_RETROCOMPAT_HPP
#define SHADERTASTIC_OBS_RETROCOMPAT_HPP

#include <obs-module.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <obs.hpp>
#include <string>

/*
 * This file adds new API calls from recent OBS updates, allowing to stay
 * retro-compatible when the functionnality is optional.
 */

template<typename fn_type> fn_type get_retrocompat_fn(const char *fn_name) {
    #ifdef _WIN32
    fn_type fn = reinterpret_cast<fn_type>(GetProcAddress(GetModuleHandleA("obs.dll"), fn_name));
    if (fn == nullptr) {
        fn = reinterpret_cast<fn_type>(GetProcAddress(GetModuleHandleA("obs-frontend-api.dll"), fn_name));
    }
    #else
    fn_type fn = reinterpret_cast<fn_type>(
        dlsym(RTLD_DEFAULT, fn_name));
    #endif

    return fn;
}

obs_source_t *(*obs_retrocompat_frontend_add_transition)(const char *, const char *, obs_data_t *){nullptr};

void obs_retrocompat_init() {
    obs_retrocompat_frontend_add_transition = get_retrocompat_fn<obs_source_t *(*)(const char *, const char *, obs_data_t *)>("obs_frontend_add_transition");
}

#endif //SHADERTASTIC_OBS_RETROCOMPAT_HPP
