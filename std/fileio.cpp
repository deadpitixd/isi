#include "other.hpp"
#include <fstream>

size_t vp_readfileM(char* ptr, const char* fileName, const size_t bufSize, const char* mode){
    FILE *fptr = fopen(fileName, mode);

    if (!fptr){
        ptr[0] = '\0';
        return 0;
    }

    size_t bytesRead = fread(ptr, 1, bufSize - 1, fptr);
    ptr[bytesRead] = '\0';

    fclose(fptr);
    return bytesRead;
}

extern "C"{
    Value getFileSize(std::vector<Value>& args){
        FILE *fptr = fopen(valueToString(args[0]).c_str(), "rb");

        if (!fptr){
            return 0;
        }
        fseek(fptr, 0L, SEEK_END);
        size_t size = ftell(fptr);
        fclose(fptr);
        return (int)size;
    }
    Value readFile(std::vector<Value>& args){
        const std::string fname = valueToString(args[0]);
        char* buffer = (char*)malloc(valueToInt(getFileSize(args)) + 2);
        vp_readfileM(buffer, fname.c_str(), valueToInt(getFileSize(args)) + 1, valueToString(args[1]).c_str());
        const std::string out = buffer;
        free(buffer);
        return out;
    }
    Value writeFile(std::vector<Value>& args){
        return ISI_NULL;
    }
    Value exists(std::vector<Value>& args){
        std::ofstream file(valueToString(args[0]), std::ios::in);
        if (!file){
            return 0;
        }
        else 
            return 1;
    }
}