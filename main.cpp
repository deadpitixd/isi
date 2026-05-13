/*
    THIS PROJECT USES C++26
*/

bool useDevEnv = false;
bool debug = false;
bool compiledBin = false;
#include <unordered_set>
std::unordered_set<std::string> flags = {};
#include <print>
#include <fstream>
#include <iostream>
#include <meta>
#include <Other.hpp>
#include <filesystem>
#include <Compiler.hpp>
#define __VAPOR_H_EXC_INI
// some stuff for some easier thingies
#include <vapor/vapor.h>

namespace fs = std::filesystem;
std::string fileName;

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
    if (fileName.ends_with(".isic")) compiledBin = true;
    if (!compiledBin && !fileName.ends_with(".isi")) {fileName+=".isi";};
    if (debug) std::cout << fileName << "\n";
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
        else if (arg == "--debug"){
            debug = true;
        }
        else if (arg == "--devenv"){
            useDevEnv = true;
        }
        else
        {
            flags.insert(arg);
        }
    }
    }

    VirtualMachine vm;

    if (compiledBin){
        char *buf = static_cast<char*>(malloc(vp_getFileSize(fileName.c_str())+1));
        vp_readfileS(buf,fileName.c_str(), vp_getFileSize(fileName.c_str()));
        const std::string code = buf;
        free(buf);
        std::vector<std::string> opc;
        opc.push_back("");
        for (char c : code){
            if (c == '|'){
                opc.push_back("");
                continue;
            }
            if (c == ';'){
                opc.push_back("");
            }
            if (c == '\n'){
                opc.push_back("");
                continue;
            }
            else
            {
                opc.back() += c;
                continue;
            }
        }
        std::vector<Instruction> instr{};
        uint op = 256;
        Value val{};
        for (std::string s : opc){
            if (s == ";") {
                instr.push_back({(OpCode)op,val});
                val = {};
                op = 256;
                continue;
            }
            if (op == 256){
                op = (OpCode)(uint)std::stoi(s);
                continue;
            }
            else
            {
                val = s;
                continue;
            }
        }

        if (debug){
        for (Instruction i : instr){
            if (!valueToString(i.value).empty())
                std::print("{},{}\n",std::to_string((uint)i.op), valueToString(i.value));
            else
                std::print("{}\n",std::to_string((uint)i.op));
        }
        }

        vm.run(instr);
        
        return 0;
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

    if (flags.contains("--compile")){
        //filename.isic
        const std::string nFileName = (fileName + "c").c_str();
        // Makes an empty file
        vp_RWwritefile(nFileName.c_str(), "");
        for (Instruction i : compiled){
            if (valueToString(i.value).empty())
                vp_Awritefile(nFileName.c_str(), (std::to_string((uint)i.op).c_str() + (std::string)";\n").c_str());
            else
                vp_Awritefile(nFileName.c_str(), (std::to_string((uint)i.op).c_str() + (std::string)"|" + valueToString(i.value) + ";\n").c_str());
        }
    }

    int errc = vm.run(compiled);
    if (debug) std::print("Program executed with code '{}'.\n", errc);
}