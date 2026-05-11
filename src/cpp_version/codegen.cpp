#include "codegen.h"
#include <cassert>

CodeGenerator::CodeGenerator(const IRModule& module,
                             std::vector<RegisterAllocator*>& allocators)
    : module_(module), allocators_(allocators) {}

std::string CodeGenerator::spillAddr(int slot) const {
    int offset = spillBase_ + (slot + 1) * 8;
    return "-" + std::to_string(offset) + "(%rbp)";
}

std::string CodeGenerator::operand(int ssaId) const {
    if (ssaId < 0) return "$0";
    Reg r = alloc_->getRegister(ssaId);
    if (r != Reg::NONE) return regName64(r);
    int slot = alloc_->getSpillSlot(ssaId);
    if (slot >= 0) return spillAddr(slot);
    return "$0"; // fallback
}

std::string CodeGenerator::operand32(int ssaId) const {
    if (ssaId < 0) return "$0";
    Reg r = alloc_->getRegister(ssaId);
    if (r != Reg::NONE) return regName32(r);
    int slot = alloc_->getSpillSlot(ssaId);
    if (slot >= 0) return spillAddr(slot);
    return "$0";
}

void CodeGenerator::loadInto(std::ostream& out, int ssaId, Reg target) const {
    Reg r = alloc_->getRegister(ssaId);
    if (r != Reg::NONE) {
        if (r != target)
            out << "    movq " << regName64(r) << ", " << regName64(target) << "\n";
    } else {
        int slot = alloc_->getSpillSlot(ssaId);
        if (slot >= 0)
            out << "    movq " << spillAddr(slot) << ", " << regName64(target) << "\n";
    }
}

void CodeGenerator::storeFrom(std::ostream& out, Reg src, int ssaId) const {
    Reg r = alloc_->getRegister(ssaId);
    if (r != Reg::NONE) {
        if (r != src)
            out << "    movq " << regName64(src) << ", " << regName64(r) << "\n";
    } else {
        int slot = alloc_->getSpillSlot(ssaId);
        if (slot >= 0)
            out << "    movq " << regName64(src) << ", " << spillAddr(slot) << "\n";
    }
}

// ============================================================================
// Data Section
// ============================================================================
void CodeGenerator::emitDataSection(std::ostream& out) {
    out << ".section .data\n";
    for (auto& [label, value] : module_.stringConstants) {
        out << label << ": .asciz " << value << "\n";
    }
    for (auto& var : module_.staticVars) {
        out << var << ": .quad 0\n";
    }
    out << ".Lfmt_int: .asciz \"%ld\"\n";
    out << ".Lfmt_str: .asciz \"%s\"\n";
    out << ".Lfmt_nl: .asciz \"\\n\"\n";
    out << "\n";
}

// ============================================================================
// Function Generation
// ============================================================================
void CodeGenerator::generate(std::ostream& out) {
    emitDataSection(out);
    out << ".section .text\n";
    out << ".globl main\n\n";

    // Emit __str_concat helper (rdi=str1, rsi=str2, returns rax=new string)
    out << "__str_concat:\n";
    out << "    pushq %rbp\n";
    out << "    movq %rsp, %rbp\n";
    out << "    pushq %rbx\n";
    out << "    pushq %r12\n";
    out << "    pushq %r13\n";
    out << "    subq $8, %rsp\n";  // 16-byte align
    out << "    movq %rdi, %r12\n";  // r12 = str1
    out << "    movq %rsi, %r13\n";  // r13 = str2
    out << "    movq %r12, %rdi\n";
    out << "    call strlen\n";
    out << "    movq %rax, %rbx\n";  // rbx = len1
    out << "    movq %r13, %rdi\n";
    out << "    call strlen\n";
    out << "    addq %rbx, %rax\n";  // rax = len1+len2
    out << "    addq $1, %rax\n";    // +1 for null terminator
    out << "    movq %rax, %rdi\n";
    out << "    call malloc\n";
    out << "    movq %rax, %rbx\n";  // rbx = new buffer
    out << "    movq %rbx, %rdi\n";
    out << "    movq %r12, %rsi\n";
    out << "    call strcpy\n";
    out << "    movq %rbx, %rdi\n";
    out << "    movq %r13, %rsi\n";
    out << "    call strcat\n";
    out << "    movq %rbx, %rax\n";  // return new string
    out << "    addq $8, %rsp\n";
    out << "    popq %r13\n";
    out << "    popq %r12\n";
    out << "    popq %rbx\n";
    out << "    popq %rbp\n";
    out << "    ret\n\n";

    for (int i = 0; i < (int)module_.functions.size(); i++) {
        emitFunction(out, i);
    }
}

void CodeGenerator::emitFunction(std::ostream& out, int funcIdx) {
    func_ = &module_.functions[funcIdx];
    alloc_ = allocators_[funcIdx];

    // Calculate stack frame size
    auto& calleeSave = alloc_->usedCalleeSave();
    int numCalleeSave = (int)calleeSave.size();
    int numSpills = alloc_->totalSpillSlots();
    // Space: callee-save regs + spill slots + local vars
    // spillBase_ = offset from rbp where spills start (after callee-save pushes)
    spillBase_ = numCalleeSave * 8;
    stackFrameSize_ = spillBase_ + numSpills * 8;
    // Align to 16 bytes
    // After push rbp (8 bytes) + return address (8 bytes) = 16-byte aligned
    // Then we push callee-saves and sub rsp
    // Total pushes: 1 (rbp) + numCalleeSave
    // Stack must be 16-aligned before call, so after prologue:
    // (1 + numCalleeSave) * 8 + stackFrameSize_ must be 16-aligned relative to entry
    int totalPushed = (1 + numCalleeSave) * 8; // rbp + callee-saves
    int allocSize = numSpills * 8;
    if ((totalPushed + allocSize) % 16 != 0)
        allocSize += 8;
    stackFrameSize_ = spillBase_ + allocSize;

    out << func_->name << ":\n";
    emitPrologue(out);

    for (auto& block : func_->blocks) {
        emitBlock(out, block);
    }

    out << "\n";
}

void CodeGenerator::emitPrologue(std::ostream& out) {
    out << "    pushq %rbp\n";
    out << "    movq %rsp, %rbp\n";

    // Push callee-save registers
    auto& calleeSave = alloc_->usedCalleeSave();
    for (Reg r : calleeSave) {
        out << "    pushq " << regName64(r) << "\n";
    }

    // Allocate space for spills
    int allocSize = stackFrameSize_ - spillBase_;
    if (allocSize > 0)
        out << "    subq $" << allocSize << ", %rsp\n";

    // Move parameters from argument registers to their allocated locations
    for (int i = 0; i < func_->numParams && i < 6; i++) {
        if (i < (int)func_->values.size()) {
            Reg src = Reg::NONE;
            switch (i) {
                case 0: src = Reg::RDI; break;
                case 1: src = Reg::RSI; break;
                case 2: src = Reg::RDX; break;
                case 3: src = Reg::RCX; break;
                case 4: src = Reg::R8; break;
                case 5: src = Reg::R9; break;
            }
            Reg dst = alloc_->getRegister(i);
            if (dst != Reg::NONE && dst != src) {
                out << "    movq " << regName64(src) << ", " << regName64(dst) << "\n";
            } else if (dst == Reg::NONE) {
                // Spilled parameter
                storeFrom(out, src, i);
            }
        }
    }
}

void CodeGenerator::emitEpilogue(std::ostream& out) {
    int allocSize = stackFrameSize_ - spillBase_;
    if (allocSize > 0)
        out << "    addq $" << allocSize << ", %rsp\n";

    // Restore callee-save registers (in reverse order)
    auto& calleeSave = alloc_->usedCalleeSave();
    std::vector<Reg> saves(calleeSave.begin(), calleeSave.end());
    for (int i = (int)saves.size() - 1; i >= 0; i--) {
        out << "    popq " << regName64(saves[i]) << "\n";
    }

    out << "    popq %rbp\n";
    out << "    ret\n";
}

void CodeGenerator::emitPhiMoves(std::ostream& out, int fromBlock, int toBlock) {
    // Insert moves for PHI nodes in the target block
    for (auto& instr : func_->blocks[toBlock].instrs) {
        if (instr.op != IROpcode::PHI) break; // PHIs are always at block start
        // Find which operand corresponds to fromBlock
        for (size_t i = 0; i < instr.phiBlocks.size(); i++) {
            if (instr.phiBlocks[i] == fromBlock && i < instr.operands.size()) {
                int srcSSA = instr.operands[i];
                int dstSSA = instr.dst;
                // Move src → dst if they're in different locations
                Reg srcReg = alloc_->getRegister(srcSSA);
                Reg dstReg = alloc_->getRegister(dstSSA);
                if (srcReg != Reg::NONE && dstReg != Reg::NONE) {
                    if (srcReg != dstReg)
                        out << "    movq " << regName64(srcReg) << ", " << regName64(dstReg) << "\n";
                } else {
                    loadInto(out, srcSSA, Reg::RAX);
                    storeFrom(out, Reg::RAX, dstSSA);
                }
                break;
            }
        }
    }
}

void CodeGenerator::emitBlock(std::ostream& out, const BasicBlock& block) {
    if (block.id != 0) // Don't emit label for entry block (it follows function label)
        out << ".L" << func_->name << "_" << block.label << ":\n";

    for (auto& instr : block.instrs) {
        emitInstr(out, instr, block.id);
    }
}

void CodeGenerator::emitInstr(std::ostream& out, const IRInstr& instr, int blockId) {
    switch (instr.op) {
        case IROpcode::CONST_INT: {
            Reg dst = alloc_->getRegister(instr.dst);
            if (dst != Reg::NONE) {
                out << "    movq $" << instr.imm << ", " << regName64(dst) << "\n";
            } else {
                out << "    movq $" << instr.imm << ", %rax\n";
                storeFrom(out, Reg::RAX, instr.dst);
            }
            break;
        }
        case IROpcode::CONST_BOOL: {
            Reg dst = alloc_->getRegister(instr.dst);
            if (dst != Reg::NONE) {
                out << "    movq $" << instr.imm << ", " << regName64(dst) << "\n";
            } else {
                out << "    movq $" << instr.imm << ", %rax\n";
                storeFrom(out, Reg::RAX, instr.dst);
            }
            break;
        }
        case IROpcode::CONST_STR: {
            Reg dst = alloc_->getRegister(instr.dst);
            if (dst != Reg::NONE) {
                out << "    leaq " << instr.label << "(%rip), " << regName64(dst) << "\n";
            } else {
                out << "    leaq " << instr.label << "(%rip), %rax\n";
                storeFrom(out, Reg::RAX, instr.dst);
            }
            break;
        }
        case IROpcode::ADD: case IROpcode::SUB: case IROpcode::MUL: {
            // Load operands
            loadInto(out, instr.operands[0], Reg::RAX);
            loadInto(out, instr.operands[1], Reg::RCX);
            if (instr.op == IROpcode::ADD)
                out << "    addq %rcx, %rax\n";
            else if (instr.op == IROpcode::SUB)
                out << "    subq %rcx, %rax\n";
            else
                out << "    imulq %rcx, %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::DIV: {
            loadInto(out, instr.operands[0], Reg::RAX);
            loadInto(out, instr.operands[1], Reg::RCX);
            out << "    cqo\n";
            out << "    idivq %rcx\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::NEG: {
            loadInto(out, instr.operands[0], Reg::RAX);
            out << "    negq %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::CMP_LT: case IROpcode::CMP_GT:
        case IROpcode::CMP_LE: case IROpcode::CMP_GE:
        case IROpcode::CMP_EQ: case IROpcode::CMP_NE: {
            loadInto(out, instr.operands[0], Reg::RAX);
            loadInto(out, instr.operands[1], Reg::RCX);
            out << "    cmpq %rcx, %rax\n";
            const char* setInstr = "sete";
            switch (instr.op) {
                case IROpcode::CMP_LT: setInstr = "setl"; break;
                case IROpcode::CMP_GT: setInstr = "setg"; break;
                case IROpcode::CMP_LE: setInstr = "setle"; break;
                case IROpcode::CMP_GE: setInstr = "setge"; break;
                case IROpcode::CMP_EQ: setInstr = "sete"; break;
                case IROpcode::CMP_NE: setInstr = "setne"; break;
                default: break;
            }
            out << "    " << setInstr << " %al\n";
            out << "    movzbq %al, %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::AND: {
            loadInto(out, instr.operands[0], Reg::RAX);
            loadInto(out, instr.operands[1], Reg::RCX);
            out << "    andq %rcx, %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::OR: {
            loadInto(out, instr.operands[0], Reg::RAX);
            loadInto(out, instr.operands[1], Reg::RCX);
            out << "    orq %rcx, %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::NOT: {
            loadInto(out, instr.operands[0], Reg::RAX);
            out << "    xorq $1, %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::COPY: {
            loadInto(out, instr.operands[0], Reg::RAX);
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::LOAD_STATIC: {
            out << "    movq " << instr.label << "(%rip), %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::STORE_STATIC: {
            loadInto(out, instr.operands[0], Reg::RAX);
            out << "    movq %rax, " << instr.label << "(%rip)\n";
            break;
        }
        case IROpcode::ALLOC_ARRAY: {
            // operands[0] = size
            loadInto(out, instr.operands[0], Reg::RDI);
            // Allocate (size+1)*8 bytes: [length | elem0 | elem1 | ...]
            out << "    addq $1, %rdi\n";
            out << "    shlq $3, %rdi\n";  // *8
            // Save size on stack temporarily before call
            out << "    pushq %rdi\n";
            // Align stack for call
            out << "    subq $8, %rsp\n";
            // Reload size for malloc
            out << "    movq 8(%rsp), %rdi\n";
            out << "    call malloc\n";
            out << "    addq $8, %rsp\n";
            out << "    popq %rcx\n";  // restore (size+1)*8
            // rcx has (size+1)*8, so size = rcx/8 - 1
            out << "    shrq $3, %rcx\n";
            out << "    decq %rcx\n";
            // Store length at [rax]
            out << "    movq %rcx, (%rax)\n";
            // Return pointer past the length header
            out << "    addq $8, %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::ARRAY_LOAD: {
            // operands[0] = base, operands[1] = index
            loadInto(out, instr.operands[0], Reg::RAX);
            loadInto(out, instr.operands[1], Reg::RCX);
            out << "    movq (%rax,%rcx,8), %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::ARRAY_STORE: {
            // operands[0] = base, operands[1] = index, operands[2] = value
            loadInto(out, instr.operands[0], Reg::RAX);
            loadInto(out, instr.operands[1], Reg::RCX);
            loadInto(out, instr.operands[2], Reg::RDX);
            out << "    movq %rdx, (%rax,%rcx,8)\n";
            break;
        }
        case IROpcode::ARRAY_LENGTH: {
            // Length is stored at [base - 8]
            loadInto(out, instr.operands[0], Reg::RAX);
            out << "    movq -8(%rax), %rax\n";
            storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::BRANCH: {
            emitPhiMoves(out, blockId, instr.targetBlock);
            out << "    jmp .L" << func_->name << "_"
                << func_->blocks[instr.targetBlock].label << "\n";
            break;
        }
        case IROpcode::CBRANCH: {
            // For CBRANCH, we need to emit PHI moves for both targets.
            // Since PHI moves happen before the branch, and the two targets
            // may need different moves, we split into explicit branches.
            loadInto(out, instr.operands[0], Reg::RAX);
            out << "    testq %rax, %rax\n";
            // Check if either target has PHI nodes
            bool trueHasPhi = !func_->blocks[instr.trueBlock].instrs.empty() &&
                              func_->blocks[instr.trueBlock].instrs[0].op == IROpcode::PHI;
            bool falseHasPhi = !func_->blocks[instr.falseBlock].instrs.empty() &&
                               func_->blocks[instr.falseBlock].instrs[0].op == IROpcode::PHI;
            if (!trueHasPhi && !falseHasPhi) {
                // Simple case: no PHI resolution needed
                out << "    jne .L" << func_->name << "_"
                    << func_->blocks[instr.trueBlock].label << "\n";
                out << "    jmp .L" << func_->name << "_"
                    << func_->blocks[instr.falseBlock].label << "\n";
            } else {
                // Need PHI resolution: use explicit branches
                std::string trueLabel = ".L" + func_->name + "_" + func_->blocks[instr.trueBlock].label;
                std::string falseLabel = ".L" + func_->name + "_" + func_->blocks[instr.falseBlock].label;
                std::string falsePhiLabel = ".L" + func_->name + "_phi_false_" + std::to_string(blockId);
                out << "    je " << falsePhiLabel << "\n";
                emitPhiMoves(out, blockId, instr.trueBlock);
                out << "    jmp " << trueLabel << "\n";
                out << falsePhiLabel << ":\n";
                emitPhiMoves(out, blockId, instr.falseBlock);
                out << "    jmp " << falseLabel << "\n";
            }
            break;
        }
        case IROpcode::PHI: {
            // PHI nodes are resolved at predecessor block boundaries.
            // No code emitted here - emitPhiMoves handles it.
            break;
        }
        case IROpcode::PARAM: {
            // Move argument to the appropriate register
            int argIdx = (int)instr.imm;
            if (argIdx < 6 && !instr.operands.empty()) {
                Reg target = Reg::NONE;
                switch (argIdx) {
                    case 0: target = Reg::RDI; break;
                    case 1: target = Reg::RSI; break;
                    case 2: target = Reg::RDX; break;
                    case 3: target = Reg::RCX; break;
                    case 4: target = Reg::R8; break;
                    case 5: target = Reg::R9; break;
                }
                loadInto(out, instr.operands[0], target);
            }
            break;
        }
        case IROpcode::CALL: {
            // Arguments should already be in place (emitted by PARAM instructions)
            // For variadic functions (printf), zero rax
            if (instr.label == "printf" || instr.label == "atoi" || instr.label == "malloc" ||
                instr.label == "__str_concat") {
                out << "    xorl %eax, %eax\n";
            }

            // Setup args for simple calls (that don't use separate PARAM instructions)
            if (instr.label == "atoi" && !instr.operands.empty()) {
                loadInto(out, instr.operands[0], Reg::RDI);
            }

            out << "    call " << instr.label << "\n";

            if (instr.dst >= 0)
                storeFrom(out, Reg::RAX, instr.dst);
            break;
        }
        case IROpcode::RETURN_VAL: {
            if (!instr.operands.empty())
                loadInto(out, instr.operands[0], Reg::RAX);
            emitEpilogue(out);
            break;
        }
        case IROpcode::PRINT_INT: {
            // printf("%ld", value)
            loadInto(out, instr.operands[0], Reg::RSI);
            out << "    leaq .Lfmt_int(%rip), %rdi\n";
            out << "    xorl %eax, %eax\n";
            out << "    call printf\n";
            break;
        }
        case IROpcode::PRINT_STR: {
            loadInto(out, instr.operands[0], Reg::RSI);
            out << "    leaq .Lfmt_str(%rip), %rdi\n";
            out << "    xorl %eax, %eax\n";
            out << "    call printf\n";
            break;
        }
        case IROpcode::PRINT_NEWLINE: {
            out << "    leaq .Lfmt_nl(%rip), %rdi\n";
            out << "    xorl %eax, %eax\n";
            out << "    call printf\n";
            break;
        }
        case IROpcode::NOP:
            break;
    }
}
