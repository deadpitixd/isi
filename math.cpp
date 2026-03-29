#include "include/Enviroment.hpp"
#include <math.h>
#include <numbers>

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
Value funcPi(std::vector<Value> args){
    return std::numbers::pi;
}
Value funcCos(std::vector<Value> args){
    return cos(valueToFloat(args[0]));
}
Value funcSin(std::vector<Value> args){
    return sin(valueToFloat(args[0]));
}
Value funcTan(std::vector<Value> args){
    return tan(valueToFloat(args[0]));
}
Value funcAbs(std::vector<Value> args){
    return abs(valueToFloat(args[0]));
}
Value funcLog(std::vector<Value> args){
    return log(valueToFloat(args[0]));
}
Value funcRound(std::vector<Value> args){
    return round(valueToFloat(args[0]));
}
Value funcFloor(std::vector<Value> args){
    return floor(valueToFloat(args[0]));
}
Value funcCeil(std::vector<Value> args){
    return ceil(valueToFloat(args[0]));
}

extern "C" {
    #ifdef _WIN32
        __declspec(dllexport)
    #endif
    void register_plugin(Environment& env) {
        env.addNativeFunction("sqrt", funcSqrt);
        env.addNativeFunction("pow", funcPow);
        env.addNativeFunction("pi", funcPi);
        env.addNativeFunction("cos", funcCos);
        env.addNativeFunction("sin", funcSin);
        env.addNativeFunction("tan", funcTan);
        env.addNativeFunction("log", funcLog);
        env.addNativeFunction("round", funcRound);
        env.addNativeFunction("floor", funcFloor);
        env.addNativeFunction("ceil", funcCeil);
        env.addNativeFunction("abs", funcAbs);
    }
}