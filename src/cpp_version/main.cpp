#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symtab.h"
#include "typecheck.h"
#include "ir.h"
#include "regalloc.h"
#include "codegen.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include <memory>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: mjc [--no-regalloc] <input.java>\n";
        return 1;
    }

    bool forceSpill = false;
    int fileArg = 1;
    if (argc >= 3 && std::string(argv[1]) == "--no-regalloc") {
        forceSpill = true;
        fileArg = 2;
    }

    // Read source file
    std::ifstream file(argv[fileArg]);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open file " << argv[fileArg] << "\n";
        return 1;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    // Phase 1: Lex + Parse -> AST
    Lexer lexer(source);
    Parser parser(lexer);
    auto ast = parser.parse();
    if (!ast) {
        std::cerr << "Parse error\n";
        return 1;
    }

    // Phase 2: Build symbol tables + type check
    MethodTable methods;
    TypeChecker checker(methods);
    checker.buildSymbolTables(ast.get());
    if (!checker.checkTypes(ast.get())) {
        checker.reportErrors();
        return 1;
    }
    if (checker.hasErrors()) {
        checker.reportErrors();
        return 1;
    }

    // Phase 3: Generate SSA IR
    IRGenerator irgen;
    IRModule module = irgen.generate(ast.get(), methods);

    // Phase 4: Register allocation (per function)
    std::vector<std::unique_ptr<RegisterAllocator>> allocPtrs;
    for (auto& func : module.functions) {
        auto a = std::make_unique<RegisterAllocator>(func, forceSpill);
        a->allocate();
        allocPtrs.push_back(std::move(a));
    }

    // Build raw pointer vector for CodeGenerator
    std::vector<RegisterAllocator*> allocators;
    for (auto& p : allocPtrs) allocators.push_back(p.get());

    // Phase 5: Emit x86-64 assembly
    std::string infile = argv[fileArg];
    std::string outname;
    size_t dotPos = infile.rfind('.');
    if (dotPos != std::string::npos)
        outname = infile.substr(0, dotPos) + ".s";
    else
        outname = infile + ".s";

    std::ofstream outfile(outname);
    if (!outfile.is_open()) {
        std::cerr << "Error: cannot create output file " << outname << "\n";
        return 1;
    }

    CodeGenerator codegen(module, allocators);
    codegen.generate(outfile);
    outfile.close();

    return 0;
}
