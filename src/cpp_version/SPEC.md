# MiniJava Compiler Specification (`mjc`)

Reference for re-implementing the C++17 MiniJava compiler in `src/cpp_version/`. The compiler reads a single `.java` file and emits GAS-syntax x86-64 assembly linkable against libc. This spec describes the design and inter-phase contracts; it does not paste source. Re-implementations may use any data structures (e.g., a class hierarchy instead of a tagged struct) as long as the external behavior matches.

---

## 1. Overview

`mjc` is a non-optimizing single-pass-per-phase compiler for **MiniJava** (one MainClass per file, *only* `static` methods/fields — no instance objects, no inheritance, no `this`). It targets **x86-64 System V AMD64** (Linux), emits **GAS (AT&T) syntax** assembly, and links against libc (`malloc`, `printf`, `strlen`, `strcpy`, `strcat`, `atoi`).

CLI: `mjc [--no-regalloc] <input.java>`. The optional flag forces every SSA value to spill (useful for testing the spill path). Output goes to a sibling `.s` file (`<basename>.s`); the compiler never reads stdin or writes stdout. Exit code is `0` on success, `1` on any error. Type errors are printed as `Type violation in line N` to stderr, sorted ascending.

Build: `make` produces `./mjc`. End-to-end use: `./mjc foo.java && gcc -no-pie foo.s -o foo && ./foo arg1 arg2`.

---

## 2. MiniJava grammar

### Lexical structure

- Whitespace (spaces, tabs, CR, LF) is discarded; newlines bump the line counter.
- Line comments `// ... \n` and block comments `/* ... */` (no nesting).
- Identifiers `[A-Za-z_][A-Za-z0-9_]*`. Integer literals `[0-9]+` (decimal). String literals `"..."` — the lexer keeps the quotes in `text` so the lexeme can be emitted directly into `.asciz` directives; escapes are kept literally and let the assembler interpret them. Boolean literals `true`/`false`.
- Reserved keywords: `boolean class else false true int main public static String void return if while new length private`.
- Compound tokens (recognized via lookahead, must end at a word boundary): `System.out.print` → `SYSTEM_OUT_PRINT`; `System.out.println` → `SYSTEM_OUT_PRINTLN`; `Integer.parseInt` → `INTEGER_PARSEINT`.
- Operators: `&& || == != <= >= < > + - * / !`. Punctuation: `. ; ( ) { } , = [ ]`. A bare `&` or `|` yields `TOK_ERROR`.

### EBNF (the language `mjc` accepts)

```
Program        ::= MainClass
MainClass      ::= 'class' ID '{' VarDecls MethodDecls MainMethod '}'
VarDecls       ::= ( 'private' 'static' Type ID Init? (',' ID Init?)* ';' )*
Init           ::= '=' Exp
MethodDecls    ::= ( 'public' 'static' Type ID '(' FormalList? ')' '{' StatementList '}' )*
MainMethod     ::= 'public' 'static' 'void' 'main'
                   '(' 'String' '[' ']' ID ')' '{' StatementList '}'
FormalList     ::= Type ID ( ',' Type ID )*
Type           ::= PrimeType ('[' ']')? ('[' ']')?
PrimeType      ::= 'int' | 'boolean' | 'String'
StatementList  ::= Statement*
Statement      ::= '{' StatementList '}'
                 | 'if' '(' Exp ')' Statement 'else' Statement
                 | 'while' '(' Exp ')' Statement
                 | ('System.out.println' | 'System.out.print') '(' Exp ')' ';'
                 | 'return' Exp ';'
                 | Type ID ('=' Exp)? ( ',' ID ('=' Exp)? )* ';'  (* VarDecl *)
                 | LeftValue '=' Exp ';'                            (* Assign *)
                 | ID '(' ExpList? ')' ';'                          (* MethodCall stmt *)
LeftValue      ::= ID | ID '[' Exp ']' | ID '[' Exp ']' '[' Exp ']'
ExpList        ::= Exp ( ',' Exp )*
Exp            ::= Exp BinOp Exp | UnaryOp Exp
                 | INTEGER_LITERAL | STRING_LITERAL | 'true' | 'false' | '(' Exp ')'
                 | LeftValue | LeftValue '.' 'length' | ID '.' 'length'
                 | 'new' PrimeType '[' Exp ']' ('[' Exp ']')?
                 | ID '(' ExpList? ')' | 'Integer.parseInt' '(' Exp ')'
BinOp          ::= '+' | '-' | '*' | '/' | '<' | '>' | '<=' | '>='
                 | '==' | '!=' | '&&' | '||'
UnaryOp        ::= '!' | '-' | '+'
```

**`else` is mandatory** — the existing parser unconditionally calls `expect(KW_ELSE)` after the `if` then-branch, so dangling-else cannot occur.

### Operator precedence (lowest to highest)

| Level | Operators            | Associativity |
| ----- | -------------------- | ------------- |
| 1     | `\|\|`               | Left          |
| 2     | `&&`                 | Left          |
| 3     | `==`  `!=`           | Left          |
| 4     | `<` `>` `<=` `>=`    | Left          |
| 5     | `+` `-`              | Left          |
| 6     | `*` `/`              | Left          |
| 7     | unary `!` `-` `+`    | Right (prefix) |

Parentheses force any precedence.

### Primitive types

| Type      | Surface syntax | Backing storage              |
| --------- | -------------- | ---------------------------- |
| `int`     | `int`          | 64-bit signed integer        |
| `boolean` | `boolean`      | 64-bit (0 or 1)              |
| `String`  | `String`       | C-style `char*` (libc)       |
| 1-D array | `T[]`          | `int64_t*` (header at -8)    |
| 2-D array | `T[][]`        | array of `int64_t*` rows     |

All allocations are 8-byte slots: arrays of `boolean` and `int` and pointers all use the same `(base, index, 8)` addressing.

---

## 3. Pipeline

```
source (.java)
    │  Lexer (lookahead = 2)
    ▼
Tokens
    │  Parser (recursive-descent + precedence climbing)
    ▼
AST  (ast.h: tagged ASTNode tree)
    │  TypeChecker.buildSymbolTables  (creates SymbolTables on every braced block,
    │                                   plus MethodTable for top-level methods)
    │  TypeChecker.checkTypes
    ▼
Typed AST
    │  IRGenerator.generate
    ▼
SSA IR (IRModule of IRFunctions of BasicBlocks of IRInstrs, with explicit PHIs)
    │  RegisterAllocator (per function): liveness → interference → graph coloring
    ▼
Allocated IR  (each SSA value has either a Reg or a stack spill slot)
    │  CodeGenerator.generate
    ▼
x86-64 GAS assembly (.s)
```

Each stage runs after the previous one finishes. Phases never run concurrently. Symbol tables are **owned by the TypeChecker** (in a `vector<unique_ptr<SymbolTable>>`) and pointers into them are stored on AST nodes (`ASTNode::scope`); subsequent phases (IR gen) walk the AST and consult those scope pointers, so the symbol tables must outlive IR generation.

---

## 4. Lexer

`Lexer` wraps the source string and exposes `nextToken()`, `peek()`, `peekNext()` (2-token lookahead). Internal buffer holds tokens that `peek`/`peekNext` have prefetched.

Algorithm:

1. Skip whitespace and comments.
2. EOF → `TOK_EOF`.
3. Alpha/`_`: scan an identifier, then probe for compound tokens. `System` followed by `.out.println` (longest first) or `.out.print` becomes the corresponding compound token; `Integer` followed by `.parseInt` becomes `INTEGER_PARSEINT`. The match must end at a word boundary; on miss, roll back to just after the identifier and consult the keyword map.
4. Digit: scan an integer literal; populate `int_value` via `atoi`.
5. `"`: scan a string literal, preserving the surrounding quotes in `text` (escapes are kept literally — the assembler interprets them).
6. Otherwise: 1- or 2-char punctuation. Lone `&` or `|` yields `TOK_ERROR`.

Each `Token` carries kind, raw text, line, and (for integers) `int_value`. Line numbers are 1-based, incremented on `\n`. The lexer never aborts; the parser does on a `TOK_ERROR` or unexpected token (`exit(1)`).

---

## 5. Parser

Hand-written **recursive-descent** with **precedence climbing** for expressions. The parser keeps the current token in `cur_` and uses `peek()`/`peekNext()` at exactly one place: distinguishing a static method (`public static T name(...)`) from the main method (`public static void main(...)`).

Entry points (mirroring the grammar):

- `parseProgram` wraps the single `MainClass` under a `PROGRAM` node.
- `parseMainClass` reads `class ID {` then `StaticVarDeclList`, `StaticMethodDeclList`, `MainMethod`, `}` in that order.
- `parseStaticVarDeclList` loops while the next token is `private`.
- `parseStaticMethodDeclList` loops while the next is `public`, confirming via `peek` that the third token is *not* `void` (which would belong to `main`).
- `parseStatement` dispatches on the leading token: `{`, `if` (mandatory else — no dangling-else), `while`, the two `System.out.*` tokens, `return`, a type keyword (→ var decl, possibly comma-list), or `IDENTIFIER`. For an identifier:
  - followed by `(` → method call. `main(...)` becomes `CALL_MAIN`, otherwise `METHOD_CALL`.
  - followed by `[` → if the next token is `]` it's a type-followed-by-decl (`T[] x;` or `T[][] x;`); otherwise it's an indexed left-value being assigned (`a[i] = e;` or `a[i][j] = e;`).
  - followed by `=` → simple assignment.
- `parseExpression(minPrec)` is precedence climbing: parse a unary prefix expression or primary, then while the current operator's precedence ≥ `minPrec`, consume it and parse the right operand at `prec+1` (left-associative for all binary ops). The `NodeType` tag depends on the operator: `+` → `EXP_PLUS`; `-`,`*`,`/` → `EXP_OPERATION`; comparison `<`,`>`,`<=`,`>=` → `EXP_OPERATION_COMP`; equality `==`,`!=` → `EXP_COMP`; `&&`,`||` → `EXP_LOGIC`. `str_val` holds the operator text.
- `parsePrimary` handles literals, parenthesized expressions, `new T[e]` / `new T[e][e]`, `Integer.parseInt(...)`, identifier references, indexed reads, and trailing `.length` (after `ID`, `ID[e]`, or `ID[e][e]`).

Error policy: any mismatch prints `Syntax error at line N: ...` to stderr and exits 1; no recovery.

The parser also wraps numerous sub-expressions in a single-child `EXP` node, matching the original Bison shape. The type-checker's `unwrapExp` helper transparently descends through these wrappers; a re-implementation may drop them if downstream consumers compensate.

---

## 6. AST

Single struct `ASTNode` tagged by an enum `NodeType`. All children are `unique_ptr<ASTNode>` in a `vector`; child positions encode meaning. Each node carries:

| Field             | Type                  | Purpose                                              |
| ----------------- | --------------------- | ---------------------------------------------------- |
| `node_type`       | `NodeType`            | discriminant                                         |
| `line`            | `int`                 | source line number                                   |
| `data_type`       | `DataType`            | for `PRIME_TYPE`, `EXP_TYPE`, etc.                   |
| `str_val`         | `string`              | identifier name, operator string, string literal     |
| `int_val`         | `int`                 | for integer literals                                 |
| `bool_val`        | `bool`                | for boolean literals                                 |
| `children`        | `vector<unique_ptr>`  | sub-tree                                             |
| `parent`          | raw pointer           | back-pointer (set by `addChild`)                     |
| `scope`           | `SymbolTable*`        | filled by symbol-table builder (see §7)              |
| `is_static`       | `bool`                | true for static var decls                            |
| `has_curly_braces`| `bool`                | marker for nodes that introduce a new scope          |

`DataType` ∈ `{ UNDEFINED, STRING, INT, BOOLEAN }`.

### Node-by-node child layout

`str_val`/`int_val`/`bool_val`/`data_type` are payload fields. `EXP` is a transparent one-child wrapper used around sub-expressions in `IF`/`WHILE` conds, `ASSIGN` rhs, and operands of binary ops; `unwrapExp` peels them.

Top-level structure:
- `PROGRAM[MainClass]`; `MAINCLASS[VarDeclList, MethodDeclList, MainMethod]` (`str_val`=class name, braced); `MAINMETHOD[StatementList]` (`str_val`=args param name, braced).
- `STATIC_VAR_DECL_LIST` and `STATIC_METHOD_DECL_LIST` hold lists of `STATIC_VAR_DECL`/`STATIC_METHOD_DECL`.
- `STATIC_VAR_DECL[VAR_DECL]` (inner `is_static=true`).
- `STATIC_METHOD_DECL[ReturnType, FormalList?, StatementList]` (`str_val`=method name, braced).
- `VAR_DECL[Type, (INIT|ID_EXP_LIST)*]` (`str_val`=first id); `INIT[Exp]`; `ID_EXP_LIST[INIT?]` (`str_val`=additional id).
- `FORMAL_LIST[Type, FORMAL_LIST?]` cons-list (`str_val`=param name).
- `PRIME_TYPE[]` (`data_type`); `TYPE_ONE_ARR[PRIME_TYPE]`; `TYPE_TWO_ARR[PRIME_TYPE]`.

Statements:
- `STATEMENT_LIST` (list, braced); `STATEMENT[StatementList]` for `{...}` blocks (braced).
- `IF[Cond, Then, Else]`; `WHILE[Cond, Body]`; `ASSIGN[LeftValue, Exp]`.
- `RETURN[Exp]`, `PRINT[Exp]`, `PRINTLN[Exp]`.
- `STATEMENT_METHOD_CALL[METHOD_CALL|CALL_MAIN|PARSE_INT]`.

Calls and lists:
- `METHOD_CALL[COMMA_EXP_LIST?]` and `CALL_MAIN[COMMA_EXP_LIST?]` (`str_val`=name).
- `EXP_METHOD_CALL[METHOD_CALL|CALL_MAIN|PARSE_INT]` for calls in expression position.
- `PARSE_INT[Exp]`. `COMMA_EXP_LIST` is a list of `Exp`.

Expressions:
- `EXP_TYPE[]` literal (int/bool/string); `EXP_LEFT[LeftValue]` read.
- Binary `[Exp,Exp]` with `str_val`=op: `EXP_PLUS` (int + or string concat), `EXP_OPERATION` (`-`/`*`/`/`), `EXP_OPERATION_COMP` (`<`/`>`/`<=`/`>=`), `EXP_COMP` (`==`/`!=`), `EXP_LOGIC` (`&&`/`||`).
- Unary `[Exp]`: `EXP_LOGIC_NOT` (`!`), `EXP_NEG` (`-`), `EXP_POS` (`+`).
- `EXP_ONE_ARR[PRIME_TYPE, SizeExp]` and `EXP_TWO_ARR[PRIME_TYPE, SizeExp, SizeExp]` for `new`.
- `EXP_LENGTH[LeftValue]`.

Left-values (`str_val`=name):
- `LEFTVALUE_ID[]`, `LEFTVALUE_ONE_ARR[Index]`, `LEFTVALUE_TWO_ARR[Index1, Index2]`.

A re-implementation may use a class hierarchy (one class per kind) instead of a tagged struct. The downstream phases only need: kind discrimination, ordered child access, and the four payload fields.

---

## 7. Symbol table

Two structures:

**`SymbolTable`** — lexically scoped. Hash map (`unordered_map<string, SymbolInfo>`) + parent pointer. `lookupLocal` searches only this scope; `lookup` walks the chain. `insert` returns false on duplicate. `SymbolInfo` holds: `name`, `type`, `is_method`, `is_static`, `is_arg`, `num_params`, `array_dim` (0/1/2), `stack_slot`, `method_scope*`. A new scope is created for every AST node with `has_curly_braces == true`: `MAINCLASS`, `MAINMETHOD` body, each `STATIC_METHOD_DECL` body, every `STATEMENT_LIST`, every explicit `{...}` statement. Other nodes inherit their parent's scope. Chains capture program → class → method → block.

**`MethodTable`** — flat hash map of name → `SymbolInfo`. Holds every static method and `main`. Separate namespace from variables (looked up only when an identifier is a callee). Duplicate insertion is a type error at the duplicate's line.

What gets inserted: `VAR_DECL` and each `ID_EXP_LIST` (with type & dim; duplicate-in-scope is an error at that line); `FORMAL_LIST` parameters (`is_arg=true`); the implicit `args: String[]` in the main method body; static methods and `main` into the `MethodTable`. `ASSIGN` records `leftVal->scope = currentScope` so name resolution works later.

---

## 8. Type checker

Two-pass: build symbol tables (`fillTable`), then bottom-up `checkNode` walk. Violations append to a `vector<int>` of line numbers; at end they are sorted ascending and printed as `Type violation in line N`. Compilation stops if any errors exist.

**`findExpType(expr)`** (after unwrapping `EXP`):

| Node                          | Type                                          |
| ----------------------------- | --------------------------------------------- |
| `EXP_TYPE`                    | the literal's `data_type`                     |
| `EXP_ONE_ARR`, `EXP_TWO_ARR`  | element type from inner `PRIME_TYPE`          |
| `EXP_LENGTH`                  | `INT`                                         |
| `EXP_LEFT`                    | type of the looked-up left value              |
| `EXP_METHOD_CALL`             | `INT` for `parseInt`; else method's ret type  |
| `EXP_PLUS`                    | type of the left operand                      |
| `EXP_OPERATION`               | `INT`                                         |
| `EXP_OPERATION_COMP`, `EXP_COMP`, `EXP_LOGIC`, `EXP_LOGIC_NOT` | `BOOLEAN`    |
| `EXP_POS`, `EXP_NEG`          | `INT`                                         |

If a type can't be resolved, return `UNDEFINED`; checks treat `UNDEFINED` operands as "skip" to avoid cascade errors.

### Per-construct typing rules

| Construct                          | Accepted operands                              | Errors on            |
| ---------------------------------- | ---------------------------------------------- | -------------------- |
| `+`                                | `INT,INT` or `STRING,STRING`                   | mixed/other          |
| `-`, `*`, `/`                      | `INT,INT`                                      | non-int              |
| `<`, `>`, `<=`, `>=`               | `INT,INT`                                      | non-int              |
| `==`, `!=`                         | `INT,INT` or `BOOLEAN,BOOLEAN`                 | mixed/`STRING`       |
| `&&`, `\|\|`, `!`                  | `BOOLEAN(s)`                                   | non-bool             |
| unary `+`, unary `-`               | `INT`                                          | non-int              |
| `if(e)`, `while(e)`                | `BOOLEAN` or `INT`                             | other                |
| `T x = e;`                         | `type(e) == T` (when init present)             | mismatch             |
| `lv = e;`                          | LHS in scope; `type(lv) == type(e)`            | undeclared, mismatch |
| `lv.length`                        | `array_dim > 0`                                | non-array            |
| `id[e]`, `id[e][e]`                | `array_dim ≥ 1` / `≥ 2` respectively           | not array of right dim |
| `f(args)`                          | `f` exists in `MethodTable`                    | unknown name         |

The checker does **not** verify method-call arity or argument types — only callee existence. `Integer.parseInt` always type-checks; IR maps it to `atoi`.

---

## 9. Intermediate Representation (IR)

The IR is **SSA-form three-address code** organized into per-function **CFGs of basic blocks**. Each `IRFunction` owns:

- `name`, `returnType`, `paramNames[]`, `paramTypes[]`, `numParams`
- `blocks[]` — vector of `BasicBlock { id, label, instrs, preds[], succs[] }`. Block `0` is entry. The label format used at emit time is `<role>_<id>` (e.g., `entry_0`, `if_then_3`).
- `values[]` — vector of `SSAValue { id, name, type, isSpilled }`. SSA value IDs are simply the index in `values`.

Each `IRInstr`:

| Field         | Meaning                                                        |
| ------------- | -------------------------------------------------------------- |
| `op`          | `IROpcode`                                                     |
| `dst`         | SSA id produced (or `-1` for void instructions)                |
| `operands`    | SSA ids consumed                                               |
| `imm`         | integer immediate (for constants, param index, …)              |
| `label`       | string (for static-var name, string-constant label, callee, …) |
| `phiBlocks`   | parallel array to `operands`, the predecessor block per arg    |
| `trueBlock` / `falseBlock` | for `CBRANCH`                                     |
| `targetBlock` | for `BRANCH`                                                   |

### Opcodes

| Opcode                   | Operands              | `dst` | Semantics                                              |
| ------------------------ | --------------------- | ----- | ------------------------------------------------------ |
| `CONST_INT`, `CONST_BOOL`| 0                     | yes   | `dst = imm`                                            |
| `CONST_STR`              | 0                     | yes   | `dst = address of string at .label`                    |
| `ADD`, `SUB`, `MUL`, `DIV` | 2                   | yes   | integer arithmetic                                     |
| `NEG`                    | 1                     | yes   | `dst = -op0`                                           |
| `CMP_LT/GT/LE/GE/EQ/NE`  | 2                     | yes   | boolean (0/1)                                          |
| `AND`, `OR`              | 2                     | yes   | bitwise on 0/1 values (no short-circuit)               |
| `NOT`                    | 1                     | yes   | `dst = op0 XOR 1`                                      |
| `COPY`                   | 1                     | yes   | `dst = op0`                                            |
| `LOAD_STATIC`            | 0                     | yes   | `dst = *static[label]`                                 |
| `STORE_STATIC`           | 1                     | no    | `static[label] = op0`                                  |
| `ALLOC_ARRAY`            | 1                     | yes   | `dst = malloc((op0+1)*8) + 8`; length at `dst-8`       |
| `ARRAY_LOAD`             | 2 (base, idx)         | yes   | `dst = base[idx]` (8-byte slot)                        |
| `ARRAY_STORE`            | 3 (base, idx, val)    | no    | `base[idx] = val`                                      |
| `ARRAY_LENGTH`           | 1                     | yes   | `dst = base[-8]`                                       |
| `BRANCH`                 | 0                     | no    | jump to `targetBlock`                                  |
| `CBRANCH`                | 1 (cond)              | no    | `trueBlock` if cond≠0 else `falseBlock`                |
| `PHI`                    | n                     | yes   | operand `i` from `phiBlocks[i]`                        |
| `PARAM`                  | 1                     | no    | "this is argument #imm of next CALL"                   |
| `CALL`                   | 0..N                  | opt   | call `label`; ABI handled by codegen                   |
| `RETURN_VAL`             | 0 or 1                | no    | return (with value if operand present)                 |
| `PRINT_INT`, `PRINT_STR` | 1                     | no    | prints as `%ld` / `%s`                                 |
| `PRINT_NEWLINE`          | 0                     | no    | prints `\n`                                            |
| `NOP`                    | 0                     | no    | no-op                                                  |

### Lowering from AST

For each method (and `main`):

1. Allocate an `IRFunction` and entry block.
2. Create SSA values for parameters in declaration order. For `main`, inject SSA 0 = `argc` (RDI), SSA 1 = `argv` (RSI), then emit `args = argv + 8` so that user-visible `args[0]` is `argv[1]`.
3. For `main` only, emit `STORE_STATIC` for each static variable that has an initializer.
4. Generate the body.
5. For `main`, append `RETURN_VAL 0`.

A `varMap_` maps source name → current SSA id. Reading an undefined name yields a fresh `CONST_INT 0` (defensive). Static reads/writes go through `LOAD_STATIC`/`STORE_STATIC` keyed on the name.

**`if`** creates three blocks (`if_then`, `if_else`, `if_merge`). The generator snapshots `varMap_` before each branch, runs the branch, then in the merge block inserts a `PHI` for every variable whose value differs. If a branch ends in `RETURN_VAL`, no branch-to-merge is emitted and that side is excluded from PHIs. PHI predecessors must be the *actual* `curBlock_` at the end of each branch (which may differ from the originally allocated `if_then`/`if_else` ids if nested control flow grew during generation).

**`while`** creates `while_header`, `while_body`, `while_exit`. A pre-pass walks the body (and condition) collecting every name written by `ASSIGN` or `VAR_DECL`. For each such name with a pre-loop value, the header receives a `PHI` with one operand from the pre-loop predecessor; the back-edge operand is appended after the body finishes. The condition is evaluated *after* PHIs so it sees the loop-carried values.

**Method calls** push `PARAM` instructions in order (with `imm = i`), then `CALL` with `label` = callee name. Result is an `INT` SSA value (the IR doesn't currently propagate the actual return type; codegen reads `%rax` either way). `Integer.parseInt(e)` lowers to `CALL atoi` with one PARAM. `STRING + STRING` lowers to `CALL __str_concat` with two PARAMs (helper emitted by codegen, §11).

**Array allocation**: `new T[n]` → `ALLOC_ARRAY` with operand `n`. `new T[r][c]` allocates the row-pointer array, then synthesizes a small loop (with a PHI counter) that allocates each row and stores it into the outer array.

**Array access**: 1-D is `ARRAY_LOAD`/`ARRAY_STORE`. 2-D first loads the row pointer, then loads/stores at the inner index. `arr[i].length` (2-D row) loads the row pointer first.

**Logical operators are NOT short-circuiting**: both operands are always evaluated and combined with `AND`/`OR`. The test corpus avoids side-effecting operands.

---

## 10. Register allocator

Per-function graph-coloring allocator with `K = 11` allocatable registers.

### Register pool

| Class                | Registers                                     |
| -------------------- | --------------------------------------------- |
| Allocatable (11)     | `%rbx %rsi %rdi %r8 %r9 %r10 %r11 %r12 %r13 %r14 %r15` |
| Reserved scratch     | `%rax %rcx %rdx`                              |
| Reserved system      | `%rsp %rbp`                                   |
| Caller-saved         | `%rax %rcx %rdx %rsi %rdi %r8 %r9 %r10 %r11`  |
| Callee-saved         | `%rbx %r12 %r13 %r14 %r15`                    |
| Argument registers   | `%rdi %rsi %rdx %rcx %r8 %r9` (in that order) |

`%rax`, `%rcx`, `%rdx` are kept out of the allocation pool because the code generator needs them as scratch for nearly every instruction (e.g., `idivq` requires `%rdx:%rax`). The select-order `R10, R11, R8, R9, RBX, R12, R13, R14, R15, RSI, RDI` biases allocation toward caller-saves first (so simple leaf-style temporaries don't force a callee-save to be pushed).

### Algorithm

1. **Liveness** — backward iterative dataflow per basic block: compute `use[b]` (operand read before any def in the block), `def[b]` (any SSA id assigned), then iterate `liveOut[b] = ⋃ liveIn[s]`, `liveIn[b] = use[b] ∪ (liveOut[b] − def[b])` to fixed point.
2. **Interference graph** — undirected graph over SSA value nodes plus 9 pre-colored "clobber" nodes (one per caller-save register, with negative fake SSA ids). Walk each block in reverse maintaining `live = liveOut[b]`. At each instruction: if call-like (`CALL`, `PRINT_INT/STR/NEWLINE`, `ALLOC_ARRAY`), every live value interferes with all 9 caller-save clobbers (forcing it to spill or to a callee-save). Then dst interferes with everything in `live`; `live -= {dst}`; operands enter `live`. Also link all values in `liveIn[b]` pairwise.
3. **Coloring (simplify / spill / select)** — repeatedly stack-push non-precolored nodes with degree `< K`, decrementing neighbors' degrees. When stuck, push remaining nodes by descending degree (potential spills). Pop and try to color from `allRegs` in the biased order; if no color fits, or if `--no-regalloc` was passed, assign a fresh stack spill slot. Track each callee-save chosen so the prologue can push it.

### Outputs

For each SSA id: either `getRegister(id) != NONE` (a `Reg`) or `getSpillSlot(id) >= 0` (a frame slot). Codegen also reads `totalSpillSlots()` and `usedCalleeSave()` to size the frame.

### Notes

- **Function parameters are NOT pre-colored.** SSA ids 0..numParams-1 enter the allocator with no fixed register, and the codegen prologue moves from the ABI register into wherever they ended up. This lets a long-lived parameter land in a callee-save.
- No move coalescing. The output therefore contains many short `mov` chains.

---

## 11. Code generator

Walks IR + allocation, emits text assembly. Output order:

1. `.section .data`, then string constants (`.Lstr<n>: .asciz <quoted-text>`), then static vars (`<name>: .quad 0`), then format strings (`.Lfmt_int .asciz "%ld"`, `.Lfmt_str .asciz "%s"`, `.Lfmt_nl .asciz "\n"`).
2. `.section .text` and `.globl main`.
3. The `__str_concat` helper (always emitted).
4. Each function in order.

**`__str_concat`**: receives `%rdi=str1, %rsi=str2`; saves `%rbx`/`%r12`/`%r13`; calls `strlen` on both, allocates `len1+len2+1` bytes via `malloc`, calls `strcpy` and `strcat`; returns the new pointer in `%rax`.

**Frame layout** (for `S` chosen callee-saves and `P` spill slots), top-down: caller's stack; return address; saved `%rbp`; `S` pushed callee-saves (one per `usedCalleeSave()`); `P` spill slots; an 8-byte pad if alignment requires. Spill slot `k` is at `-(S*8 + (k+1)*8)(%rbp)`. The `subq $X, %rsp` is `P*8` rounded up so that `%rsp` stays 16-byte aligned at call boundaries (after accounting for saved `%rbp` and pushed callee-saves).

**Prologue**: `pushq %rbp; movq %rsp, %rbp; <pushq each callee-save in iteration order>; subq $<allocSize>, %rsp` (if non-zero), then move each ABI argument register (`%rdi`,`%rsi`,...) into the home slot/register chosen by RA for params 0..numParams-1 (omit move when src == dst).

**Epilogue** (inlined at every `RETURN_VAL`): `addq $<allocSize>, %rsp; <popq callee-saves in REVERSE order>; popq %rbp; ret`.

### Per-opcode lowering

General pattern: `loadInto` operands to scratch (`%rax`, `%rcx`, sometimes `%rdx`), compute, then `storeFrom(%rax, dst)`. Both helpers no-op when src/dst already match.

- `CONST_INT`/`CONST_BOOL`: `movq $imm, <dst>` (through `%rax` if spilled).
- `CONST_STR`: `leaq <label>(%rip), <dst>`.
- `ADD`/`SUB`/`MUL`: ops → `%rax`/`%rcx`; `addq`/`subq`/`imulq %rcx, %rax`; store.
- `DIV`: ops → `%rax`/`%rcx`; `cqo`; `idivq %rcx`; store.
- `NEG`: op → `%rax`; `negq %rax`; store.
- `CMP_*`: ops → `%rax`/`%rcx`; `cmpq %rcx, %rax`; `set<cc> %al`; `movzbq %al, %rax`; store. Mapping: LT→`setl`, GT→`setg`, LE→`setle`, GE→`setge`, EQ→`sete`, NE→`setne`.
- `AND`/`OR`: ops → `%rax`/`%rcx`; `andq`/`orq %rcx, %rax`; store.
- `NOT`: op → `%rax`; `xorq $1, %rax`; store.
- `COPY`: op → `%rax`; store.
- `LOAD_STATIC`: `movq <name>(%rip), %rax`; store. `STORE_STATIC`: op → `%rax`; `movq %rax, <name>(%rip)`.
- `ALLOC_ARRAY`: size → `%rdi`; `addq $1`; `shlq $3` (×8); push/sub/reload to align; `call malloc`; reload original `(size+1)*8` into `%rcx`; restore `size` (`shrq $3; decq`); `movq %rcx, (%rax)` writes the length header; `addq $8, %rax` skips it; store.
- `ARRAY_LOAD`: base → `%rax`, idx → `%rcx`; `movq (%rax,%rcx,8), %rax`; store.
- `ARRAY_STORE`: base → `%rax`, idx → `%rcx`, val → `%rdx`; `movq %rdx, (%rax,%rcx,8)`.
- `ARRAY_LENGTH`: base → `%rax`; `movq -8(%rax), %rax`; store.
- `BRANCH`: emit phi-moves into target, then `jmp .L<func>_<targetLabel>`.
- `CBRANCH`: cond → `%rax`; `testq %rax, %rax`. If neither successor begins with PHIs: `jne <trueLabel>; jmp <falseLabel>`. Otherwise emit a per-edge trampoline labelled `.L<func>_phi_false_<blockId>` so PHI moves run on the correct edge.
- `PHI`: no code (resolved at branch sites).
- `PARAM`: load op into argument register `i` (`%rdi %rsi %rdx %rcx %r8 %r9`).
- `CALL`: for `printf`/`atoi`/`malloc`/`__str_concat`, zero `%eax` first (variadic/safety); for `atoi` also load op0 → `%rdi`. `call <label>`. If `dst >= 0`, store `%rax` → dst.
- `RETURN_VAL`: if operand present, load → `%rax`; then emit epilogue.
- `PRINT_INT`: val → `%rsi`; `leaq .Lfmt_int(%rip), %rdi`; `xorl %eax,%eax`; `call printf`. `PRINT_STR`: same with `.Lfmt_str`. `PRINT_NEWLINE`: same with `.Lfmt_nl` and no value.

### Label naming

- Block labels: `.L<funcName>_<role>_<id>` (the role string is what the IR generator passed to `newBlock`, e.g., `entry`, `if_then`, `while_header`). The very first block of a function (id 0) does **not** receive its own label — control falls through the function's entry-symbol label.
- String constants: `.Lstr<n>`, with `n` from a module-wide counter.
- Format strings: `.Lfmt_int .Lfmt_str .Lfmt_nl`.

---

## 12. Runtime / linking

The emitted code expects to be linked against libc. It uses:

- `printf` for all output (formatted as `%ld` for integers, `%s` for strings, then a separate `\n` for `println`).
- `malloc` for array allocation.
- `strlen`, `strcpy`, `strcat` inside the `__str_concat` helper.
- `atoi` for `Integer.parseInt`.

Assemble & link with:

```
gcc -no-pie out.s -o out
./out [args...]
```

`-no-pie` is needed because the code references global data via `(%rip)` but also uses absolute branch targets that `gcc` defaults flag under PIE. The `main` symbol is declared `.globl main`, takes the standard `(int argc, char** argv)` signature in `%rdi`/`%rsi`, and returns `0`.

Process exit is whatever `main` returns (always `0` in this compiler — the language has no exit/throw mechanism). Runtime failures (e.g., out-of-bounds array access) are not checked; they manifest as segfaults via libc.

---

## 13. CLI and file I/O

- `mjc <input.java>` reads the named file, writes the assembly next to it as `<basename>.s`.
- `mjc --no-regalloc <input.java>` does the same but forces all SSA values to spill (useful for verifying the spill code path).
- The compiler does not read stdin or write to stdout. There are no other flags.
- On any failure (missing file, parse error, type error, output write failure), the exit code is `1` and a message is printed to stderr.

---

## 14. Build

`Makefile` summary (plain English):

- Compiler: `g++`, flags `-std=c++17 -Wall -g`.
- Sources: `main.cpp lexer.cpp parser.cpp symtab.cpp typecheck.cpp ir.cpp regalloc.cpp codegen.cpp`.
- Build rule: each `.cpp` → `.o` via the implicit pattern; final link produces the executable `mjc`.
- `make clean` deletes the object files, the binary, and any `*.s` files in the directory.

---

## 15. Sample end-to-end

Source `base.java`:

```
class Base {
    public static void main(String[] args){
        System.out.println(args[0]);
    }
}
```

Expected `.s` shape (from `mjc`'s `base.s`):

1. Standard `.data` section: format strings only (no `.Lstr*`, no statics).
2. `__str_concat` helper (always emitted, even when unused).
3. `main:` with a tiny prologue (`pushq %rbp; movq %rsp, %rbp; subq $8, %rsp`; no callee-saves since none chosen by RA).
4. Move `%rdi` → SSA-home of `argc`, `%rsi` → SSA-home of `argv`. Materialize `8` as a constant, `args = argv + 8` via ADD. (In the actual output this surfaces as several `mov`s through `%rax`/`%rcx`.)
5. `args[0]`: `ARRAY_LOAD` lowers to `movq <args>, %rax; movq <0>, %rcx; movq (%rax,%rcx,8), %rax`; store result.
6. `System.out.println(<str>)`: `movq <reg>, %rsi; leaq .Lfmt_str(%rip), %rdi; xorl %eax, %eax; call printf`, then the same triple with `.Lfmt_nl`.
7. Return 0: `movq $0, <reg>; movq <reg>, %rax; addq $8, %rsp; popq %rbp; ret`.

Re-implementations should: declare `main` globally; always emit the three format strings into `.data`; implement `__str_concat` with signature `char* __str_concat(char*, char*)` and the documented semantics; lower `print` and `println` via `printf` (with an additional `\n` `printf` for `println`); use `(%rax,%rcx,8)` array addressing with length stored at `-8(%base)`. Byte equivalence to `mjc`'s output is *not* required — only functional equivalence on the test corpus.

---

## 16. Reproduction tests

For each file, compile with the reimplementation, link with `gcc -no-pie`, and verify the output matches `mjc`'s on representative inputs.

| File                                                     | Features                                |
| -------------------------------------------------------- | --------------------------------------- |
| `without_opimization/sample_java/base.java`              | main, `args[0]`, println-string         |
| `without_opimization/sample_java/int_decl.java`          | int decls + arithmetic                  |
| `without_opimization/sample_java/bool_and_geq.java`      | bool, `&&`, `>=`                        |
| `without_opimization/sample_java/if_simple.java`         | if/else, string println                 |
| `without_opimization/sample_java/dot_product.java`       | 1D array, while, load/store             |
| `without_opimization/sample_java/arr_twod_decl.java`     | 2D array allocation                     |
| `with_optimization/sample_java/while-gcd.java`           | `parseInt(args[i])`, while + if         |
| `with_optimization/sample_java/while-prime.java`         | nested control flow                     |
| `with_optimization/sample_java/rec-fact.java`            | recursion, method call                  |
| `with_optimization/sample_java/rec-fib.java`             | two recursive calls                     |
| `with_optimization/sample_java/while-random-nested.java` | deep nesting, register pressure         |

Example: `./mjc rec-fact.java && gcc -no-pie rec-fact.s -o /tmp/rf && /tmp/rf 5` should print `120`.

---

## Known ambiguities / implementation choices

Places where the design is under-specified by the source. A re-implementation should match the existing behavior or document a deviation.

1. **`STRING + STRING` calling convention** — the IR emits both `PARAM` instructions and lists the operands on `CALL`. Codegen uses the PARAMs to load `%rdi`/`%rsi`; the CALL operands are ignored. Emitting only PARAMs is fine.
2. **Method call return type in IR** — always treated as `INT` (codegen reads `%rax` regardless). To carry the real return type, consult `MethodTable`.
3. **`if` PHI predecessors** — must be the *actual* `curBlock_` at the end of each branch, not the originally allocated `if_then`/`if_else` ids (nested control flow may have advanced `curBlock_`).
4. **No short-circuit `&&`/`||`** — both operands always evaluated. Test corpus avoids guards like `i < a.length && a[i] == 0`.
5. **Duplicate method name** — type error at the duplicate's line.
6. **Bare-identifier-as-type in `parseStatement`** — vestigial path; legal MiniJava never reaches it. Re-implementations may report syntax error instead.
7. **Undefined variable read in IR** — defensively materializes a `0`. The type-checker should already have rejected such programs.
8. **PHI moves at `CBRANCH`** — when either successor has PHIs, codegen emits a per-edge trampoline labelled `.L<func>_phi_false_<blockId>` rather than splitting critical edges.
