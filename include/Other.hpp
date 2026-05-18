#pragma once
#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <map>
// (Value is from Legacy ISI)
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

template <typename E>
constexpr std::string_view enum_to_string(E value) {
    template for (constexpr auto member : std::define_static_array(std::meta::enumerators_of(^^E))) {
        if (value == [:member:]) {
            return std::meta::display_string_of(member);
        }
    }
    return "<unknown>";
}

enum class FuncType {
    INT, FLOAT, STRING, BOOL, VOID
};

enum isiTokenType : uint{
    TOKEN_EXTERN,
    TOKEN_IMPORT,
    // literals
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_RETURN,

    // keywords
    TOKEN_VAR,
    TOKEN_PRINT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_CONST,

    // operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQUALS,
    TOKEN_NOT_EQUALS,

    // symbols
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,

    // Base data types
    TOKEN_INT,
    TOKEN_STRING_T,
    TOKEN_CHAR,
    TOKEN_BOOL,
    TOKEN_FLOAT,
    TOKEN_LIB,

    // Comparison operators
    TOKEN_BIGGER,
    TOKEN_SMALLER,
    TOKEN_EQUALTO,
    TOKEN_NOT,

    // Program exit, basically return
    TOKEN_EXIT,
    
    // special
    TOKEN_EOF
};

struct Token {
    isiTokenType type;
    std::string lexeme; // raw text
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
    int address = 0;
};

struct ReturnSignal {
    Value value;
};

std::string typeToString(DataType type) {
        switch (type) {
        case DataType::INT:    return "int";
        case DataType::FLOAT:  return "float";
        case DataType::STRING: return "string";
        case DataType::BOOL:   return "bool";
        default:               return "unknown";
    }
}
std::string stringify(const Value& v) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::monostate>) { return "null"; } 
            else {
        return std::to_string(arg);
        }
    }, v);
}

inline std::vector<Instruction> cCode = {};
inline int* cI = nullptr;

// Sets parameters for errors. (Now set to an empty one)
inline void setErrParam(std::vector<Instruction> code, int* i) {
    cCode = code;
    cI = i;
}

// Throws an error with cool colors
inline void throwError(std::string msg, int errCode, bool runtimeErr = true, std::string errType = "") {
    if (!cCode.empty() && cI) {
        std::cerr << ISI_Color::b_red << msg << ISI_Color::b_cyan << ", at OpCode line: " << ISI_Color::b_blue << *cI << ISI_Color::b_cyan << " (Op: '" << ISI_Color::b_blue << std::to_string(cCode[*cI].op) << " | " << stringify(cCode[*cI].value) << ISI_Color::b_cyan << "')" << ISI_Color::reset << "\n";
    } else {
        if (runtimeErr)
            std::cerr << ISI_Color::b_red << "Error: " << ISI_Color::b_cyan << msg << ISI_Color::reset << "\n";
        else
            std::cerr << ISI_Color::b_red << errType << ": " << ISI_Color::b_cyan << msg << ISI_Color::reset << "\n";
    }
    std::exit(errCode);
}

double valueToFloat(const Value& val) {
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val);
    } else if (std::holds_alternative<int>(val)) {
        return static_cast<double>(std::get<int>(val));
    } else if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? 1.0 : 0.0;
    }
    return 0.0;
}

bool valueToBool(const Value& val) {
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val);
    } 
    else if (std::holds_alternative<int>(val)) {
        return std::get<int>(val) != 0;
    } 
    else if (std::holds_alternative<double>(val)) {
        return std::get<double>(val) != 0.0;
    } 
    else if (std::holds_alternative<std::string>(val)) {
        return !std::get<std::string>(val).empty();
    }
    return false;
}

std::string valueToString(const Value& val) {
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    } else if (std::holds_alternative<int>(val)) {
        return std::to_string(std::get<int>(val));
    } else if (std::holds_alternative<double>(val)) {
        return std::to_string(std::get<double>(val));
    } else if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? "true" : "false";
    }
    return "";
}

static DataType getValType(const Value& val) {
    if (std::holds_alternative<int>(val)) return DataType::INT;
    if (std::holds_alternative<double>(val)) return DataType::FLOAT;
    if (std::holds_alternative<std::string>(val)) return DataType::STRING;
    if (std::holds_alternative<bool>(val)) return DataType::BOOL;

    if (val.valueless_by_exception()) {
        throwError("Internal Error: Value is in an invalid state", -4);
    }

    throwError("Unknown value type during type check", -4);
    return DataType::INT;
}

std::string valueTypeToStr(const Value& val){
    return typeToString(getValType(val));
}

long long valueToInt(const Value& val, int nullVal = 0) {
    if (std::holds_alternative<int>(val)) return std::get<int>(val);
    if (std::holds_alternative<double>(val)) return static_cast<long long>(std::get<double>(val));
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? 1 : 0;
    
    if (std::holds_alternative<std::string>(val)) {
        try {
            return std::stoll(std::get<std::string>(val));
        } catch (...) {
            return nullVal; 
        }
    }
    return 0;
}

DataType valueToType(const Value& val){
    if (std::holds_alternative<std::string>(val)) {
        return DataType::STRING;
    } else if (std::holds_alternative<int>(val)) {
        return DataType::INT;
    } else if (std::holds_alternative<double>(val)) {
        return DataType::FLOAT;
    } else if (std::holds_alternative<bool>(val)) {
        return DataType::BOOL;
    }
    return DataType::INT;
}

DataType getLiteralType(const std::string& lexeme) {
    if (lexeme.find('"') != std::string::npos) return DataType::STRING;
    if (lexeme.find('.') != std::string::npos) return DataType::FLOAT;
    if (lexeme == "true" || lexeme == "false") return DataType::BOOL;
    return DataType::INT;
}

inline bool isNumeric(const Value& val) {
    return std::holds_alternative<int>(val) || std::holds_alternative<double>(val);
}

bool compatibleTypes(DataType a, DataType b){
    if (a == b) return true;
    if (a == DataType::INT && b == DataType::FLOAT || b == DataType::STRING) {
        return false;
    }
    return true;
}
