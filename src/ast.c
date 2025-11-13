#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

char *program_token_literal(Node *node)
{
    Program *p = (Program *)node;
    if (p->statement_count > 0)
    {
        return p->statements[0]->node.token_literal((Node *)p->statements[0]);
    }
    else
    {
        return "";
    }
}

Token *ast_node_token(Node *node)
{
    if (node == NULL)
    {
        return NULL;
    }

    switch (node->type)
    {
    case NODE_LET_STATEMENT:
        return &((LetStatement *)node)->token;
    case NODE_IDENTIFIER:
        return &((Identifier *)node)->token;
    case NODE_INTEGER_LITERAL:
        return &((IntegerLiteral *)node)->token;
    case NODE_STRING_LITERAL:
        return &((StringLiteral *)node)->token;
    case NODE_PREFIX_EXPRESSION:
        return &((PrefixExpression *)node)->token;
    case NODE_INFIX_EXPRESSION:
        return &((InfixExpression *)node)->token;
    case NODE_BOOLEAN:
        return &((Boolean *)node)->token;
    case NODE_NULL:
        return &((NullLiteral *)node)->token;
    case NODE_IF_EXPRESSION:
        return &((IfExpression *)node)->token;
    case NODE_BLOCK_STATEMENT:
        return &((BlockStatement *)node)->token;
    case NODE_FUNCTION_LITERAL:
        return &((FunctionLiteral *)node)->token;
    case NODE_CALL_EXPRESSION:
        return &((CallExpression *)node)->token;
    case NODE_RETURN_STATEMENT:
        return &((ReturnStatement *)node)->token;
    case NODE_EXPRESSION_STATEMENT:
        return &((ExpressionStatement *)node)->token;
    case NODE_WHILE_STATEMENT:
        return &((WhileStatement *)node)->token;
    case NODE_FOR_STATEMENT:
        return &((ForStatement *)node)->token;
    case NODE_ARRAY_LITERAL:
        return &((ArrayLiteral *)node)->token;
    case NODE_INDEX_EXPRESSION:
        return &((IndexExpression *)node)->token;
    default:
        return NULL;
    }
}

SourceLocation ast_node_location(Node *node, const char *filename)
{
    SourceLocation loc = {NULL, 0, 0, 0, 0};

    Token *tok = ast_node_token(node);
    if (tok != NULL)
    {
        loc.filename = filename;
        loc.start_line = tok->line;
        loc.start_column = tok->column;
        loc.end_line = tok->line;
        loc.end_column = tok->column;
    }

    return loc;
}
