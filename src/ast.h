#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "error.h"

typedef enum
{
    NODE_PROGRAM,
    NODE_LET_STATEMENT,
    NODE_IDENTIFIER,
    NODE_INTEGER_LITERAL,
    NODE_STRING_LITERAL,
    NODE_PREFIX_EXPRESSION,
    NODE_INFIX_EXPRESSION,
    NODE_BOOLEAN,
    NODE_NULL,
    NODE_IF_EXPRESSION,
    NODE_BLOCK_STATEMENT,
    NODE_FUNCTION_LITERAL,
    NODE_CALL_EXPRESSION,
    NODE_RETURN_STATEMENT,
    NODE_EXPRESSION_STATEMENT,
    NODE_WHILE_STATEMENT,
    NODE_FOR_STATEMENT,
    NODE_ARRAY_LITERAL,
    NODE_INDEX_EXPRESSION,
} NodeType;

// The base Node interface
typedef struct Node
{
    NodeType type;
    char *(*token_literal)(struct Node *);
} Node;

// All statement nodes implement this
typedef struct Statement
{
    Node node;
    void (*statement_node)();
} Statement;

// All expression nodes implement this
typedef struct Expression
{
    Node node;
    void (*expression_node)();
} Expression;

typedef struct Program
{
    Node node;
    Statement **statements;
    int statement_count;
} Program;

char *program_token_literal(Node *node);

typedef struct Identifier
{
    Expression expression;
    Token token; // TOKEN_IDENT
    char *value;
} Identifier;

typedef struct LetStatement
{
    Statement statement;
    Token token; // the TOKEN_LET token
    Identifier *name;
    Expression *value;
} LetStatement;

typedef struct IntegerLiteral
{
    Expression expression;
    Token token;
    int64_t value;
} IntegerLiteral;

typedef struct StringLiteral
{
    Expression expression;
    Token token;
    char *value;
} StringLiteral;

typedef struct NullLiteral
{
    Expression expression;
    Token token;
} NullLiteral;

typedef struct PrefixExpression
{
    Expression expression;
    Token token; /* The prefix token, e.g. ! */
    char *operator;
    Expression *right;
} PrefixExpression;

typedef struct InfixExpression
{
    Expression expression;
    Token token; // The operator token, e.g. +
    Expression *left;
    char *operator;
    Expression *right;
} InfixExpression;

typedef struct
{
    Expression expression;
    Token token;
    int value;
} Boolean;

typedef struct BlockStatement
{
    Statement statement;
    Token token; // the { token
    Statement **statements;
    int statement_count;
} BlockStatement;

typedef struct IfExpression
{
    Expression expression;
    Token token; // The 'if' token
    Expression *condition;
    BlockStatement *consequence;
    BlockStatement *alternative;
} IfExpression;

typedef struct FunctionLiteral
{
    Expression expression;
    Token token; // The 'function' token
    Identifier **parameters;
    int parameter_count;
    BlockStatement *body;
} FunctionLiteral;

typedef struct CallExpression
{
    Expression expression;
    Token token;          // The '(' token
    Expression *function; // Identifier or FunctionLiteral
    Expression **arguments;
    int argument_count;
} CallExpression;

typedef struct ReturnStatement
{
    Statement statement;
    Token token; // the 'return' token
    Expression *return_value;
} ReturnStatement;

typedef struct ExpressionStatement
{
    Statement statement;
    Token token; // first token of the expression
    Expression *expression;
} ExpressionStatement;

typedef struct WhileStatement
{
    Statement statement;
    Token token; // the 'while' token
    Expression *condition;
    BlockStatement *body;
} WhileStatement;

typedef struct ForStatement
{
    Statement statement;
    Token token; // the 'for' token
    Identifier *variable;
    Expression *start;
    Expression *end;
    int inclusive; // 1 for ถึง (<=), 0 for ก่อนถึง (<)
    BlockStatement *body;
} ForStatement;

typedef struct ArrayLiteral
{
    Expression expression;
    Token token; // The '[' token
    Expression **elements;
    int element_count;
} ArrayLiteral;

typedef struct IndexExpression
{
    Expression expression;
    Token token; // The '[' token
    Expression *left;
    Expression *index;
} IndexExpression;

/* Helper: extract source location from any node */
SourceLocation ast_node_location(Node *node, const char *filename);

/* Helper: get token from any node */
Token *ast_node_token(Node *node);

#endif // AST_H
