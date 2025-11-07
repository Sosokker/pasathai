#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "evaluator.h"
#include "gc.h"

static Object *TRUE_OBJ;
static Object *FALSE_OBJ;
static Object *NULL_OBJ;

static Environment *GLOBAL_ENV;

static Object *eval_block_statement_with_env(BlockStatement *block, Environment *env);

/* Runtime error helper */
static Object *runtime_error(const char *format, ...)
{
    Object *obj = gc_alloc_object();
    obj->type = OBJECT_ERROR;

    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    obj->value.error = strdup(buffer);

    fprintf(stderr, "\033[1;31merror[runtime]:\033[0m \033[1m%s\033[0m\n\n", buffer);

    return obj;
}

/* Get type name for error messages */
static const char *type_name(ObjectType type)
{
    switch (type)
    {
    case OBJECT_INTEGER:
        return "INTEGER";
    case OBJECT_BOOLEAN:
        return "BOOLEAN";
    case OBJECT_STRING:
        return "STRING";
    case OBJECT_NULL:
        return "NULL";
    case OBJECT_FUNCTION:
        return "FUNCTION";
    case OBJECT_BUILTIN:
        return "BUILTIN";
    case OBJECT_ERROR:
        return "ERROR";
    case OBJECT_RETURN_VALUE:
        return "RETURN_VALUE";
    default:
        return "UNKNOWN";
    }
}

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
            printf("%s", args[i]->value.string.data);
        }
        else if (args[i]->type == OBJECT_NULL)
        {
            printf("ว่างเปล่า");
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
    TRUE_OBJ = gc_alloc_object();
    TRUE_OBJ->type = OBJECT_BOOLEAN;
    TRUE_OBJ->value.boolean = 1;
    gc_register_singleton(TRUE_OBJ);

    FALSE_OBJ = gc_alloc_object();
    FALSE_OBJ->type = OBJECT_BOOLEAN;
    FALSE_OBJ->value.boolean = 0;
    gc_register_singleton(FALSE_OBJ);

    NULL_OBJ = gc_alloc_object();
    NULL_OBJ->type = OBJECT_NULL;
    gc_register_singleton(NULL_OBJ);

    GLOBAL_ENV = new_environment();
    gc_set_global_env(GLOBAL_ENV);

    /* Register built-in functions */
    Object *print_obj = gc_alloc_object();
    print_obj->type = OBJECT_BUILTIN;
    print_obj->value.builtin = builtin_print;
    environment_set(GLOBAL_ENV, "แสดง", print_obj);
}

static Object *apply_function(Object *fn, Expression **args, int arg_count)
{
    if (fn->type == OBJECT_ERROR)
    {
        return fn;
    }

    if (fn->type == OBJECT_BUILTIN)
    {
        /* Evaluate arguments */
        Object **evaluated_args = malloc(sizeof(Object *) * arg_count);
        for (int i = 0; i < arg_count; i++)
        {
            evaluated_args[i] = eval((Node *)args[i]);
            if (evaluated_args[i]->type == OBJECT_ERROR)
            {
                Object *err = evaluated_args[i];
                free(evaluated_args);
                return err;
            }
        }
        Object *result = fn->value.builtin(evaluated_args, arg_count);
        free(evaluated_args);
        return result;
    }

    if (fn->type != OBJECT_FUNCTION)
    {
        return runtime_error("not a function: %s", type_name(fn->type));
    }

    /* Check argument count */
    if (arg_count != fn->value.function.parameter_count)
    {
        return runtime_error("wrong number of arguments: expected %d, got %d",
                             fn->value.function.parameter_count, arg_count);
    }

    Environment *extended_env = new_environment();
    extended_env->outer = fn->value.function.env;
    gc_push_env(extended_env);

    for (int i = 0; i < arg_count; i++)
    {
        Object *evaluated_arg = eval((Node *)args[i]);
        if (evaluated_arg->type == OBJECT_ERROR)
        {
            gc_pop_env();
            return evaluated_arg;
        }
        environment_set(extended_env, fn->value.function.parameters[i]->value, evaluated_arg);
    }

    Object *result = eval_block_statement_with_env(fn->value.function.body, extended_env);

    gc_pop_env();

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
    gc_push_env(env);

    for (int i = 0; i < block->statement_count; i++)
    {
        result = eval((Node *)block->statements[i]);

        /* If we hit a return statement, unwrap it and propagate */
        if (result != NULL && result->type == OBJECT_RETURN_VALUE)
        {
            gc_pop_env();
            GLOBAL_ENV = old_env;
            return result;
        }
    }

    gc_pop_env();
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
        return runtime_error("type error: cannot negate %s", type_name(right->type));
    }

    int64_t value = right->value.integer;
    Object *obj = gc_alloc_object();
    obj->type = OBJECT_INTEGER;
    obj->value.integer = -value;
    return obj;
}

static Object *eval_prefix_expression(PrefixExpression *exp)
{
    Object *right = eval((Node *)exp->right);

    if (right->type == OBJECT_ERROR)
    {
        return right;
    }

    if (strcmp(exp->operator, "!") == 0)
    {
        return eval_bang_operator_expression(right);
    }
    if (strcmp(exp->operator, "-") == 0)
    {
        return eval_minus_prefix_operator_expression(right);
    }
    return runtime_error("unknown operator: %s%s", exp->operator, type_name(right->type));
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

    Object *obj = gc_alloc_object();
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
    if (strcmp(operator, "%") == 0)
    {
        if (right_val == 0)
        {
            return runtime_error("division by zero in modulo operation");
        }
        obj->value.integer = left_val % right_val;
        return obj;
    }

    return runtime_error("unknown operator: %s %s %s",
                         type_name(OBJECT_INTEGER), operator, type_name(OBJECT_INTEGER));
}

static Object *eval_infix_expression(InfixExpression *exp)
{
    Object *left = eval((Node *)exp->left);
    if (left->type == OBJECT_ERROR)
    {
        return left;
    }

    Object *right = eval((Node *)exp->right);
    if (right->type == OBJECT_ERROR)
    {
        return right;
    }

    if (left->type == OBJECT_INTEGER && right->type == OBJECT_INTEGER)
    {
        return eval_integer_infix_expression(exp->operator, left, right);
    }

    if (left->type == OBJECT_STRING && right->type == OBJECT_STRING)
    {
        if (strcmp(exp->operator, "+") == 0)
        {
            /* String concatenation */
            int len1 = strlen(left->value.string.data);
            int len2 = strlen(right->value.string.data);
            char *result = malloc(len1 + len2 + 1);
            strcpy(result, left->value.string.data);
            strcat(result, right->value.string.data);

            Object *obj = gc_alloc_object();
            obj->type = OBJECT_STRING;
            obj->value.string.data = result;
            obj->value.string.owned = 1;
            return obj;
        }

        if (strcmp(exp->operator, "==") == 0)
        {
            return strcmp(left->value.string.data, right->value.string.data) == 0 ? TRUE_OBJ : FALSE_OBJ;
        }

        if (strcmp(exp->operator, "!=") == 0)
        {
            return strcmp(left->value.string.data, right->value.string.data) != 0 ? TRUE_OBJ : FALSE_OBJ;
        }

        return runtime_error("unknown operator: %s %s %s",
                             type_name(left->type), exp->operator, type_name(right->type));
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

    /* Null comparison */
    if (left->type == OBJECT_NULL || right->type == OBJECT_NULL)
    {
        if (strcmp(exp->operator, "==") == 0)
        {
            return (left == NULL_OBJ && right == NULL_OBJ) ? TRUE_OBJ : FALSE_OBJ;
        }
        if (strcmp(exp->operator, "!=") == 0)
        {
            return (left == NULL_OBJ && right == NULL_OBJ) ? FALSE_OBJ : TRUE_OBJ;
        }
    }

    /* Type mismatch error */
    if (left->type != right->type)
    {
        return runtime_error("type mismatch: %s %s %s",
                             type_name(left->type), exp->operator, type_name(right->type));
    }

    return runtime_error("unknown operator: %s %s %s",
                         type_name(left->type), exp->operator, type_name(right->type));
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
    Object *obj = gc_alloc_object();
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
    Object *obj = gc_alloc_object();
    obj->type = OBJECT_STRING;
    obj->value.string.data = literal->value;
    obj->value.string.owned = 0; /* Borrowed from AST, don't free */
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
    case NODE_NULL:
        return NULL_OBJ;
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
        Object *fn = gc_alloc_object();
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
        Object *return_obj = gc_alloc_object();
        return_obj->type = OBJECT_RETURN_VALUE;
        return_obj->value.return_value = val;
        return return_obj;
    }
    case NODE_WHILE_STATEMENT:
        return eval_while_statement((WhileStatement *)node);
    case NODE_EXPRESSION_STATEMENT:
        return eval((Node *)((ExpressionStatement *)node)->expression);
    case NODE_IDENTIFIER:
    {
        Object *val = environment_get(GLOBAL_ENV, ((Identifier *)node)->value);
        if (val == NULL)
        {
            return runtime_error("undefined variable: '%s'", ((Identifier *)node)->value);
        }
        return val;
    }
    default:
        return NULL;
    }
}
