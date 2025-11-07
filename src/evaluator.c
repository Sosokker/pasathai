#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "evaluator.h"

static Object *TRUE_OBJ;
static Object *FALSE_OBJ;
static Object *NULL_OBJ;

static Environment *GLOBAL_ENV;

static Object *eval_block_statement_with_env(BlockStatement *block, Environment *env);

/* Built-in functions */
static Object *builtin_print(Object **args, int arg_count)
{
    for (int i = 0; i < arg_count; i++)
    {
        if (args[i]->type == OBJECT_INTEGER)
        {
            printf("%lld", args[i]->value.integer);
        }
        else if (args[i]->type == OBJECT_BOOLEAN)
        {
            printf("%s", args[i]->value.boolean ? "จริง" : "เท็จ");
        }
        else if (args[i]->type == OBJECT_STRING)
        {
            printf("%s", args[i]->value.string);
        }
        else if (args[i]->type == OBJECT_NULL)
        {
            printf("null");
        }
        if (i < arg_count - 1)
        {
            printf(" ");
        }
    }
    printf("\n");
    return NULL_OBJ;
}

void init_evaluator()
{
    TRUE_OBJ = malloc(sizeof(Object));
    TRUE_OBJ->type = OBJECT_BOOLEAN;
    TRUE_OBJ->value.boolean = 1;

    FALSE_OBJ = malloc(sizeof(Object));
    FALSE_OBJ->type = OBJECT_BOOLEAN;
    FALSE_OBJ->value.boolean = 0;

    NULL_OBJ = malloc(sizeof(Object));
    NULL_OBJ->type = OBJECT_NULL;

    GLOBAL_ENV = new_environment();

    /* Register built-in functions */
    Object *print_obj = malloc(sizeof(Object));
    print_obj->type = OBJECT_BUILTIN;
    print_obj->value.builtin = builtin_print;
    environment_set(GLOBAL_ENV, "แสดง", print_obj);
}

static Object *apply_function(Object *fn, Expression **args, int arg_count)
{
    if (fn->type == OBJECT_BUILTIN)
    {
        /* Evaluate arguments */
        Object **evaluated_args = malloc(sizeof(Object *) * arg_count);
        for (int i = 0; i < arg_count; i++)
        {
            evaluated_args[i] = eval((Node *)args[i]);
        }
        Object *result = fn->value.builtin(evaluated_args, arg_count);
        free(evaluated_args);
        return result;
    }

    if (fn->type != OBJECT_FUNCTION)
    {
        return NULL; // Error
    }

    Environment *extended_env = new_environment();
    extended_env->outer = fn->value.function.env;

    for (int i = 0; i < arg_count; i++)
    {
        Object *evaluated_arg = eval((Node *)args[i]);
        environment_set(extended_env, fn->value.function.parameters[i]->value, evaluated_arg);
    }

    Object *result = eval_block_statement_with_env(fn->value.function.body, extended_env);

    /* Unwrap return value */
    if (result != NULL && result->type == OBJECT_RETURN_VALUE)
    {
        return result->value.return_value;
    }

    return result;
}

static Object *eval_block_statement_with_env(BlockStatement *block, Environment *env)
{
    Object *result = NULL;

    Environment *old_env = GLOBAL_ENV;
    GLOBAL_ENV = env;

    for (int i = 0; i < block->statement_count; i++)
    {
        result = eval((Node *)block->statements[i]);

        /* If we hit a return statement, unwrap it and propagate */
        if (result != NULL && result->type == OBJECT_RETURN_VALUE)
        {
            GLOBAL_ENV = old_env;
            return result;
        }
    }

    GLOBAL_ENV = old_env;

    return result;
}

static Object *eval_bang_operator_expression(Object *right)
{
    if (right == TRUE_OBJ)
    {
        return FALSE_OBJ;
    }
    if (right == FALSE_OBJ)
    {
        return TRUE_OBJ;
    }
    if (right == NULL_OBJ)
    {
        return TRUE_OBJ;
    }
    return FALSE_OBJ;
}

static Object *eval_minus_prefix_operator_expression(Object *right)
{
    if (right->type != OBJECT_INTEGER)
    {
        return NULL; // Error
    }

    int64_t value = right->value.integer;
    Object *obj = malloc(sizeof(Object));
    obj->type = OBJECT_INTEGER;
    obj->value.integer = -value;
    return obj;
}

static Object *eval_prefix_expression(PrefixExpression *exp)
{
    Object *right = eval((Node *)exp->right);
    if (strcmp(exp->operator, "!") == 0)
    {
        return eval_bang_operator_expression(right);
    }
    if (strcmp(exp->operator, "-") == 0)
    {
        return eval_minus_prefix_operator_expression(right);
    }
    return NULL; // Error
}

static Object *eval_integer_infix_expression(const char *operator, Object *left, Object *right)
{
    int64_t left_val = left->value.integer;
    int64_t right_val = right->value.integer;

    if (strcmp(operator, "<") == 0)
    {
        return left_val < right_val ? TRUE_OBJ : FALSE_OBJ;
    }
    if (strcmp(operator, ">") == 0)
    {
        return left_val > right_val ? TRUE_OBJ : FALSE_OBJ;
    }
    if (strcmp(operator, "==") == 0)
    {
        return left_val == right_val ? TRUE_OBJ : FALSE_OBJ;
    }
    if (strcmp(operator, "!=") == 0)
    {
        return left_val != right_val ? TRUE_OBJ : FALSE_OBJ;
    }

    Object *obj = malloc(sizeof(Object));
    obj->type = OBJECT_INTEGER;

    if (strcmp(operator, "+") == 0)
    {
        obj->value.integer = left_val + right_val;
        return obj;
    }
    if (strcmp(operator, "-") == 0)
    {
        obj->value.integer = left_val - right_val;
        return obj;
    }
    if (strcmp(operator, "*") == 0)
    {
        obj->value.integer = left_val * right_val;
        return obj;
    }
    if (strcmp(operator, "/") == 0)
    {
        obj->value.integer = left_val / right_val;
        return obj;
    }

    return NULL; // Error
}

static Object *eval_infix_expression(InfixExpression *exp)
{
    Object *left = eval((Node *)exp->left);
    Object *right = eval((Node *)exp->right);

    if (left->type == OBJECT_INTEGER && right->type == OBJECT_INTEGER)
    {
        return eval_integer_infix_expression(exp->operator, left, right);
    }

    if (left->type == OBJECT_STRING && right->type == OBJECT_STRING)
    {
        if (strcmp(exp->operator, "+") == 0)
        {
            /* String concatenation */
            int len1 = strlen(left->value.string);
            int len2 = strlen(right->value.string);
            char *result = malloc(len1 + len2 + 1);
            strcpy(result, left->value.string);
            strcat(result, right->value.string);

            Object *obj = malloc(sizeof(Object));
            obj->type = OBJECT_STRING;
            obj->value.string = result;
            return obj;
        }

        if (strcmp(exp->operator, "==") == 0)
        {
            return strcmp(left->value.string, right->value.string) == 0 ? TRUE_OBJ : FALSE_OBJ;
        }

        if (strcmp(exp->operator, "!=") == 0)
        {
            return strcmp(left->value.string, right->value.string) != 0 ? TRUE_OBJ : FALSE_OBJ;
        }
    }

    if (left->type == OBJECT_BOOLEAN && right->type == OBJECT_BOOLEAN)
    {
        if (strcmp(exp->operator, "==") == 0)
        {
            return left == right ? TRUE_OBJ : FALSE_OBJ;
        }
        if (strcmp(exp->operator, "!=") == 0)
        {
            return left != right ? TRUE_OBJ : FALSE_OBJ;
        }
    }

    return NULL; // Error
}

static Object *eval_block_statement(BlockStatement *block)
{
    return eval_block_statement_with_env(block, GLOBAL_ENV);
}

static Object *eval_if_expression(IfExpression *exp)
{
    Object *condition = eval((Node *)exp->condition);

    if (condition == TRUE_OBJ)
    {
        return eval((Node *)exp->consequence);
    }
    else if (exp->alternative != NULL)
    {
        return eval((Node *)exp->alternative);
    }
    else
    {
        return NULL_OBJ;
    }
}

static Object *eval_boolean(Boolean *b)
{
    return b->value ? TRUE_OBJ : FALSE_OBJ;
}

static Object *eval_integer_literal(IntegerLiteral *literal)
{
    Object *obj = malloc(sizeof(Object));
    obj->type = OBJECT_INTEGER;
    obj->value.integer = literal->value;
    return obj;
}

static Object *eval_while_statement(WhileStatement *stmt)
{
    Object *result = NULL_OBJ;

    while (1)
    {
        Object *condition = eval((Node *)stmt->condition);
        if (condition != TRUE_OBJ)
        {
            break;
        }

        result = eval_block_statement(stmt->body);

        /* If result is a return value, break out of the loop */
        if (result != NULL && result->type == OBJECT_RETURN_VALUE)
        {
            break;
        }
    }

    return result;
}

static Object *eval_string_literal(StringLiteral *literal)
{
    Object *obj = malloc(sizeof(Object));
    obj->type = OBJECT_STRING;
    obj->value.string = literal->value;
    return obj;
}

Object *eval(Node *node)
{
    switch (node->type)
    {
    case NODE_INTEGER_LITERAL:
        return eval_integer_literal((IntegerLiteral *)node);
    case NODE_STRING_LITERAL:
        return eval_string_literal((StringLiteral *)node);
    case NODE_BOOLEAN:
        return eval_boolean((Boolean *)node);
    case NODE_PREFIX_EXPRESSION:
        return eval_prefix_expression((PrefixExpression *)node);
    case NODE_INFIX_EXPRESSION:
        return eval_infix_expression((InfixExpression *)node);
    case NODE_BLOCK_STATEMENT:
        return eval_block_statement((BlockStatement *)node);
    case NODE_IF_EXPRESSION:
        return eval_if_expression((IfExpression *)node);
    case NODE_FUNCTION_LITERAL:
    {
        Object *fn = malloc(sizeof(Object));
        fn->type = OBJECT_FUNCTION;
        fn->value.function.parameters = ((FunctionLiteral *)node)->parameters;
        fn->value.function.parameter_count = ((FunctionLiteral *)node)->parameter_count;
        fn->value.function.body = ((FunctionLiteral *)node)->body;
        fn->value.function.env = GLOBAL_ENV;
        return fn;
    }
    case NODE_CALL_EXPRESSION:
    {
        Object *fn = eval((Node *)((CallExpression *)node)->function);
        return apply_function(fn, ((CallExpression *)node)->arguments, ((CallExpression *)node)->argument_count);
    }
    case NODE_LET_STATEMENT:
    {
        Object *val = eval((Node *)((LetStatement *)node)->value);
        environment_set(GLOBAL_ENV, ((LetStatement *)node)->name->value, val);
        return val;
    }
    case NODE_RETURN_STATEMENT:
    {
        Object *val = eval((Node *)((ReturnStatement *)node)->return_value);
        Object *return_obj = malloc(sizeof(Object));
        return_obj->type = OBJECT_RETURN_VALUE;
        return_obj->value.return_value = val;
        return return_obj;
    }
    case NODE_WHILE_STATEMENT:
        return eval_while_statement((WhileStatement *)node);
    case NODE_EXPRESSION_STATEMENT:
        return eval((Node *)((ExpressionStatement *)node)->expression);
    case NODE_IDENTIFIER:
        return environment_get(GLOBAL_ENV, ((Identifier *)node)->value);
    default:
        return NULL;
    }
}
