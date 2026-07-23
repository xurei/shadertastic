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

#ifndef SHADERTASTIC_HLSL_EXPANDER_HPP
#define SHADERTASTIC_HLSL_EXPANDER_HPP

#include <filesystem>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <optional>
#include <iostream>
#include <list>
#include <unordered_set>

class HLSLExpander {
    private:
        std::list<std::string> include_paths;
        typedef std::function<std::optional<std::string>(const std::string&)> LoadFileFn;

    public:
        inline void addPath(const std::string &path) {
            std::cerr << "addPath " << path << std::endl;
            include_paths.push_back(path);
        }
        inline void addPath(const char *path) {
            auto tmp = std::string(path);
            addPath(tmp);
        }
        #ifndef _WIN32
        #pragma clang diagnostic push
        #pragma ide diagnostic ignored "misc-no-recursion"
        #endif
        std::string expand_includes_recursive(const std::string& source, const std::filesystem::path &current_directory, const LoadFileFn& load_file, std::unordered_set<std::filesystem::path>& include_stack) {
            std::stringstream input(source);
            std::ostringstream output;

            std::string line;

            while (std::getline(input, line)) {
                std::string_view view(line);

                //Trim line
                size_t pos = 0;
                while (pos < view.length() && (view[pos] == ' ' || view[pos] == '\t')) {
                    ++pos;
                }

                constexpr std::string_view include_keyword = "@include";
                if (view.substr(pos, include_keyword.length()) != include_keyword) {
                    output << line << '\n';
                }
                else {
                    pos += include_keyword.length();

                    while (pos < view.length() && (view[pos] == ' ' || view[pos] == '\t')) {
                        ++pos;
                    }

                    if (pos >= view.length() || view[pos] != '"') {
                        throw std::runtime_error("Invalid @include syntax: " + line);
                    }

                    ++pos;

                    const size_t end = view.find('"', pos);
                    if (end == std::string_view::npos) {
                        throw std::runtime_error("Invalid @include syntax: " + line);
                    };

                    std::string file_base = std::string(view.substr(pos, end - pos));
                    auto include_path = try_file_exists(current_directory, file_base, include_stack);
                    if (!include_path.has_value()) {
                        // Searching for the file in the include path
                        for (auto &include_dir : include_paths) {
                            std::cerr << "try " << include_dir << std::endl;
                            include_path = try_file_exists(include_dir, file_base, include_stack);
                            if (include_path.has_value()) {
                                std::cerr << "found " << include_path.value() << std::endl;
                                break;
                            }
                        }
                    }

                    if (!include_path.has_value()) {
                        throw std::runtime_error("Invalid @include : File '" + file_base + "' not found");
                    }
                    else {
                        include_stack.insert(include_path.value());
                        auto included_source = load_file(include_path.value());
                        const std::string expanded_included_source = expand_includes_recursive(
                            included_source.value(),
                            include_path.value().parent_path(),
                            load_file,
                            include_stack
                        );

                        //std::cerr << expanded_included_source << std::endl;

                        output << expanded_included_source;

                        include_stack.erase(include_path.value());
                    }
                }
            }

            return output.str();
        }
        #ifndef _WIN32
        #pragma clang diagnostic pop
        #endif

        std::optional<std::filesystem::path> try_file_exists(const std::filesystem::path &current_directory, std::string &file_path_str, std::unordered_set<std::filesystem::path> &include_stack) {
            std::filesystem::path include_path = current_directory / file_path_str;
            include_path = std::filesystem::weakly_canonical(include_path);

            if (include_stack.find(include_path) != include_stack.end()) {
                throw std::runtime_error("Circular @include detected: " + include_path.string());
            }

            if (std::filesystem::exists(include_path)) {
                return include_path;
            }
            else {
                return std::nullopt;
            }
        }

        std::string expand_includes(
            const std::string& main_source,
            const std::filesystem::path& main_directory,
            const LoadFileFn &load_file)
        {
            std::unordered_set<std::filesystem::path> include_stack;

            return expand_includes_recursive(
                main_source,
                main_directory,
                load_file,
                include_stack);
        }
};

#endif //SHADERTASTIC_HLSL_EXPANDER_HPP
