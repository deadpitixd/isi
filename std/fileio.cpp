#include "other.hpp"
#include <fstream>
#include <filesystem>
#define fs std::filesystem
int vp_Mwritefile(const char* fileName, const char* data, const char* mode){
    FILE* fptr = fopen(fileName, mode);
    if (fptr == NULL)
        return 0;

    fputs(data, fptr);
    fclose(fptr);
    return 1;
}

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
        if (args.size() < 2) return ISI_NULL;
        return vp_Mwritefile(valueToString(args[0]).c_str(), valueToString(args[1]).c_str(),valueToString(args[2]).c_str());
    }
    Value exists(std::vector<Value>& args){
        std::ofstream file(valueToString(args[0]), std::ios::in);
        if (!file){
            return 0;
        }
        else 
            return 1;
    }
    Value copyFile(std::vector<Value>& args){
        const string fileName = valueToString(args[0]);
        const string out = valueToString(args[1]);
        fs::copy(fileName, out);
        return null;
    }
    Value deleteFile(std::vector<Value>& args){
        const string fileName = valueToString(args[0]);
        return fs::remove(fileName);
    }
    Value createDirectory(std::vector<Value>& args){
        const string dirname = valueToString(args[0]);

        return fs::create_directories(dirname);
    }
}