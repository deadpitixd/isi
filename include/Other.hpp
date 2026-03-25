#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Value.hpp"

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
        std::cerr << msg << ", at token line: " << *cI << " (token: '" << (*cCode)[*cI].text << "')" << "\n";
    } else {
        std::cerr << "Error: " << msg << "\n";
    }
    std::exit(errCode);
}