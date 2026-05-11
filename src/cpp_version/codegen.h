#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"
#include "regalloc.h"
#include <ostream>
#include <vector>
#include <string>

class CodeGenerator {
public:
    CodeGenerator(const IRModule& module,
                  std::vector<RegisterAllocator*>& allocators);
    void generate(std::ostream& out);

private:
    const IRModule& module_;
    std::vector<RegisterAllocator*>& allocators_;

    // Current function state
    const IRFunction* func_ = nullptr;
    RegisterAllocator* alloc_ = nullptr;
    int stackFrameSize_ = 0;
    int spillBase_ = 0;

    void emitDataSection(std::ostream& out);
    void emitFunction(std::ostream& out, int funcIdx);
    void emitPrologue(std::ostream& out);
    void emitEpilogue(std::ostream& out);
    void emitBlock(std::ostream& out, const BasicBlock& block);
    void emitInstr(std::ostream& out, const IRInstr& instr, int blockId);
    void emitPhiMoves(std::ostream& out, int fromBlock, int toBlock);

    // Get the register or spill location for an SSA value
    std::string operand(int ssaId) const;
    std::string operand32(int ssaId) const;
    // Load an SSA value into a specific register (for spilled values)
    void loadInto(std::ostream& out, int ssaId, Reg target) const;
    // Store from a register to an SSA value's location
    void storeFrom(std::ostream& out, Reg src, int ssaId) const;
    std::string spillAddr(int slot) const;
};

#endif
