#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>
#include <string>

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::unique_ptr<ASTNode> parse();

private:
    Lexer& lex_;
    Token cur_;

    Token advance();
    Token expect(TokenKind kind);
    bool check(TokenKind kind);
    bool match(TokenKind kind);
    void error(const std::string& msg);

    // Grammar rules
    std::unique_ptr<ASTNode> parseProgram();
    std::unique_ptr<ASTNode> parseMainClass();
    std::unique_ptr<ASTNode> parseMainMethod();
    std::unique_ptr<ASTNode> parseStaticVarDeclList();
    std::unique_ptr<ASTNode> parseStaticMethodDeclList();
    std::unique_ptr<ASTNode> parseStaticVarDecl();
    std::unique_ptr<ASTNode> parseStaticMethodDecl();
    std::unique_ptr<ASTNode> parseFormalList();
    std::unique_ptr<ASTNode> parseType();
    std::unique_ptr<ASTNode> parsePrimeType();
    std::unique_ptr<ASTNode> parseStatementList();
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseVarDeclRest(std::unique_ptr<ASTNode> typeNode, const std::string& id, int line);
    std::unique_ptr<ASTNode> parseExpList();

    // Expression parsing (precedence climbing)
    std::unique_ptr<ASTNode> parseExpression(int minPrec = 0);
    std::unique_ptr<ASTNode> parseUnary();
    std::unique_ptr<ASTNode> parsePrimary();

    // Helpers
    bool isTypeKeyword(TokenKind k);
    int getPrec(TokenKind k);
    NodeType getBinOpNodeType(TokenKind k);
    std::string getBinOpStr(TokenKind k);
};

#endif
