#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast.h"
#include "object.h"

/* Evaluation context for tracking source information */
typedef struct EvalContext
{
    const char *source;   /* Full source code */
    const char *filename; /* Source filename */
} EvalContext;

/* Initialize evaluator with context */
void evaluator_init(const char *source, const char *filename);

/* Get current evaluation context */
EvalContext *evaluator_get_context(void);

/* Main evaluation function */
Object *eval(Node *node);

#endif // EVALUATOR_H
