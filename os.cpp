#include "include/Enviroment.hpp"
#include <filesystem>

Value funcExists(std::vector<Value> args) {
    if (args.empty()) return false;
    if (valueTypeToStr(args[0]) != "string")
    { 
        throwError("exists() only takes in strings!", -10); 
    }
    std::string path = valueToString(args[0]);
    return std::filesystem::exists(path);
}

Value funcRemove(std::vector<Value> args) {
    if (args.empty()) return false;
    if (valueTypeToStr(args[0]) != "string")
    { 
        throwError("remove() only takes in strings!", -10); 
    }
    std::string path = valueToString(args[0]);
    return std::filesystem::remove(path);
}

Value funcSystem(std::vector<Value> args) {
    if (valueTypeToStr(args[0]) != "string")
    { 
        throwError("system() only takes in strings!", -10); 
    }
    system(valueToString(args[0]).c_str());
    return std::monostate {};
}

extern "C" {
    #ifdef _WIN32
        __declspec(dllexport)
    #endif
    void register_plugin(Environment& env) {
        env.addNativeFunction("system", funcSystem);
        env.addNativeFunction("exists", funcExists);
        env.addNativeFunction("remove", funcRemove);
    }
}