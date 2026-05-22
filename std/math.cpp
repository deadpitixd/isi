#include <cmath>
#include "other.hpp"

extern "C" {
    Value sqrt_(std::vector<Value> args){
        if (args.empty()) return ISI_NULL;
        return __builtin_sqrt(valueToFloat(args[0]));
    }
}
