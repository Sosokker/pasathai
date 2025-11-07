#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

/* Forward declarations from ast.h */
typedef struct Identifier Identifier;
typedef struct BlockStatement BlockStatement;

/* Forward declaration for Object */
typedef struct Object Object;

typedef enum
{
    OBJECT_INTEGER,
    OBJECT_BOOLEAN,
    OBJECT_NULL,
    OBJECT_RETURN_VALUE,
    OBJECT_FUNCTION,
    OBJECT_BUILTIN,
    OBJECT_STRING,
    OBJECT_ARRAY,
    OBJECT_ERROR,
} ObjectType;

typedef struct Environment_Binding
{
    char *name;
    Object *value;
    struct Environment_Binding *next;
} Environment_Binding;

typedef struct Environment
{
    Environment_Binding *bindings;
    struct Environment *outer;
} Environment;

Environment *new_environment();
Object *environment_get(Environment *env, char *name);
void environment_set(Environment *env, char *name, Object *value);

typedef Object *(*BuiltinFunction)(Object **args, int arg_count);

struct Object
{
    ObjectType type;

    /* Garbage collection fields */
    int marked;      /* Mark bit for GC mark phase */
    Object *gc_next; /* Next object in GC linked list */

    union
    {
        int64_t integer;
        int boolean;
        struct
        {
            char *data;
            int owned; /* 1 if string is malloc'd and should be freed, 0 if borrowed from AST */
        } string;
        char *error;
        Object *return_value;
        struct
        {
            Identifier **parameters;
            int parameter_count;
            BlockStatement *body;
            Environment *env;
        } function;
        struct
        {
            Object **elements;
            int length;
            int capacity;
        } array;
        BuiltinFunction builtin;
    } value;
};

#endif /* OBJECT_H */
