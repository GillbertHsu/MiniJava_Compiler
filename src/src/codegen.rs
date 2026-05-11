use std::io::Write;

use crate::ir::{BasicBlock, IRFunction, IRInstr, IRModule, IROpcode};
use crate::regalloc::{reg_name_32, reg_name_64, Reg, RegisterAllocator};

pub struct CodeGenerator<'a> {
    module: &'a IRModule,
    allocators: Vec<&'a RegisterAllocator<'a>>,

    // Current function state
    func: Option<&'a IRFunction>,
    alloc: Option<&'a RegisterAllocator<'a>>,
    stack_frame_size: i32,
    spill_base: i32,
}

impl<'a> CodeGenerator<'a> {
    pub fn new(module: &'a IRModule, allocators: Vec<&'a RegisterAllocator<'a>>) -> Self {
        CodeGenerator {
            module,
            allocators,
            func: None,
            alloc: None,
            stack_frame_size: 0,
            spill_base: 0,
        }
    }

    // ========================================================================
    // Helpers
    // ========================================================================

    fn spill_addr(&self, slot: i32) -> String {
        let offset = self.spill_base + (slot + 1) * 8;
        format!("-{}(%rbp)", offset)
    }

    fn operand(&self, ssa_id: i32) -> String {
        if ssa_id < 0 {
            return "$0".to_string();
        }
        let alloc = self.alloc.unwrap();
        let r = alloc.get_register(ssa_id);
        if r != Reg::NONE {
            return reg_name_64(r).to_string();
        }
        let slot = alloc.get_spill_slot(ssa_id);
        if slot >= 0 {
            return self.spill_addr(slot);
        }
        "$0".to_string() // fallback
    }

    fn operand32(&self, ssa_id: i32) -> String {
        if ssa_id < 0 {
            return "$0".to_string();
        }
        let alloc = self.alloc.unwrap();
        let r = alloc.get_register(ssa_id);
        if r != Reg::NONE {
            return reg_name_32(r).to_string();
        }
        let slot = alloc.get_spill_slot(ssa_id);
        if slot >= 0 {
            return self.spill_addr(slot);
        }
        "$0".to_string()
    }

    fn load_into(&self, out: &mut impl Write, ssa_id: i32, target: Reg) {
        let alloc = self.alloc.unwrap();
        let r = alloc.get_register(ssa_id);
        if r != Reg::NONE {
            if r != target {
                writeln!(out, "    movq {}, {}", reg_name_64(r), reg_name_64(target)).unwrap();
            }
        } else {
            let slot = alloc.get_spill_slot(ssa_id);
            if slot >= 0 {
                writeln!(
                    out,
                    "    movq {}, {}",
                    self.spill_addr(slot),
                    reg_name_64(target)
                )
                .unwrap();
            }
        }
    }

    fn store_from(&self, out: &mut impl Write, src: Reg, ssa_id: i32) {
        let alloc = self.alloc.unwrap();
        let r = alloc.get_register(ssa_id);
        if r != Reg::NONE {
            if r != src {
                writeln!(out, "    movq {}, {}", reg_name_64(src), reg_name_64(r)).unwrap();
            }
        } else {
            let slot = alloc.get_spill_slot(ssa_id);
            if slot >= 0 {
                writeln!(
                    out,
                    "    movq {}, {}",
                    reg_name_64(src),
                    self.spill_addr(slot)
                )
                .unwrap();
            }
        }
    }

    // ========================================================================
    // Data section
    // ========================================================================

    fn emit_data_section(&self, out: &mut impl Write) {
        writeln!(out, ".section .data").unwrap();
        for (label, value) in &self.module.string_constants {
            writeln!(out, "{}: .asciz {}", label, value).unwrap();
        }
        for var in &self.module.static_vars {
            writeln!(out, "{}: .quad 0", var).unwrap();
        }
        writeln!(out, ".Lfmt_int: .asciz \"%ld\"").unwrap();
        writeln!(out, ".Lfmt_str: .asciz \"%s\"").unwrap();
        writeln!(out, ".Lfmt_nl: .asciz \"\\n\"").unwrap();
        writeln!(out).unwrap();
    }

    // ========================================================================
    // Top-level generation
    // ========================================================================

    pub fn generate(&mut self, out: &mut impl Write) {
        self.emit_data_section(out);
        writeln!(out, ".section .text").unwrap();
        writeln!(out, ".globl main").unwrap();
        writeln!(out).unwrap();

        // Emit __str_concat helper (rdi=str1, rsi=str2, returns rax=new string)
        writeln!(out, "__str_concat:").unwrap();
        writeln!(out, "    pushq %rbp").unwrap();
        writeln!(out, "    movq %rsp, %rbp").unwrap();
        writeln!(out, "    pushq %rbx").unwrap();
        writeln!(out, "    pushq %r12").unwrap();
        writeln!(out, "    pushq %r13").unwrap();
        writeln!(out, "    subq $8, %rsp").unwrap(); // 16-byte align
        writeln!(out, "    movq %rdi, %r12").unwrap(); // r12 = str1
        writeln!(out, "    movq %rsi, %r13").unwrap(); // r13 = str2
        writeln!(out, "    movq %r12, %rdi").unwrap();
        writeln!(out, "    call strlen").unwrap();
        writeln!(out, "    movq %rax, %rbx").unwrap(); // rbx = len1
        writeln!(out, "    movq %r13, %rdi").unwrap();
        writeln!(out, "    call strlen").unwrap();
        writeln!(out, "    addq %rbx, %rax").unwrap(); // rax = len1+len2
        writeln!(out, "    addq $1, %rax").unwrap(); // +1 for null terminator
        writeln!(out, "    movq %rax, %rdi").unwrap();
        writeln!(out, "    call malloc").unwrap();
        writeln!(out, "    movq %rax, %rbx").unwrap(); // rbx = new buffer
        writeln!(out, "    movq %rbx, %rdi").unwrap();
        writeln!(out, "    movq %r12, %rsi").unwrap();
        writeln!(out, "    call strcpy").unwrap();
        writeln!(out, "    movq %rbx, %rdi").unwrap();
        writeln!(out, "    movq %r13, %rsi").unwrap();
        writeln!(out, "    call strcat").unwrap();
        writeln!(out, "    movq %rbx, %rax").unwrap(); // return new string
        writeln!(out, "    addq $8, %rsp").unwrap();
        writeln!(out, "    popq %r13").unwrap();
        writeln!(out, "    popq %r12").unwrap();
        writeln!(out, "    popq %rbx").unwrap();
        writeln!(out, "    popq %rbp").unwrap();
        writeln!(out, "    ret").unwrap();
        writeln!(out).unwrap();

        for i in 0..self.module.functions.len() {
            self.emit_function(out, i);
        }
    }

    // ========================================================================
    // Function emission
    // ========================================================================

    fn emit_function(&mut self, out: &mut impl Write, func_idx: usize) {
        self.func = Some(&self.module.functions[func_idx]);
        self.alloc = Some(self.allocators[func_idx]);

        let alloc = self.alloc.unwrap();
        let func = self.func.unwrap();

        // Calculate stack frame size
        let callee_save = alloc.used_callee_save();
        let num_callee_save = callee_save.len() as i32;
        let num_spills = alloc.total_spill_slots();

        // spillBase_ = offset from rbp where spills start (after callee-save pushes)
        self.spill_base = num_callee_save * 8;

        // Alignment: after push rbp (8 bytes) + return address (8 bytes) = 16-byte aligned
        // Total pushes: 1 (rbp) + numCalleeSave
        // Stack must be 16-aligned before call, so after prologue:
        // (1 + numCalleeSave) * 8 + allocSize must be 16-aligned relative to entry
        let total_pushed = (1 + num_callee_save) * 8; // rbp + callee-saves
        let mut alloc_size = num_spills * 8;
        if (total_pushed + alloc_size) % 16 != 0 {
            alloc_size += 8;
        }
        self.stack_frame_size = self.spill_base + alloc_size;

        writeln!(out, "{}:", func.name).unwrap();
        self.emit_prologue(out);

        for block in &func.blocks {
            self.emit_block(out, block);
        }

        writeln!(out).unwrap();
    }

    // ========================================================================
    // Prologue / Epilogue
    // ========================================================================

    fn emit_prologue(&self, out: &mut impl Write) {
        let func = self.func.unwrap();
        let alloc = self.alloc.unwrap();

        writeln!(out, "    pushq %rbp").unwrap();
        writeln!(out, "    movq %rsp, %rbp").unwrap();

        // Push callee-save registers
        let callee_save = alloc.used_callee_save();
        for &r in callee_save.iter() {
            writeln!(out, "    pushq {}", reg_name_64(r)).unwrap();
        }

        // Allocate space for spills
        let alloc_size = self.stack_frame_size - self.spill_base;
        if alloc_size > 0 {
            writeln!(out, "    subq ${}, %rsp", alloc_size).unwrap();
        }

        // Move parameters from argument registers to their allocated locations
        let param_regs = [Reg::RDI, Reg::RSI, Reg::RDX, Reg::RCX, Reg::R8, Reg::R9];
        for i in 0..std::cmp::min(func.num_params as usize, 6) {
            if i < func.values.len() {
                let src = param_regs[i];
                let dst = alloc.get_register(i as i32);
                if dst != Reg::NONE && dst != src {
                    writeln!(
                        out,
                        "    movq {}, {}",
                        reg_name_64(src),
                        reg_name_64(dst)
                    )
                    .unwrap();
                } else if dst == Reg::NONE {
                    // Spilled parameter
                    self.store_from(out, src, i as i32);
                }
            }
        }
    }

    fn emit_epilogue(&self, out: &mut impl Write) {
        let alloc = self.alloc.unwrap();

        let alloc_size = self.stack_frame_size - self.spill_base;
        if alloc_size > 0 {
            writeln!(out, "    addq ${}, %rsp", alloc_size).unwrap();
        }

        // Restore callee-save registers (in reverse order)
        let callee_save = alloc.used_callee_save();
        let saves: Vec<Reg> = callee_save.iter().copied().collect();
        for i in (0..saves.len()).rev() {
            writeln!(out, "    popq {}", reg_name_64(saves[i])).unwrap();
        }

        writeln!(out, "    popq %rbp").unwrap();
        writeln!(out, "    ret").unwrap();
    }

    // ========================================================================
    // PHI resolution
    // ========================================================================

    fn emit_phi_moves(&self, out: &mut impl Write, from_block: i32, to_block: i32) {
        let func = self.func.unwrap();
        let alloc = self.alloc.unwrap();

        for instr in &func.blocks[to_block as usize].instrs {
            if instr.op != IROpcode::Phi {
                break; // PHIs are always at block start
            }
            // Find which operand corresponds to from_block
            for i in 0..instr.phi_blocks.len() {
                if instr.phi_blocks[i] == from_block && i < instr.operands.len() {
                    let src_ssa = instr.operands[i];
                    let dst_ssa = instr.dst;
                    // Move src -> dst if they're in different locations
                    let src_reg = alloc.get_register(src_ssa);
                    let dst_reg = alloc.get_register(dst_ssa);
                    if src_reg != Reg::NONE && dst_reg != Reg::NONE {
                        if src_reg != dst_reg {
                            writeln!(
                                out,
                                "    movq {}, {}",
                                reg_name_64(src_reg),
                                reg_name_64(dst_reg)
                            )
                            .unwrap();
                        }
                    } else {
                        self.load_into(out, src_ssa, Reg::RAX);
                        self.store_from(out, Reg::RAX, dst_ssa);
                    }
                    break;
                }
            }
        }
    }

    // ========================================================================
    // Block emission
    // ========================================================================

    fn emit_block(&self, out: &mut impl Write, block: &BasicBlock) {
        let func = self.func.unwrap();
        if block.id != 0 {
            // Don't emit label for entry block (it follows function label)
            writeln!(out, ".L{}_{}:", func.name, block.label).unwrap();
        }

        for instr in &block.instrs {
            self.emit_instr(out, instr, block.id);
        }
    }

    // ========================================================================
    // Instruction emission
    // ========================================================================

    fn emit_instr(&self, out: &mut impl Write, instr: &IRInstr, block_id: i32) {
        let func = self.func.unwrap();
        let alloc = self.alloc.unwrap();

        match instr.op {
            IROpcode::ConstInt => {
                let dst = alloc.get_register(instr.dst);
                if dst != Reg::NONE {
                    writeln!(out, "    movq ${}, {}", instr.imm, reg_name_64(dst)).unwrap();
                } else {
                    writeln!(out, "    movq ${}, %rax", instr.imm).unwrap();
                    self.store_from(out, Reg::RAX, instr.dst);
                }
            }

            IROpcode::ConstBool => {
                let dst = alloc.get_register(instr.dst);
                if dst != Reg::NONE {
                    writeln!(out, "    movq ${}, {}", instr.imm, reg_name_64(dst)).unwrap();
                } else {
                    writeln!(out, "    movq ${}, %rax", instr.imm).unwrap();
                    self.store_from(out, Reg::RAX, instr.dst);
                }
            }

            IROpcode::ConstStr => {
                let dst = alloc.get_register(instr.dst);
                if dst != Reg::NONE {
                    writeln!(
                        out,
                        "    leaq {}(%rip), {}",
                        instr.label,
                        reg_name_64(dst)
                    )
                    .unwrap();
                } else {
                    writeln!(out, "    leaq {}(%rip), %rax", instr.label).unwrap();
                    self.store_from(out, Reg::RAX, instr.dst);
                }
            }

            IROpcode::Add | IROpcode::Sub | IROpcode::Mul => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.load_into(out, instr.operands[1], Reg::RCX);
                match instr.op {
                    IROpcode::Add => writeln!(out, "    addq %rcx, %rax").unwrap(),
                    IROpcode::Sub => writeln!(out, "    subq %rcx, %rax").unwrap(),
                    IROpcode::Mul => writeln!(out, "    imulq %rcx, %rax").unwrap(),
                    _ => unreachable!(),
                }
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::Div => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.load_into(out, instr.operands[1], Reg::RCX);
                writeln!(out, "    cqo").unwrap();
                writeln!(out, "    idivq %rcx").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::Neg => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                writeln!(out, "    negq %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::CmpLt
            | IROpcode::CmpGt
            | IROpcode::CmpLe
            | IROpcode::CmpGe
            | IROpcode::CmpEq
            | IROpcode::CmpNe => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.load_into(out, instr.operands[1], Reg::RCX);
                writeln!(out, "    cmpq %rcx, %rax").unwrap();
                let set_instr = match instr.op {
                    IROpcode::CmpLt => "setl",
                    IROpcode::CmpGt => "setg",
                    IROpcode::CmpLe => "setle",
                    IROpcode::CmpGe => "setge",
                    IROpcode::CmpEq => "sete",
                    IROpcode::CmpNe => "setne",
                    _ => unreachable!(),
                };
                writeln!(out, "    {} %al", set_instr).unwrap();
                writeln!(out, "    movzbq %al, %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::And => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.load_into(out, instr.operands[1], Reg::RCX);
                writeln!(out, "    andq %rcx, %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::Or => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.load_into(out, instr.operands[1], Reg::RCX);
                writeln!(out, "    orq %rcx, %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::Not => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                writeln!(out, "    xorq $1, %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::Copy => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::LoadStatic => {
                writeln!(out, "    movq {}(%rip), %rax", instr.label).unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::StoreStatic => {
                self.load_into(out, instr.operands[0], Reg::RAX);
                writeln!(out, "    movq %rax, {}(%rip)", instr.label).unwrap();
            }

            IROpcode::AllocArray => {
                // operands[0] = size
                self.load_into(out, instr.operands[0], Reg::RDI);
                // Allocate (size+1)*8 bytes: [length | elem0 | elem1 | ...]
                writeln!(out, "    addq $1, %rdi").unwrap();
                writeln!(out, "    shlq $3, %rdi").unwrap(); // *8
                // Save size on stack temporarily before call
                writeln!(out, "    pushq %rdi").unwrap();
                // Align stack for call
                writeln!(out, "    subq $8, %rsp").unwrap();
                // Reload size for malloc
                writeln!(out, "    movq 8(%rsp), %rdi").unwrap();
                writeln!(out, "    call malloc").unwrap();
                writeln!(out, "    addq $8, %rsp").unwrap();
                writeln!(out, "    popq %rcx").unwrap(); // restore (size+1)*8
                // rcx has (size+1)*8, so size = rcx/8 - 1
                writeln!(out, "    shrq $3, %rcx").unwrap();
                writeln!(out, "    decq %rcx").unwrap();
                // Store length at [rax]
                writeln!(out, "    movq %rcx, (%rax)").unwrap();
                // Return pointer past the length header
                writeln!(out, "    addq $8, %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::ArrayLoad => {
                // operands[0] = base, operands[1] = index
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.load_into(out, instr.operands[1], Reg::RCX);
                writeln!(out, "    movq (%rax,%rcx,8), %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::ArrayStore => {
                // operands[0] = base, operands[1] = index, operands[2] = value
                self.load_into(out, instr.operands[0], Reg::RAX);
                self.load_into(out, instr.operands[1], Reg::RCX);
                self.load_into(out, instr.operands[2], Reg::RDX);
                writeln!(out, "    movq %rdx, (%rax,%rcx,8)").unwrap();
            }

            IROpcode::ArrayLength => {
                // Length is stored at [base - 8]
                self.load_into(out, instr.operands[0], Reg::RAX);
                writeln!(out, "    movq -8(%rax), %rax").unwrap();
                self.store_from(out, Reg::RAX, instr.dst);
            }

            IROpcode::Branch => {
                self.emit_phi_moves(out, block_id, instr.target_block);
                writeln!(
                    out,
                    "    jmp .L{}_{}",
                    func.name, func.blocks[instr.target_block as usize].label
                )
                .unwrap();
            }

            IROpcode::Cbranch => {
                // For CBRANCH, we need to emit PHI moves for both targets.
                // Since PHI moves happen before the branch, and the two targets
                // may need different moves, we split into explicit branches.
                self.load_into(out, instr.operands[0], Reg::RAX);
                writeln!(out, "    testq %rax, %rax").unwrap();

                // Check if either target has PHI nodes
                let true_block = &func.blocks[instr.true_block as usize];
                let false_block = &func.blocks[instr.false_block as usize];
                let true_has_phi = !true_block.instrs.is_empty()
                    && true_block.instrs[0].op == IROpcode::Phi;
                let false_has_phi = !false_block.instrs.is_empty()
                    && false_block.instrs[0].op == IROpcode::Phi;

                if !true_has_phi && !false_has_phi {
                    // Simple case: no PHI resolution needed
                    writeln!(
                        out,
                        "    jne .L{}_{}",
                        func.name, true_block.label
                    )
                    .unwrap();
                    writeln!(
                        out,
                        "    jmp .L{}_{}",
                        func.name, false_block.label
                    )
                    .unwrap();
                } else {
                    // Need PHI resolution: use explicit branches
                    let true_label =
                        format!(".L{}_{}", func.name, true_block.label);
                    let false_label =
                        format!(".L{}_{}", func.name, false_block.label);
                    let false_phi_label =
                        format!(".L{}_phi_false_{}", func.name, block_id);

                    writeln!(out, "    je {}", false_phi_label).unwrap();
                    self.emit_phi_moves(out, block_id, instr.true_block);
                    writeln!(out, "    jmp {}", true_label).unwrap();
                    writeln!(out, "{}:", false_phi_label).unwrap();
                    self.emit_phi_moves(out, block_id, instr.false_block);
                    writeln!(out, "    jmp {}", false_label).unwrap();
                }
            }

            IROpcode::Phi => {
                // PHI nodes are resolved at predecessor block boundaries.
                // No code emitted here - emit_phi_moves handles it.
            }

            IROpcode::Param => {
                // Move argument to the appropriate register
                let arg_idx = instr.imm as usize;
                if arg_idx < 6 && !instr.operands.is_empty() {
                    let target = match arg_idx {
                        0 => Reg::RDI,
                        1 => Reg::RSI,
                        2 => Reg::RDX,
                        3 => Reg::RCX,
                        4 => Reg::R8,
                        5 => Reg::R9,
                        _ => Reg::NONE,
                    };
                    self.load_into(out, instr.operands[0], target);
                }
            }

            IROpcode::Call => {
                // Arguments should already be in place (emitted by PARAM instructions)
                // For variadic functions (printf), zero rax
                if instr.label == "printf"
                    || instr.label == "atoi"
                    || instr.label == "malloc"
                    || instr.label == "__str_concat"
                {
                    writeln!(out, "    xorl %eax, %eax").unwrap();
                }

                // Setup args for simple calls (that don't use separate PARAM instructions)
                if instr.label == "atoi" && !instr.operands.is_empty() {
                    self.load_into(out, instr.operands[0], Reg::RDI);
                }

                writeln!(out, "    call {}", instr.label).unwrap();

                if instr.dst >= 0 {
                    self.store_from(out, Reg::RAX, instr.dst);
                }
            }

            IROpcode::ReturnVal => {
                if !instr.operands.is_empty() {
                    self.load_into(out, instr.operands[0], Reg::RAX);
                }
                self.emit_epilogue(out);
            }

            IROpcode::PrintInt => {
                // printf("%ld", value)
                self.load_into(out, instr.operands[0], Reg::RSI);
                writeln!(out, "    leaq .Lfmt_int(%rip), %rdi").unwrap();
                writeln!(out, "    xorl %eax, %eax").unwrap();
                writeln!(out, "    call printf").unwrap();
            }

            IROpcode::PrintStr => {
                self.load_into(out, instr.operands[0], Reg::RSI);
                writeln!(out, "    leaq .Lfmt_str(%rip), %rdi").unwrap();
                writeln!(out, "    xorl %eax, %eax").unwrap();
                writeln!(out, "    call printf").unwrap();
            }

            IROpcode::PrintNewline => {
                writeln!(out, "    leaq .Lfmt_nl(%rip), %rdi").unwrap();
                writeln!(out, "    xorl %eax, %eax").unwrap();
                writeln!(out, "    call printf").unwrap();
            }

            IROpcode::Nop => {
                // No operation
            }
        }
    }
}
