#include "include/Enviroment.hpp"
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

Value funcSleep(std::vector<Value> args) {
    if (args.empty()) return std::monostate{};
    
    int ms = 0;
    if (std::holds_alternative<int>(args[0])) ms = std::get<int>(args[0]);
    else if (std::holds_alternative<double>(args[0])) ms = (int)std::get<double>(args[0]);

    if (ms > 0) {
        #ifdef _WIN32
            Sleep(ms);
        #else
            usleep(ms * 1000);
        #endif
    }
    return std::monostate{};
}

extern "C" {
    #ifdef _WIN32
        __declspec(dllexport)
    #endif
    void register_plugin(Environment& env) {
        env.addNativeFunction("sleep", funcSleep);
    }
}