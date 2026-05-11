use crate::ast::*;
use crate::lexer::*;

pub struct Parser {
    lex: Lexer,
    cur: Token,
}

impl Parser {
    pub fn new(mut lex: Lexer) -> Self {
        let cur = lex.next_token();
        Parser { lex, cur }
    }

    fn advance(&mut self) -> Token {
        let prev = self.cur.clone();
        self.cur = self.lex.next_token();
        prev
    }

    fn expect(&mut self, kind: TokenKind) -> Token {
        if self.cur.kind != kind {
            eprintln!(
                "Syntax error at line {}: expected {:?}, got '{}'",
                self.cur.line, kind, self.cur.text
            );
            std::process::exit(1);
        }
        self.advance()
    }

    fn check(&self, kind: TokenKind) -> bool {
        self.cur.kind == kind
    }

    fn match_tok(&mut self, kind: TokenKind) -> bool {
        if self.check(kind) {
            self.advance();
            true
        } else {
            false
        }
    }

    fn error(&self, msg: &str) -> ! {
        eprintln!("Syntax error at line {}: {}", self.cur.line, msg);
        std::process::exit(1);
    }

    fn is_type_keyword(&self, k: TokenKind) -> bool {
        matches!(k, TokenKind::KwInt | TokenKind::KwBoolean | TokenKind::KwString)
    }

    fn get_prec(&self, k: TokenKind) -> i32 {
        match k {
            TokenKind::TokOr => 1,
            TokenKind::TokAnd => 2,
            TokenKind::TokEq | TokenKind::TokNeq => 3,
            TokenKind::TokLt | TokenKind::TokGt | TokenKind::TokLte | TokenKind::TokGte => 4,
            TokenKind::TokPlus | TokenKind::TokMinus => 5,
            TokenKind::TokStar | TokenKind::TokSlash => 6,
            _ => -1,
        }
    }

    fn get_bin_op_node_type(&self, k: TokenKind) -> NodeType {
        match k {
            TokenKind::TokPlus => NodeType::ExpPlus,
            TokenKind::TokMinus | TokenKind::TokStar | TokenKind::TokSlash => {
                NodeType::ExpOperation
            }
            TokenKind::TokAnd | TokenKind::TokOr => NodeType::ExpLogic,
            TokenKind::TokEq | TokenKind::TokNeq => NodeType::ExpComp,
            TokenKind::TokLt | TokenKind::TokGt | TokenKind::TokLte | TokenKind::TokGte => {
                NodeType::ExpOperationComp
            }
            _ => NodeType::ExpOperation,
        }
    }

    fn get_bin_op_str(&self, k: TokenKind) -> &'static str {
        match k {
            TokenKind::TokPlus => "+",
            TokenKind::TokMinus => "-",
            TokenKind::TokStar => "*",
            TokenKind::TokSlash => "/",
            TokenKind::TokAnd => "&&",
            TokenKind::TokOr => "||",
            TokenKind::TokEq => "==",
            TokenKind::TokNeq => "!=",
            TokenKind::TokLt => "<",
            TokenKind::TokGt => ">",
            TokenKind::TokLte => "<=",
            TokenKind::TokGte => ">=",
            _ => "",
        }
    }

    pub fn parse(&mut self) -> Box<ASTNode> {
        self.parse_program()
    }

    fn parse_program(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::Program, self.cur.line);
        let main_class = self.parse_main_class();
        add_child(&mut node, Some(main_class));
        node
    }

    fn parse_main_class(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::MainClass, self.cur.line);
        node.has_curly_braces = true;
        self.expect(TokenKind::KwClass);
        let name = self.expect(TokenKind::Identifier);
        node.str_val = name.text;
        self.expect(TokenKind::TokLbrace);

        let var_list = self.parse_static_var_decl_list();
        add_child(&mut node, Some(var_list));

        let method_list = self.parse_static_method_decl_list();
        add_child(&mut node, Some(method_list));

        let main_method = self.parse_main_method();
        add_child(&mut node, Some(main_method));

        self.expect(TokenKind::TokRbrace);
        node
    }

    fn parse_main_method(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::MainMethod, self.cur.line);
        node.has_curly_braces = true;
        self.expect(TokenKind::KwPublic);
        self.expect(TokenKind::KwStatic);
        self.expect(TokenKind::KwVoid);
        self.expect(TokenKind::KwMain);
        self.expect(TokenKind::TokLparen);
        self.expect(TokenKind::KwString);
        self.expect(TokenKind::TokLbracket);
        self.expect(TokenKind::TokRbracket);
        let args_name = self.expect(TokenKind::Identifier);
        node.str_val = args_name.text;
        self.expect(TokenKind::TokRparen);
        self.expect(TokenKind::TokLbrace);
        let stmts = self.parse_statement_list();
        add_child(&mut node, Some(stmts));
        self.expect(TokenKind::TokRbrace);
        node
    }

    fn parse_static_var_decl_list(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::StaticVarDeclList, self.cur.line);
        while self.check(TokenKind::KwPrivate) {
            let decl = self.parse_static_var_decl();
            add_child(&mut node, Some(decl));
        }
        node
    }

    fn parse_static_method_decl_list(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::StaticMethodDeclList, self.cur.line);
        while self.check(TokenKind::KwPublic) {
            let p2 = self.lex.peek();
            if p2.kind != TokenKind::KwStatic {
                break;
            }
            let p3 = self.lex.peek_next();
            if p3.kind == TokenKind::KwVoid {
                break;
            }

            let line = self.cur.line;
            self.advance(); // consume "public"
            self.advance(); // consume "static"

            let mut method_node = make_node(NodeType::StaticMethodDecl, line);
            method_node.has_curly_braces = true;
            let ret_type = self.parse_type();
            add_child(&mut method_node, Some(ret_type));
            let method_name = self.expect(TokenKind::Identifier);
            method_node.str_val = method_name.text;
            self.expect(TokenKind::TokLparen);
            let formals = self.parse_formal_list();
            add_child(&mut method_node, formals);
            self.expect(TokenKind::TokRparen);
            self.expect(TokenKind::TokLbrace);
            let body = self.parse_statement_list();
            add_child(&mut method_node, Some(body));
            self.expect(TokenKind::TokRbrace);
            add_child(&mut node, Some(method_node));
        }
        node
    }

    fn parse_static_var_decl(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::StaticVarDecl, self.cur.line);
        self.expect(TokenKind::KwPrivate);
        self.expect(TokenKind::KwStatic);
        let type_node = self.parse_type();
        let id = self.expect(TokenKind::Identifier);

        let mut var_decl = make_node(NodeType::VarDecl, id.line);
        var_decl.str_val = id.text;
        var_decl.is_static = true;
        add_child(&mut var_decl, Some(type_node));

        if self.match_tok(TokenKind::TokAssign) {
            let mut init = make_node(NodeType::Init, self.cur.line);
            let exp = self.parse_expression(0);
            add_child(&mut init, Some(exp));
            add_child(&mut var_decl, Some(init));
        }

        while self.match_tok(TokenKind::TokComma) {
            let next_id = self.expect(TokenKind::Identifier);
            let mut id_exp_node = make_node(NodeType::IdExpList, next_id.line);
            id_exp_node.str_val = next_id.text;
            if self.match_tok(TokenKind::TokAssign) {
                let mut init2 = make_node(NodeType::Init, self.cur.line);
                let exp2 = self.parse_expression(0);
                add_child(&mut init2, Some(exp2));
                add_child(&mut id_exp_node, Some(init2));
            }
            add_child(&mut var_decl, Some(id_exp_node));
        }
        self.expect(TokenKind::TokSemi);
        add_child(&mut node, Some(var_decl));
        node
    }

    fn parse_formal_list(&mut self) -> Option<Box<ASTNode>> {
        if self.check(TokenKind::TokRparen) {
            return None;
        }

        let type_node = self.parse_type();
        let id = self.expect(TokenKind::Identifier);
        let mut node = make_node(NodeType::FormalList, id.line);
        node.str_val = id.text;
        add_child(&mut node, Some(type_node));

        if self.match_tok(TokenKind::TokComma) {
            let rest = self.parse_formal_list();
            add_child(&mut node, rest);
        }
        Some(node)
    }

    fn parse_type(&mut self) -> Box<ASTNode> {
        let prime = self.parse_prime_type();
        if self.match_tok(TokenKind::TokLbracket) {
            self.expect(TokenKind::TokRbracket);
            if self.match_tok(TokenKind::TokLbracket) {
                self.expect(TokenKind::TokRbracket);
                let mut node = make_node(NodeType::TypeTwoArr, prime.line);
                add_child(&mut node, Some(prime));
                return node;
            }
            let mut node = make_node(NodeType::TypeOneArr, prime.line);
            add_child(&mut node, Some(prime));
            return node;
        }
        prime
    }

    fn parse_prime_type(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::PrimeType, self.cur.line);
        if self.match_tok(TokenKind::KwInt) {
            node.data_type = DataType::Int;
        } else if self.match_tok(TokenKind::KwBoolean) {
            node.data_type = DataType::Boolean;
        } else if self.match_tok(TokenKind::KwString) {
            node.data_type = DataType::String;
        } else {
            self.error("Expected type (int, boolean, or String)");
        }
        node
    }

    fn parse_statement_list(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::StatementList, self.cur.line);
        node.has_curly_braces = true;

        while !self.check(TokenKind::TokRbrace) && !self.check(TokenKind::TokEof) {
            let stmt = self.parse_statement();
            add_child(&mut node, Some(stmt));
        }
        node
    }

    fn parse_var_decl_rest(
        &mut self,
        type_node: Box<ASTNode>,
        id: &str,
        line: i32,
    ) -> Box<ASTNode> {
        let mut node = make_node(NodeType::VarDecl, line);
        node.str_val = id.to_string();
        add_child(&mut node, Some(type_node));

        if self.match_tok(TokenKind::TokAssign) {
            let mut init = make_node(NodeType::Init, self.cur.line);
            let exp = self.parse_expression(0);
            add_child(&mut init, Some(exp));
            add_child(&mut node, Some(init));
        }

        while self.match_tok(TokenKind::TokComma) {
            let next_id = self.expect(TokenKind::Identifier);
            let mut id_exp_node = make_node(NodeType::IdExpList, next_id.line);
            id_exp_node.str_val = next_id.text;
            if self.match_tok(TokenKind::TokAssign) {
                let mut init2 = make_node(NodeType::Init, self.cur.line);
                let exp2 = self.parse_expression(0);
                add_child(&mut init2, Some(exp2));
                add_child(&mut id_exp_node, Some(init2));
            }
            add_child(&mut node, Some(id_exp_node));
        }
        self.expect(TokenKind::TokSemi);
        node
    }

    fn parse_statement(&mut self) -> Box<ASTNode> {
        let line = self.cur.line;

        // { StatementList }
        if self.match_tok(TokenKind::TokLbrace) {
            let mut stmt = make_node(NodeType::Statement, line);
            stmt.has_curly_braces = true;
            let stmts = self.parse_statement_list();
            add_child(&mut stmt, Some(stmts));
            self.expect(TokenKind::TokRbrace);
            return stmt;
        }

        // if ( Exp ) Statement else Statement
        if self.match_tok(TokenKind::KwIf) {
            let mut node = make_node(NodeType::If, line);
            self.expect(TokenKind::TokLparen);
            let cond = self.parse_expression(0);
            let mut cond_wrap = make_node(NodeType::Exp, cond.line);
            add_child(&mut cond_wrap, Some(cond));
            add_child(&mut node, Some(cond_wrap));
            self.expect(TokenKind::TokRparen);
            let then_stmt = self.parse_statement();
            add_child(&mut node, Some(then_stmt));
            self.expect(TokenKind::KwElse);
            let else_stmt = self.parse_statement();
            add_child(&mut node, Some(else_stmt));
            return node;
        }

        // while ( Exp ) Statement
        if self.match_tok(TokenKind::KwWhile) {
            let mut node = make_node(NodeType::While, line);
            self.expect(TokenKind::TokLparen);
            let cond = self.parse_expression(0);
            let mut cond_wrap = make_node(NodeType::Exp, cond.line);
            add_child(&mut cond_wrap, Some(cond));
            add_child(&mut node, Some(cond_wrap));
            self.expect(TokenKind::TokRparen);
            let body = self.parse_statement();
            add_child(&mut node, Some(body));
            return node;
        }

        // System.out.println ( Exp ) ;
        if self.match_tok(TokenKind::SystemOutPrintln) {
            let mut node = make_node(NodeType::Println, line);
            self.expect(TokenKind::TokLparen);
            let exp = self.parse_expression(0);
            add_child(&mut node, Some(exp));
            self.expect(TokenKind::TokRparen);
            self.expect(TokenKind::TokSemi);
            return node;
        }

        // System.out.print ( Exp ) ;
        if self.match_tok(TokenKind::SystemOutPrint) {
            let mut node = make_node(NodeType::Print, line);
            self.expect(TokenKind::TokLparen);
            let exp = self.parse_expression(0);
            add_child(&mut node, Some(exp));
            self.expect(TokenKind::TokRparen);
            self.expect(TokenKind::TokSemi);
            return node;
        }

        // return Exp ;
        if self.match_tok(TokenKind::KwReturn) {
            let mut node = make_node(NodeType::Return, line);
            let exp = self.parse_expression(0);
            add_child(&mut node, Some(exp));
            self.expect(TokenKind::TokSemi);
            return node;
        }

        // VarDecl: Type ID ...
        if self.is_type_keyword(self.cur.kind) {
            let type_node = self.parse_type();
            let id = self.expect(TokenKind::Identifier);
            return self.parse_var_decl_rest(type_node, &id.text, line);
        }

        // ID can be: assignment, method call, or array ops
        if self.check(TokenKind::Identifier) {
            let id = self.advance();

            // Method call: ID ( ExpList )
            if self.check(TokenKind::TokLparen) {
                if id.text == "main" {
                    let mut call_node = make_node(NodeType::CallMain, line);
                    call_node.str_val = id.text;
                    self.expect(TokenKind::TokLparen);
                    if !self.check(TokenKind::TokRparen) {
                        let args = self.parse_exp_list();
                        add_child(&mut call_node, Some(args));
                    }
                    self.expect(TokenKind::TokRparen);
                    let mut stmt_call = make_node(NodeType::StatementMethodCall, line);
                    add_child(&mut stmt_call, Some(call_node));
                    self.expect(TokenKind::TokSemi);
                    return stmt_call;
                }
                let mut method_node = make_node(NodeType::MethodCall, line);
                method_node.str_val = id.text;
                self.expect(TokenKind::TokLparen);
                if !self.check(TokenKind::TokRparen) {
                    let args = self.parse_exp_list();
                    add_child(&mut method_node, Some(args));
                }
                self.expect(TokenKind::TokRparen);
                let mut stmt_call = make_node(NodeType::StatementMethodCall, line);
                add_child(&mut stmt_call, Some(method_node));
                self.expect(TokenKind::TokSemi);
                return stmt_call;
            }

            // Array index or type declaration
            if self.check(TokenKind::TokLbracket) {
                self.advance(); // consume [
                if self.check(TokenKind::TokRbracket) {
                    // Type declaration: ID[] or ID[][]
                    self.advance(); // consume ]
                    let mut prime_type = make_node(NodeType::PrimeType, line);
                    match id.text.as_str() {
                        "int" => prime_type.data_type = DataType::Int,
                        "boolean" => prime_type.data_type = DataType::Boolean,
                        "String" => prime_type.data_type = DataType::String,
                        _ => self.error(&format!("Unknown type: {}", id.text)),
                    }

                    let type_node;
                    if self.match_tok(TokenKind::TokLbracket) {
                        self.expect(TokenKind::TokRbracket);
                        let mut tn = make_node(NodeType::TypeTwoArr, line);
                        add_child(&mut tn, Some(prime_type));
                        type_node = tn;
                    } else {
                        let mut tn = make_node(NodeType::TypeOneArr, line);
                        add_child(&mut tn, Some(prime_type));
                        type_node = tn;
                    }
                    let var_id = self.expect(TokenKind::Identifier);
                    return self.parse_var_decl_rest(type_node, &var_id.text, line);
                } else {
                    // Array assignment: ID[exp]... = Exp
                    let index1 = self.parse_expression(0);
                    self.expect(TokenKind::TokRbracket);

                    if self.check(TokenKind::TokLbracket) {
                        // 2D: ID[exp][exp] = Exp
                        self.advance(); // consume [
                        let index2 = self.parse_expression(0);
                        self.expect(TokenKind::TokRbracket);
                        self.expect(TokenKind::TokAssign);
                        let rhs = self.parse_expression(0);
                        self.expect(TokenKind::TokSemi);

                        let mut assign_node = make_node(NodeType::Assign, line);
                        let mut left_val = make_node(NodeType::LeftvalueTwoArr, line);
                        left_val.str_val = id.text;
                        add_child(&mut left_val, Some(index1));
                        add_child(&mut left_val, Some(index2));
                        add_child(&mut assign_node, Some(left_val));
                        let mut rhs_wrap = make_node(NodeType::Exp, rhs.line);
                        add_child(&mut rhs_wrap, Some(rhs));
                        add_child(&mut assign_node, Some(rhs_wrap));
                        return assign_node;
                    }

                    if self.check(TokenKind::TokDot) {
                        self.error("Cannot use array access expression as statement");
                    }

                    self.expect(TokenKind::TokAssign);
                    let rhs = self.parse_expression(0);
                    self.expect(TokenKind::TokSemi);

                    let mut assign_node = make_node(NodeType::Assign, line);
                    let mut left_val = make_node(NodeType::LeftvalueOneArr, line);
                    left_val.str_val = id.text;
                    add_child(&mut left_val, Some(index1));
                    add_child(&mut assign_node, Some(left_val));
                    let mut rhs_wrap = make_node(NodeType::Exp, rhs.line);
                    add_child(&mut rhs_wrap, Some(rhs));
                    add_child(&mut assign_node, Some(rhs_wrap));
                    return assign_node;
                }
            }

            // Simple assignment: ID = Exp ;
            if self.match_tok(TokenKind::TokAssign) {
                let rhs = self.parse_expression(0);
                self.expect(TokenKind::TokSemi);
                let mut assign_node = make_node(NodeType::Assign, line);
                let mut left_val = make_node(NodeType::LeftvalueId, line);
                left_val.str_val = id.text;
                add_child(&mut assign_node, Some(left_val));
                let mut rhs_wrap = make_node(NodeType::Exp, rhs.line);
                add_child(&mut rhs_wrap, Some(rhs));
                add_child(&mut assign_node, Some(rhs_wrap));
                return assign_node;
            }

            self.error(&format!("Unexpected token after identifier '{}'", id.text));
        }

        // Integer.parseInt as statement
        if self.match_tok(TokenKind::IntegerParseInt) {
            self.expect(TokenKind::TokLparen);
            let exp = self.parse_expression(0);
            self.expect(TokenKind::TokRparen);
            self.expect(TokenKind::TokSemi);
            let mut parse_int_node = make_node(NodeType::ParseInt, line);
            add_child(&mut parse_int_node, Some(exp));
            let mut stmt_call = make_node(NodeType::StatementMethodCall, line);
            add_child(&mut stmt_call, Some(parse_int_node));
            return stmt_call;
        }

        self.error(&format!("Unexpected token: {}", self.cur.text));
    }

    fn parse_exp_list(&mut self) -> Box<ASTNode> {
        let mut node = make_node(NodeType::CommaExpList, self.cur.line);
        let first = self.parse_expression(0);
        add_child(&mut node, Some(first));
        while self.match_tok(TokenKind::TokComma) {
            let next = self.parse_expression(0);
            add_child(&mut node, Some(next));
        }
        node
    }

    fn parse_expression(&mut self, min_prec: i32) -> Box<ASTNode> {
        let mut left = self.parse_unary();

        loop {
            let prec = self.get_prec(self.cur.kind);
            if prec < min_prec {
                break;
            }

            let op = self.cur.kind;
            let line = self.cur.line;
            self.advance();

            let right = self.parse_expression(prec + 1);
            let mut bin_node = make_node(self.get_bin_op_node_type(op), line);
            bin_node.str_val = self.get_bin_op_str(op).to_string();
            let mut left_wrap = make_node(NodeType::Exp, left.line);
            add_child(&mut left_wrap, Some(left));
            let mut right_wrap = make_node(NodeType::Exp, right.line);
            add_child(&mut right_wrap, Some(right));
            add_child(&mut bin_node, Some(left_wrap));
            add_child(&mut bin_node, Some(right_wrap));
            left = bin_node;
        }
        left
    }

    fn parse_unary(&mut self) -> Box<ASTNode> {
        let line = self.cur.line;
        if self.match_tok(TokenKind::TokNot) {
            let operand = self.parse_unary();
            let mut node = make_node(NodeType::ExpLogicNot, line);
            let mut wrap = make_node(NodeType::Exp, operand.line);
            add_child(&mut wrap, Some(operand));
            add_child(&mut node, Some(wrap));
            return node;
        }
        if self.match_tok(TokenKind::TokMinus) {
            let operand = self.parse_unary();
            let mut node = make_node(NodeType::ExpNeg, line);
            let mut wrap = make_node(NodeType::Exp, operand.line);
            add_child(&mut wrap, Some(operand));
            add_child(&mut node, Some(wrap));
            return node;
        }
        if self.match_tok(TokenKind::TokPlus) {
            let operand = self.parse_unary();
            let mut node = make_node(NodeType::ExpPos, line);
            let mut wrap = make_node(NodeType::Exp, operand.line);
            add_child(&mut wrap, Some(operand));
            add_child(&mut node, Some(wrap));
            return node;
        }
        self.parse_primary()
    }

    fn parse_primary(&mut self) -> Box<ASTNode> {
        let line = self.cur.line;

        // Integer literal
        if self.check(TokenKind::IntegerLiteral) {
            let t = self.advance();
            let mut node = make_node(NodeType::ExpType, line);
            node.data_type = DataType::Int;
            node.int_val = t.int_value;
            return node;
        }

        // String literal
        if self.check(TokenKind::StringLiteral) {
            let t = self.advance();
            let mut node = make_node(NodeType::ExpType, line);
            node.data_type = DataType::String;
            node.str_val = t.text;
            return node;
        }

        // Boolean literals
        if self.match_tok(TokenKind::KwTrue) {
            let mut node = make_node(NodeType::ExpType, line);
            node.data_type = DataType::Boolean;
            node.bool_val = true;
            return node;
        }
        if self.match_tok(TokenKind::KwFalse) {
            let mut node = make_node(NodeType::ExpType, line);
            node.data_type = DataType::Boolean;
            node.bool_val = false;
            return node;
        }

        // Parenthesized expression
        if self.match_tok(TokenKind::TokLparen) {
            let exp = self.parse_expression(0);
            self.expect(TokenKind::TokRparen);
            return exp;
        }

        // new PrimeType[Exp] or new PrimeType[Exp][Exp]
        if self.match_tok(TokenKind::KwNew) {
            let prime_type = self.parse_prime_type();
            self.expect(TokenKind::TokLbracket);
            let size1 = self.parse_expression(0);
            self.expect(TokenKind::TokRbracket);

            if self.match_tok(TokenKind::TokLbracket) {
                let size2 = self.parse_expression(0);
                self.expect(TokenKind::TokRbracket);
                let mut node = make_node(NodeType::ExpTwoArr, line);
                add_child(&mut node, Some(prime_type));
                let mut wrap1 = make_node(NodeType::Exp, size1.line);
                add_child(&mut wrap1, Some(size1));
                add_child(&mut node, Some(wrap1));
                let mut wrap2 = make_node(NodeType::Exp, size2.line);
                add_child(&mut wrap2, Some(size2));
                add_child(&mut node, Some(wrap2));
                return node;
            }
            let mut node = make_node(NodeType::ExpOneArr, line);
            add_child(&mut node, Some(prime_type));
            let mut wrap1 = make_node(NodeType::Exp, size1.line);
            add_child(&mut wrap1, Some(size1));
            add_child(&mut node, Some(wrap1));
            return node;
        }

        // Integer.parseInt(Exp)
        if self.match_tok(TokenKind::IntegerParseInt) {
            self.expect(TokenKind::TokLparen);
            let exp = self.parse_expression(0);
            self.expect(TokenKind::TokRparen);
            let mut parse_int_node = make_node(NodeType::ParseInt, line);
            add_child(&mut parse_int_node, Some(exp));
            let mut call_node = make_node(NodeType::ExpMethodCall, line);
            add_child(&mut call_node, Some(parse_int_node));
            return call_node;
        }

        // Identifier
        if self.check(TokenKind::Identifier) {
            let id = self.advance();

            // Method call: ID ( ExpList )
            if self.check(TokenKind::TokLparen) {
                let mut call_node = if id.text == "main" {
                    make_node(NodeType::CallMain, line)
                } else {
                    make_node(NodeType::MethodCall, line)
                };
                call_node.str_val = id.text;
                self.expect(TokenKind::TokLparen);
                if !self.check(TokenKind::TokRparen) {
                    let args = self.parse_exp_list();
                    add_child(&mut call_node, Some(args));
                }
                self.expect(TokenKind::TokRparen);
                let mut exp_call = make_node(NodeType::ExpMethodCall, line);
                add_child(&mut exp_call, Some(call_node));
                return exp_call;
            }

            // Array access
            if self.check(TokenKind::TokLbracket) {
                self.advance(); // consume [
                let index1 = self.parse_expression(0);
                self.expect(TokenKind::TokRbracket);

                if self.check(TokenKind::TokLbracket) {
                    self.advance(); // consume [
                    let index2 = self.parse_expression(0);
                    self.expect(TokenKind::TokRbracket);

                    let mut left_val = make_node(NodeType::LeftvalueTwoArr, line);
                    left_val.str_val = id.text;
                    add_child(&mut left_val, Some(index1));
                    add_child(&mut left_val, Some(index2));

                    if self.check(TokenKind::TokDot) {
                        self.advance();
                        self.expect(TokenKind::KwLength);
                        let mut len_node = make_node(NodeType::ExpLength, line);
                        add_child(&mut len_node, Some(left_val));
                        return len_node;
                    }

                    let mut exp_left = make_node(NodeType::ExpLeft, line);
                    add_child(&mut exp_left, Some(left_val));
                    return exp_left;
                }

                let mut left_val = make_node(NodeType::LeftvalueOneArr, line);
                left_val.str_val = id.text;
                add_child(&mut left_val, Some(index1));

                if self.check(TokenKind::TokDot) {
                    self.advance();
                    self.expect(TokenKind::KwLength);
                    let mut len_node = make_node(NodeType::ExpLength, line);
                    add_child(&mut len_node, Some(left_val));
                    return len_node;
                }

                let mut exp_left = make_node(NodeType::ExpLeft, line);
                add_child(&mut exp_left, Some(left_val));
                return exp_left;
            }

            // Simple variable access or .length
            let mut left_val = make_node(NodeType::LeftvalueId, line);
            left_val.str_val = id.text;

            if self.check(TokenKind::TokDot) {
                self.advance();
                self.expect(TokenKind::KwLength);
                let mut len_node = make_node(NodeType::ExpLength, line);
                add_child(&mut len_node, Some(left_val));
                return len_node;
            }

            let mut exp_left = make_node(NodeType::ExpLeft, line);
            add_child(&mut exp_left, Some(left_val));
            return exp_left;
        }

        self.error(&format!(
            "Unexpected token in expression: {}",
            self.cur.text
        ));
    }
}
