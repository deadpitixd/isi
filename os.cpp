#include "include/Enviroment.hpp"
#include <filesystem>

Value funcExists(std::vector<Value> args) {
    if (args.empty()) return false;
    std::string path = valueToString(args[0]);
    return std::filesystem::exists(path);
}

Value funcRemove(std::vector<Value> args) {
    if (args.empty()) return false;
    std::string path = valueToString(args[0]);
    return std::filesystem::remove(path);
}

Value funcSystem(std::vector<Value> args) {
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