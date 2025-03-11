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

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#endif

#include <filesystem>
#include <iostream>
#include <obs-module.h>
#include <util/platform.h>
#include <vector>
#include <zip.h>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "file_util.h"
#include "../logging_functions.hpp"
namespace fs = std::filesystem;

std::string normalize_path(const std::string &input) {
    std::string result(input);
    #if defined(_WIN32)
        size_t pos = 0;
        while ((pos = result.find('/', pos)) != std::string::npos) {
            result.replace(pos, 1, "\\");
            pos += 1; // Move past the replaced character
        }
    #endif
    return result;
}

char *load_file_zipped_or_local(const std::string &source_path) {
    std::string path = normalize_path(source_path);
    debug("load_file_zipped_or_local %s", path.c_str());

    if (os_file_exists(path.c_str())) {
        return os_quick_read_utf8_file(path.c_str());
    }
    else {
        fs::path fs_path(path);
        std::string zip_path = fs_path.parent_path().string();
        debug("FS PATH: %s", zip_path.c_str());
        if (!ends_with(zip_path, ".shadertastic")) {
            return nullptr;
        }

        int zip_err_code;
        std::string zip_entry = fs_path.filename().string();
        debug("zip_entry: %s", zip_entry.c_str());
        struct zip_stat file_stat{};
        zip_t *zip_archive = zip_open(zip_path.c_str(), ZIP_RDONLY, &zip_err_code);

        if (zip_archive == nullptr) {
            zip_error_t error;
            zip_error_init_with_code(&error, zip_err_code);
            log_error("Cannot open shadertastic archive '%s': %s\n", zip_path.c_str(), zip_error_strerror(&error));
            zip_error_fini(&error);
            return nullptr;
        }

        if (zip_stat(zip_archive, zip_entry.c_str(), 0, &file_stat) != 0) {
            log_error("Cannot open shadertastic file in archive '%s': unable to stat entry file %s : %s\n", zip_path.c_str(), zip_entry.c_str(), zip_error_strerror(zip_get_error(zip_archive)));
            zip_close(zip_archive);
            return nullptr;
        }

        zip_file_t *zipped_file = zip_fopen(zip_archive, zip_entry.c_str(), 0);

        if (zipped_file == nullptr) {
            log_error("Cannot open shadertastic file in archive '%s': unable to open entry file %s\n", zip_path.c_str(), zip_entry.c_str());
            zip_close(zip_archive);
            return nullptr;
        }

        char *file_content = static_cast<char *>(bzalloc(file_stat.size + 1));
        file_content[file_stat.size] = 0; // Normally not needed, but there for safety

        zip_fread(zipped_file, file_content, file_stat.size);
        zip_fclose(zipped_file);
        zip_close(zip_archive);
        return file_content;
    }
}

bool extract_file_zipped_or_local(const std::string &source_path, const std::string &destination_path) {
    std::string normalized_path = normalize_path(source_path);
    debug("extract_file_zipped_or_local %s -> %s", normalized_path.c_str(), destination_path.c_str());

    // Check if the file exists locally
    if (os_file_exists(normalized_path.c_str())) {
        debug("File exists locally. Copying to destination...");
        try {
            std::filesystem::copy_file(normalized_path, destination_path, std::filesystem::copy_options::overwrite_existing);
            return true;
        }
        catch (const std::filesystem::filesystem_error &e) {
            log_error("Error copying file: %s\n", e.what());
            return false;
        }
    }
    else {
        // If not local, check if it's in a zip archive
        std::filesystem::path fs_path(normalized_path);
        std::string zip_path = fs_path.parent_path().string();
        if (!ends_with(zip_path, ".shadertastic")) {
            log_error("Not a valid .shadertastic archive: %s\n", zip_path.c_str());
            return false;
        }

        int zip_err_code;
        std::string zip_entry = fs_path.filename().string();
        debug("Extracting %s from archive %s", zip_entry.c_str(), zip_path.c_str());

        zip_t *zip_archive = zip_open(zip_path.c_str(), ZIP_RDONLY, &zip_err_code);
        if (zip_archive == nullptr) {
            zip_error_t error;
            zip_error_init_with_code(&error, zip_err_code);
            log_error("Cannot open shadertastic archive '%s': %s\n", zip_path.c_str(), zip_error_strerror(&error));
            zip_error_fini(&error);
            return false;
        }

        struct zip_stat file_stat{};
        if (zip_stat(zip_archive, zip_entry.c_str(), 0, &file_stat) != 0) {
            log_error("Cannot stat file '%s' in archive '%s': %s\n", zip_entry.c_str(), zip_path.c_str(), zip_error_strerror(zip_get_error(zip_archive)));
            zip_close(zip_archive);
            return false;
        }

        zip_file_t *zipped_file = zip_fopen(zip_archive, zip_entry.c_str(), 0);
        if (zipped_file == nullptr) {
            log_error("Cannot open file '%s' in archive '%s': %s\n", zip_entry.c_str(), zip_path.c_str(), zip_error_strerror(zip_get_error(zip_archive)));
            zip_close(zip_archive);
            return false;
        }

        std::ofstream out_file(destination_path, std::ios::binary);
        if (!out_file.is_open()) {
            log_error("Cannot create destination file: %s\n", destination_path.c_str());
            zip_fclose(zipped_file);
            zip_close(zip_archive);
            return false;
        }

        // Read the file 1kB at a time
        const size_t buffer_size = 1024;
        char buffer[buffer_size];
        zip_int64_t bytes_read = 0;
        while ((bytes_read = zip_fread(zipped_file, buffer, buffer_size)) > 0) {
            out_file.write(buffer, bytes_read);
            if (!out_file.good()) {
                log_error("Error writing to destination file: %s\n", destination_path.c_str());
                zip_fclose(zipped_file);
                zip_close(zip_archive);
                return false;
            }
        }

        if (bytes_read < 0) {
            log_error("Error reading file '%s' from archive '%s': %s\n", zip_entry.c_str(), zip_path.c_str(), zip_error_strerror(zip_get_error(zip_archive)));
            zip_fclose(zipped_file);
            zip_close(zip_archive);
            return false;
        }

        zip_fclose(zipped_file);
        zip_close(zip_archive);
        debug("File successfully extracted to %s", destination_path.c_str());
        return true;
    }
}

std::vector<std::string> list_files(const char* folderPath, std::string &extension) {
    std::vector<std::string> files;

    try {
        // Check if the directory exists
        if (fs::exists(folderPath) && fs::is_directory(folderPath)) {
            for (const auto& entry: fs::directory_iterator(folderPath)) {
                std::string path = entry.path().string();
                //std::cout << path.substr(path.length() - extension.length()) << std::endl;

                if (ends_with(path, extension)) {
                    // entry is an fs::directory_entry object representing a file or directory in the directory
                    files.push_back(entry.path().string());
                    //std::cout << entry.path().string() << " FOUND" << std::endl;
                }
            }
        }
        else {
            std::cerr << "The specified path is not a valid directory." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }

    return files;
}

std::vector<std::string> list_directories(const char* folderPath) {
    std::vector<std::string> files;

    #if defined(_WIN32)
        std::string searchPath = folderPath;
        searchPath += "\\*";
        WIN32_FIND_DATAA fileInfo;
        HANDLE searchHandle = FindFirstFileA(searchPath.c_str(), &fileInfo);
        if (searchHandle != INVALID_HANDLE_VALUE) {
            do {
                if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (strcmp(fileInfo.cFileName, ".") != 0 && strcmp(fileInfo.cFileName, "..") != 0) {
                        files.push_back(fileInfo.cFileName);
                    }
                }
            } while (FindNextFileA(searchHandle, &fileInfo));
            FindClose(searchHandle);
        }
    #else
        DIR* dir = opendir(folderPath);
        if (dir != nullptr) {
            dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    files.push_back(entry->d_name);
                }
            }
            closedir(dir);
        }
    #endif

    return files;
}

std::string create_temp_file() {
    #ifdef _WIN32
        char temp_path[MAX_PATH];
        char temp_file[MAX_PATH];

        // Get the path to the temp directory
        if (GetTempPathA(MAX_PATH, temp_path) == 0) {
            throw std::runtime_error("Failed to get temp path");
        }

        // Generate a temporary file name
        if (GetTempFileNameA(temp_path, "TMP", 0, temp_file) == 0) {
            throw std::runtime_error("Failed to create temp file name");
        }

        // Create an empty file
        std::ofstream file(temp_file);
        if (!file) {
            throw std::runtime_error("Failed to create temp file");
        }
        file.close();

        return std::string(temp_file);
    #else
        char temp_file[] = "/tmp/shadertastic-tmpfileXXXXXX";

        // Create a unique temporary file
        int fd = mkstemp(temp_file);
        if (fd == -1) {
            throw std::runtime_error("Failed to create temp file");
        }

        // Close the file descriptor immediately, leaving the file for later use
        close(fd);

        return temp_file;
    #endif
}

