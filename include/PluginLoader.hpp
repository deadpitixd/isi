#pragma once
#include <string>
#include <vector>
#include "Enviroment.hpp"
#include "Value.hpp"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    typedef HMODULE LibHandle;
    #define LIB_LOAD(path) LoadLibraryA(path)
    #define LIB_FUNC(lib, name) GetProcAddress(lib, name)
    const std::string LIB_EXT = ".dll";
#else
    #include <dlfcn.h>
    typedef void* LibHandle;
    #define LIB_LOAD(path) dlopen(path, RTLD_LAZY)
    #define LIB_FUNC(lib, name) dlsym(lib, name)
    const std::string LIB_EXT = ".so";
#endif

typedef void (*RegisterPluginFn)(Environment&);