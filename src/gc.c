#include <stdlib.h>
#include <stdio.h>
#include "gc.h"
#include "object.h"

/* GC State */
static Object *gc_objects = NULL;         /* Linked list of all allocated objects */
static int gc_num_objects = 0;            /* Current number of tracked objects */
static int gc_alloc_count = 0;            /* Allocations since last GC */
static GC_Stats gc_stats = {0, 0, 0};     /* Statistics */
static Environment *gc_global_env = NULL; /* Root environment */

/* Stack of temporary environments (for function calls, block scopes) */
#define MAX_ENV_STACK 256
static Environment *gc_env_stack[MAX_ENV_STACK];
static int gc_env_stack_top = 0;

/* Singleton objects that should never be freed */
static Object *gc_singletons[3] = {NULL, NULL, NULL};

void gc_init(void)
{
    gc_objects = NULL;
    gc_num_objects = 0;
    gc_alloc_count = 0;
    gc_stats.objects_allocated = 0;
    gc_stats.objects_freed = 0;
    gc_stats.collections_run = 0;
    gc_global_env = NULL;
    gc_env_stack_top = 0;
}

void gc_set_global_env(Environment *env)
{
    gc_global_env = env;
}

void gc_push_env(Environment *env)
{
    if (gc_env_stack_top < MAX_ENV_STACK)
    {
        gc_env_stack[gc_env_stack_top++] = env;
    }
}

void gc_pop_env(void)
{
    if (gc_env_stack_top > 0)
    {
        gc_env_stack_top--;
    }
}

/* Register singleton objects that should never be collected */
void gc_register_singleton(Object *obj)
{
    if (obj == NULL)
    {
        return;
    }

    obj->marked = 1; /* Always marked */

    /* Store in singletons array */
    for (int i = 0; i < 3; i++)
    {
        if (gc_singletons[i] == NULL)
        {
            gc_singletons[i] = obj;
            break;
        }
    }
}

Object *gc_alloc_object(void)
{
    /* Trigger GC if threshold reached */
    if (gc_alloc_count >= GC_THRESHOLD)
    {
        gc_collect();
        gc_alloc_count = 0;
    }

    Object *obj = malloc(sizeof(Object));
    if (obj == NULL)
    {
        fprintf(stderr, "GC: Failed to allocate object\n");
        exit(1);
    }

    obj->marked = 0;
    obj->gc_next = gc_objects;
    gc_objects = obj;

    gc_num_objects++;
    gc_alloc_count++;
    gc_stats.objects_allocated++;

    return obj;
}

void gc_mark_object(Object *obj)
{
    if (obj == NULL || obj->marked)
    {
        return;
    }

    obj->marked = 1;

    /* Mark nested objects based on type */
    switch (obj->type)
    {
    case OBJECT_RETURN_VALUE:
        gc_mark_object(obj->value.return_value);
        break;

    case OBJECT_FUNCTION:
        /* Mark function's closure environment */
        if (obj->value.function.env != NULL)
        {
            gc_mark_env(obj->value.function.env);
        }
        break;

    case OBJECT_INTEGER:
    case OBJECT_BOOLEAN:
    case OBJECT_NULL:
    case OBJECT_STRING:
    case OBJECT_BUILTIN:
    case OBJECT_ERROR:
        /* These types don't contain object references */
        break;
    }
}

void gc_mark_env(Environment *env)
{
    if (env == NULL)
    {
        return;
    }

    /* Mark all values in this environment */
    Environment_Binding *binding = env->bindings;
    while (binding != NULL)
    {
        gc_mark_object(binding->value);
        binding = binding->next;
    }

    /* Recursively mark outer environment */
    if (env->outer != NULL)
    {
        gc_mark_env(env->outer);
    }
}

static void gc_mark_roots(void)
{
    /* Mark global environment */
    if (gc_global_env != NULL)
    {
        gc_mark_env(gc_global_env);
    }

    /* Mark all environments in the stack */
    for (int i = 0; i < gc_env_stack_top; i++)
    {
        if (gc_env_stack[i] != NULL)
        {
            gc_mark_env(gc_env_stack[i]);
        }
    }

    /* Mark singletons */
    for (int i = 0; i < 3; i++)
    {
        if (gc_singletons[i] != NULL)
        {
            gc_singletons[i]->marked = 1;
        }
    }
}

static void gc_sweep(void)
{
    Object **obj_ptr = &gc_objects;

    while (*obj_ptr != NULL)
    {
        Object *obj = *obj_ptr;

        if (!obj->marked)
        {
            /* Unlink from list */
            *obj_ptr = obj->gc_next;

            /* Free object-specific memory */
            if (obj->type == OBJECT_STRING && obj->value.string.owned && obj->value.string.data != NULL)
            {
                free(obj->value.string.data);
            }
            else if (obj->type == OBJECT_ERROR && obj->value.error != NULL)
            {
                free(obj->value.error);
            }

            /* Free the object itself */
            free(obj);
            gc_num_objects--;
            gc_stats.objects_freed++;
        }
        else
        {
            /* Reset mark for next collection */
            obj->marked = 0;
            obj_ptr = &obj->gc_next;
        }
    }
}

void gc_collect(void)
{
#ifdef GC_DEBUG
    int before = gc_num_objects;
#endif

    /* Mark phase */
    gc_mark_roots();

    /* Sweep phase */
    gc_sweep();

    gc_stats.collections_run++;

#ifdef GC_DEBUG
    printf("GC: Collected %d objects (%d remaining)\n",
           before - gc_num_objects, gc_num_objects);
#endif
}

GC_Stats gc_get_stats(void)
{
    return gc_stats;
}
