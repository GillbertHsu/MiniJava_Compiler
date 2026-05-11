use std::collections::{BTreeSet, HashMap, HashSet};

use crate::ir::{IRFunction, IROpcode};

// x86-64 registers (allocatable)
#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, PartialOrd, Ord)]
#[repr(u8)]
pub enum Reg {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSI = 4,
    RDI = 5,
    R8 = 6,
    R9 = 7,
    R10 = 8,
    R11 = 9,
    R12 = 10,
    R13 = 11,
    R14 = 12,
    R15 = 13,
    NONE = 255,
}

pub fn reg_name_64(r: Reg) -> &'static str {
    match r {
        Reg::RAX => "%rax",
        Reg::RCX => "%rcx",
        Reg::RDX => "%rdx",
        Reg::RBX => "%rbx",
        Reg::RSI => "%rsi",
        Reg::RDI => "%rdi",
        Reg::R8 => "%r8",
        Reg::R9 => "%r9",
        Reg::R10 => "%r10",
        Reg::R11 => "%r11",
        Reg::R12 => "%r12",
        Reg::R13 => "%r13",
        Reg::R14 => "%r14",
        Reg::R15 => "%r15",
        Reg::NONE => "%rax",
    }
}

pub fn reg_name_32(r: Reg) -> &'static str {
    match r {
        Reg::RAX => "%eax",
        Reg::RCX => "%ecx",
        Reg::RDX => "%edx",
        Reg::RBX => "%ebx",
        Reg::RSI => "%esi",
        Reg::RDI => "%edi",
        Reg::R8 => "%r8d",
        Reg::R9 => "%r9d",
        Reg::R10 => "%r10d",
        Reg::R11 => "%r11d",
        Reg::R12 => "%r12d",
        Reg::R13 => "%r13d",
        Reg::R14 => "%r14d",
        Reg::R15 => "%r15d",
        Reg::NONE => "%eax",
    }
}

pub fn reg_name_8(r: Reg) -> &'static str {
    match r {
        Reg::RAX => "%al",
        Reg::RCX => "%cl",
        Reg::RDX => "%dl",
        Reg::RBX => "%bl",
        Reg::RSI => "%sil",
        Reg::RDI => "%dil",
        Reg::R8 => "%r8b",
        Reg::R9 => "%r9b",
        Reg::R10 => "%r10b",
        Reg::R11 => "%r11b",
        Reg::R12 => "%r12b",
        Reg::R13 => "%r13b",
        Reg::R14 => "%r14b",
        Reg::R15 => "%r15b",
        Reg::NONE => "%al",
    }
}

pub fn is_callee_save(r: Reg) -> bool {
    matches!(r, Reg::RBX | Reg::R12 | Reg::R13 | Reg::R14 | Reg::R15)
}

/// 11 allocatable registers (RAX, RCX, RDX reserved as scratch for codegen)
pub const NUM_REGS: usize = 11;

/// The allocation order: caller-save first (R10, R11, R8, R9), then
/// callee-save (RBX, R12-R15), then RSI/RDI.
const ALL_REGS: [Reg; NUM_REGS] = [
    Reg::R10,
    Reg::R11,
    Reg::R8,
    Reg::R9,
    Reg::RBX,
    Reg::R12,
    Reg::R13,
    Reg::R14,
    Reg::R15,
    Reg::RSI,
    Reg::RDI,
];

/// Caller-save registers that are clobbered across calls.
const CALLER_SAVE_REGS: [Reg; 9] = [
    Reg::RAX,
    Reg::RCX,
    Reg::RDX,
    Reg::RSI,
    Reg::RDI,
    Reg::R8,
    Reg::R9,
    Reg::R10,
    Reg::R11,
];

pub struct InterferenceNode {
    pub ssa_id: i32,
    pub precolored: Reg,
    pub assigned_reg: Reg,
    pub spill_slot: i32,
    pub neighbors: HashSet<usize>,
    pub degree: i32,
    pub removed: bool,
}

impl InterferenceNode {
    fn new(ssa_id: i32) -> Self {
        InterferenceNode {
            ssa_id,
            precolored: Reg::NONE,
            assigned_reg: Reg::NONE,
            spill_slot: -1,
            neighbors: HashSet::new(),
            degree: 0,
            removed: false,
        }
    }
}

pub struct RegisterAllocator<'a> {
    func: &'a IRFunction,
    force_spill: bool,
    nodes: Vec<InterferenceNode>,
    ssa_to_node: HashMap<i32, usize>,
    spill_slot_count: i32,
    used_callee_save: BTreeSet<Reg>,
    live_in: Vec<HashSet<i32>>,
    live_out: Vec<HashSet<i32>>,
}

impl<'a> RegisterAllocator<'a> {
    pub fn new(func: &'a IRFunction, force_spill: bool) -> Self {
        RegisterAllocator {
            func,
            force_spill,
            nodes: Vec::new(),
            ssa_to_node: HashMap::new(),
            spill_slot_count: 0,
            used_callee_save: BTreeSet::new(),
            live_in: Vec::new(),
            live_out: Vec::new(),
        }
    }

    pub fn allocate(&mut self) {
        if self.func.values.is_empty() {
            return;
        }
        self.compute_liveness();
        self.build_interference_graph();
        self.color_graph();
    }

    pub fn get_register(&self, ssa_id: i32) -> Reg {
        match self.ssa_to_node.get(&ssa_id) {
            Some(&idx) => self.nodes[idx].assigned_reg,
            None => Reg::RAX,
        }
    }

    pub fn get_spill_slot(&self, ssa_id: i32) -> i32 {
        match self.ssa_to_node.get(&ssa_id) {
            Some(&idx) => self.nodes[idx].spill_slot,
            None => -1,
        }
    }

    pub fn total_spill_slots(&self) -> i32 {
        self.spill_slot_count
    }

    pub fn used_callee_save(&self) -> &BTreeSet<Reg> {
        &self.used_callee_save
    }

    // ========================================================================
    // Internal helpers
    // ========================================================================

    fn get_or_create_node(&mut self, ssa_id: i32) -> usize {
        if let Some(&idx) = self.ssa_to_node.get(&ssa_id) {
            return idx;
        }
        let idx = self.nodes.len();
        self.nodes.push(InterferenceNode::new(ssa_id));
        self.ssa_to_node.insert(ssa_id, idx);
        idx
    }

    fn add_edge(&mut self, u: usize, v: usize) {
        if u == v {
            return;
        }
        if self.nodes[u].neighbors.insert(v) {
            self.nodes[u].degree += 1;
        }
        if self.nodes[v].neighbors.insert(u) {
            self.nodes[v].degree += 1;
        }
    }

    // ========================================================================
    // Liveness Analysis
    // ========================================================================

    fn compute_liveness(&mut self) {
        let num_blocks = self.func.blocks.len();
        self.live_in.resize(num_blocks, HashSet::new());
        self.live_out.resize(num_blocks, HashSet::new());

        // Compute use and def sets for each block
        let mut use_sets: Vec<HashSet<i32>> = vec![HashSet::new(); num_blocks];
        let mut def_sets: Vec<HashSet<i32>> = vec![HashSet::new(); num_blocks];

        for b in 0..num_blocks {
            for instr in &self.func.blocks[b].instrs {
                // Uses: operands that haven't been defined yet in this block
                for &op in &instr.operands {
                    if op >= 0 && !def_sets[b].contains(&op) {
                        use_sets[b].insert(op);
                    }
                }
                // Defs
                if instr.dst >= 0 {
                    def_sets[b].insert(instr.dst);
                }
            }
        }

        // Iterative dataflow
        let mut changed = true;
        while changed {
            changed = false;
            for b in (0..num_blocks).rev() {
                // LVout = union of LVin of successors
                let mut new_out = HashSet::new();
                for &s in &self.func.blocks[b].succs {
                    let s = s as usize;
                    for &v in &self.live_in[s] {
                        new_out.insert(v);
                    }
                }

                // LVin = use union (LVout - def)
                let mut new_in = HashSet::new();
                for &v in &new_out {
                    if !def_sets[b].contains(&v) {
                        new_in.insert(v);
                    }
                }
                for &v in &use_sets[b] {
                    new_in.insert(v);
                }

                if new_in != self.live_in[b] || new_out != self.live_out[b] {
                    self.live_in[b] = new_in;
                    self.live_out[b] = new_out;
                    changed = true;
                }
            }
        }
    }

    // ========================================================================
    // Interference Graph
    // ========================================================================

    fn build_interference_graph(&mut self) {
        // Create pre-colored nodes for caller-save registers.
        // These act as "clobber" nodes at call sites.
        // We use negative IDs for clobber nodes: -(reg_value + 100)
        let mut clobber_nodes: HashMap<Reg, usize> = HashMap::new();
        for &r in &CALLER_SAVE_REGS {
            let fake_id = -((r as i32) + 100);
            let node_idx = self.get_or_create_node(fake_id);
            self.nodes[node_idx].precolored = r;
            self.nodes[node_idx].assigned_reg = r;
            clobber_nodes.insert(r, node_idx);
        }

        let num_blocks = self.func.blocks.len();

        for b in 0..num_blocks {
            let mut live: HashSet<i32> = self.live_out[b].clone();

            // Walk instructions in reverse
            let num_instrs = self.func.blocks[b].instrs.len();
            for i in (0..num_instrs).rev() {
                let op = self.func.blocks[b].instrs[i].op;
                let dst = self.func.blocks[b].instrs[i].dst;
                let operands = self.func.blocks[b].instrs[i].operands.clone();

                // At CALL instructions, all caller-save regs are clobbered.
                // Values live across the call must interfere with caller-save regs.
                if matches!(
                    op,
                    IROpcode::Call
                        | IROpcode::PrintInt
                        | IROpcode::PrintStr
                        | IROpcode::PrintNewline
                        | IROpcode::AllocArray
                ) {
                    // Collect live SSA IDs and their node indices
                    let live_nodes: Vec<(i32, usize)> = live
                        .iter()
                        .filter(|&&l| l >= 0)
                        .map(|&l| {
                            let n = self.get_or_create_node(l);
                            (l, n)
                        })
                        .collect();

                    for &(_l, l_node) in &live_nodes {
                        for &c_idx in clobber_nodes.values() {
                            self.add_edge(l_node, c_idx);
                        }
                    }
                }

                if dst >= 0 {
                    let dst_node = self.get_or_create_node(dst);
                    // dst interferes with everything currently live (except itself)
                    let live_nodes: Vec<(i32, usize)> = live
                        .iter()
                        .filter(|&&l| l != dst)
                        .map(|&l| {
                            let n = self.get_or_create_node(l);
                            (l, n)
                        })
                        .collect();

                    for &(_l, l_node) in &live_nodes {
                        self.add_edge(dst_node, l_node);
                    }
                    live.remove(&dst);
                }

                // Add uses to live set
                for &op_id in &operands {
                    if op_id >= 0 {
                        self.get_or_create_node(op_id);
                        live.insert(op_id);
                    }
                }
            }

            // Also link all values in liveIn together
            let live_vec: Vec<i32> = self.live_in[b].iter().copied().collect();
            // Pre-create all nodes first
            let node_indices: Vec<usize> = live_vec
                .iter()
                .map(|&v| self.get_or_create_node(v))
                .collect();
            for i in 0..node_indices.len() {
                for j in (i + 1)..node_indices.len() {
                    let ni = node_indices[i];
                    let nj = node_indices[j];
                    self.add_edge(ni, nj);
                }
            }
        }
    }

    // ========================================================================
    // Graph Coloring with Spilling
    // ========================================================================

    fn color_graph(&mut self) {
        let total_nodes = self.nodes.len();
        let mut on_stack = vec![false; total_nodes];
        let mut color_stack: Vec<usize> = Vec::new();

        // Simplify: iteratively remove nodes with degree < NUM_REGS
        let mut progress = true;
        while progress {
            progress = false;
            for i in 0..total_nodes {
                if on_stack[i] || self.nodes[i].removed {
                    continue;
                }
                if self.nodes[i].precolored != Reg::NONE {
                    continue;
                }
                if (self.nodes[i].degree as usize) < NUM_REGS {
                    color_stack.push(i);
                    on_stack[i] = true;
                    self.nodes[i].removed = true;
                    // Decrease degree of neighbors
                    let neighbors: Vec<usize> =
                        self.nodes[i].neighbors.iter().copied().collect();
                    for n in neighbors {
                        if !self.nodes[n].removed {
                            self.nodes[n].degree -= 1;
                        }
                    }
                    progress = true;
                }
            }
        }

        // Any remaining uncolored nodes are potential spills.
        // Push them too (highest degree first).
        let mut remaining: Vec<usize> = Vec::new();
        for i in 0..total_nodes {
            if !on_stack[i] && !self.nodes[i].removed && self.nodes[i].precolored == Reg::NONE {
                remaining.push(i);
            }
        }
        remaining.sort_by(|&a, &b| self.nodes[b].degree.cmp(&self.nodes[a].degree));

        for i in remaining {
            color_stack.push(i);
            on_stack[i] = true;
            self.nodes[i].removed = true;
            let neighbors: Vec<usize> = self.nodes[i].neighbors.iter().copied().collect();
            for n in neighbors {
                if !self.nodes[n].removed {
                    self.nodes[n].degree -= 1;
                }
            }
        }

        // Select: pop and try to color
        while let Some(idx) = color_stack.pop() {
            self.nodes[idx].removed = false;

            // Collect colors used by neighbors
            let mut used_colors: HashSet<Reg> = HashSet::new();
            let neighbors: Vec<usize> = self.nodes[idx].neighbors.iter().copied().collect();
            for n in neighbors {
                if self.nodes[n].assigned_reg != Reg::NONE {
                    used_colors.insert(self.nodes[n].assigned_reg);
                }
            }

            // Try to find a free color
            let mut chosen = Reg::NONE;
            if !self.force_spill {
                for &r in &ALL_REGS {
                    if !used_colors.contains(&r) {
                        chosen = r;
                        break;
                    }
                }
            }

            if chosen != Reg::NONE {
                self.nodes[idx].assigned_reg = chosen;
                if is_callee_save(chosen) {
                    self.used_callee_save.insert(chosen);
                }
            } else {
                // Spill this value
                self.nodes[idx].spill_slot = self.spill_slot_count;
                self.spill_slot_count += 1;
                self.nodes[idx].assigned_reg = Reg::NONE;
            }
        }
    }
}
