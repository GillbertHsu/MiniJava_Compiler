#include "regalloc.h"
#include <algorithm>
#include <queue>

const char* regName64(Reg r) {
    switch (r) {
        case Reg::RAX: return "%rax"; case Reg::RCX: return "%rcx";
        case Reg::RDX: return "%rdx"; case Reg::RBX: return "%rbx";
        case Reg::RSI: return "%rsi"; case Reg::RDI: return "%rdi";
        case Reg::R8:  return "%r8";  case Reg::R9:  return "%r9";
        case Reg::R10: return "%r10"; case Reg::R11: return "%r11";
        case Reg::R12: return "%r12"; case Reg::R13: return "%r13";
        case Reg::R14: return "%r14"; case Reg::R15: return "%r15";
        default: return "%rax";
    }
}

const char* regName32(Reg r) {
    switch (r) {
        case Reg::RAX: return "%eax"; case Reg::RCX: return "%ecx";
        case Reg::RDX: return "%edx"; case Reg::RBX: return "%ebx";
        case Reg::RSI: return "%esi"; case Reg::RDI: return "%edi";
        case Reg::R8:  return "%r8d";  case Reg::R9:  return "%r9d";
        case Reg::R10: return "%r10d"; case Reg::R11: return "%r11d";
        case Reg::R12: return "%r12d"; case Reg::R13: return "%r13d";
        case Reg::R14: return "%r14d"; case Reg::R15: return "%r15d";
        default: return "%eax";
    }
}

const char* regName8(Reg r) {
    switch (r) {
        case Reg::RAX: return "%al";  case Reg::RCX: return "%cl";
        case Reg::RDX: return "%dl";  case Reg::RBX: return "%bl";
        case Reg::RSI: return "%sil"; case Reg::RDI: return "%dil";
        case Reg::R8:  return "%r8b"; case Reg::R9:  return "%r9b";
        case Reg::R10: return "%r10b"; case Reg::R11: return "%r11b";
        case Reg::R12: return "%r12b"; case Reg::R13: return "%r13b";
        case Reg::R14: return "%r14b"; case Reg::R15: return "%r15b";
        default: return "%al";
    }
}

bool isCalleeSave(Reg r) {
    return r == Reg::RBX || r == Reg::R12 || r == Reg::R13 ||
           r == Reg::R14 || r == Reg::R15;
}

// Arguments are passed in: RDI, RSI, RDX, RCX, R8, R9
static Reg argReg(int i) {
    static Reg regs[] = {Reg::RDI, Reg::RSI, Reg::RDX, Reg::RCX, Reg::R8, Reg::R9};
    if (i >= 0 && i < 6) return regs[i];
    return Reg::NONE;
}

RegisterAllocator::RegisterAllocator(IRFunction& func, bool forceSpill)
    : func_(func), forceSpill_(forceSpill) {}

int RegisterAllocator::getOrCreateNode(int ssaId) {
    auto it = ssaToNode_.find(ssaId);
    if (it != ssaToNode_.end()) return it->second;
    int idx = (int)nodes_.size();
    InterferenceNode node;
    node.ssaId = ssaId;
    nodes_.push_back(node);
    ssaToNode_[ssaId] = idx;
    return idx;
}

Reg RegisterAllocator::getRegister(int ssaId) const {
    auto it = ssaToNode_.find(ssaId);
    if (it == ssaToNode_.end()) return Reg::RAX;
    return nodes_[it->second].assignedReg;
}

int RegisterAllocator::getSpillSlot(int ssaId) const {
    auto it = ssaToNode_.find(ssaId);
    if (it == ssaToNode_.end()) return -1;
    return nodes_[it->second].spillSlot;
}

// ============================================================================
// Liveness Analysis
// ============================================================================
void RegisterAllocator::computeLiveness() {
    int numBlocks = (int)func_.blocks.size();
    liveIn_.resize(numBlocks);
    liveOut_.resize(numBlocks);

    // Compute use and def sets for each block
    std::vector<std::set<int>> use(numBlocks), def(numBlocks);

    for (int b = 0; b < numBlocks; b++) {
        for (auto& instr : func_.blocks[b].instrs) {
            // Uses: operands that haven't been defined yet in this block
            for (int op : instr.operands) {
                if (op >= 0 && def[b].find(op) == def[b].end())
                    use[b].insert(op);
            }
            // Defs
            if (instr.dst >= 0)
                def[b].insert(instr.dst);
        }
    }

    // Iterative dataflow
    bool changed = true;
    while (changed) {
        changed = false;
        for (int b = numBlocks - 1; b >= 0; b--) {
            // LVout = union of LVin of successors
            std::set<int> newOut;
            for (int s : func_.blocks[b].succs) {
                newOut.insert(liveIn_[s].begin(), liveIn_[s].end());
            }

            // LVin = use union (LVout - def)
            std::set<int> diff;
            std::set_difference(newOut.begin(), newOut.end(),
                               def[b].begin(), def[b].end(),
                               std::inserter(diff, diff.begin()));
            std::set<int> newIn;
            std::set_union(use[b].begin(), use[b].end(),
                          diff.begin(), diff.end(),
                          std::inserter(newIn, newIn.begin()));

            if (newIn != liveIn_[b] || newOut != liveOut_[b]) {
                liveIn_[b] = newIn;
                liveOut_[b] = newOut;
                changed = true;
            }
        }
    }
}

// ============================================================================
// Interference Graph
// ============================================================================
void RegisterAllocator::addEdge(int u, int v) {
    if (u == v) return;
    if (nodes_[u].neighbors.insert(v).second) nodes_[u].degree++;
    if (nodes_[v].neighbors.insert(u).second) nodes_[v].degree++;
}

void RegisterAllocator::buildInterferenceGraph() {
    // Create pre-colored nodes for caller-save registers
    // These act as "clobber" nodes at call sites
    static Reg callerSaveRegs[] = {
        Reg::RAX, Reg::RCX, Reg::RDX, Reg::RSI, Reg::RDI,
        Reg::R8, Reg::R9, Reg::R10, Reg::R11
    };
    // We use negative IDs for clobber nodes (shifted by -100)
    std::unordered_map<Reg, int> clobberNodes;
    for (Reg r : callerSaveRegs) {
        int fakeId = -((int)r + 100);
        int nodeIdx = getOrCreateNode(fakeId);
        nodes_[nodeIdx].precolored = r;
        nodes_[nodeIdx].assignedReg = r;
        clobberNodes[r] = nodeIdx;
    }

    int numBlocks = (int)func_.blocks.size();

    for (int b = 0; b < numBlocks; b++) {
        std::set<int> live = liveOut_[b];

        // Walk instructions in reverse
        for (int i = (int)func_.blocks[b].instrs.size() - 1; i >= 0; i--) {
            auto& instr = func_.blocks[b].instrs[i];

            // At CALL instructions, all caller-save regs are clobbered.
            // Values live across the call must interfere with caller-save regs.
            if (instr.op == IROpcode::CALL || instr.op == IROpcode::PRINT_INT ||
                instr.op == IROpcode::PRINT_STR || instr.op == IROpcode::PRINT_NEWLINE ||
                instr.op == IROpcode::ALLOC_ARRAY) {
                for (int l : live) {
                    if (l < 0) continue; // skip clobber nodes
                    int lNode = getOrCreateNode(l);
                    for (auto& [reg, cIdx] : clobberNodes) {
                        addEdge(lNode, cIdx);
                    }
                }
            }

            if (instr.dst >= 0) {
                int dstNode = getOrCreateNode(instr.dst);
                // dst interferes with everything currently live (except itself)
                for (int l : live) {
                    int lNode = getOrCreateNode(l);
                    if (l != instr.dst)
                        addEdge(dstNode, lNode);
                }
                live.erase(instr.dst);
            }

            // Add uses to live set
            for (int op : instr.operands) {
                if (op >= 0) {
                    getOrCreateNode(op);
                    live.insert(op);
                }
            }
        }

        // Also link all values in liveIn together
        std::vector<int> liveVec(liveIn_[b].begin(), liveIn_[b].end());
        for (size_t i = 0; i < liveVec.size(); i++) {
            int ni = getOrCreateNode(liveVec[i]);
            for (size_t j = i + 1; j < liveVec.size(); j++) {
                int nj = getOrCreateNode(liveVec[j]);
                addEdge(ni, nj);
            }
        }
    }
}

// ============================================================================
// Graph Coloring with Spilling
// ============================================================================
void RegisterAllocator::colorGraph() {
    // Do NOT pre-color function parameters - they will be moved from ABI
    // registers in the prologue. This lets the allocator assign them to
    // callee-save registers if they are live across calls.

    // Simplify: push nodes with degree < K onto stack
    std::stack<int> colorStack;
    int totalNodes = (int)nodes_.size();
    std::vector<bool> onStack(totalNodes, false);

    // Iteratively remove nodes
    bool progress = true;
    while (progress) {
        progress = false;
        for (int i = 0; i < totalNodes; i++) {
            if (onStack[i] || nodes_[i].removed) continue;
            if (nodes_[i].precolored != Reg::NONE) continue;
            if (nodes_[i].degree < NUM_REGS) {
                colorStack.push(i);
                onStack[i] = true;
                nodes_[i].removed = true;
                // Decrease degree of neighbors
                for (int n : nodes_[i].neighbors) {
                    if (!nodes_[n].removed) nodes_[n].degree--;
                }
                progress = true;
            }
        }
    }

    // Any remaining uncolored nodes are potential spills
    // Push them too (highest degree first, as potential spills)
    std::vector<int> remaining;
    for (int i = 0; i < totalNodes; i++) {
        if (!onStack[i] && !nodes_[i].removed && nodes_[i].precolored == Reg::NONE)
            remaining.push_back(i);
    }
    std::sort(remaining.begin(), remaining.end(), [&](int a, int b) {
        return nodes_[a].degree > nodes_[b].degree;
    });
    for (int i : remaining) {
        colorStack.push(i);
        onStack[i] = true;
        nodes_[i].removed = true;
        for (int n : nodes_[i].neighbors) {
            if (!nodes_[n].removed) nodes_[n].degree--;
        }
    }

    // Select: pop and try to color
    // RAX, RCX, RDX excluded — reserved as scratch for codegen
    static Reg allRegs[] = {
        Reg::R10, Reg::R11, Reg::R8, Reg::R9,
        Reg::RBX, Reg::R12, Reg::R13, Reg::R14, Reg::R15,
        Reg::RSI, Reg::RDI
    };

    while (!colorStack.empty()) {
        int idx = colorStack.top();
        colorStack.pop();
        nodes_[idx].removed = false;

        // Collect colors used by neighbors
        std::set<Reg> usedColors;
        for (int n : nodes_[idx].neighbors) {
            if (nodes_[n].assignedReg != Reg::NONE)
                usedColors.insert(nodes_[n].assignedReg);
        }

        // Try to find a free color
        Reg chosen = Reg::NONE;
        if (!forceSpill_) {
            for (Reg r : allRegs) {
                if (usedColors.find(r) == usedColors.end()) {
                    chosen = r;
                    break;
                }
            }
        }

        if (chosen != Reg::NONE) {
            nodes_[idx].assignedReg = chosen;
            if (isCalleeSave(chosen))
                usedCalleeSave_.insert(chosen);
        } else {
            // Spill this value
            nodes_[idx].spillSlot = spillSlotCount_++;
            nodes_[idx].assignedReg = Reg::NONE;
        }
    }
}

void RegisterAllocator::allocate() {
    if (func_.values.empty()) return;
    computeLiveness();
    buildInterferenceGraph();
    colorGraph();
}
