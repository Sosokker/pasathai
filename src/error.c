#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

Error *error_new(ErrorType type, const char *message, const char *filename,
                 int line, int column, const char *source_line)
{
    Error *err = malloc(sizeof(Error));
    if (err == NULL)
    {
        return NULL;
    }

    err->type = type;
    err->message = strdup(message);
    err->filename = filename ? strdup(filename) : NULL;
    err->line = line;
    err->column = column;
    err->source_line = source_line ? strdup(source_line) : NULL;
    err->next = NULL;

    return err;
}

void error_append(Error **list, Error *err)
{
    if (err == NULL)
    {
        return;
    }

    if (*list == NULL)
    {
        *list = err;
        return;
    }

    Error *current = *list;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = err;
}

void error_print(const Error *err)
{
    if (err == NULL)
    {
        return;
    }

    /* Error header */
    const char *error_type = (err->type == ERROR_PARSE) ? "parse" : "runtime";
    fprintf(stderr, "\033[1;31merror[%s]:\033[0m \033[1m%s\033[0m\n",
            error_type, err->message);

    /* Location */
    if (err->filename != NULL)
    {
        fprintf(stderr, "  \033[1;34m-->\033[0m %s:%d:%d\n",
                err->filename, err->line, err->column);
    }
    else
    {
        fprintf(stderr, "  \033[1;34m-->\033[0m line %d:%d\n",
                err->line, err->column);
    }

    /* Source context */
    if (err->source_line != NULL)
    {
        fprintf(stderr, "   \033[1;34m|\033[0m\n");
        fprintf(stderr, "\033[1;34m%3d |\033[0m %s\n", err->line, err->source_line);
        fprintf(stderr, "   \033[1;34m|\033[0m ");

        /* Print caret pointer */
        for (int i = 0; i < err->column - 1; i++)
        {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "\033[1;31m^\033[0m\n");
    }

    fprintf(stderr, "\n");
}

void error_print_all(const Error *list)
{
    const Error *current = list;
    int count = 0;

    while (current != NULL)
    {
        error_print(current);
        current = current->next;
        count++;
    }

    if (count > 1)
    {
        fprintf(stderr, "\033[1;31merror:\033[0m aborting due to %d previous errors\n\n", count);
    }
}

void error_free_all(Error *list)
{
    while (list != NULL)
    {
        Error *next = list->next;
        free(list->message);
        free(list->filename);
        free(list->source_line);
        free(list);
        list = next;
    }
}

char *error_get_source_line(const char *input, int line)
{
    if (input == NULL || line < 1)
    {
        return NULL;
    }

    int current_line = 1;
    const char *line_start = input;
    const char *ptr = input;

    /* Find the start of the target line */
    while (*ptr != '\0' && current_line < line)
    {
        if (*ptr == '\n')
        {
            current_line++;
            line_start = ptr + 1;
        }
        ptr++;
    }

    if (current_line != line)
    {
        return NULL; /* Line not found */
    }

    /* Find the end of the line */
    const char *line_end = line_start;
    while (*line_end != '\0' && *line_end != '\n')
    {
        line_end++;
    }

    /* Copy the line */
    int len = line_end - line_start;
    char *result = malloc(len + 1);
    if (result == NULL)
    {
        return NULL;
    }

    strncpy(result, line_start, len);
    result[len] = '\0';

    return result;
}
