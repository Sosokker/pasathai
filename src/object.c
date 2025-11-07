#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "object.h"

/* External error function from evaluator */
extern void report_undefined_variable(const char *name, Environment *env);

Environment *new_environment()
{
    Environment *env = malloc(sizeof(Environment));
    env->bindings = NULL;
    env->outer = NULL;
    return env;
}

Object *environment_get(Environment *env, char *name)
{
    Environment_Binding *binding = env->bindings;
    while (binding != NULL)
    {
        if (strcmp(binding->name, name) == 0)
        {
            return binding->value;
        }
        binding = binding->next;
    }

    if (env->outer != NULL)
    {
        return environment_get(env->outer, name);
    }

    return NULL;
}

void environment_set(Environment *env, char *name, Object *value)
{
    Environment_Binding *new_binding = malloc(sizeof(Environment_Binding));
    new_binding->name = name;
    new_binding->value = value;
    new_binding->next = env->bindings;
    env->bindings = new_binding;
}
