#include <cmath>
#include "other.hpp"

namespace math{
extern "C" {
    Value sqrt(std::vector<Value> args){
        if (args.empty()) return ISI_NULL;
        return __sqrt(valueToFloat(args[0]));
    }
}
}