#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

static Statement *parse_statement(Parser *p);
static LetStatement *parse_let_statement(Parser *p);
static ReturnStatement *parse_return_statement(Parser *p);
static WhileStatement *parse_while_statement(Parser *p);
static ForStatement *parse_for_statement(Parser *p);
static ExpressionStatement *parse_expression_statement(Parser *p);
static Expression *parse_expression(Parser *p, int precedence);
static Expression *parse_integer_literal(Parser *p);

static Expression *parse_string_literal(Parser *p);

static Expression *parse_prefix_expression(Parser *p);

static Expression *parse_infix_expression(Parser *p, Expression *left);

static Expression *parse_boolean(Parser *p);

static Expression *parse_null(Parser *p);

static Expression *parse_if_expression(Parser *p);

static Expression *parse_function_literal(Parser *p);
static Expression *parse_call_expression(Parser *p, Expression *function);

static Expression *parse_identifier(Parser *p);

static Expression *parse_array_literal(Parser *p);
static Expression *parse_index_expression(Parser *p, Expression *left);

static void register_prefix(Parser *p, TokenType token_type, prefix_parse_fn fn)
{
    p->prefix_parse_fns[token_type] = fn;
}

static void register_infix(Parser *p, TokenType token_type, infix_parse_fn fn)
{
    p->infix_parse_fns[token_type] = fn;
}

Parser *new_parser(Lexer *l)
{
    Parser *p = malloc(sizeof(Parser));
    p->l = l;
    p->errors = NULL;
    p->source = NULL;
    p->filename = NULL;

    memset(p->prefix_parse_fns, 0, sizeof(p->prefix_parse_fns));
    register_prefix(p, TOKEN_IDENT, parse_identifier);
    register_prefix(p, TOKEN_INT, parse_integer_literal);
    register_prefix(p, TOKEN_STRING, parse_string_literal);
    register_prefix(p, TOKEN_BANG, parse_prefix_expression);
    register_prefix(p, TOKEN_MINUS, parse_prefix_expression);
    register_prefix(p, TOKEN_TRUE, parse_boolean);
    register_prefix(p, TOKEN_FALSE, parse_boolean);
    register_prefix(p, TOKEN_NULL, parse_null);
    register_prefix(p, TOKEN_IF, parse_if_expression);
    register_prefix(p, TOKEN_FUNCTION, parse_function_literal);
    register_prefix(p, TOKEN_LBRACKET, parse_array_literal);

    memset(p->infix_parse_fns, 0, sizeof(p->infix_parse_fns));
    register_infix(p, TOKEN_PLUS, parse_infix_expression);
    register_infix(p, TOKEN_MINUS, parse_infix_expression);
    register_infix(p, TOKEN_SLASH, parse_infix_expression);
    register_infix(p, TOKEN_ASTERISK, parse_infix_expression);
    register_infix(p, TOKEN_MODULO, parse_infix_expression);
    register_infix(p, TOKEN_EQ, parse_infix_expression);
    register_infix(p, TOKEN_NOT_EQ, parse_infix_expression);
    register_infix(p, TOKEN_LT, parse_infix_expression);
    register_infix(p, TOKEN_GT, parse_infix_expression);
    register_infix(p, TOKEN_LPAREN, parse_call_expression);
    register_infix(p, TOKEN_LBRACKET, parse_index_expression);

    parser_next_token(p);
    parser_next_token(p);
    return p;
}

/* Error reporting helpers */
void parser_set_source(Parser *p, const char *source, const char *filename)
{
    p->source = source;
    p->filename = filename;
}

Error *parser_get_errors(Parser *p)
{
    return p->errors;
}

int parser_has_errors(Parser *p)
{
    return p->errors != NULL;
}

void parser_print_errors(Parser *p)
{
    if (p->errors != NULL)
    {
        error_print_all(p->errors);
    }
}

/* Helper to add parse error with context */
static void parser_error(Parser *p, const char *message)
{
    char *source_line = NULL;
    if (p->source != NULL)
    {
        source_line = error_get_source_line(p->source, p->cur_token.line);
    }

    Error *err = error_new(ERROR_PARSE, message, p->filename,
                           p->cur_token.line, p->cur_token.column,
                           source_line);

    error_append(&p->errors, err);

    if (source_line != NULL)
    {
        free(source_line);
    }
}

/* Helper for "expected X but got Y" errors */
static void parser_error_expected(Parser *p, const char *expected)
{
    char message[256];
    snprintf(message, sizeof(message), "expected %s, got '%s' instead",
             expected, p->cur_token.literal);
    parser_error(p, message);
}

void parser_next_token(Parser *p)
{
    p->cur_token = p->peek_token;
    next_token(p->l, &p->peek_token);
}

Program *parse_program(Parser *p)
{
    Program *program = malloc(sizeof(Program));
    program->node.type = NODE_PROGRAM;
    int capacity = 100; /* Increased from 10 to 100 to avoid buffer overflow */
    program->statements = malloc(sizeof(Statement *) * capacity);
    program->statement_count = 0;

    while (p->cur_token.type != TOKEN_EOF)
    {
        Statement *stmt = parse_statement(p);
        if (stmt != NULL)
        {
            program->statements[program->statement_count++] = stmt;
        }
        parser_next_token(p);
    }

    return program;
}

static ReturnStatement *parse_return_statement(Parser *p);

static Statement *parse_statement(Parser *p)
{
    switch (p->cur_token.type)
    {
    case TOKEN_LET:
        return (Statement *)parse_let_statement(p);
    case TOKEN_RETURN:
        return (Statement *)parse_return_statement(p);
    case TOKEN_WHILE:
        return (Statement *)parse_while_statement(p);
    case TOKEN_FOR:
        return (Statement *)parse_for_statement(p);
    default:
        return (Statement *)parse_expression_statement(p);
    }
}

static LetStatement *parse_let_statement(Parser *p)
{
    LetStatement *stmt = malloc(sizeof(LetStatement));
    stmt->statement.node.type = NODE_LET_STATEMENT;
    stmt->token = p->cur_token;

    if (p->peek_token.type != TOKEN_IDENT)
    {
        parser_error_expected(p, "identifier after 'ให้'");
        return NULL;
    }
    parser_next_token(p);

    Identifier *ident = malloc(sizeof(Identifier));
    ident->expression.node.type = NODE_IDENTIFIER;
    ident->token = p->cur_token;
    ident->value = p->cur_token.literal;
    stmt->name = ident;

    if (p->peek_token.type != TOKEN_ASSIGN)
    {
        parser_error_expected(p, "'=' after identifier");
        return NULL;
    }
    parser_next_token(p);

    parser_next_token(p);

    stmt->value = parse_expression(p, 0);

    if (p->peek_token.type == TOKEN_SEMICOLON)
    {
        parser_next_token(p);
    }

    return stmt;
}

typedef enum
{
    PREC_LOWEST,
    PREC_EQUALS,      // ==
    PREC_LESSGREATER, // > or <
    PREC_SUM,         // +
    PREC_PRODUCT,     // *
    PREC_PREFIX,      // -X or !X
    PREC_CALL         // myFunction(X)
} Precedence;

static int precedences[] = {
    [TOKEN_EQ] = PREC_EQUALS,
    [TOKEN_NOT_EQ] = PREC_EQUALS,
    [TOKEN_LT] = PREC_LESSGREATER,
    [TOKEN_GT] = PREC_LESSGREATER,
    [TOKEN_PLUS] = PREC_SUM,
    [TOKEN_MINUS] = PREC_SUM,
    [TOKEN_SLASH] = PREC_PRODUCT,
    [TOKEN_ASTERISK] = PREC_PRODUCT,
    [TOKEN_MODULO] = PREC_PRODUCT,
    [TOKEN_LPAREN] = PREC_CALL,
    [TOKEN_LBRACKET] = PREC_CALL,
};

static int peek_precedence(Parser *p)
{
    return precedences[p->peek_token.type];
}

static int cur_precedence(Parser *p)
{
    return precedences[p->cur_token.type];
}

static Expression *parse_expression(Parser *p, int precedence)
{
    prefix_parse_fn prefix = p->prefix_parse_fns[p->cur_token.type];
    if (prefix == NULL)
    {
        char message[256];
        snprintf(message, sizeof(message),
                 "no prefix parse function for '%s'", p->cur_token.literal);
        parser_error(p, message);
        return NULL;
    }
    Expression *left_exp = prefix(p);

    while (p->peek_token.type != TOKEN_SEMICOLON && precedence < peek_precedence(p))
    {
        infix_parse_fn infix = p->infix_parse_fns[p->peek_token.type];
        if (infix == NULL)
        {
            return left_exp;
        }

        parser_next_token(p);

        left_exp = infix(p, left_exp);
    }

    return left_exp;
}

static Expression *parse_integer_literal(Parser *p)
{
    IntegerLiteral *literal = malloc(sizeof(IntegerLiteral));
    literal->expression.node.type = NODE_INTEGER_LITERAL;
    literal->token = p->cur_token;
    literal->value = strtoll(p->cur_token.literal, NULL, 10);
    return (Expression *)literal;
}

static Expression *parse_string_literal(Parser *p)
{
    StringLiteral *literal = malloc(sizeof(StringLiteral));
    literal->expression.node.type = NODE_STRING_LITERAL;
    literal->token = p->cur_token;
    literal->value = p->cur_token.literal;
    return (Expression *)literal;
}

static Expression *parse_prefix_expression(Parser *p)
{
    PrefixExpression *exp = malloc(sizeof(PrefixExpression));
    exp->expression.node.type = NODE_PREFIX_EXPRESSION;
    exp->token = p->cur_token;
    exp->operator = p->cur_token.literal;

    parser_next_token(p);

    exp->right = parse_expression(p, 6); // Precedence for prefix operators

    return (Expression *)exp;
}

static Expression *parse_infix_expression(Parser *p, Expression *left)
{
    InfixExpression *exp = malloc(sizeof(InfixExpression));
    exp->expression.node.type = NODE_INFIX_EXPRESSION;
    exp->token = p->cur_token;
    exp->operator = p->cur_token.literal;
    exp->left = left;

    int precedence = cur_precedence(p);
    parser_next_token(p);
    exp->right = parse_expression(p, precedence);
    return (Expression *)exp;
}

static Expression *parse_boolean(Parser *p)
{
    Boolean *b = malloc(sizeof(Boolean));
    b->expression.node.type = NODE_BOOLEAN;
    b->token = p->cur_token;
    b->value = (p->cur_token.type == TOKEN_TRUE);
    return (Expression *)b;
}

static Expression *parse_null(Parser *p)
{
    NullLiteral *null = malloc(sizeof(NullLiteral));
    null->expression.node.type = NODE_NULL;
    null->token = p->cur_token;
    return (Expression *)null;
}

static BlockStatement *parse_block_statement(Parser *p)
{
    BlockStatement *block = malloc(sizeof(BlockStatement));
    block->statement.node.type = NODE_BLOCK_STATEMENT;
    block->token = p->cur_token;
    block->statements = malloc(sizeof(Statement *) * 10); // Start with capacity for 10 statements
    block->statement_count = 0;

    parser_next_token(p);

    while (p->cur_token.type != TOKEN_RBRACE && p->cur_token.type != TOKEN_EOF)
    {
        Statement *stmt = parse_statement(p);
        if (stmt != NULL)
        {
            block->statements[block->statement_count++] = stmt;
        }
        parser_next_token(p);
    }

    return block;
}

static Expression *parse_if_expression(Parser *p)
{
    IfExpression *exp = malloc(sizeof(IfExpression));
    exp->expression.node.type = NODE_IF_EXPRESSION;
    exp->token = p->cur_token;

    if (p->peek_token.type != TOKEN_LPAREN)
    {
        return NULL; // Error
    }
    parser_next_token(p);
    parser_next_token(p);

    exp->condition = parse_expression(p, PREC_LOWEST);

    if (p->peek_token.type != TOKEN_RPAREN)
    {
        return NULL; // Error
    }
    parser_next_token(p);

    if (p->peek_token.type != TOKEN_LBRACE)
    {
        return NULL; // Error
    }
    parser_next_token(p);

    exp->consequence = parse_block_statement(p);

    if (p->peek_token.type == TOKEN_ELSE)
    {
        parser_next_token(p);

        if (p->peek_token.type != TOKEN_LBRACE)
        {
            return NULL; // Error
        }
        parser_next_token(p);

        exp->alternative = parse_block_statement(p);
    }

    return (Expression *)exp;
}

static Identifier **parse_function_parameters(Parser *p, int *count)
{
    Identifier **params = malloc(sizeof(Identifier *) * 10);
    int i = 0;

    if (p->peek_token.type == TOKEN_RPAREN)
    {
        parser_next_token(p);
        *count = 0;
        return params;
    }

    parser_next_token(p);

    Identifier *ident = malloc(sizeof(Identifier));
    ident->expression.node.type = NODE_IDENTIFIER;
    ident->token = p->cur_token;
    ident->value = p->cur_token.literal;
    params[i++] = ident;

    while (p->peek_token.type == TOKEN_COMMA)
    {
        parser_next_token(p);
        parser_next_token(p);
        ident = malloc(sizeof(Identifier));
        ident->expression.node.type = NODE_IDENTIFIER;
        ident->token = p->cur_token;
        ident->value = p->cur_token.literal;
        params[i++] = ident;
    }

    if (p->peek_token.type != TOKEN_RPAREN)
    {
        return NULL; // Error
    }
    parser_next_token(p);

    *count = i;
    return params;
}

static Expression *parse_function_literal(Parser *p)
{
    FunctionLiteral *lit = malloc(sizeof(FunctionLiteral));
    lit->expression.node.type = NODE_FUNCTION_LITERAL;
    lit->token = p->cur_token;

    if (p->peek_token.type != TOKEN_LPAREN)
    {
        return NULL; // Error
    }
    parser_next_token(p);

    lit->parameters = parse_function_parameters(p, &lit->parameter_count);

    if (p->peek_token.type != TOKEN_LBRACE)
    {
        return NULL; // Error
    }
    parser_next_token(p);

    lit->body = parse_block_statement(p);

    return (Expression *)lit;
}

static Expression *parse_call_expression(Parser *p, Expression *function)
{
    CallExpression *exp = malloc(sizeof(CallExpression));
    exp->expression.node.type = NODE_CALL_EXPRESSION;
    exp->token = p->cur_token;
    exp->function = function;

    /* Parse arguments and count them */
    Expression **args = malloc(sizeof(Expression *) * 10);
    int arg_count = 0;

    if (p->peek_token.type == TOKEN_RPAREN)
    {
        parser_next_token(p);
    }
    else
    {
        parser_next_token(p);
        args[arg_count++] = parse_expression(p, PREC_LOWEST);

        while (p->peek_token.type == TOKEN_COMMA)
        {
            parser_next_token(p);
            parser_next_token(p);
            args[arg_count++] = parse_expression(p, PREC_LOWEST);
        }

        if (p->peek_token.type != TOKEN_RPAREN)
        {
            return NULL; // Error
        }
        parser_next_token(p);
    }

    exp->arguments = args;
    exp->argument_count = arg_count;
    return (Expression *)exp;
}

static Expression *parse_identifier(Parser *p)
{
    Identifier *ident = malloc(sizeof(Identifier));
    ident->expression.node.type = NODE_IDENTIFIER;
    ident->token = p->cur_token;
    ident->value = p->cur_token.literal;
    return (Expression *)ident;
}

static ReturnStatement *parse_return_statement(Parser *p)
{
    ReturnStatement *stmt = malloc(sizeof(ReturnStatement));
    stmt->statement.node.type = NODE_RETURN_STATEMENT;
    stmt->token = p->cur_token;

    parser_next_token(p);

    stmt->return_value = parse_expression(p, PREC_LOWEST);

    if (p->peek_token.type == TOKEN_SEMICOLON)
    {
        parser_next_token(p);
    }

    return stmt;
}

static WhileStatement *parse_while_statement(Parser *p)
{
    WhileStatement *stmt = malloc(sizeof(WhileStatement));
    stmt->statement.node.type = NODE_WHILE_STATEMENT;
    stmt->token = p->cur_token;

    if (p->peek_token.type != TOKEN_LPAREN)
    {
        return NULL; // Error
    }
    parser_next_token(p);
    parser_next_token(p);

    stmt->condition = parse_expression(p, PREC_LOWEST);

    if (p->peek_token.type != TOKEN_RPAREN)
    {
        return NULL; // Error
    }
    parser_next_token(p);

    if (p->peek_token.type != TOKEN_LBRACE)
    {
        return NULL; // Error
    }
    parser_next_token(p);

    stmt->body = parse_block_statement(p);

    return stmt;
}

static ForStatement *parse_for_statement(Parser *p)
{
    ForStatement *stmt = malloc(sizeof(ForStatement));
    stmt->statement.node.type = NODE_FOR_STATEMENT;
    stmt->token = p->cur_token; // TOKEN_FOR

    // Expect identifier (loop variable)
    if (p->peek_token.type != TOKEN_IDENT)
    {
        parser_error(p, "expected identifier after 'สำหรับ'");
        return NULL;
    }
    parser_next_token(p);

    Identifier *var = malloc(sizeof(Identifier));
    var->expression.node.type = NODE_IDENTIFIER;
    var->token = p->cur_token;
    var->value = p->cur_token.literal;
    stmt->variable = var;

    // Expect จาก (from)
    if (p->peek_token.type != TOKEN_FROM)
    {
        parser_next_token(p);
        parser_error_expected(p, "'จาก'");
        return NULL;
    }
    parser_next_token(p); // consume จาก
    parser_next_token(p); // move to start expression

    stmt->start = parse_expression(p, PREC_LOWEST);

    // Expect ถึง or ก่อนถึง
    if (p->peek_token.type == TOKEN_TO)
    {
        stmt->inclusive = 1;
        parser_next_token(p);
    }
    else if (p->peek_token.type == TOKEN_BEFORE_TO)
    {
        stmt->inclusive = 0;
        parser_next_token(p);
    }
    else
    {
        parser_next_token(p);
        parser_error_expected(p, "'ถึง' or 'ก่อนถึง'");
        return NULL;
    }

    parser_next_token(p); // move to end expression
    stmt->end = parse_expression(p, PREC_LOWEST);

    // Expect {
    if (p->peek_token.type != TOKEN_LBRACE)
    {
        parser_next_token(p);
        parser_error_expected(p, "'{'");
        return NULL;
    }
    parser_next_token(p);

    stmt->body = parse_block_statement(p);

    return stmt;
}

static ExpressionStatement *parse_expression_statement(Parser *p)
{
    ExpressionStatement *stmt = malloc(sizeof(ExpressionStatement));
    stmt->statement.node.type = NODE_EXPRESSION_STATEMENT;
    stmt->token = p->cur_token;

    stmt->expression = parse_expression(p, PREC_LOWEST);

    if (p->peek_token.type == TOKEN_SEMICOLON)
    {
        parser_next_token(p);
    }

    return stmt;
}

static Expression *parse_array_literal(Parser *p)
{
    ArrayLiteral *arr = malloc(sizeof(ArrayLiteral));
    arr->expression.node.type = NODE_ARRAY_LITERAL;
    arr->token = p->cur_token; /* The '[' token */

    /* Allocate initial array for elements */
    Expression **elements = malloc(sizeof(Expression *) * 10);
    int element_count = 0;

    /* Empty array case: [] */
    if (p->peek_token.type == TOKEN_RBRACKET)
    {
        parser_next_token(p);
        arr->elements = elements;
        arr->element_count = 0;
        return (Expression *)arr;
    }

    /* Parse first element */
    parser_next_token(p);
    elements[element_count++] = parse_expression(p, PREC_LOWEST);

    /* Parse remaining elements */
    while (p->peek_token.type == TOKEN_COMMA)
    {
        parser_next_token(p); /* Consume comma */
        parser_next_token(p); /* Move to next element */
        elements[element_count++] = parse_expression(p, PREC_LOWEST);
    }

    /* Expect closing bracket */
    if (p->peek_token.type != TOKEN_RBRACKET)
    {
        parser_next_token(p); /* Move to peek to set as cur_token */
        parser_error_expected(p, "']'");
        return NULL;
    }
    parser_next_token(p);

    arr->elements = elements;
    arr->element_count = element_count;
    return (Expression *)arr;
}

static Expression *parse_index_expression(Parser *p, Expression *left)
{
    IndexExpression *exp = malloc(sizeof(IndexExpression));
    exp->expression.node.type = NODE_INDEX_EXPRESSION;
    exp->token = p->cur_token; /* The '[' token */
    exp->left = left;

    /* Parse index expression */
    parser_next_token(p);
    exp->index = parse_expression(p, PREC_LOWEST);

    /* Expect closing bracket */
    if (p->peek_token.type != TOKEN_RBRACKET)
    {
        parser_next_token(p); /* Move to peek to set as cur_token */
        parser_error_expected(p, "']'");
        return NULL;
    }
    parser_next_token(p);

    return (Expression *)exp;
}
