#include <fstream>
#include <iostream>
#include <Other.hpp>
#include <filesystem>
#include <Compiler.hpp>
#include <unordered_set>

namespace fs = std::filesystem;
std::string fileName;
std::unordered_set<std::string> flags = {};

int main(int argc, char* argv[]){   
    // Self explanatory
    if (argc<2) { throwError("No file name given",-1);}
    fileName = argv[1];
    // Adds extension
    if (!fileName.ends_with(".isi")) {fileName+=".isi";};
    std::cout << fileName << "\n";
    // Checks if the file exists
    if (!fs::exists(fileName)) { throwError("Couldn't find " + fileName, -1); }
    bool debug = false;
    // This gets the arguments
    if (argc > 2){
    for (int i = 2; i < argc; i++){
        std::string arg = argv[i];
        flags.insert(arg);
        if (arg == "--test"){
            std::cout << "Works!\n";
        }
        if (arg == "--debug"){
            debug = true;
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
    for (Token i : lexedOut){
        std::cout << i.lexeme << ", " << i.type << "\n";
    }
    std::vector<Token> lexed = compiler.lex(code);
    compiler.compile(compiler.makeAST(lexed));
    std::vector<Instruction> compiled = compiler.getCode();

    vm.run(compiled);
}