#include "lexer.h"
#include <cctype>
#include <cstdlib>

std::unordered_map<std::string, TokenKind> Lexer::keywords_ = {
    {"boolean", TokenKind::KW_BOOLEAN}, {"class", TokenKind::KW_CLASS},
    {"false", TokenKind::KW_FALSE},     {"true", TokenKind::KW_TRUE},
    {"int", TokenKind::KW_INT},         {"main", TokenKind::KW_MAIN},
    {"public", TokenKind::KW_PUBLIC},   {"static", TokenKind::KW_STATIC},
    {"String", TokenKind::KW_STRING},   {"void", TokenKind::KW_VOID},
    {"return", TokenKind::KW_RETURN},   {"if", TokenKind::KW_IF},
    {"else", TokenKind::KW_ELSE},       {"while", TokenKind::KW_WHILE},
    {"new", TokenKind::KW_NEW},         {"length", TokenKind::KW_LENGTH},
    {"private", TokenKind::KW_PRIVATE},
};

Lexer::Lexer(const std::string& source) : src_(source) {}

char Lexer::current() const {
    if (pos_ >= src_.size()) return '\0';
    return src_[pos_];
}

char Lexer::advance() {
    char c = current();
    if (c == '\n') line_++;
    pos_++;
    return c;
}

bool Lexer::match(char c) {
    if (current() == c) { advance(); return true; }
    return false;
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < src_.size()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
            // single-line comment
            while (pos_ < src_.size() && current() != '\n') advance();
        } else if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '*') {
            // block comment
            advance(); advance(); // skip /*
            while (pos_ < src_.size()) {
                if (current() == '*' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenKind kind, const std::string& text) {
    Token t;
    t.kind = kind;
    t.text = text;
    t.line = line_;
    return t;
}

bool Lexer::tryMatchSequence(const std::string& seq) {
    if (pos_ + seq.size() > src_.size()) return false;
    for (size_t i = 0; i < seq.size(); i++) {
        if (src_[pos_ + i] != seq[i]) return false;
    }
    // Make sure next char after sequence isn't alphanumeric (word boundary)
    size_t end = pos_ + seq.size();
    if (end < src_.size() && (std::isalnum(src_[end]) || src_[end] == '_'))
        return false;
    for (size_t i = 0; i < seq.size(); i++) {
        if (src_[pos_] == '\n') line_++;
        pos_++;
    }
    return true;
}

Token Lexer::scanIdentifierOrKeyword() {
    size_t start = pos_;
    while (pos_ < src_.size() && (std::isalnum(current()) || current() == '_'))
        advance();
    std::string word = src_.substr(start, pos_ - start);

    // Check for compound keywords: System.out.print, System.out.println, Integer.parseInt
    if (word == "System") {
        size_t saved_pos = pos_;
        int saved_line = line_;
        // Try to match .out.println first (longer match)
        if (tryMatchSequence(".out.println"))
            return makeToken(TokenKind::SYSTEM_OUT_PRINTLN, "System.out.println");
        pos_ = saved_pos; line_ = saved_line;
        if (tryMatchSequence(".out.print"))
            return makeToken(TokenKind::SYSTEM_OUT_PRINT, "System.out.print");
        pos_ = saved_pos; line_ = saved_line;
    } else if (word == "Integer") {
        size_t saved_pos = pos_;
        int saved_line = line_;
        if (tryMatchSequence(".parseInt"))
            return makeToken(TokenKind::INTEGER_PARSEINT, "Integer.parseInt");
        pos_ = saved_pos; line_ = saved_line;
    }

    auto it = keywords_.find(word);
    if (it != keywords_.end())
        return makeToken(it->second, word);
    return makeToken(TokenKind::IDENTIFIER, word);
}

Token Lexer::scanNumber() {
    size_t start = pos_;
    while (pos_ < src_.size() && std::isdigit(current()))
        advance();
    std::string num = src_.substr(start, pos_ - start);
    Token t = makeToken(TokenKind::INTEGER_LITERAL, num);
    t.int_value = std::atoi(num.c_str());
    return t;
}

Token Lexer::scanString() {
    advance(); // skip opening "
    std::string val = "\"";
    while (pos_ < src_.size() && current() != '"') {
        if (current() == '\\') {
            val += advance(); // backslash
            if (pos_ < src_.size()) val += advance(); // escaped char
        } else {
            val += advance();
        }
    }
    if (pos_ < src_.size()) advance(); // skip closing "
    val += '"';
    return makeToken(TokenKind::STRING_LITERAL, val);
}

Token Lexer::scanRawToken() {
    skipWhitespaceAndComments();
    if (pos_ >= src_.size())
        return makeToken(TokenKind::TOK_EOF, "");

    char c = current();

    if (std::isalpha(c) || c == '_') return scanIdentifierOrKeyword();
    if (std::isdigit(c)) return scanNumber();
    if (c == '"') return scanString();

    advance();
    switch (c) {
        case '&': if (match('&')) return makeToken(TokenKind::TOK_AND, "&&");
                  return makeToken(TokenKind::TOK_ERROR, "&");
        case '|': if (match('|')) return makeToken(TokenKind::TOK_OR, "||");
                  return makeToken(TokenKind::TOK_ERROR, "|");
        case '=': if (match('=')) return makeToken(TokenKind::TOK_EQ, "==");
                  return makeToken(TokenKind::TOK_ASSIGN, "=");
        case '!': if (match('=')) return makeToken(TokenKind::TOK_NEQ, "!=");
                  return makeToken(TokenKind::TOK_NOT, "!");
        case '<': if (match('=')) return makeToken(TokenKind::TOK_LTE, "<=");
                  return makeToken(TokenKind::TOK_LT, "<");
        case '>': if (match('=')) return makeToken(TokenKind::TOK_GTE, ">=");
                  return makeToken(TokenKind::TOK_GT, ">");
        case '+': return makeToken(TokenKind::TOK_PLUS, "+");
        case '-': return makeToken(TokenKind::TOK_MINUS, "-");
        case '*': return makeToken(TokenKind::TOK_STAR, "*");
        case '/': return makeToken(TokenKind::TOK_SLASH, "/");
        case '.': return makeToken(TokenKind::TOK_DOT, ".");
        case ';': return makeToken(TokenKind::TOK_SEMI, ";");
        case '(': return makeToken(TokenKind::TOK_LPAREN, "(");
        case ')': return makeToken(TokenKind::TOK_RPAREN, ")");
        case '{': return makeToken(TokenKind::TOK_LBRACE, "{");
        case '}': return makeToken(TokenKind::TOK_RBRACE, "}");
        case ',': return makeToken(TokenKind::TOK_COMMA, ",");
        case '[': return makeToken(TokenKind::TOK_LBRACKET, "[");
        case ']': return makeToken(TokenKind::TOK_RBRACKET, "]");
        default:  return makeToken(TokenKind::TOK_ERROR, std::string(1, c));
    }
}

void Lexer::fillBuffer(int n) {
    while ((int)buffer_.size() < n) {
        buffer_.push_back(scanRawToken());
    }
}

Token Lexer::nextToken() {
    if (!buffer_.empty()) {
        Token t = buffer_.front();
        buffer_.erase(buffer_.begin());
        return t;
    }
    return scanRawToken();
}

Token Lexer::peek() {
    fillBuffer(1);
    return buffer_[0];
}

Token Lexer::peekNext() {
    fillBuffer(2);
    return buffer_[1];
}
