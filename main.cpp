/*
    THIS PROJECT USES C++26
*/

bool useDevEnv = false;
bool debug = false;
#include <fstream>
#include <iostream>
#include <meta>
#include <Other.hpp>
#include <filesystem>
#include <Compiler.hpp>
#include <unordered_set>
#include <print>

namespace fs = std::filesystem;
std::string fileName;
std::unordered_set<std::string> flags = {};

int main(int argc, char* argv[]){   
    // Self explanatory
    if (argc<2) { throwError("No file name given",-1);}
    fileName = argv[1];
    bool readFlag=true;
    if (fileName == "--percentflag") {
        readFlag = false;
        fileName = argv[2];
    }
    // Adds extension
    if (!fileName.ends_with(".isi")) {fileName+=".isi";};
    std::cout << fileName << "\n";
    // Checks if the file exists
    if (!fs::exists(fileName)) { throwError("Couldn't find " + fileName, -1); }
    // This gets the arguments
    if (argc > 2){
    for (int i = 2; i < argc; i++){
        std::string arg = argv[i];
        if (arg == "%") readFlag = true;
        if (!readFlag) continue;
        flags.insert(arg);
        if (arg == "--test"){
            std::cout << "Works!\n";
        }
        if (arg == "--debug"){
            debug = true;
        }
        if (arg == "--devenv"){
            useDevEnv = true;
        }
    }
    }
    
    std::fstream file;
    file.open(fileName);
    std::string code;

    std::string line;
    while (getline(file, line)){
        code+=line;
    }
    file.close();
    if (debug && flags.contains("--print-chars")){
    for (int i = 0; i < code.size(); i++) {
        std::cout << "[" << i << "] " << code[i] << std::endl;
    }
    }
    Compiler compiler;
    VirtualMachine vm;
    std::vector<Token> lexedOut = compiler.lex(code);
    if (debug && !useDevEnv){
        for (Token i : lexedOut){
            std::cout << i.lexeme << ", " << i.type << "\n";
        }
    }
    if (useDevEnv){
        std::cout << "tokens\n";
        for (Token i : lexedOut){
            std::cout << i.lexeme << "\n" << i.type << "\n";
        }
    }
    std::vector<Token> lexed = compiler.lex(code);
    auto nodes = compiler.makeAST(lexed);
    compiler.compile(nodes);
    std::vector<Instruction> compiled = compiler.getCode();

    vm.run(compiled);
}