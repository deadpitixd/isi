#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Value.hpp"

namespace ISI_Color {
    // Controls
    inline constexpr const char* reset     = "\033[0m";
    inline constexpr const char* clear     = "\033[2J\033[H";

    // Styles
    inline constexpr const char* bold      = "\033[1m";
    inline constexpr const char* underline = "\033[4m";

    // Foreground (Text)
    inline constexpr const char* f_red     = "\033[31m";
    inline constexpr const char* f_green   = "\033[32m";
    inline constexpr const char* f_yellow  = "\033[33m";
    inline constexpr const char* f_blue    = "\033[34m";
    inline constexpr const char* f_cyan    = "\033[36m";
    inline constexpr const char* f_white   = "\033[37m";

    // Background
    inline constexpr const char* b_red     = "\033[41m";
    inline constexpr const char* b_blue    = "\033[44m";
    inline constexpr const char* b_black   = "\033[40m";

    // Bright variants
    inline constexpr const char* f_b_red   = "\033[91m";
    inline constexpr const char* f_b_green = "\033[92m";
}

struct ReturnSignal {
    Value value;
};

struct Symbol {
    DataType type;
    Value value;
};

inline std::vector<Token>* cCode = nullptr;
inline int* cI = nullptr;

inline void setErrParam(std::vector<Token>* code, int* i) {
    cCode = code;
    cI = i;
}

inline void throwError(std::string msg, int errCode, bool runtimeErr = true) {
    if (cCode && cI) {
        std::cerr << ISI_Color::f_red << msg << ISI_Color::f_cyan << ", at token line: " << ISI_Color::f_blue << *cI << ISI_Color::f_cyan << " (token: '" << ISI_Color::f_blue << (*cCode)[*cI].text << ISI_Color::f_cyan << "')" << ISI_Color::reset << "\n";
    } else {
        std::cerr << ISI_Color::f_red << "Error: " << ISI_Color::f_cyan << msg << ISI_Color::reset << "\n";
    }
    std::exit(errCode);
}