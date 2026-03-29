#include <filesystem>
#include "include/Enviroment.hpp"
#include <vector>
#include <string>
#include <fstream>

namespace fs = std::filesystem;

Value funcReadFile(std::vector<Value> args){
    if (args.empty()){
        throwError("readFile(path, newLines?) requires 1 argument.", -1);
    }
    std::ifstream file(valueToString(args[0]));
    if(!file.is_open()){
        throwError("File not found: " + valueToString(args[0]), -1);
    }
    bool newLines = valueToBool(args[1]);
    
    std::string fileContent, line;
    while (std::getline(file, line)) {
        fileContent += line;
        if (newLines){
            fileContent += "\n";
        }
    }
    return fileContent;
}

Value funcWriteFile(std::vector<Value> args) {
    if (args.size() < 2) {
        std::cerr << "writeFile(path, content) requires 2 arguments." << std::endl;
        return false; 
    }

    std::string path = valueToString(args[0]);
    std::string content = valueToString(args[1]);

    std::ofstream file(path); 
    
    if (file.is_open()) {
        file << content;
        file.close();
        return true;
    } else {
        std::cerr << "Could not open file for writing: " << path << std::endl;
        return false;
    }
}

Value funcWriteLine(std::vector<Value> args) {
    if (args.size() < 3) return false;

    std::string path = valueToString(args[0]);
    int targetLine = std::get<int>(args[1]);
    std::string newContent = valueToString(args[2]);

    std::vector<std::string> lines;
    std::string line;
    std::ifstream inFile(path);
    
    while (std::getline(inFile, line)) {
        lines.push_back(line);
    }
    inFile.close();

    if (targetLine > 0 && targetLine <= (int)lines.size()) {
        lines[targetLine - 1] = newContent;
    } else {
        lines.push_back(newContent);
    }

    std::ofstream outFile(path, std::ios::trunc);
    if (outFile.is_open()) {
        for (const auto& l : lines) {
            outFile << l << "\n";
        }
        outFile.close();
        return true;
    }
    
    return false;
}

extern "C" {
    #ifdef _WIN32
        __declspec(dllexport)
    #endif
    void register_plugin(Environment& env) {
        env.addNativeFunction("readFile", funcReadFile);
        env.addNativeFunction("writeFile", funcWriteFile);
        env.addNativeFunction("writeLine", funcWriteLine);
    }
}