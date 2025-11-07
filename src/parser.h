#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct Parser Parser;

typedef Expression *(*prefix_parse_fn)(Parser *);
typedef Expression *(*infix_parse_fn)(Parser *, Expression *);

struct Parser
{
    Lexer *l;
    Token cur_token;
    Token peek_token;

    prefix_parse_fn prefix_parse_fns[TOKEN_WHILE + 1];
    infix_parse_fn infix_parse_fns[TOKEN_WHILE + 1];
};

Parser *new_parser(Lexer *l);
void parser_next_token(Parser *p);
Program *parse_program(Parser *p);

#endif // PARSER_H
