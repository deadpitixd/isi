/*
    THIS PROJECT USES C++26
*/

bool useDevEnv = false;
bool debug = false;
bool compiledBin = false;
int assembleMode=0;
#include <unordered_set>
#include <vector>
std::unordered_set<std::string> flags = {};
std::vector<std::pair<std::string,std::string>> options;
#include <print>
#include <fstream>
#include <sstream>
#include <iostream>
#include <meta>
#include <Other.hpp>
#include <filesystem>
#include <Compiler.hpp>
#include <Optimizer.hpp>
// some stuff for some easier thingies
#include <vapor/vapor.h>

namespace fs = std::filesystem;
std::string fileName;

std::string resolveImports(const std::string& source, const fs::path& currentDir, std::unordered_set<std::string>& seenFiles) {
    std::string output;
    output.reserve(source.size());
    size_t i = 0;

    while (i < source.size()) {
        if (source[i] == '#') {
            output.push_back(source[i++]);
            while (i < source.size() && source[i] != '#') {
                output.push_back(source[i++]);
            }
            if (i < source.size()) output.push_back(source[i++]);
            continue;
        }

        if (source[i] == '"') {
            output.push_back(source[i++]);
            while (i < source.size()) {
                if (source[i] == '\\' && i + 1 < source.size()) {
                    output.push_back(source[i++]);
                    output.push_back(source[i++]);
                    continue;
                }
                output.push_back(source[i]);
                if (source[i] == '"') {
                    i++;
                    break;
                }
                i++;
            }
            continue;
        }

        if (i + 6 <= source.size() && source.compare(i, 6, "import") == 0) {
            char next = (i + 6 < source.size()) ? source[i + 6] : '\0';
            if (!std::isalnum(static_cast<unsigned char>(next)) && next != '_' && (std::isspace(static_cast<unsigned char>(next)) || next == '"')) {
                size_t j = i + 6;
                while (j < source.size() && std::isspace(static_cast<unsigned char>(source[j]))) {
                    j++;
                }
                if (j < source.size() && source[j] == '"') {
                    j++;
                    size_t pathStart = j;
                    while (j < source.size() && source[j] != '"') {
                        j++;
                    }
                    if (j >= source.size()) {
                        throwError("Unterminated import string", -1);
                    }
                    std::string importPath = source.substr(pathStart, j - pathStart);
                    j++;
                    while (j < source.size() && std::isspace(static_cast<unsigned char>(source[j]))) {
                        j++;
                    }
                    if (j < source.size() && source[j] == ';') {
                        j++;
                    }
                    i = j;

                    if (importPath.empty()) {
                        continue;
                    }

                    fs::path targetPath = currentDir / importPath;
                    if (!fs::exists(targetPath)) {
                        throwError("Couldn't find imported file " + targetPath.string(), -1);
                    }
                    fs::path canonicalPath = fs::canonical(targetPath);
                    std::string canonicalKey = canonicalPath.string();
                    if (seenFiles.contains(canonicalKey)) {
                        continue;
                    }
                    seenFiles.insert(canonicalKey);

                    std::ifstream importedFile(canonicalPath);
                    if (!importedFile) {
                        throwError("Failed to open imported file " + canonicalPath.string(), -1);
                    }
                    std::ostringstream buffer;
                    buffer << importedFile.rdbuf();
                    std::string importedSource = buffer.str();
                    importedFile.close();

                    // Inject source directory marker before imported content
                    fs::path importSourceDir = canonicalPath.parent_path();
                    output += "# __SOURCE_DIR__ \"" + importSourceDir.string() + "\" #\n";
                    output += resolveImports(importedSource, importSourceDir, seenFiles);
                    continue;
                }
            }
        }

        output.push_back(source[i++]);
    }

    return output;
}

int main(int argc, char* argv[]){   
    // Self explanatory
    if (argc<2) { throwError("No file name given",-1);}
    fileName = argv[1];
    bool readFlag=true;
    if (fileName == "%") {
        readFlag = false;
        fileName = argv[2];
    }
    if (fileName == "--assemble"){
        assembleMode=1;
        fileName = argv[2];
    }
    if (fileName == "--disassemble"){
        assembleMode=2;
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

    // upgrade later
    if (compiledBin) {
        char *buf = static_cast<char*>(malloc(vp_getFileSize(fileName.c_str()) + 1));
        size_t size = vp_readfileS(buf, fileName.c_str(), vp_getFileSize(fileName.c_str()));
        buf[size] = '\0';
        std::string code = buf;
        free(buf);

        std::vector<Instruction> instr;
        std::vector<DataType> loadedTypes;
        std::string currentLine;
        std::stringstream ss(code);

        while (std::getline(ss, currentLine, ';')) {
            currentLine.erase(std::remove(currentLine.begin(), currentLine.end(), '\n'), currentLine.end());
            if (currentLine.empty()) continue;

            if (currentLine.starts_with("TYPES|")) {
                std::string typesPart = currentLine.substr(6);
                std::stringstream tss(typesPart);
                std::string typeVal;
                while (std::getline(tss, typeVal, ',')) {
                    if (!typeVal.empty()) {
                        loadedTypes.push_back(static_cast<DataType>(std::stoi(typeVal)));
                    }
                }
                continue;
            }
            if (1 and 1){}

            size_t pipePos = currentLine.find('|');
            if (pipePos != std::string::npos) {
                std::string opPart = currentLine.substr(0, pipePos);
                std::string valPart = currentLine.substr(pipePos + 1);
 
                OpCode op = static_cast<OpCode>(std::stoi(opPart));
                Value val;

                if (!valPart.empty() && (std::isdigit(valPart[0]) || valPart[0] == '-')) {
                    if (valPart.find('.') != std::string::npos) val = std::stod(valPart);
                    else val = std::stoi(valPart);
                } else {
                    val = valPart;
                }
                instr.push_back({op, val});
            } else {
                OpCode op = static_cast<OpCode>(std::stoi(currentLine));
                instr.push_back({op, {}});
            }
        }

        if (debug) {
            for (const auto& i : instr) {
                std::println("Op: {}, Val: {}", enum_to_string(i.op), valueToString(i.value));
            }
        }
        
        int errc = vm.run(instr, loadedTypes);
        if (debug || flags.contains("--output")) std::print("Program executed with code '{}'.\n", errc);
        return errc;
    }
    
    std::fstream file;
    file.open(fileName);
    std::ostringstream codeBuffer;
    codeBuffer << file.rdbuf();
    std::string code = codeBuffer.str();
    file.close();

    std::unordered_set<std::string> seenImports;
    std::string resolvedCode = resolveImports(code, fs::path(fileName).parent_path(), seenImports);
    if (debug && flags.contains("--print-chars")){
    for (int i = 0; i < code.size(); i++) {
        std::cout << "[" << i << "] " << code[i] << std::endl;
    }
    }
    Compiler compiler;
    std::vector<Token> lexedOut = compiler.lex(resolvedCode);
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
    std::vector<Token> lexed = compiler.lex(resolvedCode);
    auto nodes = compiler.makeAST(lexed);
    compiler.compile(nodes);
    std::vector<Instruction> compiled = compiler.getCode();
    
    if (flags.contains("--o")){
        compiled = Optimize(compiled);
    }

    if (flags.contains("--compile")){
        //throwError("--compile parameter doesn't work with this ISI.", -1, false, "Information");
        const std::string nFileName = (fileName + "c").c_str();
        vp_RWwritefile(nFileName.c_str(), "");
        
        std::string header = "TYPES|";
        const auto& types = compiler.getIndexTypes();
        for (size_t i = 0; i < types.size(); i++) {
            header += std::to_string(static_cast<int>(types[i]));
            if (i + 1 < types.size()) {
                header += ",";
            }
        }
        header += ";\n";
        vp_Awritefile(nFileName.c_str(), header.c_str());

        for (Instruction i : compiled){
            if (valueToString(i.value).empty())
                vp_Awritefile(nFileName.c_str(), (std::to_string((uint)i.op).c_str() + (std::string)";\n").c_str());
            else
                vp_Awritefile(nFileName.c_str(), (std::to_string((uint)i.op).c_str() + (std::string)"|" + valueToString(i.value) + ";\n").c_str());
        }
    }

    vm.setFunctions(compiler.getFunctionTable());
    int errc = vm.run(compiled, compiler.getIndexTypes());
    if (debug || flags.contains("--out")) std::print("{}Program executed with code '{}{}{}'.{}\n", ISI_Color::cyan, ISI_Color::green , errc, ISI_Color::cyan, ISI_Color::reset);
    return errc;
}