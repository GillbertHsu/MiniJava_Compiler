use crate::ast::{ASTNode, DataType, NodeType};
use crate::symtab::{MethodTable, SymbolInfo, SymbolTableArena};

pub struct TypeChecker<'a> {
    pub scopes: SymbolTableArena,
    methods: &'a mut MethodTable,
    error_lines: Vec<i32>,
}

impl<'a> TypeChecker<'a> {
    pub fn new(methods: &'a mut MethodTable) -> Self {
        TypeChecker {
            scopes: SymbolTableArena::new(),
            methods,
            error_lines: Vec::new(),
        }
    }

    // ------------------------------------------------------------------
    // Public interface
    // ------------------------------------------------------------------

    pub fn build_symbol_tables(&mut self, root: &mut ASTNode) {
        self.fill_table(root, None);
    }

    pub fn check_types(&mut self, root: &ASTNode) -> bool {
        self.check_types_inner(root);
        !self.has_errors()
    }

    pub fn has_errors(&self) -> bool {
        !self.error_lines.is_empty()
    }

    pub fn report_errors(&self) {
        let mut sorted = self.error_lines.clone();
        sorted.sort();
        for line in sorted {
            eprintln!("Type violation in line {}", line);
        }
    }

    /// Consume the type-checker and return the symbol-table arena so later
    /// compiler phases can use it.
    pub fn into_scopes(self) -> SymbolTableArena {
        self.scopes
    }

    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    fn add_error(&mut self, line: i32) {
        self.error_lines.push(line);
    }

    fn create_scope(&mut self, node: &mut ASTNode, parent: Option<usize>) -> usize {
        let idx = self.scopes.create_scope(parent);
        node.scope = Some(idx);
        idx
    }

    fn find_type(&self, type_node: &ASTNode) -> DataType {
        if type_node.node_type == NodeType::PrimeType {
            return type_node.data_type;
        }
        if let Some(first_child) = type_node.children.first() {
            return self.find_type(first_child);
        }
        DataType::Undefined
    }

    fn unwrap_exp<'b>(&self, node: &'b ASTNode) -> &'b ASTNode {
        if node.node_type == NodeType::Exp {
            if let Some(first_child) = node.children.first() {
                return self.unwrap_exp(first_child);
            }
        }
        node
    }

    fn count_params(&self, formal_list: &ASTNode) -> i32 {
        let mut count = 1;
        // The second child of FORMAL_LIST is the next param (chained)
        if formal_list.children.len() > 1 {
            count += self.count_params(&formal_list.children[1]);
        }
        count
    }

    /// Look up the SymbolInfo associated with a left-value or method-call node.
    /// Returns a clone since we cannot hold references across arena + method table
    /// simultaneously with &self.
    fn find_left_value_info(&self, node: &ASTNode) -> Option<SymbolInfo> {
        match node.node_type {
            NodeType::LeftvalueId | NodeType::LeftvalueOneArr | NodeType::LeftvalueTwoArr => {
                if let Some(scope_idx) = node.scope {
                    if let Some(info) = self.scopes.lookup(scope_idx, &node.str_val) {
                        return Some(info.clone());
                    }
                }
                self.methods.lookup(&node.str_val).cloned()
            }
            NodeType::MethodCall | NodeType::CallMain => {
                self.methods.lookup(&node.str_val).cloned()
            }
            NodeType::VarDecl | NodeType::IdExpList => {
                if let Some(scope_idx) = node.scope {
                    return self.scopes.lookup_local(scope_idx, &node.str_val).cloned();
                }
                None
            }
            NodeType::ExpMethodCall => {
                if let Some(first_child) = node.children.first() {
                    return self.find_left_value_info(first_child);
                }
                None
            }
            _ => None,
        }
    }

    fn find_exp_type(&self, node: &ASTNode) -> DataType {
        let node = self.unwrap_exp(node);

        match node.node_type {
            NodeType::ExpType => node.data_type,

            NodeType::ExpOneArr | NodeType::ExpTwoArr => {
                if let Some(first_child) = node.children.first() {
                    self.find_type(first_child)
                } else {
                    DataType::Undefined
                }
            }

            NodeType::ExpLength => DataType::Int,

            NodeType::ExpLeft => {
                if let Some(first_child) = node.children.first() {
                    if let Some(info) = self.find_left_value_info(first_child) {
                        return info.data_type;
                    }
                }
                DataType::Undefined
            }

            NodeType::ExpMethodCall => {
                if let Some(first_child) = node.children.first() {
                    if first_child.node_type == NodeType::ParseInt {
                        return DataType::Int;
                    }
                    if let Some(info) = self.find_left_value_info(first_child) {
                        return info.data_type;
                    }
                }
                DataType::Undefined
            }

            NodeType::ExpPlus => {
                if let Some(first_child) = node.children.first() {
                    let left = self.unwrap_exp(first_child);
                    self.find_exp_type(left)
                } else {
                    DataType::Undefined
                }
            }

            NodeType::ExpOperation => DataType::Int,

            NodeType::ExpOperationComp | NodeType::ExpComp | NodeType::ExpLogic => {
                DataType::Boolean
            }

            NodeType::ExpLogicNot | NodeType::ExpPos | NodeType::ExpNeg => {
                if let Some(first_child) = node.children.first() {
                    self.find_exp_type(first_child)
                } else {
                    DataType::Undefined
                }
            }

            _ => {
                if let Some(first_child) = node.children.first() {
                    self.find_exp_type(first_child)
                } else {
                    DataType::Undefined
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // Phase 1: Build Symbol Tables
    // ------------------------------------------------------------------

    fn fill_table(&mut self, root: &mut ASTNode, current_scope: Option<usize>) {
        let current_scope = if root.has_curly_braces {
            let idx = self.create_scope(root, current_scope);
            Some(idx)
        } else {
            root.scope = current_scope;
            current_scope
        };

        // Handle main method: add "main" to method table and "args" to scope
        if root.node_type == NodeType::MainMethod {
            if let Some(scope_idx) = current_scope {
                let main_info = SymbolInfo {
                    name: "main".to_string(),
                    data_type: DataType::Undefined,
                    is_method: true,
                    num_params: 1,
                    method_scope: current_scope,
                    ..SymbolInfo::default()
                };
                self.methods.insert("main", main_info);

                let args_info = SymbolInfo {
                    name: "args".to_string(),
                    data_type: DataType::String,
                    array_dim: 1,
                    is_arg: true,
                    ..SymbolInfo::default()
                };
                self.scopes.insert(scope_idx, "args", args_info);
            }
        }

        // Handle variable declarations
        if root.node_type == NodeType::VarDecl {
            let mut var_type = DataType::Undefined;
            let mut dim = 0;
            if let Some(type_node) = root.children.first() {
                var_type = self.find_type(type_node);
                match type_node.node_type {
                    NodeType::TypeOneArr => dim = 1,
                    NodeType::TypeTwoArr => dim = 2,
                    _ => {}
                }
            }

            if let Some(scope_idx) = current_scope {
                if self.scopes.lookup_local(scope_idx, &root.str_val).is_some() {
                    self.add_error(root.line);
                } else {
                    let info = SymbolInfo {
                        name: root.str_val.clone(),
                        data_type: var_type,
                        array_dim: dim,
                        is_static: root.is_static,
                        ..SymbolInfo::default()
                    };
                    self.scopes.insert(scope_idx, &root.str_val, info);
                }
            }

            // Handle additional IDs in the same declaration (idExpList)
            for i in 1..root.children.len() {
                if root.children[i].node_type == NodeType::IdExpList {
                    root.children[i].scope = current_scope;
                    if let Some(scope_idx) = current_scope {
                        let child_name = root.children[i].str_val.clone();
                        let child_line = root.children[i].line;
                        if self.scopes.lookup_local(scope_idx, &child_name).is_some() {
                            self.add_error(child_line);
                        } else {
                            let info2 = SymbolInfo {
                                name: child_name.clone(),
                                data_type: var_type,
                                array_dim: dim,
                                is_static: root.is_static,
                                ..SymbolInfo::default()
                            };
                            self.scopes.insert(scope_idx, &child_name, info2);
                        }
                    }
                }
            }
        }

        // Handle static method declarations
        if root.node_type == NodeType::StaticMethodDecl {
            let mut ret_type = DataType::Undefined;
            let mut ret_dim = 0;
            if let Some(type_node) = root.children.first() {
                ret_type = self.find_type(type_node);
                match type_node.node_type {
                    NodeType::TypeOneArr => ret_dim = 1,
                    NodeType::TypeTwoArr => ret_dim = 2,
                    _ => {}
                }
            }

            if self.methods.lookup(&root.str_val).is_some() {
                self.add_error(root.line);
            } else {
                let mut method_info = SymbolInfo {
                    name: root.str_val.clone(),
                    data_type: ret_type,
                    is_method: true,
                    array_dim: ret_dim,
                    method_scope: current_scope,
                    ..SymbolInfo::default()
                };
                if root.children.len() > 1 {
                    method_info.num_params =
                        self.count_params(&root.children[1]);
                }
                self.methods.insert(&root.str_val, method_info);
            }
        }

        // Handle assignments: check that variable exists
        if root.node_type == NodeType::Assign {
            if let Some(left_val) = root.children.first_mut() {
                if let Some(scope_idx) = current_scope {
                    left_val.scope = current_scope;
                    let name = &left_val.str_val;
                    if self.scopes.lookup(scope_idx, name).is_none()
                        && self.methods.lookup(name).is_none()
                    {
                        self.add_error(root.line);
                    }
                }
            }
        }

        // Handle formal parameters
        if root.node_type == NodeType::FormalList
            || root.node_type == NodeType::TypeIdList
        {
            let mut p_type = DataType::Undefined;
            let mut p_dim = 0;
            if let Some(type_node) = root.children.first() {
                p_type = self.find_type(type_node);
                match type_node.node_type {
                    NodeType::TypeOneArr => p_dim = 1,
                    NodeType::TypeTwoArr => p_dim = 2,
                    _ => {}
                }
            }
            if let Some(scope_idx) = current_scope {
                if self.scopes.lookup_local(scope_idx, &root.str_val).is_some() {
                    self.add_error(root.line);
                } else {
                    let p_info = SymbolInfo {
                        name: root.str_val.clone(),
                        data_type: p_type,
                        array_dim: p_dim,
                        is_arg: true,
                        ..SymbolInfo::default()
                    };
                    self.scopes.insert(scope_idx, &root.str_val, p_info);
                }
            }
        }

        // Recurse into children
        for child in &mut root.children {
            self.fill_table(child, current_scope);
        }
    }

    // ------------------------------------------------------------------
    // Phase 2: Type Checking
    // ------------------------------------------------------------------

    fn check_types_inner(&mut self, root: &ASTNode) {
        for child in &root.children {
            self.check_types_inner(child);
        }

        match root.node_type {
            NodeType::ExpPlus => {
                if !self.check_exp_plus(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::ExpOperation | NodeType::ExpOperationComp => {
                if !self.check_operation_and_comp(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::ExpLogic => {
                if !self.check_exp_logic(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::ExpComp => {
                if !self.check_exp_comp(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::ExpLogicNot => {
                if !self.check_exp_logic_not(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::ExpPos | NodeType::ExpNeg => {
                if !self.check_exp_pos_neg(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::If | NodeType::While => {
                if !self.check_if_while(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::VarDecl => {
                if !self.check_var_decl(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::Assign => {
                if !self.check_assign(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::ExpLength => {
                if !self.check_exp_length(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::LeftvalueOneArr | NodeType::LeftvalueTwoArr => {
                if !self.check_left_value_arr(root) {
                    self.add_error(root.line);
                }
            }
            NodeType::ExpMethodCall | NodeType::StatementMethodCall => {
                if !self.check_method_call(root) {
                    self.add_error(root.line);
                }
            }
            _ => {}
        }
    }

    // ------------------------------------------------------------------
    // Check helpers
    // ------------------------------------------------------------------

    fn check_exp_plus(&self, node: &ASTNode) -> bool {
        if node.children.len() < 2 {
            return false;
        }
        let left = self.unwrap_exp(&node.children[0]);
        let right = self.unwrap_exp(&node.children[1]);
        let lt = self.find_exp_type(left);
        let rt = self.find_exp_type(right);
        if lt == DataType::Undefined || rt == DataType::Undefined {
            return true;
        }
        (lt == DataType::Int && rt == DataType::Int)
            || (lt == DataType::String && rt == DataType::String)
    }

    fn check_operation_and_comp(&self, node: &ASTNode) -> bool {
        if node.children.len() < 2 {
            return false;
        }
        let left = self.unwrap_exp(&node.children[0]);
        let right = self.unwrap_exp(&node.children[1]);
        let lt = self.find_exp_type(left);
        let rt = self.find_exp_type(right);
        if lt == DataType::Undefined || rt == DataType::Undefined {
            return true;
        }
        lt == DataType::Int && rt == DataType::Int
    }

    fn check_exp_comp(&self, node: &ASTNode) -> bool {
        if node.children.len() < 2 {
            return false;
        }
        let left = self.unwrap_exp(&node.children[0]);
        let right = self.unwrap_exp(&node.children[1]);
        let lt = self.find_exp_type(left);
        let rt = self.find_exp_type(right);
        if lt == DataType::Undefined || rt == DataType::Undefined {
            return true;
        }
        (lt == DataType::Int && rt == DataType::Int)
            || (lt == DataType::Boolean && rt == DataType::Boolean)
    }

    fn check_exp_logic(&self, node: &ASTNode) -> bool {
        if node.children.len() < 2 {
            return false;
        }
        let left = self.unwrap_exp(&node.children[0]);
        let right = self.unwrap_exp(&node.children[1]);
        let lt = self.find_exp_type(left);
        let rt = self.find_exp_type(right);
        if lt == DataType::Undefined || rt == DataType::Undefined {
            return true;
        }
        lt == DataType::Boolean && rt == DataType::Boolean
    }

    fn check_exp_logic_not(&self, node: &ASTNode) -> bool {
        if node.children.is_empty() {
            return false;
        }
        let child = self.unwrap_exp(&node.children[0]);
        let t = self.find_exp_type(child);
        if t == DataType::Undefined {
            return true;
        }
        t == DataType::Boolean
    }

    fn check_exp_pos_neg(&self, node: &ASTNode) -> bool {
        if node.children.is_empty() {
            return false;
        }
        let child = self.unwrap_exp(&node.children[0]);
        let t = self.find_exp_type(child);
        if t == DataType::Undefined {
            return true;
        }
        t == DataType::Int
    }

    fn check_if_while(&self, node: &ASTNode) -> bool {
        if node.children.is_empty() {
            return false;
        }
        let cond = self.unwrap_exp(&node.children[0]);
        let t = self.find_exp_type(cond);
        if t == DataType::Undefined {
            return true;
        }
        t == DataType::Boolean || t == DataType::Int
    }

    fn check_var_decl(&self, node: &ASTNode) -> bool {
        // If there's an initializer, check type compatibility
        if node.children.len() < 2 {
            return true;
        }
        let init_node = &node.children[1];
        if init_node.node_type != NodeType::Init {
            return true;
        }
        if init_node.children.is_empty() {
            return true;
        }
        let decl_type = self.find_type(&node.children[0]);
        let init_type = self.find_exp_type(&init_node.children[0]);
        if decl_type == DataType::Undefined || init_type == DataType::Undefined {
            return true;
        }
        decl_type == init_type
    }

    fn check_assign(&self, node: &ASTNode) -> bool {
        if node.children.len() < 2 {
            return false;
        }
        let left = &node.children[0];
        let right = self.unwrap_exp(&node.children[1]);
        let info = match self.find_left_value_info(left) {
            Some(i) => i,
            None => return false,
        };
        let lt = info.data_type;
        let rt = self.find_exp_type(right);
        if lt == DataType::Undefined || rt == DataType::Undefined {
            return true;
        }
        lt == rt
    }

    fn check_exp_length(&self, node: &ASTNode) -> bool {
        if node.children.is_empty() {
            return false;
        }
        let child = &node.children[0];
        let info = match self.find_left_value_info(child) {
            Some(i) => i,
            None => return false,
        };
        info.array_dim > 0
    }

    fn check_left_value_arr(&self, node: &ASTNode) -> bool {
        let info = match self.find_left_value_info(node) {
            Some(i) => i,
            None => return false,
        };
        match node.node_type {
            NodeType::LeftvalueOneArr => info.array_dim >= 1,
            NodeType::LeftvalueTwoArr => info.array_dim >= 2,
            _ => true,
        }
    }

    fn check_method_call(&self, node: &ASTNode) -> bool {
        if node.children.is_empty() {
            return true;
        }
        let call_node = &node.children[0];
        if call_node.node_type == NodeType::ParseInt {
            return true;
        }
        self.methods.lookup(&call_node.str_val).is_some()
    }
}

// Re-export a convenience constructor-style function so callers can
// simply do:  let mut tc = TypeChecker::new(&mut method_table);
//             tc.build_symbol_tables(&mut ast_root);
//             tc.check_types(&ast_root);
//             if tc.has_errors() { tc.report_errors(); }

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ast::make_node;

    #[test]
    fn test_empty_program() {
        let mut methods = MethodTable::new();
        let mut tc = TypeChecker::new(&mut methods);
        let mut root = make_node(NodeType::Program, 0);
        tc.build_symbol_tables(&mut root);
        assert!(tc.check_types(&root));
        assert!(!tc.has_errors());
    }
}
