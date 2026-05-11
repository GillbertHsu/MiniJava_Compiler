#include "parser.h"
#include <cstdio>
#include <cstdlib>

Parser::Parser(Lexer& lexer) : lex_(lexer) {
    cur_ = lex_.nextToken();
}

Token Parser::advance() {
    Token prev = cur_;
    cur_ = lex_.nextToken();
    return prev;
}

Token Parser::expect(TokenKind kind) {
    if (cur_.kind != kind) {
        fprintf(stderr, "Syntax error at line %d: expected token, got '%s'\n",
                cur_.line, cur_.text.c_str());
        exit(1);
    }
    return advance();
}

bool Parser::check(TokenKind kind) { return cur_.kind == kind; }

bool Parser::match(TokenKind kind) {
    if (check(kind)) { advance(); return true; }
    return false;
}

void Parser::error(const std::string& msg) {
    fprintf(stderr, "Syntax error at line %d: %s\n", cur_.line, msg.c_str());
    exit(1);
}

bool Parser::isTypeKeyword(TokenKind k) {
    return k == TokenKind::KW_INT || k == TokenKind::KW_BOOLEAN || k == TokenKind::KW_STRING;
}

int Parser::getPrec(TokenKind k) {
    switch (k) {
        case TokenKind::TOK_OR:    return 1;
        case TokenKind::TOK_AND:   return 2;
        case TokenKind::TOK_EQ:
        case TokenKind::TOK_NEQ:   return 3;
        case TokenKind::TOK_LT:
        case TokenKind::TOK_GT:
        case TokenKind::TOK_LTE:
        case TokenKind::TOK_GTE:   return 4;
        case TokenKind::TOK_PLUS:
        case TokenKind::TOK_MINUS: return 5;
        case TokenKind::TOK_STAR:
        case TokenKind::TOK_SLASH: return 6;
        default:                    return -1;
    }
}

NodeType Parser::getBinOpNodeType(TokenKind k) {
    switch (k) {
        case TokenKind::TOK_PLUS:  return NodeType::EXP_PLUS;
        case TokenKind::TOK_MINUS:
        case TokenKind::TOK_STAR:
        case TokenKind::TOK_SLASH: return NodeType::EXP_OPERATION;
        case TokenKind::TOK_AND:
        case TokenKind::TOK_OR:    return NodeType::EXP_LOGIC;
        case TokenKind::TOK_EQ:
        case TokenKind::TOK_NEQ:   return NodeType::EXP_COMP;
        case TokenKind::TOK_LT:
        case TokenKind::TOK_GT:
        case TokenKind::TOK_LTE:
        case TokenKind::TOK_GTE:   return NodeType::EXP_OPERATION_COMP;
        default:                    return NodeType::EXP_OPERATION;
    }
}

std::string Parser::getBinOpStr(TokenKind k) {
    switch (k) {
        case TokenKind::TOK_PLUS:  return "+";
        case TokenKind::TOK_MINUS: return "-";
        case TokenKind::TOK_STAR:  return "*";
        case TokenKind::TOK_SLASH: return "/";
        case TokenKind::TOK_AND:   return "&&";
        case TokenKind::TOK_OR:    return "||";
        case TokenKind::TOK_EQ:    return "==";
        case TokenKind::TOK_NEQ:   return "!=";
        case TokenKind::TOK_LT:    return "<";
        case TokenKind::TOK_GT:    return ">";
        case TokenKind::TOK_LTE:   return "<=";
        case TokenKind::TOK_GTE:   return ">=";
        default: return "";
    }
}

std::unique_ptr<ASTNode> Parser::parse() { return parseProgram(); }

// Program -> class ID { StaticVarDeclList StaticMethodDeclList MainMethod }
std::unique_ptr<ASTNode> Parser::parseProgram() {
    auto node = makeNode(NodeType::PROGRAM, cur_.line);
    auto mainClass = parseMainClass();
    addChild(node.get(), std::move(mainClass));
    return node;
}

std::unique_ptr<ASTNode> Parser::parseMainClass() {
    auto node = makeNode(NodeType::MAINCLASS, cur_.line);
    node->has_curly_braces = true;
    expect(TokenKind::KW_CLASS);
    Token name = expect(TokenKind::IDENTIFIER);
    node->str_val = name.text;
    expect(TokenKind::TOK_LBRACE);

    auto varList = parseStaticVarDeclList();
    addChild(node.get(), std::move(varList));

    auto methodList = parseStaticMethodDeclList();
    addChild(node.get(), std::move(methodList));

    auto mainMethod = parseMainMethod();
    addChild(node.get(), std::move(mainMethod));

    expect(TokenKind::TOK_RBRACE);
    return node;
}

// MainMethod -> public static void main ( String [] ID ) { StatementList }
std::unique_ptr<ASTNode> Parser::parseMainMethod() {
    auto node = makeNode(NodeType::MAINMETHOD, cur_.line);
    node->has_curly_braces = true;
    expect(TokenKind::KW_PUBLIC);
    expect(TokenKind::KW_STATIC);
    expect(TokenKind::KW_VOID);
    expect(TokenKind::KW_MAIN);
    expect(TokenKind::TOK_LPAREN);
    expect(TokenKind::KW_STRING);
    expect(TokenKind::TOK_LBRACKET);
    expect(TokenKind::TOK_RBRACKET);
    Token argsName = expect(TokenKind::IDENTIFIER);
    node->str_val = argsName.text;
    expect(TokenKind::TOK_RPAREN);
    expect(TokenKind::TOK_LBRACE);
    auto stmts = parseStatementList();
    addChild(node.get(), std::move(stmts));
    expect(TokenKind::TOK_RBRACE);
    return node;
}

// StaticVarDeclList -> (private static VarDecl)*
std::unique_ptr<ASTNode> Parser::parseStaticVarDeclList() {
    auto node = makeNode(NodeType::STATIC_VAR_DECL_LIST, cur_.line);
    // Look ahead: "private static" signals a static var decl
    while (check(TokenKind::KW_PRIVATE)) {
        auto decl = parseStaticVarDecl();
        addChild(node.get(), std::move(decl));
    }
    return node;
}

// StaticMethodDeclList -> (public static Type ID (...) { ... })*
std::unique_ptr<ASTNode> Parser::parseStaticMethodDeclList() {
    auto node = makeNode(NodeType::STATIC_METHOD_DECL_LIST, cur_.line);
    // Look ahead: "public static" but NOT "void" (that's the main method)
    while (check(TokenKind::KW_PUBLIC)) {
        // Use 2-token lookahead: peek at what follows "public"
        Token p2 = lex_.peek();
        if (p2.kind != TokenKind::KW_STATIC) break;
        // Now peek one more: the token after "public static"
        Token p3 = lex_.peekNext();
        if (p3.kind == TokenKind::KW_VOID) break; // That's the main method

        // It's a method declaration: public static Type ID(...)
        int line = cur_.line;
        advance(); // consume "public"
        advance(); // consume "static"

        auto methodNode = makeNode(NodeType::STATIC_METHOD_DECL, line);
        methodNode->has_curly_braces = true;
        auto retType = parseType();
        addChild(methodNode.get(), std::move(retType));
        Token methodName = expect(TokenKind::IDENTIFIER);
        methodNode->str_val = methodName.text;
        expect(TokenKind::TOK_LPAREN);
        auto formals = parseFormalList();
        addChild(methodNode.get(), std::move(formals));
        expect(TokenKind::TOK_RPAREN);
        expect(TokenKind::TOK_LBRACE);
        auto body = parseStatementList();
        addChild(methodNode.get(), std::move(body));
        expect(TokenKind::TOK_RBRACE);
        addChild(node.get(), std::move(methodNode));
    }
    return node;
}

// StaticVarDecl -> private static VarDecl
std::unique_ptr<ASTNode> Parser::parseStaticVarDecl() {
    auto node = makeNode(NodeType::STATIC_VAR_DECL, cur_.line);
    expect(TokenKind::KW_PRIVATE);
    expect(TokenKind::KW_STATIC);
    // Now parse the type
    auto typeNode = parseType();
    Token id = expect(TokenKind::IDENTIFIER);

    auto varDecl = makeNode(NodeType::VAR_DECL, id.line);
    varDecl->str_val = id.text;
    varDecl->is_static = true;
    addChild(varDecl.get(), std::move(typeNode));

    // Check for init: = Exp
    if (match(TokenKind::TOK_ASSIGN)) {
        auto init = makeNode(NodeType::INIT, cur_.line);
        auto exp = parseExpression();
        addChild(init.get(), std::move(exp));
        addChild(varDecl.get(), std::move(init));
    }

    // Check for idExpList: , ID = Exp, ...
    while (match(TokenKind::TOK_COMMA)) {
        Token nextId = expect(TokenKind::IDENTIFIER);
        auto idExpNode = makeNode(NodeType::ID_EXP_LIST, nextId.line);
        idExpNode->str_val = nextId.text;
        if (match(TokenKind::TOK_ASSIGN)) {
            auto init2 = makeNode(NodeType::INIT, cur_.line);
            auto exp2 = parseExpression();
            addChild(init2.get(), std::move(exp2));
            addChild(idExpNode.get(), std::move(init2));
        }
        addChild(varDecl.get(), std::move(idExpNode));
    }
    expect(TokenKind::TOK_SEMI);
    addChild(node.get(), std::move(varDecl));
    return node;
}

// FormalList -> (Type ID (, Type ID)*)?
std::unique_ptr<ASTNode> Parser::parseFormalList() {
    if (check(TokenKind::TOK_RPAREN)) return nullptr;

    auto typeNode = parseType();
    Token id = expect(TokenKind::IDENTIFIER);
    auto node = makeNode(NodeType::FORMAL_LIST, id.line);
    node->str_val = id.text;
    addChild(node.get(), std::move(typeNode));

    // Additional parameters
    if (match(TokenKind::TOK_COMMA)) {
        auto rest = parseFormalList();
        addChild(node.get(), std::move(rest));
    }
    return node;
}

// Type -> PrimeType ([] | [][] )?
std::unique_ptr<ASTNode> Parser::parseType() {
    auto prime = parsePrimeType();
    if (match(TokenKind::TOK_LBRACKET)) {
        expect(TokenKind::TOK_RBRACKET);
        if (match(TokenKind::TOK_LBRACKET)) {
            expect(TokenKind::TOK_RBRACKET);
            auto node = makeNode(NodeType::TYPE_TWO_ARR, prime->line);
            addChild(node.get(), std::move(prime));
            return node;
        }
        auto node = makeNode(NodeType::TYPE_ONE_ARR, prime->line);
        addChild(node.get(), std::move(prime));
        return node;
    }
    return prime;
}

// PrimeType -> int | boolean | String
std::unique_ptr<ASTNode> Parser::parsePrimeType() {
    auto node = makeNode(NodeType::PRIME_TYPE, cur_.line);
    if (match(TokenKind::KW_INT)) {
        node->data_type = DataType::INT;
    } else if (match(TokenKind::KW_BOOLEAN)) {
        node->data_type = DataType::BOOLEAN;
    } else if (match(TokenKind::KW_STRING)) {
        node->data_type = DataType::STRING;
    } else {
        error("Expected type (int, boolean, or String)");
    }
    return node;
}

// StatementList -> Statement*
std::unique_ptr<ASTNode> Parser::parseStatementList() {
    auto node = makeNode(NodeType::STATEMENT_LIST, cur_.line);
    node->has_curly_braces = true;

    while (!check(TokenKind::TOK_RBRACE) && !check(TokenKind::TOK_EOF)) {
        auto stmt = parseStatement();
        if (stmt) addChild(node.get(), std::move(stmt));
    }
    return node;
}

// VarDeclRest: we already consumed Type and ID, parse the rest
std::unique_ptr<ASTNode> Parser::parseVarDeclRest(std::unique_ptr<ASTNode> typeNode,
                                                    const std::string& id, int line) {
    auto node = makeNode(NodeType::VAR_DECL, line);
    node->str_val = id;
    addChild(node.get(), std::move(typeNode));

    // Optional init: = Exp
    if (match(TokenKind::TOK_ASSIGN)) {
        auto init = makeNode(NodeType::INIT, cur_.line);
        auto exp = parseExpression();
        addChild(init.get(), std::move(exp));
        addChild(node.get(), std::move(init));
    }

    // Optional additional declarations: , ID (= Exp)?
    while (match(TokenKind::TOK_COMMA)) {
        Token nextId = expect(TokenKind::IDENTIFIER);
        auto idExpNode = makeNode(NodeType::ID_EXP_LIST, nextId.line);
        idExpNode->str_val = nextId.text;
        if (match(TokenKind::TOK_ASSIGN)) {
            auto init2 = makeNode(NodeType::INIT, cur_.line);
            auto exp2 = parseExpression();
            addChild(init2.get(), std::move(exp2));
            addChild(idExpNode.get(), std::move(init2));
        }
        addChild(node.get(), std::move(idExpNode));
    }
    expect(TokenKind::TOK_SEMI);
    return node;
}

/*
 Statement ->
   { StatementList }
   | if ( Exp ) Statement else Statement
   | while ( Exp ) Statement
   | System.out.println ( Exp ) ;
   | System.out.print ( Exp ) ;
   | return Exp ;
   | Type ID ... ; (VarDecl)
   | LeftValue = Exp ; (Assign)
   | MethodCall ;
*/
std::unique_ptr<ASTNode> Parser::parseStatement() {
    int line = cur_.line;

    // { StatementList }
    if (match(TokenKind::TOK_LBRACE)) {
        auto stmt = makeNode(NodeType::STATEMENT, line);
        stmt->has_curly_braces = true;
        auto stmts = parseStatementList();
        addChild(stmt.get(), std::move(stmts));
        expect(TokenKind::TOK_RBRACE);
        return stmt;
    }

    // if ( Exp ) Statement else Statement
    if (match(TokenKind::KW_IF)) {
        auto node = makeNode(NodeType::IF, line);
        expect(TokenKind::TOK_LPAREN);
        auto cond = parseExpression();
        auto condWrap = makeNode(NodeType::EXP, cond->line);
        addChild(condWrap.get(), std::move(cond));
        addChild(node.get(), std::move(condWrap));
        expect(TokenKind::TOK_RPAREN);
        auto thenStmt = parseStatement();
        addChild(node.get(), std::move(thenStmt));
        expect(TokenKind::KW_ELSE);
        auto elseStmt = parseStatement();
        addChild(node.get(), std::move(elseStmt));
        return node;
    }

    // while ( Exp ) Statement
    if (match(TokenKind::KW_WHILE)) {
        auto node = makeNode(NodeType::WHILE, line);
        expect(TokenKind::TOK_LPAREN);
        auto cond = parseExpression();
        auto condWrap = makeNode(NodeType::EXP, cond->line);
        addChild(condWrap.get(), std::move(cond));
        addChild(node.get(), std::move(condWrap));
        expect(TokenKind::TOK_RPAREN);
        auto body = parseStatement();
        addChild(node.get(), std::move(body));
        return node;
    }

    // System.out.println ( Exp ) ;
    if (match(TokenKind::SYSTEM_OUT_PRINTLN)) {
        auto node = makeNode(NodeType::PRINTLN, line);
        expect(TokenKind::TOK_LPAREN);
        auto exp = parseExpression();
        addChild(node.get(), std::move(exp));
        expect(TokenKind::TOK_RPAREN);
        expect(TokenKind::TOK_SEMI);
        return node;
    }

    // System.out.print ( Exp ) ;
    if (match(TokenKind::SYSTEM_OUT_PRINT)) {
        auto node = makeNode(NodeType::PRINT, line);
        expect(TokenKind::TOK_LPAREN);
        auto exp = parseExpression();
        addChild(node.get(), std::move(exp));
        expect(TokenKind::TOK_RPAREN);
        expect(TokenKind::TOK_SEMI);
        return node;
    }

    // return Exp ;
    if (match(TokenKind::KW_RETURN)) {
        auto node = makeNode(NodeType::RETURN, line);
        auto exp = parseExpression();
        addChild(node.get(), std::move(exp));
        expect(TokenKind::TOK_SEMI);
        return node;
    }

    // VarDecl: Type ID ...
    // Type starts with: int, boolean, String
    if (isTypeKeyword(cur_.kind)) {
        auto typeNode = parseType();
        Token id = expect(TokenKind::IDENTIFIER);
        return parseVarDeclRest(std::move(typeNode), id.text, line);
    }

    // ID can be: assignment (LeftValue = Exp), method call (ID(...)),
    //            or var decl if ID is a class/type name (not applicable in MiniJava)
    if (check(TokenKind::IDENTIFIER)) {
        Token id = advance();

        // Method call: ID ( ExpList )
        if (check(TokenKind::TOK_LPAREN)) {
            // But also check for main(...)
            if (id.text == "main") {
                auto callNode = makeNode(NodeType::CALL_MAIN, line);
                callNode->str_val = id.text;
                expect(TokenKind::TOK_LPAREN);
                if (!check(TokenKind::TOK_RPAREN)) {
                    auto args = parseExpList();
                    addChild(callNode.get(), std::move(args));
                }
                expect(TokenKind::TOK_RPAREN);
                auto stmtCall = makeNode(NodeType::STATEMENT_METHOD_CALL, line);
                addChild(stmtCall.get(), std::move(callNode));
                expect(TokenKind::TOK_SEMI);
                return stmtCall;
            }
            auto methodNode = makeNode(NodeType::METHOD_CALL, line);
            methodNode->str_val = id.text;
            expect(TokenKind::TOK_LPAREN);
            if (!check(TokenKind::TOK_RPAREN)) {
                auto args = parseExpList();
                addChild(methodNode.get(), std::move(args));
            }
            expect(TokenKind::TOK_RPAREN);
            auto stmtCall = makeNode(NodeType::STATEMENT_METHOD_CALL, line);
            addChild(stmtCall.get(), std::move(methodNode));
            expect(TokenKind::TOK_SEMI);
            return stmtCall;
        }

        // Array index or simple assignment: ID [ Exp ] ([ Exp ])? = Exp
        // Or: ID = Exp
        if (check(TokenKind::TOK_LBRACKET)) {
            // Could be array type declaration: ID[] id = ...
            // Or array assignment: ID[exp] = ...
            // Peek: if [ is followed by ], it's a type declaration
            advance(); // consume [
            if (check(TokenKind::TOK_RBRACKET)) {
                // Type declaration: ID[] or ID[][]
                advance(); // consume ]
                auto primeType = makeNode(NodeType::PRIME_TYPE, line);
                // ID must be a known type name - in MiniJava, types are only int/boolean/String
                // But the grammar allows identifiers as type names in this position
                if (id.text == "int") primeType->data_type = DataType::INT;
                else if (id.text == "boolean") primeType->data_type = DataType::BOOLEAN;
                else if (id.text == "String") primeType->data_type = DataType::STRING;
                else error("Unknown type: " + id.text);

                std::unique_ptr<ASTNode> typeNode;
                if (match(TokenKind::TOK_LBRACKET)) {
                    expect(TokenKind::TOK_RBRACKET);
                    typeNode = makeNode(NodeType::TYPE_TWO_ARR, line);
                    addChild(typeNode.get(), std::move(primeType));
                } else {
                    typeNode = makeNode(NodeType::TYPE_ONE_ARR, line);
                    addChild(typeNode.get(), std::move(primeType));
                }
                Token varId = expect(TokenKind::IDENTIFIER);
                return parseVarDeclRest(std::move(typeNode), varId.text, line);
            } else {
                // Array assignment: ID[exp]... = Exp
                auto index1 = parseExpression();
                expect(TokenKind::TOK_RBRACKET);

                if (check(TokenKind::TOK_LBRACKET)) {
                    // 2D: ID[exp][exp] = Exp
                    advance(); // consume [
                    auto index2 = parseExpression();
                    expect(TokenKind::TOK_RBRACKET);
                    expect(TokenKind::TOK_ASSIGN);
                    auto rhs = parseExpression();
                    expect(TokenKind::TOK_SEMI);

                    auto assignNode = makeNode(NodeType::ASSIGN, line);
                    auto leftVal = makeNode(NodeType::LEFTVALUE_TWO_ARR, line);
                    leftVal->str_val = id.text;
                    addChild(leftVal.get(), std::move(index1));
                    addChild(leftVal.get(), std::move(index2));
                    addChild(assignNode.get(), std::move(leftVal));
                    auto rhsWrap = makeNode(NodeType::EXP, rhs->line);
                    addChild(rhsWrap.get(), std::move(rhs));
                    addChild(assignNode.get(), std::move(rhsWrap));
                    return assignNode;
                }

                // Check for .length
                if (check(TokenKind::TOK_DOT)) {
                    // This would be id[exp].length used in an expression, not a statement
                    // In statement context this shouldn't appear, but handle gracefully
                    error("Cannot use array access expression as statement");
                }

                expect(TokenKind::TOK_ASSIGN);
                auto rhs = parseExpression();
                expect(TokenKind::TOK_SEMI);

                auto assignNode = makeNode(NodeType::ASSIGN, line);
                auto leftVal = makeNode(NodeType::LEFTVALUE_ONE_ARR, line);
                leftVal->str_val = id.text;
                addChild(leftVal.get(), std::move(index1));
                addChild(assignNode.get(), std::move(leftVal));
                auto rhsWrap = makeNode(NodeType::EXP, rhs->line);
                addChild(rhsWrap.get(), std::move(rhs));
                addChild(assignNode.get(), std::move(rhsWrap));
                return assignNode;
            }
        }

        // Simple assignment: ID = Exp ;
        if (match(TokenKind::TOK_ASSIGN)) {
            auto rhs = parseExpression();
            expect(TokenKind::TOK_SEMI);
            auto assignNode = makeNode(NodeType::ASSIGN, line);
            auto leftVal = makeNode(NodeType::LEFTVALUE_ID, line);
            leftVal->str_val = id.text;
            addChild(assignNode.get(), std::move(leftVal));
            auto rhsWrap = makeNode(NodeType::EXP, rhs->line);
            addChild(rhsWrap.get(), std::move(rhs));
            addChild(assignNode.get(), std::move(rhsWrap));
            return assignNode;
        }

        error("Unexpected token after identifier '" + id.text + "'");
    }

    // Integer.parseInt as statement (unlikely but handle)
    if (match(TokenKind::INTEGER_PARSEINT)) {
        expect(TokenKind::TOK_LPAREN);
        auto exp = parseExpression();
        expect(TokenKind::TOK_RPAREN);
        expect(TokenKind::TOK_SEMI);
        auto parseIntNode = makeNode(NodeType::PARSE_INT, line);
        addChild(parseIntNode.get(), std::move(exp));
        auto stmtCall = makeNode(NodeType::STATEMENT_METHOD_CALL, line);
        addChild(stmtCall.get(), std::move(parseIntNode));
        return stmtCall;
    }

    error("Unexpected token: " + cur_.text);
    return nullptr;
}

// ExpList -> Exp (, Exp)*
std::unique_ptr<ASTNode> Parser::parseExpList() {
    auto node = makeNode(NodeType::COMMA_EXP_LIST, cur_.line);
    auto first = parseExpression();
    addChild(node.get(), std::move(first));
    while (match(TokenKind::TOK_COMMA)) {
        auto next = parseExpression();
        addChild(node.get(), std::move(next));
    }
    return node;
}

// Expression parsing with precedence climbing
std::unique_ptr<ASTNode> Parser::parseExpression(int minPrec) {
    auto left = parseUnary();

    while (true) {
        int prec = getPrec(cur_.kind);
        if (prec < minPrec) break;

        TokenKind op = cur_.kind;
        int line = cur_.line;
        advance();

        auto right = parseExpression(prec + 1); // left-associative
        auto binNode = makeNode(getBinOpNodeType(op), line);
        binNode->str_val = getBinOpStr(op);
        auto leftWrap = makeNode(NodeType::EXP, left->line);
        addChild(leftWrap.get(), std::move(left));
        auto rightWrap = makeNode(NodeType::EXP, right->line);
        addChild(rightWrap.get(), std::move(right));
        addChild(binNode.get(), std::move(leftWrap));
        addChild(binNode.get(), std::move(rightWrap));
        left = std::move(binNode);
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnary() {
    int line = cur_.line;
    if (match(TokenKind::TOK_NOT)) {
        auto operand = parseUnary();
        auto node = makeNode(NodeType::EXP_LOGIC_NOT, line);
        auto wrap = makeNode(NodeType::EXP, operand->line);
        addChild(wrap.get(), std::move(operand));
        addChild(node.get(), std::move(wrap));
        return node;
    }
    if (match(TokenKind::TOK_MINUS)) {
        auto operand = parseUnary();
        auto node = makeNode(NodeType::EXP_NEG, line);
        auto wrap = makeNode(NodeType::EXP, operand->line);
        addChild(wrap.get(), std::move(operand));
        addChild(node.get(), std::move(wrap));
        return node;
    }
    if (match(TokenKind::TOK_PLUS)) {
        auto operand = parseUnary();
        auto node = makeNode(NodeType::EXP_POS, line);
        auto wrap = makeNode(NodeType::EXP, operand->line);
        addChild(wrap.get(), std::move(operand));
        addChild(node.get(), std::move(wrap));
        return node;
    }
    return parsePrimary();
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    int line = cur_.line;

    // Integer literal
    if (check(TokenKind::INTEGER_LITERAL)) {
        Token t = advance();
        auto node = makeNode(NodeType::EXP_TYPE, line);
        node->data_type = DataType::INT;
        node->int_val = t.int_value;
        return node;
    }

    // String literal
    if (check(TokenKind::STRING_LITERAL)) {
        Token t = advance();
        auto node = makeNode(NodeType::EXP_TYPE, line);
        node->data_type = DataType::STRING;
        node->str_val = t.text;
        return node;
    }

    // Boolean literals
    if (match(TokenKind::KW_TRUE)) {
        auto node = makeNode(NodeType::EXP_TYPE, line);
        node->data_type = DataType::BOOLEAN;
        node->bool_val = true;
        return node;
    }
    if (match(TokenKind::KW_FALSE)) {
        auto node = makeNode(NodeType::EXP_TYPE, line);
        node->data_type = DataType::BOOLEAN;
        node->bool_val = false;
        return node;
    }

    // Parenthesized expression
    if (match(TokenKind::TOK_LPAREN)) {
        auto exp = parseExpression();
        expect(TokenKind::TOK_RPAREN);
        return exp;
    }

    // new PrimeType[Exp] or new PrimeType[Exp][Exp]
    if (match(TokenKind::KW_NEW)) {
        auto primeType = parsePrimeType();
        expect(TokenKind::TOK_LBRACKET);
        auto size1 = parseExpression();
        expect(TokenKind::TOK_RBRACKET);

        if (match(TokenKind::TOK_LBRACKET)) {
            auto size2 = parseExpression();
            expect(TokenKind::TOK_RBRACKET);
            auto node = makeNode(NodeType::EXP_TWO_ARR, line);
            addChild(node.get(), std::move(primeType));
            auto wrap1 = makeNode(NodeType::EXP, size1->line);
            addChild(wrap1.get(), std::move(size1));
            addChild(node.get(), std::move(wrap1));
            auto wrap2 = makeNode(NodeType::EXP, size2->line);
            addChild(wrap2.get(), std::move(size2));
            addChild(node.get(), std::move(wrap2));
            return node;
        }
        auto node = makeNode(NodeType::EXP_ONE_ARR, line);
        addChild(node.get(), std::move(primeType));
        auto wrap1 = makeNode(NodeType::EXP, size1->line);
        addChild(wrap1.get(), std::move(size1));
        addChild(node.get(), std::move(wrap1));
        return node;
    }

    // Integer.parseInt(Exp)
    if (match(TokenKind::INTEGER_PARSEINT)) {
        expect(TokenKind::TOK_LPAREN);
        auto exp = parseExpression();
        expect(TokenKind::TOK_RPAREN);
        auto parseIntNode = makeNode(NodeType::PARSE_INT, line);
        addChild(parseIntNode.get(), std::move(exp));
        auto callNode = makeNode(NodeType::EXP_METHOD_CALL, line);
        addChild(callNode.get(), std::move(parseIntNode));
        return callNode;
    }

    // Identifier: could be variable reference, array access, method call, or .length
    if (check(TokenKind::IDENTIFIER)) {
        Token id = advance();

        // Method call: ID ( ExpList )
        if (check(TokenKind::TOK_LPAREN)) {
            std::unique_ptr<ASTNode> callNode;
            if (id.text == "main") {
                callNode = makeNode(NodeType::CALL_MAIN, line);
            } else {
                callNode = makeNode(NodeType::METHOD_CALL, line);
            }
            callNode->str_val = id.text;
            expect(TokenKind::TOK_LPAREN);
            if (!check(TokenKind::TOK_RPAREN)) {
                auto args = parseExpList();
                addChild(callNode.get(), std::move(args));
            }
            expect(TokenKind::TOK_RPAREN);
            auto expCall = makeNode(NodeType::EXP_METHOD_CALL, line);
            addChild(expCall.get(), std::move(callNode));
            return expCall;
        }

        // Array access: ID[Exp] or ID[Exp][Exp], or .length after array/id
        if (check(TokenKind::TOK_LBRACKET)) {
            advance(); // consume [
            auto index1 = parseExpression();
            expect(TokenKind::TOK_RBRACKET);

            if (check(TokenKind::TOK_LBRACKET)) {
                advance(); // consume [
                auto index2 = parseExpression();
                expect(TokenKind::TOK_RBRACKET);

                auto leftVal = makeNode(NodeType::LEFTVALUE_TWO_ARR, line);
                leftVal->str_val = id.text;
                addChild(leftVal.get(), std::move(index1));
                addChild(leftVal.get(), std::move(index2));

                // Check for .length
                if (check(TokenKind::TOK_DOT)) {
                    advance();
                    expect(TokenKind::KW_LENGTH);
                    auto lenNode = makeNode(NodeType::EXP_LENGTH, line);
                    addChild(lenNode.get(), std::move(leftVal));
                    return lenNode;
                }

                auto expLeft = makeNode(NodeType::EXP_LEFT, line);
                addChild(expLeft.get(), std::move(leftVal));
                return expLeft;
            }

            auto leftVal = makeNode(NodeType::LEFTVALUE_ONE_ARR, line);
            leftVal->str_val = id.text;
            addChild(leftVal.get(), std::move(index1));

            // Check for .length
            if (check(TokenKind::TOK_DOT)) {
                advance();
                expect(TokenKind::KW_LENGTH);
                auto lenNode = makeNode(NodeType::EXP_LENGTH, line);
                addChild(lenNode.get(), std::move(leftVal));
                return lenNode;
            }

            auto expLeft = makeNode(NodeType::EXP_LEFT, line);
            addChild(expLeft.get(), std::move(leftVal));
            return expLeft;
        }

        // Simple variable access: ID or ID.length
        auto leftVal = makeNode(NodeType::LEFTVALUE_ID, line);
        leftVal->str_val = id.text;

        if (check(TokenKind::TOK_DOT)) {
            advance();
            expect(TokenKind::KW_LENGTH);
            auto lenNode = makeNode(NodeType::EXP_LENGTH, line);
            addChild(lenNode.get(), std::move(leftVal));
            return lenNode;
        }

        auto expLeft = makeNode(NodeType::EXP_LEFT, line);
        addChild(expLeft.get(), std::move(leftVal));
        return expLeft;
    }

    error("Unexpected token in expression: " + cur_.text);
    return nullptr;
}
