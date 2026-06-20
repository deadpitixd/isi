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
#include <cstring>
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

    if (compiledBin) {
        std::ifstream in(fileName, std::ios::binary);
        if (!in) {
            throwError("Failed to open binary file", errors::uncaughtException);
        }

        char magic[4];
        in.read(magic, 4);
        if (std::strncmp(magic, "ISIC", 4) != 0) {
            throwError("Invalid or corrupted ISIC binary file", errors::uncaughtException);
        }

        std::vector<DataType> loadedTypes;
        size_t numTypes;
        in.read(reinterpret_cast<char*>(&numTypes), sizeof(numTypes));
        for (size_t i = 0; i < numTypes; i++) {
            int typeVal;
            in.read(reinterpret_cast<char*>(&typeVal), sizeof(typeVal));
            loadedTypes.push_back(static_cast<DataType>(typeVal));
        }

        std::vector<Instruction> instr;
        size_t numInstr;
        in.read(reinterpret_cast<char*>(&numInstr), sizeof(numInstr));
        for (size_t i = 0; i < numInstr; i++) {
            uint32_t opInt;
            in.read(reinterpret_cast<char*>(&opInt), sizeof(opInt));
            OpCode op = static_cast<OpCode>(opInt);

            uint8_t tag;
            in.read(reinterpret_cast<char*>(&tag), sizeof(tag));

            Value val;
            if (tag == 1) {
                int v;
                in.read(reinterpret_cast<char*>(&v), sizeof(v));
                val = v;
            } else if (tag == 2) {
                double v;
                in.read(reinterpret_cast<char*>(&v), sizeof(v));
                val = v;
            } else if (tag == 3) {
                bool v;
                in.read(reinterpret_cast<char*>(&v), sizeof(v));
                val = v;
            } else if (tag == 4) {
                char v;
                in.read(reinterpret_cast<char*>(&v), sizeof(v));
                val = v;
            } else if (tag == 5) {
                size_t len;
                in.read(reinterpret_cast<char*>(&len), sizeof(len));
                std::string v(len, '\0');
                in.read(v.data(), len);
                val = v;
            } else {
                val = std::monostate{};
            }

            instr.push_back({op, val});
        }
        in.close();

        if (debug) {
            for (const auto& i : instr) {
                std::println("Op: {}, Val: {}", enum_to_string(i.op), stringify(i.value));
            }
        }
        
        int errc = vm.run(instr, loadedTypes);
        if (debug || flags.contains("--out")) std::print("{}Program executed with code '{}{}{}'.{}\n", ISI_Color::cyan, ISI_Color::green , errc, ISI_Color::cyan, ISI_Color::reset);
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
    for (size_t i = 0; i < code.size(); i++) {
        std::cout << "[" << i << "] " << code[i] << std::endl;
    }
    }
    Compiler compiler;
    std::vector<Token> lexedOut = compiler.lex(resolvedCode);
    if (debug && !useDevEnv){
        int cur;
        for (Token i : lexedOut){
            std::println("{}, {}, {}",cur, i.lexeme, enum_to_string(i.type));
            cur++;
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
        const std::string nFileName = fileName + "c";
        std::ofstream out(nFileName, std::ios::binary);
        if (!out) {
            throwError("Failed to open binary file for writing", errors::uncaughtException);
        }

        out.write("ISIC", 4);

        const auto& types = compiler.getIndexTypes();
        size_t numTypes = types.size();
        out.write(reinterpret_cast<const char*>(&numTypes), sizeof(numTypes));
        for (DataType t : types) {
            int typeVal = static_cast<int>(t);
            out.write(reinterpret_cast<const char*>(&typeVal), sizeof(typeVal));
        }

        size_t numInstr = compiled.size();
        out.write(reinterpret_cast<const char*>(&numInstr), sizeof(numInstr));
        for (const Instruction& instr : compiled) {
            uint32_t op = static_cast<uint32_t>(instr.op);
            out.write(reinterpret_cast<const char*>(&op), sizeof(op));

            uint8_t tag = 0;
            if (std::holds_alternative<int>(instr.value)) tag = 1;
            else if (std::holds_alternative<double>(instr.value)) tag = 2;
            else if (std::holds_alternative<bool>(instr.value)) tag = 3;
            else if (std::holds_alternative<char>(instr.value)) tag = 4;
            else if (std::holds_alternative<std::string>(instr.value)) tag = 5;

            out.write(reinterpret_cast<const char*>(&tag), sizeof(tag));

            if (tag == 1) {
                int v = std::get<int>(instr.value);
                out.write(reinterpret_cast<const char*>(&v), sizeof(v));
            } else if (tag == 2) {
                double v = std::get<double>(instr.value);
                out.write(reinterpret_cast<const char*>(&v), sizeof(v));
            } else if (tag == 3) {
                bool v = std::get<bool>(instr.value);
                out.write(reinterpret_cast<const char*>(&v), sizeof(v));
            } else if (tag == 4) {
                char v = std::get<char>(instr.value);
                out.write(reinterpret_cast<const char*>(&v), sizeof(v));
            } else if (tag == 5) {
                const std::string& v = std::get<std::string>(instr.value);
                size_t len = v.size();
                out.write(reinterpret_cast<const char*>(&len), sizeof(len));
                out.write(v.data(), len);
            }
        }
        out.close();
    }
    vm.setFunctions(compiler.getFunctionTable());
    int errc = vm.run(compiled, compiler.getIndexTypes());
    if (debug || flags.contains("--out")) std::print("{}Program executed with code '{}{}{}'.{}\n", ISI_Color::cyan, ISI_Color::green , errc, ISI_Color::cyan, ISI_Color::reset);
    return errc;
}