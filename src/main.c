#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "gc.h"

void init_evaluator();

static void run_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error: Cannot open file '%s'\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *input = malloc(length + 1);
    fread(input, 1, length, file);
    input[length] = '\0';
    fclose(file);

    Lexer *l = new_lexer(input);
    Parser *p = new_parser(l);
    Program *program = parse_program(p);

    if (program == NULL)
    {
        printf("Error: Failed to parse program\n");
        free(input);
        exit(1);
    }

    for (int i = 0; i < program->statement_count; i++)
    {
        if (program->statements[i] != NULL)
        {
            eval((Node *)program->statements[i]);
        }
    }

    free(input);
}

static void run_repl(void)
{
    printf("Pasathai v0.1.0 - Thai Programming Language\n");
    printf("Type 'exit' or press Ctrl+C to quit\n\n");

    char line[1024];
    while (1)
    {
        printf(">> ");
        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            break;
        }

        /* Remove trailing newline */
        line[strcspn(line, "\n")] = 0;

        /* Check for exit command */
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0)
        {
            break;
        }

        /* Skip empty lines */
        if (strlen(line) == 0)
        {
            continue;
        }

        Lexer *l = new_lexer(line);
        Parser *p = new_parser(l);
        Program *program = parse_program(p);

        if (program == NULL)
        {
            printf("Error: Failed to parse input\n");
            continue;
        }

        for (int i = 0; i < program->statement_count; i++)
        {
            if (program->statements[i] != NULL)
            {
                Object *result = eval((Node *)program->statements[i]);

                /* Print non-null results in REPL mode */
                if (result != NULL && result->type != OBJECT_NULL)
                {
                    if (result->type == OBJECT_INTEGER)
                    {
                        printf("%lld\n", result->value.integer);
                    }
                    else if (result->type == OBJECT_BOOLEAN)
                    {
                        printf("%s\n", result->value.boolean ? "จริง" : "เท็จ");
                    }
                    else if (result->type == OBJECT_STRING)
                    {
                        printf("%s\n", result->value.string.data);
                    }
                    else if (result->type == OBJECT_ERROR)
                    {
                        printf("Error: %s\n", result->value.error);
                    }
                }
            }
        }
    }

    printf("\nGoodbye!\n");
}

static void print_usage(const char *program_name)
{
    printf("Usage: %s [options] [file]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n\n");
    printf("Examples:\n");
    printf("  %s                Run in interactive REPL mode\n", program_name);
    printf("  %s program.thai  Execute a Thai program file\n", program_name);
}

int main(int argc, char *argv[])
{
    gc_init();
    init_evaluator();

    /* No arguments - run REPL */
    if (argc == 1)
    {
        run_repl();
        return 0;
    }

    /* Check for help flag */
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        print_usage(argv[0]);
        return 0;
    }

    /* Check for version flag */
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0))
    {
        printf("Pasathai v0.1.0\n");
        printf("Thai Programming Language\n");
        return 0;
    }

    /* Run file */
    if (argc == 2)
    {
        run_file(argv[1]);
        return 0;
    }

    /* Invalid usage */
    printf("Error: Too many arguments\n\n");
    print_usage(argv[0]);
    return 1;
}
