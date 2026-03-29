# Interscript Interpreted

Website:
https://deadpitixd.github.io/isi/

# Quick Start
`./isi yourcode.isi`

# Compiling
Use:
`g++ main.cpp -o isi -std=c++20`

# C++ Functions
> For C++ functions it is recommended to read: "stl/math.cpp"

To create C++ functions, you need to include "includes/Enviroment.hpp"<br>
And make a function in it named like this:<br>

```cpp
extern "C" {
    #ifdef _WIN32
        __declspec(dllexport)
    #endif
    void register_plugin(Environment& env) {
    }
}
```

In <code>register_plugin()</code>, you register functions that get imported into isi.<br>

For example, you have a function:<br>
> sqrt(float x) -> float

How do you add it?
<br>
You need to import <math.h>
<br>
And make a function like this:
```cpp
Value funcSqrt(std::vector<Value> args) {
    if (args.empty()) {
        return 0.0;
    }
    return std::sqrt(valueToFloat(args[0]));
}
```
> Every function must return a Value and input std::vector<Value> args.

Here, the code checks if the arguments are empty.
<br>If they aren't,<br>
first, the code turns argument 1 into a float.
> valueToFloat() is from the Enviroment.hpp header.
<br> Theres also valueToString().

After that, it returns the square root of args[0].

To register the plugin you need to register the function in register_plugin():

```cpp
        env.addNativeFunction("sqrt", funcSqrt);
```

Everything should looks like this:
```cpp
#include "includes/Enviroment.hpp"
#include <math.h>

Value funcSqrt(std::vector<Value> args) {
    if (args.empty()) {
        return 0.0;
    }
    return std::sqrt(valueToFloat(args[0]));
}

extern "C" {
    #ifdef _WIN32
        __declspec(dllexport)
    #endif
    void register_plugin(Environment& env) {
        env.addNativeFunction("sqrt", funcSqrt);
    }
}
```

Then you compile it into a dll or so using this (for gdb):

> g++ -shared -fPIC -std=c++20 -I. file.cpp -o file.dll

To use it, you just use:

```isi
import file

cout(sqrt(2));
```
> Output: 1.414214