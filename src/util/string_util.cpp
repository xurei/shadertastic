#ifndef STRING_UTIL_HPP
#define STRING_UTIL_HPP

#include "string_util.h"

bool ends_with(const std::string &input, const std::string &suffix) {
    if (suffix.length() > input.length()) {
        return false;
    }
    else {
        return (input.substr(input.length() - suffix.length()) == suffix);
    }
}

bool starts_with(const std::string &input, const std::string &prefix) {
    if (prefix.length() > input.length()) {
        return false;
    }
    else {
        return (input.substr(0, prefix.length()) == prefix);
    }
}

#endif //STRING_UTIL_HPP
