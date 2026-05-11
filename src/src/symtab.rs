use std::collections::HashMap;

use crate::ast::DataType;

#[derive(Debug, Clone)]
pub struct SymbolInfo {
    pub name: String,
    pub data_type: DataType,
    pub is_method: bool,
    pub is_static: bool,
    pub is_arg: bool,
    pub num_params: i32,
    pub array_dim: i32,
    pub stack_slot: i32,
    pub method_scope: Option<usize>,
}

impl Default for SymbolInfo {
    fn default() -> Self {
        SymbolInfo {
            name: String::new(),
            data_type: DataType::Undefined,
            is_method: false,
            is_static: false,
            is_arg: false,
            num_params: 0,
            array_dim: 0,
            stack_slot: -1,
            method_scope: None,
        }
    }
}

pub struct SymbolTable {
    entries: HashMap<String, SymbolInfo>,
    parent: Option<usize>,
}

impl SymbolTable {
    pub fn new(parent: Option<usize>) -> Self {
        SymbolTable {
            entries: HashMap::new(),
            parent,
        }
    }

    pub fn insert(&mut self, name: &str, info: SymbolInfo) -> bool {
        if self.entries.contains_key(name) {
            return false;
        }
        self.entries.insert(name.to_string(), info);
        true
    }

    pub fn lookup_local(&self, name: &str) -> Option<&SymbolInfo> {
        self.entries.get(name)
    }

    pub fn parent(&self) -> Option<usize> {
        self.parent
    }
}

pub struct SymbolTableArena {
    pub tables: Vec<SymbolTable>,
}

impl SymbolTableArena {
    pub fn new() -> Self {
        SymbolTableArena { tables: Vec::new() }
    }

    pub fn create_scope(&mut self, parent: Option<usize>) -> usize {
        let idx = self.tables.len();
        self.tables.push(SymbolTable::new(parent));
        idx
    }

    pub fn insert(&mut self, scope: usize, name: &str, info: SymbolInfo) -> bool {
        self.tables[scope].insert(name, info)
    }

    pub fn lookup_local(&self, scope: usize, name: &str) -> Option<&SymbolInfo> {
        self.tables[scope].lookup_local(name)
    }

    pub fn lookup(&self, scope: usize, name: &str) -> Option<&SymbolInfo> {
        let info = self.tables[scope].lookup_local(name);
        if info.is_some() {
            return info;
        }
        if let Some(parent) = self.tables[scope].parent() {
            return self.lookup(parent, name);
        }
        None
    }
}

pub struct MethodTable {
    methods: HashMap<String, SymbolInfo>,
}

impl MethodTable {
    pub fn new() -> Self {
        MethodTable {
            methods: HashMap::new(),
        }
    }

    pub fn insert(&mut self, name: &str, info: SymbolInfo) -> bool {
        if self.methods.contains_key(name) {
            return false;
        }
        self.methods.insert(name.to_string(), info);
        true
    }

    pub fn lookup(&self, name: &str) -> Option<&SymbolInfo> {
        self.methods.get(name)
    }
}
