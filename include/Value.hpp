#pragma once
#ifndef VALUE_HPP
#define VALUE_HPP

#include <variant>
#include <string>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    LITERAL,
    OPERATOR,
    SYMBOL
};


struct Token {
    std::string text;
    TokenType type;
};

enum class DataType { INT, FLOAT, STRING, BOOL };

using Value = std::variant<int, double, std::string, bool>;

#endif