#include "Enviroment.hpp"
#include <math.h>

Value funcSqrt(std::vector<Value> args) {
    if (args.empty()) {
        return 0.0;
    }
    return std::sqrt(valueToFloat(args[0]));
}
Value funcPow(std::vector<Value> args){
    if(args.empty()){
        return 0.0;
    }
    if (args.size() < 2){
        return valueToFloat(args[0]) * valueToFloat(args[0]);
    }
    else{
        return pow(valueToFloat(args[0]),valueToFloat(args[1]));
    }

}

extern "C" {
    #ifdef _WIN32
        __declspec(dllexport)
    #endif
    void register_plugin(Environment& env) {
        env.addNativeFunction("sqrt", funcSqrt);
        env.addNativeFunction("pow", funcPow);
    }
}