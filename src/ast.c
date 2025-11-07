#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

char* program_token_literal(Node* node) {
    Program* p = (Program*)node;
    if (p->statement_count > 0) {
        return p->statements[0]->node.token_literal((Node*)p->statements[0]);
    } else {
        return "";
    }
}
