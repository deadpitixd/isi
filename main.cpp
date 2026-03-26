#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <stdexcept>
#include <string.h>
#include <cctype>
#include <chrono>
#include "include/Value.hpp"
#include "include/Enviroment.hpp"
#include "include/Other.hpp"

std::vector<bool> boolSettings = {false};

bool isKeyword(const std::string& s) {
    static const std::unordered_set<std::string> kws = {
        "int", "float", "double", "string", "bool", "cout", "input", "while", "if", "endl", "return"
    };
    return kws.contains(s);
}

const char newLine = '`';
std::unordered_set<char> nlChars = {';', ' ','(',')','{','}',','};

std::string typeToString(DataType type) {
    switch (type) {
        case DataType::INT: return "int";
        case DataType::FLOAT: return "float";
        case DataType::STRING: return "string";
        case DataType::BOOL: return "bool";
        default: return "unknown";
    }
}


std::unordered_set<std::string> keywords = {"cout", "int"};

std::vector<Token> Lexer(std::vector<std::string> lines) {
    std::vector<Token> tokens;
    for (const std::string& line : lines) {
        int i = 0;
        while (i < line.length()) {
            char c = line[i];
            if (std::isspace(c)) { i++; continue; }

            if (c == '"') {
                std::string str = "";
                i++;
                while (i < line.length() && line[i] != '"') {
                    str += line[i];
                    i++;
                }
                tokens.push_back({str, TokenType::LITERAL});
                i++; continue;
            }
            
            if (c == '{' || c == '}') {
                tokens.push_back({std::string(1, c), TokenType::SYMBOL});
                i++;
                continue;
            }

            if (std::isalpha(c) || c == '_') {
                std::string word;
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) {
                    word += line[i]; i++;
                }
                TokenType t = isKeyword(word) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
                tokens.push_back({word, t});
                continue;
            }

            if (std::isdigit(c)) {
                std::string num;
                while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.')) {
                    num += line[i]; i++;
                }
                tokens.push_back({num, TokenType::LITERAL});
                continue;
            }

            std::string op(1, c);
            if (i + 1 < line.length()) {
                std::string twoChar = line.substr(i, 2);
                if (twoChar == "==" || twoChar == "!=" || twoChar == "<=" || twoChar == ">=") {
                    tokens.push_back({twoChar, TokenType::OPERATOR});
                    i += 2; continue;
                }
            }
            
            TokenType t = (ispunct(c) && std::string("+-*/=< >").find(c) != std::string::npos) 
                        ? TokenType::OPERATOR : TokenType::SYMBOL;
            tokens.push_back({op, t});
            i++;
        }
    }
    return tokens;
}

void execute(const std::vector<Token>& code, Environment& env) {
    for (int i = 0; i < code.size(); i++) {
        Token t = code[i];

        if (t.type == TokenType::KEYWORD && (t.text == "int" || t.text == "float" || t.text == "double" || t.text == "string" || t.text == "bool")) {
            DataType type = (t.text == "int") ? DataType::INT : 
                            (t.text == "bool") ? DataType::BOOL : 
                            (t.text == "string") ? DataType::STRING : DataType::FLOAT;
            i++;
            std::string name = code[i].text;
            
            Value initialVal;
            if (type == DataType::INT) initialVal = 0;
            else if (type == DataType::FLOAT) initialVal = 0.0;
            else if (type == DataType::BOOL) initialVal = false;
            else initialVal = std::string("");

            env.define(name, type, initialVal);

            if (i + 1 < code.size() && code[i+1].text == "=") {
                i += 2;
                std::vector<Token> expr;
                while (i < code.size() && code[i].text != ";") {
                    expr.push_back(code[i]);
                    i++;
                }
                env.assign(name, env.evaluateExpression(expr, env));
            }
        }

        else if (t.type == TokenType::IDENTIFIER) {
            if (i + 1 < code.size() && code[i+1].text == "=") {
                std::string varName = t.text;
                i += 2;
                std::vector<Token> expr;
                while (i < code.size() && code[i].text != ";") {
                    expr.push_back(code[i]);
                    i++;
                }
                env.assign(varName, env.evaluateExpression(expr, env));
            }
        }

        else if (t.text == "cout") {
            i++;
            if (i < code.size() && code[i].text == "(") {
                i++;
                while (i < code.size() && code[i].text != ")") {
                    std::vector<Token> expr;
                    while (i < code.size() && code[i].text != "," && code[i].text != ")") {
                        expr.push_back(code[i]);
                        i++;
                    }
                    if (!expr.empty()) {
                        std::cout << env.stringify(env.evaluateExpression(expr, env));
                    }
                    if (code[i].text == ",") i++; 
                }
            }
        }

        else if (t.text == "if") {
            i++; 
            if (code[i].text == "(") i++;
            std::vector<Token> conditionTokens;
            while (i < code.size() && code[i].text != ")") {
                conditionTokens.push_back(code[i]);
                i++;
            }
            i++;

            bool shouldExecute = env.asBool(env.evaluateExpression(conditionTokens, env));

            if (i < code.size() && code[i].text == "{") {
                i++;
                int start = i;
                int braceCount = 1;
                while (i < code.size() && braceCount > 0) {
                    if (code[i].text == "{") braceCount++;
                    else if (code[i].text == "}") braceCount--;
                    i++;
                }
                int end = i - 1;

                if (shouldExecute) {
                    std::vector<Token> block(code.begin() + start, code.begin() + end);
                    execute(block, env);
                }
                i--;
            }
        }
        
        if (t.text == "return") {
            i++;
            std::vector<Token> expr;
            while (i < code.size() && code[i].text != ";") {
                expr.push_back(code[i]);
                i++;
            }
            throw ReturnSignal{ env.evaluateExpression(expr, env) };
        }
    }
}

void run(std::vector<Token> code) {
    int i = 0;
    setErrParam(&code, &i);
    Environment env;
    env.define("endl", DataType::STRING, "\n");
    execute(code, env);
}
int main(int argc, char* argv[]){
    bool debug = false;
    bool *ignUnkFlags = new bool;
    *ignUnkFlags = false;
    if (argc > 0){
        for (int i = 2; i < argc; i++){
            if (strcmp(argv[i], "--ignore-unk-flags") == 0){
                *ignUnkFlags = true;
            }
            else if (strcmp(argv[i], "--debug") == 0){
                debug = true;
            }
            else if (strcmp(argv[i], "--time") == 0){
                boolSettings[0] = true;
            }
            else
            {
                if (!*ignUnkFlags){
                    std::cerr << "Unknown flag: '" << argv[i] << "'!\n";
                    return -2;
                }
            }
        }
    }
    delete ignUnkFlags;

    auto start = std::chrono::steady_clock::now();

    std::vector<std::string> input;
    std::string fileName = argv[1];
    if (!fileName.ends_with(".isi")){
        fileName.append(".isi");
    }

    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()){
        std::cerr << "Failed to open file: '" << argv[1] << "'.\n";
        return 1;
    }
    std::string line;

    while (std::getline(inputFile, line)){
        input.push_back(line);
    }
    inputFile.close();

    std::vector<Token> lexed = Lexer(input);

    if (debug){
        int *i = new int;
        *i = 0;
        for (const Token& l : lexed) {
            std::cout << "[" << *i << "] " << l.text << "\n";
            *i = *i + 1;
        }
        delete i;
    }

    run(lexed);

    if(boolSettings[0]){
        std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        std::cout << "\nProject ran successfully in: " << elapsed.count() << " seconds";
    }

    #ifdef linux
    std::cout << std::endl;
    #endif

    return 0;
}