#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>

enum class TokenKind {
    // Keywords
    KW_BOOLEAN, KW_CLASS, KW_FALSE, KW_TRUE, KW_INT, KW_MAIN,
    KW_PUBLIC, KW_STATIC, KW_STRING, KW_VOID, KW_RETURN,
    KW_IF, KW_ELSE, KW_WHILE, KW_NEW, KW_LENGTH, KW_PRIVATE,
    // Compound keywords
    SYSTEM_OUT_PRINT, SYSTEM_OUT_PRINTLN, INTEGER_PARSEINT,
    // Operators
    TOK_AND, TOK_OR, TOK_EQ, TOK_NEQ, TOK_LTE, TOK_GTE,
    TOK_LT, TOK_GT, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_NOT,
    // Punctuation
    TOK_DOT, TOK_SEMI, TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE, TOK_COMMA, TOK_ASSIGN,
    TOK_LBRACKET, TOK_RBRACKET,
    // Literals
    INTEGER_LITERAL, STRING_LITERAL, IDENTIFIER,
    // Special
    TOK_EOF, TOK_ERROR
};

struct Token {
    TokenKind kind;
    std::string text;
    int line;
    int int_value = 0;
};

class Lexer {
public:
    explicit Lexer(const std::string& source);
    Token nextToken();
    Token peek();
    Token peekNext();  // peek 2nd token ahead
    int currentLine() const { return line_; }

private:
    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;
    std::vector<Token> buffer_;

    static std::unordered_map<std::string, TokenKind> keywords_;

    char current() const;
    char advance();
    bool match(char c);
    void skipWhitespaceAndComments();
    Token makeToken(TokenKind kind, const std::string& text);
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    Token scanRawToken();
    bool tryMatchSequence(const std::string& seq);
    void fillBuffer(int n);
};

#endif
