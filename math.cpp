#include <cmath>
extern "C" {
    double sqrt(double in){
        return __builtin_sqrt(in);
    }
}
