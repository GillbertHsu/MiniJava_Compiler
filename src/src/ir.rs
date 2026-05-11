use std::collections::{HashMap, HashSet};

use crate::ast::{ASTNode, DataType, NodeType};
use crate::symtab::{MethodTable, SymbolInfo, SymbolTableArena};

// ============================================================================
// IR types
// ============================================================================

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum IROpcode {
    ConstInt,
    ConstBool,
    ConstStr,
    Add,
    Sub,
    Mul,
    Div,
    Neg,
    CmpLt,
    CmpGt,
    CmpLe,
    CmpGe,
    CmpEq,
    CmpNe,
    And,
    Or,
    Not,
    Copy,
    LoadStatic,
    StoreStatic,
    AllocArray,
    ArrayLoad,
    ArrayStore,
    ArrayLength,
    Branch,
    Cbranch,
    Phi,
    Param,
    Call,
    ReturnVal,
    PrintInt,
    PrintStr,
    PrintNewline,
    Nop,
}

#[derive(Debug, Clone)]
pub struct IRInstr {
    pub op: IROpcode,
    pub dst: i32,
    pub operands: Vec<i32>,
    pub imm: i64,
    pub label: String,
    pub phi_blocks: Vec<i32>,
    pub true_block: i32,
    pub false_block: i32,
    pub target_block: i32,
}

impl Default for IRInstr {
    fn default() -> Self {
        IRInstr {
            op: IROpcode::Nop,
            dst: -1,
            operands: Vec::new(),
            imm: 0,
            label: String::new(),
            phi_blocks: Vec::new(),
            true_block: -1,
            false_block: -1,
            target_block: -1,
        }
    }
}

#[derive(Debug, Clone)]
pub struct BasicBlock {
    pub id: i32,
    pub label: String,
    pub instrs: Vec<IRInstr>,
    pub preds: Vec<i32>,
    pub succs: Vec<i32>,
}

impl Default for BasicBlock {
    fn default() -> Self {
        BasicBlock {
            id: -1,
            label: String::new(),
            instrs: Vec::new(),
            preds: Vec::new(),
            succs: Vec::new(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct SSAValue {
    pub id: i32,
    pub name: String,
    pub data_type: DataType,
    pub is_spilled: bool,
}

impl Default for SSAValue {
    fn default() -> Self {
        SSAValue {
            id: -1,
            name: String::new(),
            data_type: DataType::Undefined,
            is_spilled: false,
        }
    }
}

#[derive(Debug, Clone)]
pub struct IRFunction {
    pub name: String,
    pub return_type: DataType,
    pub param_names: Vec<String>,
    pub param_types: Vec<DataType>,
    pub num_params: i32,
    pub blocks: Vec<BasicBlock>,
    pub values: Vec<SSAValue>,
    pub entry_block: i32,
}

impl IRFunction {
    pub fn new() -> Self {
        IRFunction {
            name: String::new(),
            return_type: DataType::Undefined,
            param_names: Vec::new(),
            param_types: Vec::new(),
            num_params: 0,
            blocks: Vec::new(),
            values: Vec::new(),
            entry_block: 0,
        }
    }

    pub fn new_value(&mut self, name: &str, data_type: DataType) -> i32 {
        let id = self.values.len() as i32;
        self.values.push(SSAValue {
            id,
            name: name.to_string(),
            data_type,
            is_spilled: false,
        });
        id
    }

    pub fn new_block(&mut self, label: &str) -> i32 {
        let id = self.blocks.len() as i32;
        self.blocks.push(BasicBlock {
            id,
            label: format!("{}_{}", label, id),
            instrs: Vec::new(),
            preds: Vec::new(),
            succs: Vec::new(),
        });
        id
    }

    pub fn add_succ(&mut self, from: i32, to: i32) {
        self.blocks[from as usize].succs.push(to);
        self.blocks[to as usize].preds.push(from);
    }
}

#[derive(Debug, Clone)]
pub struct IRModule {
    pub functions: Vec<IRFunction>,
    pub string_constants: Vec<(String, String)>,
    pub static_vars: Vec<String>,
    pub string_counter: i32,
}

impl IRModule {
    pub fn new() -> Self {
        IRModule {
            functions: Vec::new(),
            string_constants: Vec::new(),
            static_vars: Vec::new(),
            string_counter: 0,
        }
    }
}

// ============================================================================
// Standalone helper: collect variable names that are assigned in a subtree
// ============================================================================

fn collect_modified_vars(node: &ASTNode, vars: &mut HashSet<String>) {
    if node.node_type == NodeType::Assign && !node.children.is_empty() {
        let lv = &node.children[0];
        if !lv.str_val.is_empty() {
            vars.insert(lv.str_val.clone());
        }
    }
    if node.node_type == NodeType::VarDecl && !node.str_val.is_empty() {
        vars.insert(node.str_val.clone());
    }
    for child in &node.children {
        collect_modified_vars(child, vars);
    }
}

// ============================================================================
// IRGenerator
// ============================================================================

pub struct IRGenerator<'a> {
    module: IRModule,
    methods: &'a MethodTable,
    scopes: &'a SymbolTableArena,
    cur_func_idx: usize,
    cur_block: i32,
    temp_counter: i32,
    var_map: HashMap<String, i32>,
}

impl<'a> IRGenerator<'a> {
    /// Top-level entry point: generate IR from a parsed + semantically-checked AST.
    pub fn generate(
        root: &ASTNode,
        methods: &'a MethodTable,
        scopes: &'a SymbolTableArena,
    ) -> IRModule {
        let mut gen = IRGenerator {
            module: IRModule::new(),
            methods,
            scopes,
            cur_func_idx: 0,
            cur_block: -1,
            temp_counter: 0,
            var_map: HashMap::new(),
        };

        // Walk the AST to find main class
        if root.children.is_empty() {
            return gen.module;
        }
        let main_class = &root.children[0];

        // Collect static variable declarations and their child indices
        // We also record which children are static var decl nodes so we can
        // reference them later when generating main.
        struct StaticVarRef {
            svdl_idx: usize, // index into main_class.children (StaticVarDeclList)
            svd_idx: usize,  // index into that list's children (StaticVarDecl)
        }
        let mut static_var_refs: Vec<StaticVarRef> = Vec::new();

        for (ci, child) in main_class.children.iter().enumerate() {
            if child.node_type == NodeType::StaticVarDeclList {
                for (si, svd) in child.children.iter().enumerate() {
                    if svd.node_type == NodeType::StaticVarDecl && !svd.children.is_empty() {
                        let var_decl = &svd.children[0];
                        gen.module.static_vars.push(var_decl.str_val.clone());
                        static_var_refs.push(StaticVarRef {
                            svdl_idx: ci,
                            svd_idx: si,
                        });
                        // Also add additional variables from multi-declarations
                        for k in 1..var_decl.children.len() {
                            let extra = &var_decl.children[k];
                            if extra.node_type == NodeType::IdExpList {
                                gen.module.static_vars.push(extra.str_val.clone());
                            }
                        }
                    }
                }
            }
        }

        // Generate methods first, then main
        for child in &main_class.children {
            if child.node_type == NodeType::StaticMethodDeclList {
                for method in &child.children {
                    gen.gen_function(method, false, &[]);
                }
            }
        }

        // Collect the actual VarDecl AST node references for static var init in main.
        // We pass them by reference via a slice of references.
        let mut static_var_decl_nodes: Vec<&ASTNode> = Vec::new();
        for r in &static_var_refs {
            let svdl = &main_class.children[r.svdl_idx];
            let svd = &svdl.children[r.svd_idx];
            if !svd.children.is_empty() {
                static_var_decl_nodes.push(&svd.children[0]);
            }
        }

        for child in &main_class.children {
            if child.node_type == NodeType::MainMethod {
                gen.gen_function(child, true, &static_var_decl_nodes);
            }
        }

        gen.module
    }

    // -- helpers --------------------------------------------------------------

    fn cur_func(&self) -> &IRFunction {
        &self.module.functions[self.cur_func_idx]
    }

    fn cur_func_mut(&mut self) -> &mut IRFunction {
        &mut self.module.functions[self.cur_func_idx]
    }

    fn fresh_val(&mut self, data_type: DataType) -> i32 {
        let name = format!("t{}", self.temp_counter);
        self.temp_counter += 1;
        self.cur_func_mut().new_value(&name, data_type)
    }

    fn emit(
        &mut self,
        op: IROpcode,
        dst: i32,
        operands: Vec<i32>,
        imm: i64,
        label: &str,
    ) -> i32 {
        let instr = IRInstr {
            op,
            dst,
            operands,
            imm,
            label: label.to_string(),
            ..Default::default()
        };
        let blk = self.cur_block as usize;
        self.module.functions[self.cur_func_idx].blocks[blk]
            .instrs
            .push(instr);
        dst
    }

    fn emit_const(&mut self, val: i64) -> i32 {
        let dst = self.fresh_val(DataType::Int);
        self.emit(IROpcode::ConstInt, dst, vec![], val, "");
        dst
    }

    fn emit_const_bool(&mut self, val: bool) -> i32 {
        let dst = self.fresh_val(DataType::Boolean);
        self.emit(
            IROpcode::ConstBool,
            dst,
            vec![],
            if val { 1 } else { 0 },
            "",
        );
        dst
    }

    fn emit_copy(&mut self, src: i32) -> i32 {
        let src_type = self.module.functions[self.cur_func_idx].values[src as usize].data_type;
        let dst = self.fresh_val(src_type);
        self.emit(IROpcode::Copy, dst, vec![src], 0, "");
        dst
    }

    fn write_var(&mut self, name: &str, ssa_val: i32) {
        self.var_map.insert(name.to_string(), ssa_val);
    }

    fn read_var(&mut self, name: &str) -> i32 {
        if let Some(&val) = self.var_map.get(name) {
            return val;
        }
        // Variable not yet defined in this path - return a zero
        let v = self.emit_const(0);
        self.var_map.insert(name.to_string(), v);
        v
    }

    fn unwrap_exp<'b>(&self, node: &'b ASTNode) -> &'b ASTNode {
        if node.node_type == NodeType::Exp && !node.children.is_empty() {
            return self.unwrap_exp(&node.children[0]);
        }
        node
    }

    fn find_left_value_info<'b>(&'b self, node: &ASTNode) -> Option<&'b SymbolInfo> {
        match node.node_type {
            NodeType::LeftvalueId | NodeType::LeftvalueOneArr | NodeType::LeftvalueTwoArr => {
                if let Some(scope_idx) = node.scope {
                    if let Some(info) = self.scopes.lookup(scope_idx, &node.str_val) {
                        return Some(info);
                    }
                }
                self.methods.lookup(&node.str_val)
            }
            NodeType::VarDecl | NodeType::IdExpList => {
                if let Some(scope_idx) = node.scope {
                    if let Some(info) = self.scopes.lookup(scope_idx, &node.str_val) {
                        return Some(info);
                    }
                }
                None
            }
            NodeType::MethodCall | NodeType::CallMain => self.methods.lookup(&node.str_val),
            NodeType::ExpMethodCall => {
                if !node.children.is_empty() {
                    self.find_left_value_info(&node.children[0])
                } else {
                    None
                }
            }
            _ => None,
        }
    }

    fn find_exp_type(&self, root: &ASTNode) -> DataType {
        let root = self.unwrap_exp(root);
        match root.node_type {
            NodeType::ExpType => root.data_type,
            NodeType::ExpLength => DataType::Int,
            NodeType::ExpLeft => {
                if !root.children.is_empty() {
                    if let Some(info) = self.find_left_value_info(&root.children[0]) {
                        return info.data_type;
                    }
                }
                DataType::Undefined
            }
            NodeType::ExpMethodCall => {
                if !root.children.is_empty() {
                    if root.children[0].node_type == NodeType::ParseInt {
                        return DataType::Int;
                    }
                    if let Some(info) = self.find_left_value_info(&root.children[0]) {
                        return info.data_type;
                    }
                }
                DataType::Undefined
            }
            NodeType::ExpPlus => {
                if !root.children.is_empty() {
                    return self.find_exp_type(&root.children[0]);
                }
                DataType::Undefined
            }
            NodeType::ExpOperation => DataType::Int,
            NodeType::ExpOperationComp | NodeType::ExpComp | NodeType::ExpLogic => {
                DataType::Boolean
            }
            NodeType::ExpNeg | NodeType::ExpPos => DataType::Int,
            NodeType::ExpLogicNot => DataType::Boolean,
            _ => {
                if !root.children.is_empty() {
                    self.find_exp_type(&root.children[0])
                } else {
                    DataType::Undefined
                }
            }
        }
    }

    // -- code generation ------------------------------------------------------

    fn gen_function(
        &mut self,
        method_decl: &ASTNode,
        is_main: bool,
        static_var_decls: &[&ASTNode],
    ) {
        self.module.functions.push(IRFunction::new());
        self.cur_func_idx = self.module.functions.len() - 1;
        self.temp_counter = 0;
        self.var_map.clear();

        if is_main {
            self.cur_func_mut().name = "main".to_string();
            self.cur_func_mut().return_type = DataType::Int;
            self.cur_func_mut().num_params = 2; // argc, argv
        } else {
            self.cur_func_mut().name = method_decl.str_val.clone();
            // First child is return type, second is formal list, third is body
            if !method_decl.children.is_empty() {
                let type_node = &method_decl.children[0];
                let ret_type = if type_node.node_type == NodeType::PrimeType {
                    type_node.data_type
                } else if !type_node.children.is_empty() {
                    type_node.children[0].data_type
                } else {
                    DataType::Undefined
                };
                self.cur_func_mut().return_type = ret_type;
            }
            // Parse formal parameters
            if method_decl.children.len() > 1 {
                let mut formal: Option<&ASTNode> = Some(&method_decl.children[1]);
                while let Some(f) = formal {
                    if f.node_type != NodeType::FormalList
                        && f.node_type != NodeType::TypeIdList
                    {
                        break;
                    }
                    let p_type = if !f.children.is_empty() {
                        f.children[0].data_type
                    } else {
                        DataType::Undefined
                    };
                    let func = &mut self.module.functions[self.cur_func_idx];
                    func.param_names.push(f.str_val.clone());
                    func.param_types.push(p_type);
                    func.num_params += 1;
                    if f.children.len() > 1 {
                        formal = Some(&f.children[1]);
                    } else {
                        formal = None;
                    }
                }
            }
        }

        self.cur_block = self.cur_func_mut().new_block("entry");

        // Create SSA values for parameters
        if is_main {
            // main gets argc in rdi, argv in rsi
            let argc_val = self.cur_func_mut().new_value("argc", DataType::Int);
            let argv_val = self.cur_func_mut().new_value("argv", DataType::String);
            self.write_var("__argc", argc_val);
            self.write_var("__argv", argv_val);
            // args = argv + 8 (skip argv[0])
            let eight = self.emit_const(8);
            let args_val = self.fresh_val(DataType::String);
            self.emit(IROpcode::Add, args_val, vec![argv_val, eight], 0, "");
            self.write_var("args", args_val);
        } else {
            let num = self.cur_func().param_names.len();
            for i in 0..num {
                let pname = self.module.functions[self.cur_func_idx].param_names[i].clone();
                let ptype = self.module.functions[self.cur_func_idx].param_types[i];
                let p_val = self.cur_func_mut().new_value(&pname, ptype);
                self.write_var(&pname, p_val);
            }
        }

        // Initialize static variables at the start of main
        if is_main {
            for svd in static_var_decls {
                self.gen_var_decl(svd);
            }
        }

        // Generate body
        if is_main {
            if !method_decl.children.is_empty() {
                let body = &method_decl.children[0];
                self.gen_statement_list(body);
            }
        } else {
            // Body is the last child (statement list)
            if method_decl.children.len() > 2 {
                let body = &method_decl.children[2];
                self.gen_statement_list(body);
            } else if method_decl.children.len() > 1 {
                let body = &method_decl.children[1];
                self.gen_statement_list(body);
            }
        }

        // If main, add return 0
        if is_main {
            let zero = self.emit_const(0);
            self.emit(IROpcode::ReturnVal, -1, vec![zero], 0, "");
        }
    }

    fn gen_statement_list(&mut self, stmt_list: &ASTNode) {
        for child in &stmt_list.children {
            self.gen_statement(child);
        }
    }

    fn gen_statement(&mut self, stmt: &ASTNode) {
        match stmt.node_type {
            NodeType::StatementList => {
                self.gen_statement_list(stmt);
            }
            NodeType::Statement => {
                for c in &stmt.children {
                    self.gen_statement(c);
                }
            }
            NodeType::VarDecl => {
                self.gen_var_decl(stmt);
            }
            NodeType::Assign => {
                self.gen_assign(stmt);
            }
            NodeType::If => {
                self.gen_if(stmt);
            }
            NodeType::While => {
                self.gen_while(stmt);
            }
            NodeType::Return => {
                self.gen_return(stmt);
            }
            NodeType::Println => {
                self.gen_print(stmt, true);
            }
            NodeType::Print => {
                self.gen_print(stmt, false);
            }
            NodeType::StatementMethodCall | NodeType::ExpMethodCall => {
                self.gen_method_call(stmt);
            }
            NodeType::StaticVarDecl => {
                if !stmt.children.is_empty() {
                    self.gen_var_decl(&stmt.children[0]);
                }
            }
            _ => {
                for c in &stmt.children {
                    self.gen_statement(c);
                }
            }
        }
    }

    fn gen_expression(&mut self, expr: &ASTNode) -> i32 {
        let expr = self.unwrap_exp_owned(expr);
        match expr.node_type {
            NodeType::ExpType => {
                if expr.data_type == DataType::Int {
                    return self.emit_const(expr.int_val as i64);
                }
                if expr.data_type == DataType::Boolean {
                    return self.emit_const_bool(expr.bool_val);
                }
                if expr.data_type == DataType::String {
                    let lbl = format!(".Lstr{}", self.module.string_counter);
                    self.module.string_counter += 1;
                    self.module
                        .string_constants
                        .push((lbl.clone(), expr.str_val.clone()));
                    let dst = self.fresh_val(DataType::String);
                    self.emit(IROpcode::ConstStr, dst, vec![], 0, &lbl);
                    return dst;
                }
                self.emit_const(0)
            }
            NodeType::ExpLeft => {
                if expr.children.is_empty() {
                    return self.emit_const(0);
                }
                let lv = &*expr.children[0];
                if lv.node_type == NodeType::LeftvalueId {
                    let info = self.find_left_value_info(lv);
                    if let Some(info) = info {
                        if info.is_static {
                            let dt = info.data_type;
                            let name = lv.str_val.clone();
                            let dst = self.fresh_val(dt);
                            self.emit(IROpcode::LoadStatic, dst, vec![], 0, &name);
                            return dst;
                        }
                    }
                    let name = lv.str_val.clone();
                    return self.read_var(&name);
                }
                if lv.node_type == NodeType::LeftvalueOneArr
                    || lv.node_type == NodeType::LeftvalueTwoArr
                {
                    return self.gen_array_access(lv);
                }
                self.emit_const(0)
            }
            NodeType::ExpPlus => {
                let left = self.gen_expression(&expr.children[0]);
                let right = self.gen_expression(&expr.children[1]);
                let lt = self.find_exp_type(&expr.children[0]);
                if lt == DataType::String {
                    // String concatenation via __str_concat(str1, str2)
                    self.emit(IROpcode::Param, -1, vec![left], 0, "");
                    self.emit(IROpcode::Param, -1, vec![right], 1, "");
                    let dst = self.fresh_val(DataType::String);
                    self.emit(
                        IROpcode::Call,
                        dst,
                        vec![left, right],
                        0,
                        "__str_concat",
                    );
                    return dst;
                }
                let dst = self.fresh_val(DataType::Int);
                self.emit(IROpcode::Add, dst, vec![left, right], 0, "");
                dst
            }
            NodeType::ExpOperation => {
                let left = self.gen_expression(&expr.children[0]);
                let right = self.gen_expression(&expr.children[1]);
                let dst = self.fresh_val(DataType::Int);
                let op = match expr.str_val.as_str() {
                    "*" => IROpcode::Mul,
                    "/" => IROpcode::Div,
                    _ => IROpcode::Sub, // "-" or default
                };
                self.emit(op, dst, vec![left, right], 0, "");
                dst
            }
            NodeType::ExpOperationComp => {
                let left = self.gen_expression(&expr.children[0]);
                let right = self.gen_expression(&expr.children[1]);
                let dst = self.fresh_val(DataType::Boolean);
                let op = match expr.str_val.as_str() {
                    ">" => IROpcode::CmpGt,
                    "<=" => IROpcode::CmpLe,
                    ">=" => IROpcode::CmpGe,
                    _ => IROpcode::CmpLt, // "<" or default
                };
                self.emit(op, dst, vec![left, right], 0, "");
                dst
            }
            NodeType::ExpComp => {
                let left = self.gen_expression(&expr.children[0]);
                let right = self.gen_expression(&expr.children[1]);
                let dst = self.fresh_val(DataType::Boolean);
                let op = if expr.str_val == "==" {
                    IROpcode::CmpEq
                } else {
                    IROpcode::CmpNe
                };
                self.emit(op, dst, vec![left, right], 0, "");
                dst
            }
            NodeType::ExpLogic => {
                let left = self.gen_expression(&expr.children[0]);
                let right = self.gen_expression(&expr.children[1]);
                let dst = self.fresh_val(DataType::Boolean);
                let op = if expr.str_val == "&&" {
                    IROpcode::And
                } else {
                    IROpcode::Or
                };
                self.emit(op, dst, vec![left, right], 0, "");
                dst
            }
            NodeType::ExpLogicNot => {
                let operand = self.gen_expression(&expr.children[0]);
                let dst = self.fresh_val(DataType::Boolean);
                self.emit(IROpcode::Not, dst, vec![operand], 0, "");
                dst
            }
            NodeType::ExpNeg => {
                let operand = self.gen_expression(&expr.children[0]);
                let dst = self.fresh_val(DataType::Int);
                self.emit(IROpcode::Neg, dst, vec![operand], 0, "");
                dst
            }
            NodeType::ExpPos => self.gen_expression(&expr.children[0]),
            NodeType::ExpMethodCall => self.gen_method_call(expr),
            NodeType::ExpOneArr => self.gen_array_alloc_1d(expr),
            NodeType::ExpTwoArr => self.gen_array_alloc_2d(expr),
            NodeType::ExpLength => self.gen_array_length(expr),
            _ => {
                if !expr.children.is_empty() {
                    self.gen_expression(&expr.children[0])
                } else {
                    self.emit_const(0)
                }
            }
        }
    }

    /// unwrap_exp that returns a reference we can use in gen_expression
    /// without running into borrow-checker issues with &self.
    fn unwrap_exp_owned<'b>(&self, node: &'b ASTNode) -> &'b ASTNode {
        if node.node_type == NodeType::Exp && !node.children.is_empty() {
            return self.unwrap_exp_owned(&node.children[0]);
        }
        node
    }

    fn gen_var_decl(&mut self, decl: &ASTNode) {
        let info = self.find_left_value_info(decl).map(|i| i.is_static);
        let decl_name = decl.str_val.clone();

        // Check for initialization (children[1..])
        for i in 1..decl.children.len() {
            let child = &decl.children[i];
            if child.node_type == NodeType::Init && !child.children.is_empty() {
                let val = self.gen_expression(&child.children[0]);
                if info == Some(true) {
                    self.emit(IROpcode::StoreStatic, -1, vec![val], 0, &decl_name);
                } else {
                    self.write_var(&decl_name, val);
                }
            } else if child.node_type == NodeType::IdExpList {
                let extra_info = self
                    .find_left_value_info(child)
                    .map(|i| i.is_static);
                let child_name = child.str_val.clone();
                if !child.children.is_empty()
                    && child.children[0].node_type == NodeType::Init
                {
                    let init = &child.children[0];
                    if !init.children.is_empty() {
                        let val = self.gen_expression(&init.children[0]);
                        if extra_info == Some(true) {
                            self.emit(
                                IROpcode::StoreStatic,
                                -1,
                                vec![val],
                                0,
                                &child_name,
                            );
                        } else {
                            self.write_var(&child_name, val);
                        }
                    }
                }
            }
        }
    }

    fn gen_assign(&mut self, assign: &ASTNode) {
        if assign.children.len() < 2 {
            return;
        }
        let left_type = assign.children[0].node_type;
        let left_name = assign.children[0].str_val.clone();
        let rhs = self.gen_expression(&assign.children[1]);

        if left_type == NodeType::LeftvalueId {
            let is_static = self
                .find_left_value_info(&assign.children[0])
                .map(|i| i.is_static)
                .unwrap_or(false);
            if is_static {
                self.emit(IROpcode::StoreStatic, -1, vec![rhs], 0, &left_name);
            } else {
                self.write_var(&left_name, rhs);
            }
        } else if left_type == NodeType::LeftvalueOneArr
            || left_type == NodeType::LeftvalueTwoArr
        {
            self.gen_array_store(&assign.children[0], rhs);
        }
    }

    fn gen_if(&mut self, if_node: &ASTNode) {
        if if_node.children.len() < 3 {
            return;
        }

        let cond = self.gen_expression(&if_node.children[0]);

        let fi = self.cur_func_idx;
        let then_block = self.module.functions[fi].new_block("if_then");
        let else_block = self.module.functions[fi].new_block("if_else");
        let merge_block = self.module.functions[fi].new_block("if_merge");

        // Emit conditional branch
        {
            let cbr = IRInstr {
                op: IROpcode::Cbranch,
                operands: vec![cond],
                true_block: then_block,
                false_block: else_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(cbr);
            self.module.functions[fi].add_succ(self.cur_block, then_block);
            self.module.functions[fi].add_succ(self.cur_block, else_block);
        }

        // Save variable state before branches
        let saved_vars = self.var_map.clone();

        // Then block
        self.cur_block = then_block;
        self.gen_statement(&if_node.children[1]);
        let then_end_block = self.cur_block;
        let then_returned = {
            let blk = &self.module.functions[fi].blocks[self.cur_block as usize];
            !blk.instrs.is_empty() && blk.instrs.last().unwrap().op == IROpcode::ReturnVal
        };
        if !then_returned {
            let br = IRInstr {
                op: IROpcode::Branch,
                target_block: merge_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(br);
            self.module.functions[fi].add_succ(self.cur_block, merge_block);
        }
        let then_vars = self.var_map.clone();

        // Else block
        self.var_map = saved_vars;
        self.cur_block = else_block;
        self.gen_statement(&if_node.children[2]);
        let else_end_block = self.cur_block;
        let else_returned = {
            let blk = &self.module.functions[fi].blocks[self.cur_block as usize];
            !blk.instrs.is_empty() && blk.instrs.last().unwrap().op == IROpcode::ReturnVal
        };
        if !else_returned {
            let br = IRInstr {
                op: IROpcode::Branch,
                target_block: merge_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(br);
            self.module.functions[fi].add_succ(self.cur_block, merge_block);
        }
        let else_vars = self.var_map.clone();

        // Merge block - insert PHI nodes for variables that differ
        self.cur_block = merge_block;
        if !then_returned || !else_returned {
            for (name, then_val) in &then_vars {
                if let Some(&else_val) = else_vars.get(name) {
                    if else_val != *then_val {
                        let val_type = self.module.functions[fi].values[*then_val as usize].data_type;
                        let phi = self.fresh_val(val_type);
                        let phi_instr = IRInstr {
                            op: IROpcode::Phi,
                            dst: phi,
                            operands: vec![*then_val, else_val],
                            phi_blocks: vec![then_end_block, else_end_block],
                            ..Default::default()
                        };
                        self.module.functions[fi].blocks[merge_block as usize]
                            .instrs
                            .push(phi_instr);
                        self.var_map.insert(name.clone(), phi);
                    } else {
                        self.var_map.insert(name.clone(), *then_val);
                    }
                } else {
                    self.var_map.insert(name.clone(), *then_val);
                }
            }
            // Also pick up vars only in else
            for (name, else_val) in &else_vars {
                if !then_vars.contains_key(name) {
                    self.var_map.insert(name.clone(), *else_val);
                }
            }
        }
    }

    fn gen_while(&mut self, while_node: &ASTNode) {
        if while_node.children.len() < 2 {
            return;
        }

        let pre_block = self.cur_block;
        let pre_map = self.var_map.clone();

        let fi = self.cur_func_idx;
        let header_block = self.module.functions[fi].new_block("while_header");
        let body_block = self.module.functions[fi].new_block("while_body");
        let exit_block = self.module.functions[fi].new_block("while_exit");

        // Branch to header
        {
            let br = IRInstr {
                op: IROpcode::Branch,
                target_block: header_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(br);
            self.module.functions[fi].add_succ(self.cur_block, header_block);
        }

        // Find which variables are modified in the loop body/condition
        self.cur_block = header_block;
        let mut modified_vars = HashSet::new();
        collect_modified_vars(&while_node.children[1], &mut modified_vars);

        struct PhiInfo {
            var_name: String,
            phi_id: i32,
            _pre_val: i32,
            instr_idx: usize,
        }
        let mut phis: Vec<PhiInfo> = Vec::new();

        // Only insert PHI nodes for variables that are modified in the loop
        for (name, &pre_val) in &pre_map {
            if !modified_vars.contains(name) {
                continue;
            }
            let val_type = self.module.functions[fi].values[pre_val as usize].data_type;
            let phi = self.fresh_val(val_type);
            let phi_instr = IRInstr {
                op: IROpcode::Phi,
                dst: phi,
                operands: vec![pre_val],
                phi_blocks: vec![pre_block],
                ..Default::default()
            };
            let idx = self.module.functions[fi].blocks[header_block as usize]
                .instrs
                .len();
            self.module.functions[fi].blocks[header_block as usize]
                .instrs
                .push(phi_instr);
            self.var_map.insert(name.clone(), phi);
            phis.push(PhiInfo {
                var_name: name.clone(),
                phi_id: phi,
                _pre_val: pre_val,
                instr_idx: idx,
            });
        }

        // Evaluate condition (uses PHI values)
        let cond = self.gen_expression(&while_node.children[0]);

        // Conditional branch
        {
            let cbr = IRInstr {
                op: IROpcode::Cbranch,
                operands: vec![cond],
                true_block: body_block,
                false_block: exit_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(cbr);
            self.module.functions[fi].add_succ(self.cur_block, body_block);
            self.module.functions[fi].add_succ(self.cur_block, exit_block);
        }

        // Body
        self.cur_block = body_block;
        self.gen_statement(&while_node.children[1]);

        let body_end_block = self.cur_block;

        // Complete PHI nodes with back-edge values
        for phi in &phis {
            let body_val = self.var_map[&phi.var_name];
            self.module.functions[fi].blocks[header_block as usize].instrs[phi.instr_idx]
                .operands
                .push(body_val);
            self.module.functions[fi].blocks[header_block as usize].instrs[phi.instr_idx]
                .phi_blocks
                .push(body_end_block);
        }

        // Loop back to header
        {
            let br_back = IRInstr {
                op: IROpcode::Branch,
                target_block: header_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(br_back);
            self.module.functions[fi].add_succ(self.cur_block, header_block);
        }

        // After the loop, variables have their PHI values (from the header)
        self.cur_block = exit_block;
        for phi in &phis {
            self.var_map.insert(phi.var_name.clone(), phi.phi_id);
        }
    }

    fn gen_return(&mut self, ret_node: &ASTNode) {
        if ret_node.children.is_empty() {
            return;
        }
        let val = self.gen_expression(&ret_node.children[0]);
        self.emit(IROpcode::ReturnVal, -1, vec![val], 0, "");
    }

    fn gen_print(&mut self, print_node: &ASTNode, newline: bool) {
        if print_node.children.is_empty() {
            return;
        }
        let expr = self.unwrap_exp_owned(&print_node.children[0]);
        let etype = self.find_exp_type(expr);
        let is_string = etype == DataType::String
            || (expr.node_type == NodeType::ExpType && expr.data_type == DataType::String);
        let val = self.gen_expression(&print_node.children[0]);

        if is_string {
            self.emit(IROpcode::PrintStr, -1, vec![val], 0, "");
        } else {
            self.emit(IROpcode::PrintInt, -1, vec![val], 0, "");
        }
        if newline {
            self.emit(IROpcode::PrintNewline, -1, vec![], 0, "");
        }
    }

    fn gen_method_call(&mut self, call_node: &ASTNode) -> i32 {
        let inner = if (call_node.node_type == NodeType::ExpMethodCall
            || call_node.node_type == NodeType::StatementMethodCall)
            && !call_node.children.is_empty()
        {
            &*call_node.children[0]
        } else {
            call_node
        };

        // Integer.parseInt
        if inner.node_type == NodeType::ParseInt {
            if inner.children.is_empty() {
                return self.emit_const(0);
            }
            let arg = self.gen_expression(&inner.children[0]);
            let dst = self.fresh_val(DataType::Int);
            self.emit(IROpcode::Call, dst, vec![arg], 0, "atoi");
            return dst;
        }

        // Regular method call or main call
        let func_name = inner.str_val.clone();
        let mut args: Vec<i32> = Vec::new();

        // Process arguments
        if !inner.children.is_empty() {
            let arg_list = &inner.children[0];
            if arg_list.node_type == NodeType::CommaExpList {
                for arg in &arg_list.children {
                    let v = self.gen_expression(arg);
                    args.push(v);
                }
            } else {
                let v = self.gen_expression(arg_list);
                args.push(v);
            }
        }

        // Emit PARAM instructions for each argument
        for (i, &arg) in args.iter().enumerate() {
            self.emit(IROpcode::Param, -1, vec![arg], i as i64, "");
        }

        let dst = self.fresh_val(DataType::Int);
        self.emit(IROpcode::Call, dst, args, 0, &func_name);
        dst
    }

    fn gen_array_alloc_1d(&mut self, node: &ASTNode) -> i32 {
        // new Type[size]
        if node.children.len() < 2 {
            return self.emit_const(0);
        }
        let size = self.gen_expression(&node.children[1]);
        let dst = self.fresh_val(DataType::Int);
        self.emit(IROpcode::AllocArray, dst, vec![size], 0, "");
        dst
    }

    fn gen_array_alloc_2d(&mut self, node: &ASTNode) -> i32 {
        // new Type[rows][cols]
        if node.children.len() < 3 {
            return self.emit_const(0);
        }
        let rows = self.gen_expression(&node.children[1]);
        let cols = self.gen_expression(&node.children[2]);

        // Allocate the row pointer array
        let row_arr = self.fresh_val(DataType::Int);
        self.emit(IROpcode::AllocArray, row_arr, vec![rows], 0, "");

        // Loop: allocate each row (with PHI for counter)
        let counter_init = self.emit_const(0);
        let pre_block = self.cur_block;
        let fi = self.cur_func_idx;
        let header_block = self.module.functions[fi].new_block("alloc2d_header");
        let body_block = self.module.functions[fi].new_block("alloc2d_body");
        let exit_block = self.module.functions[fi].new_block("alloc2d_exit");

        {
            let br = IRInstr {
                op: IROpcode::Branch,
                target_block: header_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(br);
            self.module.functions[fi].add_succ(self.cur_block, header_block);
        }

        // Header: PHI for counter, then check counter < rows
        self.cur_block = header_block;
        let counter_phi = self.fresh_val(DataType::Int);
        let phi_instr = IRInstr {
            op: IROpcode::Phi,
            dst: counter_phi,
            operands: vec![counter_init],
            phi_blocks: vec![pre_block],
            ..Default::default()
        };
        let phi_idx = self.module.functions[fi].blocks[header_block as usize]
            .instrs
            .len();
        self.module.functions[fi].blocks[header_block as usize]
            .instrs
            .push(phi_instr);

        let cmp_val = self.fresh_val(DataType::Boolean);
        self.emit(IROpcode::CmpLt, cmp_val, vec![counter_phi, rows], 0, "");

        {
            let cbr = IRInstr {
                op: IROpcode::Cbranch,
                operands: vec![cmp_val],
                true_block: body_block,
                false_block: exit_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(cbr);
            self.module.functions[fi].add_succ(self.cur_block, body_block);
            self.module.functions[fi].add_succ(self.cur_block, exit_block);
        }

        // Body: allocate one row and store it
        self.cur_block = body_block;
        let row_alloc = self.fresh_val(DataType::Int);
        self.emit(IROpcode::AllocArray, row_alloc, vec![cols], 0, "");
        self.emit(
            IROpcode::ArrayStore,
            -1,
            vec![row_arr, counter_phi, row_alloc],
            0,
            "",
        );

        // counter++
        let one = self.emit_const(1);
        let new_cnt = self.fresh_val(DataType::Int);
        self.emit(IROpcode::Add, new_cnt, vec![counter_phi, one], 0, "");

        // Complete PHI with back-edge value
        let body_end = self.cur_block;
        self.module.functions[fi].blocks[header_block as usize].instrs[phi_idx]
            .operands
            .push(new_cnt);
        self.module.functions[fi].blocks[header_block as usize].instrs[phi_idx]
            .phi_blocks
            .push(body_end);

        {
            let br_back = IRInstr {
                op: IROpcode::Branch,
                target_block: header_block,
                ..Default::default()
            };
            self.module.functions[fi].blocks[self.cur_block as usize]
                .instrs
                .push(br_back);
            self.module.functions[fi].add_succ(self.cur_block, header_block);
        }

        self.cur_block = exit_block;
        row_arr
    }

    fn gen_array_access(&mut self, left_val: &ASTNode) -> i32 {
        let arr_name = left_val.str_val.clone();
        let base = self.read_var(&arr_name);

        if left_val.node_type == NodeType::LeftvalueOneArr {
            let index = self.gen_expression(&left_val.children[0]);
            let dst = self.fresh_val(DataType::Int);
            self.emit(IROpcode::ArrayLoad, dst, vec![base, index], 0, "");
            return dst;
        }
        if left_val.node_type == NodeType::LeftvalueTwoArr {
            let index1 = self.gen_expression(&left_val.children[0]);
            // Load row pointer
            let row_ptr = self.fresh_val(DataType::Int);
            self.emit(IROpcode::ArrayLoad, row_ptr, vec![base, index1], 0, "");
            let index2 = self.gen_expression(&left_val.children[1]);
            let dst = self.fresh_val(DataType::Int);
            self.emit(IROpcode::ArrayLoad, dst, vec![row_ptr, index2], 0, "");
            return dst;
        }
        self.emit_const(0)
    }

    fn gen_array_store(&mut self, left_val: &ASTNode, value_ssa: i32) {
        let arr_name = left_val.str_val.clone();
        let base = self.read_var(&arr_name);

        if left_val.node_type == NodeType::LeftvalueOneArr {
            let index = self.gen_expression(&left_val.children[0]);
            self.emit(
                IROpcode::ArrayStore,
                -1,
                vec![base, index, value_ssa],
                0,
                "",
            );
        } else if left_val.node_type == NodeType::LeftvalueTwoArr {
            let index1 = self.gen_expression(&left_val.children[0]);
            let row_ptr = self.fresh_val(DataType::Int);
            self.emit(IROpcode::ArrayLoad, row_ptr, vec![base, index1], 0, "");
            let index2 = self.gen_expression(&left_val.children[1]);
            self.emit(
                IROpcode::ArrayStore,
                -1,
                vec![row_ptr, index2, value_ssa],
                0,
                "",
            );
        }
    }

    fn gen_array_length(&mut self, node: &ASTNode) -> i32 {
        if node.children.is_empty() {
            return self.emit_const(0);
        }
        let lv = &node.children[0];
        let lv_name = lv.str_val.clone();
        let base = self.read_var(&lv_name);

        if lv.node_type == NodeType::LeftvalueOneArr {
            // Load the row first, then get its length
            let index = self.gen_expression(&lv.children[0]);
            let row_ptr = self.fresh_val(DataType::Int);
            self.emit(IROpcode::ArrayLoad, row_ptr, vec![base, index], 0, "");
            let len = self.fresh_val(DataType::Int);
            self.emit(IROpcode::ArrayLength, len, vec![row_ptr], 0, "");
            return len;
        }

        let dst = self.fresh_val(DataType::Int);
        self.emit(IROpcode::ArrayLength, dst, vec![base], 0, "");
        dst
    }
}
