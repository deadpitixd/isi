#pragma once
#ifndef VALUE_HPP
#define VALUE_HPP

#include <variant>
#include <string>

enum class isiTokenType {
    KEYWORD,
    IDENTIFIER,
    LITERAL,
    OPERATOR,
    SYMBOL
};


struct Token {
    std::string text;
    isiTokenType type;
};

enum class DataType { INT, FLOAT, STRING, BOOL };

using Value = std::variant<std::monostate, int, double, std::string, bool>;

#endif