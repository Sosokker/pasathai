#ifndef GC_H
#define GC_H

#include "object.h"

/* GC Configuration */
#define GC_THRESHOLD 1000 /* Trigger GC after this many allocations */

/* GC Statistics (for debugging/monitoring) */
typedef struct
{
    int objects_allocated;
    int objects_freed;
    int collections_run;
} GC_Stats;

/* Initialize garbage collector */
void gc_init(void);

/* Allocate a new object tracked by GC */
Object *gc_alloc_object(void);

/* Run garbage collection cycle */
void gc_collect(void);

/* Mark an object as reachable */
void gc_mark_object(Object *obj);

/* Mark all objects in an environment as reachable */
void gc_mark_env(Environment *env);

/* Get GC statistics */
GC_Stats gc_get_stats(void);

/* Set GC roots (called from evaluator) */
void gc_set_global_env(Environment *env);

/* Push/pop current evaluation environment (for temporary scopes) */
void gc_push_env(Environment *env);
void gc_pop_env(void);

/* Register singleton objects that should never be collected */
void gc_register_singleton(Object *obj);

#endif /* GC_H */
