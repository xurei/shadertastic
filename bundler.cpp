//
// Created by olivier on 21/07/26.
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "src/util/hlsl_expander.hpp"

namespace fs = std::filesystem;

std::optional<std::string> load_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        return std::nullopt;
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <main.hlsl>\n";
            return EXIT_FAILURE;
        }

        const fs::path input_path = fs::absolute(argv[1]);

        HLSLExpander expander;
        expander.addPath("/home/olivier/obs-plugins/obs-shadertastic/effects/shadertastic-lib");

        auto rootFile = load_file(input_path);
        if (!rootFile.has_value()) {
            throw std::runtime_error("Cannot open file: " + input_path.string());
        }

        const std::string result = expander.expand_includes(
            rootFile.value(),
            input_path.parent_path(),
            load_file);

        std::cout << result;

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}