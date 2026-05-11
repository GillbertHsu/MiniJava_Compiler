#ifndef REGALLOC_H
#define REGALLOC_H

#include "ir.h"
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>

// x86-64 registers (allocatable)
enum class Reg : uint8_t {
    RAX = 0, RCX, RDX, RBX, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15,
    NONE = 255
};

const char* regName64(Reg r);
const char* regName32(Reg r);
const char* regName8(Reg r);

// 11 allocatable registers (RAX, RCX, RDX reserved as scratch for codegen)
constexpr int NUM_REGS = 11;

// Caller-save: RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
// Callee-save: RBX, R12, R13, R14, R15
bool isCalleeSave(Reg r);

struct InterferenceNode {
    int ssaId = -1;
    Reg precolored = Reg::NONE;
    Reg assignedReg = Reg::NONE;
    int spillSlot = -1;
    std::set<int> neighbors;
    int degree = 0;
    bool removed = false;
};

class RegisterAllocator {
public:
    explicit RegisterAllocator(IRFunction& func, bool forceSpill = false);
    void allocate();

    Reg getRegister(int ssaId) const;
    int getSpillSlot(int ssaId) const;
    int totalSpillSlots() const { return spillSlotCount_; }
    const std::set<Reg>& usedCalleeSave() const { return usedCalleeSave_; }

private:
    IRFunction& func_;
    bool forceSpill_ = false;
    std::vector<InterferenceNode> nodes_;
    std::unordered_map<int, int> ssaToNode_;
    int spillSlotCount_ = 0;
    std::set<Reg> usedCalleeSave_;

    // Liveness
    std::vector<std::set<int>> liveIn_;
    std::vector<std::set<int>> liveOut_;

    void computeLiveness();
    void buildInterferenceGraph();
    void addEdge(int u, int v);
    void colorGraph();

    int getOrCreateNode(int ssaId);
};

#endif
