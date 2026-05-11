mod ast;
mod lexer;
mod parser;
mod symtab;
mod typecheck;
mod ir;
mod regalloc;
mod codegen;

use std::env;
use std::fs;
use std::io::Write;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: mjc [--no-regalloc] <input.java>");
        std::process::exit(1);
    }

    let mut force_spill = false;
    let mut file_arg = 1;
    if args.len() >= 3 && args[1] == "--no-regalloc" {
        force_spill = true;
        file_arg = 2;
    }

    // Read source file
    let source = match fs::read_to_string(&args[file_arg]) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Error: cannot open file {}: {}", args[file_arg], e);
            std::process::exit(1);
        }
    };

    // Phase 1: Lex + Parse -> AST
    let lex = lexer::Lexer::new(&source);
    let mut parser = parser::Parser::new(lex);
    let mut ast = parser.parse();

    // Phase 2: Build symbol tables + type check
    let mut methods = symtab::MethodTable::new();
    let mut checker = typecheck::TypeChecker::new(&mut methods);
    checker.build_symbol_tables(&mut ast);
    if !checker.check_types(&ast) {
        checker.report_errors();
        std::process::exit(1);
    }
    if checker.has_errors() {
        checker.report_errors();
        std::process::exit(1);
    }
    let scopes = checker.into_scopes();

    // Phase 3: Generate SSA IR
    let module = ir::IRGenerator::generate(&ast, &methods, &scopes);

    // Phase 4: Register allocation (per function)
    let allocators: Vec<regalloc::RegisterAllocator> = module
        .functions
        .iter()
        .map(|func| {
            let mut alloc = regalloc::RegisterAllocator::new(func, force_spill);
            alloc.allocate();
            alloc
        })
        .collect();

    // Phase 5: Emit x86-64 assembly
    let infile = &args[file_arg];
    let outname = if let Some(dot_pos) = infile.rfind('.') {
        format!("{}.s", &infile[..dot_pos])
    } else {
        format!("{}.s", infile)
    };

    let mut outfile = match fs::File::create(&outname) {
        Ok(f) => f,
        Err(e) => {
            eprintln!("Error: cannot create output file {}: {}", outname, e);
            std::process::exit(1);
        }
    };

    let alloc_refs: Vec<&regalloc::RegisterAllocator> = allocators.iter().collect();
    let mut gen = codegen::CodeGenerator::new(&module, alloc_refs);
    gen.generate(&mut outfile);
}
