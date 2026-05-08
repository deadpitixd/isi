#pragma once
#ifndef VALUE_HPP
#define VALUE_HPP

#include <variant>
#include <string>
#include "Other.hpp"

std::string getPlatformLibraryName(const std::string& baseName) {
#ifdef _WIN32
    return baseName + ".dll";
#elif __APPLE__
    return "lib" + baseName + ".dylib";
#elif __linux__
    return "lib" + baseName + ".so";
#else
    return baseName;
#endif
}

enum class DataType { INT, FLOAT, STRING, BOOL, CHAR};

using Value = std::variant<std::monostate, int, double, std::string, bool, char>;

#endif