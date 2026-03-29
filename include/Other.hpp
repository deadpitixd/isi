#pragma once
#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <map>
#include "Value.hpp"

namespace ISI_Color {
    // Controls
    inline constexpr const char* reset      = "\033[0m";
    inline constexpr const char* clear      = "\033[2J\033[H";

    // Styles
    inline constexpr const char* bold       = "\033[1m";
    inline constexpr const char* underline  = "\033[4m";
    inline constexpr const char* italic     = "\033[3m";

    // Foreground (Standard)
    inline constexpr const char* black      = "\033[30m";
    inline constexpr const char* red        = "\033[31m";
    inline constexpr const char* green      = "\033[32m";
    inline constexpr const char* yellow     = "\033[33m";
    inline constexpr const char* blue       = "\033[34m";
    inline constexpr const char* magenta    = "\033[35m";
    inline constexpr const char* cyan       = "\033[36m";
    inline constexpr const char* white      = "\033[37m";

    // Foreground (Bright)
    inline constexpr const char* b_black    = "\033[90m";
    inline constexpr const char* b_red      = "\033[91m";
    inline constexpr const char* b_green    = "\033[92m";
    inline constexpr const char* b_yellow   = "\033[93m";
    inline constexpr const char* b_blue     = "\033[94m";
    inline constexpr const char* b_magenta  = "\033[95m";
    inline constexpr const char* b_cyan     = "\033[96m";
    inline constexpr const char* b_white    = "\033[97m";

    // Background (Standard)
    inline constexpr const char* bg_black   = "\033[40m";
    inline constexpr const char* bg_red     = "\033[41m";
    inline constexpr const char* bg_green   = "\033[42m";
    inline constexpr const char* bg_yellow  = "\033[43m";
    inline constexpr const char* bg_blue    = "\033[44m";
    inline constexpr const char* bg_magenta = "\033[45m";
    inline constexpr const char* bg_cyan    = "\033[46m";
    inline constexpr const char* bg_white   = "\033[47m";
}

enum class FuncType {
    INT, FLOAT, STRING, BOOL, VOID
};

struct Parameter {
    std::string name;
    DataType type;
};

using NativeFunction = std::function<Value(std::vector<Value>)>;

using NativeHandler = std::function<Value(std::vector<Value>)>;

inline std::map<std::string, NativeHandler>& getGlobalNativeRegistry() {
    static std::map<std::string, NativeHandler> registry;
    return registry;
}

struct NativeRegister {
    NativeRegister(std::string name, NativeHandler handler) {
        getGlobalNativeRegistry()[name] = handler;
    }
};

struct Function {
    std::string name;
    DataType returnType;
    std::vector<Parameter> params;
    std::vector<Token> body;
    bool isVoid = false;

    bool isNative = false; 
    NativeFunction nativeHandler = nullptr;
};

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
        std::cerr << ISI_Color::b_red << msg << ISI_Color::b_cyan << ", at token line: " << ISI_Color::b_blue << *cI << ISI_Color::b_cyan << " (token: '" << ISI_Color::b_blue << (*cCode)[*cI].text << ISI_Color::b_cyan << "')" << ISI_Color::reset << "\n";
    } else {
        std::cerr << ISI_Color::b_red << "Error: " << ISI_Color::b_cyan << msg << ISI_Color::reset << "\n";
    }
    std::exit(errCode);
}