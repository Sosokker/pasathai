#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "error.h"

typedef struct Parser Parser;

typedef Expression *(*prefix_parse_fn)(Parser *);
typedef Expression *(*infix_parse_fn)(Parser *, Expression *);

struct Parser
{
    Lexer *l;
    Token cur_token;
    Token peek_token;

    prefix_parse_fn prefix_parse_fns[TOKEN_BEFORE_TO + 1];
    infix_parse_fn infix_parse_fns[TOKEN_BEFORE_TO + 1];

    /* Error reporting */
    Error *errors;        /* Linked list of errors */
    const char *source;   /* Source code for error context */
    const char *filename; /* Source filename */
};

Parser *new_parser(Lexer *l);
void parser_next_token(Parser *p);
Program *parse_program(Parser *p);

/* Set source for error reporting */
void parser_set_source(Parser *p, const char *source, const char *filename);

/* Get and print errors */
Error *parser_get_errors(Parser *p);
int parser_has_errors(Parser *p);
void parser_print_errors(Parser *p);

#endif // PARSER_H
