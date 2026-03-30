#pragma once
#include <vector>
#include "AST.hpp"
#include "Other.hpp"
#include "Enviroment.hpp"

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos = 0;

    Token peek() { return tokens[pos]; }
    Token advance() { return tokens[pos++]; }

public:
    Parser(const std::vector<Token>& t) : tokens(t) {}

    Expr* parseExpression() {
        return parseAddition();
    }

private:
    Expr* parsePrimary() {
        Token t = advance();

        if (t.type == isiTokenType::LITERAL) {
            return new LiteralExpr(Environment::parseLiteral(t.text));
        }

        if (t.type == isiTokenType::IDENTIFIER) {
            return new VariableExpr(t.text);
        }

        throwError("Unexpected token: " + t.text, -2, false, "Parse Error");
        return nullptr;
    }

    Expr* parseAddition() {
        Expr* left = parsePrimary();

        while (pos < tokens.size() && tokens[pos].text == "+") {
            std::string op = advance().text;
            Expr* right = parsePrimary();

            left = new BinaryExpr(left, op, right);
        }

        return left;
    }
};