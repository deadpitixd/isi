#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include "other.hpp"

using std::string;

extern "C" {
    Value input(const std::vector<Value>& args) {
        string out;
        std::cin >> out;
        return out;
    }

    Value stringToInt(const std::vector<Value>& args) {
        if (args.empty()) return ISI_NULL;
        const string str = valueToString(args[0]);
        try
        {
            return std::stoi(str);
        }
        catch(const std::exception& e)
        {
            return ISI_NULL;
        }
        return ISI_NULL;
    }
    Value stringToFloat(const std::vector<Value>& args) {
        if (args.empty()) return ISI_NULL;
        const string str = valueToString(args[0]);
        try
        {
            return std::stof(str);
        }
        catch(const std::exception& e)
        {
            return ISI_NULL;
        }
        return ISI_NULL;
    }
    Value typeAsString(const std::vector<Value>& args){
        return valueTypeToStr(args[0]);
    }
    Value let(const std::vector<Value>& args) {
        // easy one liner
        return string(1, valueToString(args[1])[valueToInt(args[0])]);
    }
    Value charToAscii(const std::vector<Value>& args){
        return std::to_string(valueToString(args[1])[0]);
    }
    Value getFlag(const std::vector<Value>& args){
        // not working currently sadly
        return std::monostate{};
    }
}