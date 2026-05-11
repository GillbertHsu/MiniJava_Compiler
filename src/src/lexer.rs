#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TokenKind {
    // Keywords
    KwBoolean,
    KwClass,
    KwFalse,
    KwTrue,
    KwInt,
    KwMain,
    KwPublic,
    KwStatic,
    KwString,
    KwVoid,
    KwReturn,
    KwIf,
    KwElse,
    KwWhile,
    KwNew,
    KwLength,
    KwPrivate,
    // Compound keywords
    SystemOutPrint,
    SystemOutPrintln,
    IntegerParseInt,
    // Operators
    TokAnd,
    TokOr,
    TokEq,
    TokNeq,
    TokLte,
    TokGte,
    TokLt,
    TokGt,
    TokPlus,
    TokMinus,
    TokStar,
    TokSlash,
    TokNot,
    // Punctuation
    TokDot,
    TokSemi,
    TokLparen,
    TokRparen,
    TokLbrace,
    TokRbrace,
    TokComma,
    TokAssign,
    TokLbracket,
    TokRbracket,
    // Literals
    IntegerLiteral,
    StringLiteral,
    Identifier,
    // Special
    TokEof,
    TokError,
}

#[derive(Debug, Clone)]
pub struct Token {
    pub kind: TokenKind,
    pub text: String,
    pub line: i32,
    pub int_value: i32,
}

pub struct Lexer {
    src: Vec<u8>,
    pos: usize,
    line: i32,
    buffer: std::collections::VecDeque<Token>,
}

impl Lexer {
    pub fn new(source: &str) -> Self {
        Lexer {
            src: source.as_bytes().to_vec(),
            pos: 0,
            line: 1,
            buffer: std::collections::VecDeque::new(),
        }
    }

    fn current(&self) -> u8 {
        if self.pos >= self.src.len() {
            0
        } else {
            self.src[self.pos]
        }
    }

    fn advance(&mut self) -> u8 {
        let c = self.current();
        if c == b'\n' {
            self.line += 1;
        }
        self.pos += 1;
        c
    }

    fn match_char(&mut self, c: u8) -> bool {
        if self.current() == c {
            self.advance();
            true
        } else {
            false
        }
    }

    fn skip_whitespace_and_comments(&mut self) {
        while self.pos < self.src.len() {
            let c = self.current();
            if c == b' ' || c == b'\t' || c == b'\r' || c == b'\n' {
                self.advance();
            } else if c == b'/'
                && self.pos + 1 < self.src.len()
                && self.src[self.pos + 1] == b'/'
            {
                while self.pos < self.src.len() && self.current() != b'\n' {
                    self.advance();
                }
            } else if c == b'/'
                && self.pos + 1 < self.src.len()
                && self.src[self.pos + 1] == b'*'
            {
                self.advance();
                self.advance();
                while self.pos < self.src.len() {
                    if self.current() == b'*'
                        && self.pos + 1 < self.src.len()
                        && self.src[self.pos + 1] == b'/'
                    {
                        self.advance();
                        self.advance();
                        break;
                    }
                    self.advance();
                }
            } else {
                break;
            }
        }
    }

    fn make_token(&self, kind: TokenKind, text: &str) -> Token {
        Token {
            kind,
            text: text.to_string(),
            line: self.line,
            int_value: 0,
        }
    }

    fn try_match_sequence(&mut self, seq: &[u8]) -> bool {
        if self.pos + seq.len() > self.src.len() {
            return false;
        }
        for i in 0..seq.len() {
            if self.src[self.pos + i] != seq[i] {
                return false;
            }
        }
        let end = self.pos + seq.len();
        if end < self.src.len()
            && (self.src[end].is_ascii_alphanumeric() || self.src[end] == b'_')
        {
            return false;
        }
        for _ in 0..seq.len() {
            if self.src[self.pos] == b'\n' {
                self.line += 1;
            }
            self.pos += 1;
        }
        true
    }

    fn scan_identifier_or_keyword(&mut self) -> Token {
        let start = self.pos;
        while self.pos < self.src.len()
            && (self.src[self.pos].is_ascii_alphanumeric() || self.src[self.pos] == b'_')
        {
            self.advance();
        }
        let word = std::str::from_utf8(&self.src[start..self.pos])
            .unwrap()
            .to_string();

        if word == "System" {
            let saved_pos = self.pos;
            let saved_line = self.line;
            if self.try_match_sequence(b".out.println") {
                return self.make_token(TokenKind::SystemOutPrintln, "System.out.println");
            }
            self.pos = saved_pos;
            self.line = saved_line;
            if self.try_match_sequence(b".out.print") {
                return self.make_token(TokenKind::SystemOutPrint, "System.out.print");
            }
            self.pos = saved_pos;
            self.line = saved_line;
        } else if word == "Integer" {
            let saved_pos = self.pos;
            let saved_line = self.line;
            if self.try_match_sequence(b".parseInt") {
                return self.make_token(TokenKind::IntegerParseInt, "Integer.parseInt");
            }
            self.pos = saved_pos;
            self.line = saved_line;
        }

        let kind = match word.as_str() {
            "boolean" => TokenKind::KwBoolean,
            "class" => TokenKind::KwClass,
            "false" => TokenKind::KwFalse,
            "true" => TokenKind::KwTrue,
            "int" => TokenKind::KwInt,
            "main" => TokenKind::KwMain,
            "public" => TokenKind::KwPublic,
            "static" => TokenKind::KwStatic,
            "String" => TokenKind::KwString,
            "void" => TokenKind::KwVoid,
            "return" => TokenKind::KwReturn,
            "if" => TokenKind::KwIf,
            "else" => TokenKind::KwElse,
            "while" => TokenKind::KwWhile,
            "new" => TokenKind::KwNew,
            "length" => TokenKind::KwLength,
            "private" => TokenKind::KwPrivate,
            _ => TokenKind::Identifier,
        };
        self.make_token(kind, &word)
    }

    fn scan_number(&mut self) -> Token {
        let start = self.pos;
        while self.pos < self.src.len() && self.src[self.pos].is_ascii_digit() {
            self.advance();
        }
        let num_str = std::str::from_utf8(&self.src[start..self.pos])
            .unwrap()
            .to_string();
        let val: i32 = num_str.parse().unwrap_or(0);
        let mut t = self.make_token(TokenKind::IntegerLiteral, &num_str);
        t.int_value = val;
        t
    }

    fn scan_string(&mut self) -> Token {
        self.advance(); // skip opening "
        let mut val = String::from("\"");
        while self.pos < self.src.len() && self.current() != b'"' {
            if self.current() == b'\\' {
                val.push(self.advance() as char);
                if self.pos < self.src.len() {
                    val.push(self.advance() as char);
                }
            } else {
                val.push(self.advance() as char);
            }
        }
        if self.pos < self.src.len() {
            self.advance(); // skip closing "
        }
        val.push('"');
        self.make_token(TokenKind::StringLiteral, &val)
    }

    fn scan_raw_token(&mut self) -> Token {
        self.skip_whitespace_and_comments();
        if self.pos >= self.src.len() {
            return self.make_token(TokenKind::TokEof, "");
        }

        let c = self.current();

        if c.is_ascii_alphabetic() || c == b'_' {
            return self.scan_identifier_or_keyword();
        }
        if c.is_ascii_digit() {
            return self.scan_number();
        }
        if c == b'"' {
            return self.scan_string();
        }

        self.advance();
        match c {
            b'&' => {
                if self.match_char(b'&') {
                    self.make_token(TokenKind::TokAnd, "&&")
                } else {
                    self.make_token(TokenKind::TokError, "&")
                }
            }
            b'|' => {
                if self.match_char(b'|') {
                    self.make_token(TokenKind::TokOr, "||")
                } else {
                    self.make_token(TokenKind::TokError, "|")
                }
            }
            b'=' => {
                if self.match_char(b'=') {
                    self.make_token(TokenKind::TokEq, "==")
                } else {
                    self.make_token(TokenKind::TokAssign, "=")
                }
            }
            b'!' => {
                if self.match_char(b'=') {
                    self.make_token(TokenKind::TokNeq, "!=")
                } else {
                    self.make_token(TokenKind::TokNot, "!")
                }
            }
            b'<' => {
                if self.match_char(b'=') {
                    self.make_token(TokenKind::TokLte, "<=")
                } else {
                    self.make_token(TokenKind::TokLt, "<")
                }
            }
            b'>' => {
                if self.match_char(b'=') {
                    self.make_token(TokenKind::TokGte, ">=")
                } else {
                    self.make_token(TokenKind::TokGt, ">")
                }
            }
            b'+' => self.make_token(TokenKind::TokPlus, "+"),
            b'-' => self.make_token(TokenKind::TokMinus, "-"),
            b'*' => self.make_token(TokenKind::TokStar, "*"),
            b'/' => self.make_token(TokenKind::TokSlash, "/"),
            b'.' => self.make_token(TokenKind::TokDot, "."),
            b';' => self.make_token(TokenKind::TokSemi, ";"),
            b'(' => self.make_token(TokenKind::TokLparen, "("),
            b')' => self.make_token(TokenKind::TokRparen, ")"),
            b'{' => self.make_token(TokenKind::TokLbrace, "{"),
            b'}' => self.make_token(TokenKind::TokRbrace, "}"),
            b',' => self.make_token(TokenKind::TokComma, ","),
            b'[' => self.make_token(TokenKind::TokLbracket, "["),
            b']' => self.make_token(TokenKind::TokRbracket, "]"),
            _ => self.make_token(TokenKind::TokError, &(c as char).to_string()),
        }
    }

    fn fill_buffer(&mut self, n: usize) {
        while self.buffer.len() < n {
            let tok = self.scan_raw_token();
            self.buffer.push_back(tok);
        }
    }

    pub fn next_token(&mut self) -> Token {
        if let Some(t) = self.buffer.pop_front() {
            t
        } else {
            self.scan_raw_token()
        }
    }

    pub fn peek(&mut self) -> Token {
        self.fill_buffer(1);
        self.buffer[0].clone()
    }

    pub fn peek_next(&mut self) -> Token {
        self.fill_buffer(2);
        self.buffer[1].clone()
    }
}
