#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

typedef enum
{
    TOKEN_ILLEGAL,
    TOKEN_EOF,

    // Identifiers + literals
    TOKEN_IDENT,  // add, foobar, x, y, ...
    TOKEN_INT,    // 1343456
    TOKEN_STRING, // "hello"

    // Operators
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_BANG,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_MODULO,

    TOKEN_LT,
    TOKEN_GT,

    TOKEN_EQ,
    TOKEN_NOT_EQ,

    // Delimiters
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    // Keywords
    TOKEN_FUNCTION,
    TOKEN_LET,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_WHILE,
    TOKEN_NULL,
    TOKEN_FOR,
    TOKEN_FROM,
    TOKEN_TO,
    TOKEN_BEFORE_TO,
} TokenType;

typedef struct
{
    TokenType type;
    char *literal;
    int line;
    int column;
} Token;

typedef struct
{
    const char *input;
    int position;      // current position in input (points to current char)
    int read_position; // current reading position in input (after current char)
    uint32_t ch;       // current char under examination
    int line;          // current line number
    int column;        // current column number
} Lexer;

Lexer *new_lexer(const char *input);
void next_token(Lexer *l, Token *tok);

#endif // LEXER_H
