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

#ifndef FILE_UTIL_HPP
#define FILE_UTIL_HPP

#include <optional>
#include <vector>
#include <filesystem>
#include "string_util.h"
namespace fs = std::filesystem;

std::string normalize_path(const std::string &input);

std::optional<std::string> load_file_local(const std::string &source_path);
std::optional<std::string> load_file_zipped_or_local(const std::string &source_path);
bool extract_file_zipped_or_local(const std::string &source_path, const std::string &destination_path);

std::vector<std::string> list_files(const char* folderPath, std::string &extension);

std::vector<std::string> list_directories(const char* folderPath);

std::string create_temp_file();

inline std::string obs_module_file_std(const char *file) {
    char *file_ = obs_module_file(file);
    std::string out = file_;
    bfree(file_);
    return out;
}
inline std::string obs_module_file_std(std::string file) {
    return obs_module_file_std(file.c_str());
}

#endif // FILE_UTIL_HPP
