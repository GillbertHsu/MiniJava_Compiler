use std::fmt;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DataType {
    Undefined,
    String,
    Int,
    Boolean,
}

impl fmt::Display for DataType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DataType::String => write!(f, "String"),
            DataType::Int => write!(f, "Integer"),
            DataType::Boolean => write!(f, "Boolean"),
            DataType::Undefined => write!(f, "Undefined"),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NodeType {
    Program,
    MainClass,
    MainMethod,
    StaticVarDeclList,
    StaticMethodDeclList,
    StatementList,
    Init,
    IdExpList,
    StaticVarDecl,
    StaticMethodDecl,
    FormalList,
    TypeIdList,
    TypeIdListId,
    PrimeType,
    Type,
    TypeOneArr,
    TypeTwoArr,
    VarDecl,
    Statement,
    Print,
    Println,
    ParseInt,
    If,
    While,
    Assign,
    Return,
    MethodCall,
    CallMain,
    StatementMethodCall,
    Exp,
    ExpList,
    CommaExpList,
    ExpPlus,
    ExpOperation,
    ExpOperationComp,
    ExpLogic,
    ExpComp,
    ExpPos,
    ExpNeg,
    ExpLogicNot,
    ExpType,
    ExpLeft,
    ExpOneArr,
    ExpTwoArr,
    ExpLength,
    ExpMethodCall,
    LeftvalueId,
    LeftvalueOneArr,
    LeftvalueTwoArr,
    BracketExpList,
}

pub struct ASTNode {
    pub node_type: NodeType,
    pub line: i32,
    pub data_type: DataType,
    pub str_val: String,
    pub int_val: i32,
    pub bool_val: bool,
    pub children: Vec<Box<ASTNode>>,
    pub scope: Option<usize>,
    pub is_static: bool,
    pub has_curly_braces: bool,
}

pub fn make_node(node_type: NodeType, line: i32) -> Box<ASTNode> {
    Box::new(ASTNode {
        node_type,
        line,
        data_type: DataType::Undefined,
        str_val: String::new(),
        int_val: 0,
        bool_val: false,
        children: Vec::new(),
        scope: None,
        is_static: false,
        has_curly_braces: false,
    })
}

pub fn add_child(parent: &mut ASTNode, child: Option<Box<ASTNode>>) {
    if let Some(c) = child {
        parent.children.push(c);
    }
}
