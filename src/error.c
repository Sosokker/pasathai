#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

/* Error builder for constructing rich errors */
struct ErrorBuilder
{
    ErrorType type;
    ErrorSeverity severity;
    char *code;
    char *message;
    ErrorSpan *spans;
    ErrorSpan *last_span;
    ErrorNote *notes;
    ErrorNote *last_note;
    char *suggestion;
};

Error *error_new(ErrorType type, const char *message, const char *filename,
                 int line, int column, const char *source_line)
{
    Error *err = malloc(sizeof(Error));
    if (err == NULL)
    {
        return NULL;
    }

    err->type = type;
    err->severity = SEVERITY_ERROR;
    err->code = NULL;
    err->message = strdup(message);
    err->next = NULL;
    err->notes = NULL;
    err->suggestion = NULL;

    /* Create single span for backward compatibility */
    err->spans = malloc(sizeof(ErrorSpan));
    if (err->spans != NULL)
    {
        err->spans->location.filename = filename ? strdup(filename) : NULL;
        err->spans->location.start_line = line;
        err->spans->location.start_column = column;
        err->spans->location.end_line = line;
        err->spans->location.end_column = column;
        err->spans->label = NULL;
        err->spans->source_lines = source_line ? strdup(source_line) : NULL;
        err->spans->next = NULL;
    }
    else
    {
        err->spans = NULL;
    }

    return err;
}

ErrorBuilder *error_builder_new(ErrorType type, const char *code, const char *message)
{
    ErrorBuilder *builder = malloc(sizeof(ErrorBuilder));
    if (builder == NULL)
    {
        return NULL;
    }

    builder->type = type;
    builder->severity = SEVERITY_ERROR;
    builder->code = code ? strdup(code) : NULL;
    builder->message = strdup(message);
    builder->spans = NULL;
    builder->last_span = NULL;
    builder->notes = NULL;
    builder->last_note = NULL;
    builder->suggestion = NULL;

    return builder;
}

void error_builder_set_severity(ErrorBuilder *builder, ErrorSeverity severity)
{
    if (builder != NULL)
    {
        builder->severity = severity;
    }
}

void error_builder_add_span(ErrorBuilder *builder, SourceLocation loc,
                            const char *source, const char *label)
{
    if (builder == NULL)
    {
        return;
    }

    ErrorSpan *span = malloc(sizeof(ErrorSpan));
    if (span == NULL)
    {
        return;
    }

    span->location = loc;
    span->location.filename = loc.filename ? strdup(loc.filename) : NULL;
    span->label = label ? strdup(label) : NULL;
    span->source_lines = source ? strdup(source) : NULL;
    span->next = NULL;

    if (builder->spans == NULL)
    {
        builder->spans = span;
        builder->last_span = span;
    }
    else
    {
        builder->last_span->next = span;
        builder->last_span = span;
    }
}

void error_builder_add_note(ErrorBuilder *builder, const char *note)
{
    if (builder == NULL || note == NULL)
    {
        return;
    }

    ErrorNote *note_obj = malloc(sizeof(ErrorNote));
    if (note_obj == NULL)
    {
        return;
    }

    note_obj->message = strdup(note);
    note_obj->next = NULL;

    if (builder->notes == NULL)
    {
        builder->notes = note_obj;
        builder->last_note = note_obj;
    }
    else
    {
        builder->last_note->next = note_obj;
        builder->last_note = note_obj;
    }
}

void error_builder_set_suggestion(ErrorBuilder *builder, const char *suggestion)
{
    if (builder == NULL || suggestion == NULL)
    {
        return;
    }

    if (builder->suggestion != NULL)
    {
        free(builder->suggestion);
    }
    builder->suggestion = strdup(suggestion);
}

Error *error_builder_build(ErrorBuilder *builder)
{
    if (builder == NULL)
    {
        return NULL;
    }

    Error *err = malloc(sizeof(Error));
    if (err == NULL)
    {
        return NULL;
    }

    err->type = builder->type;
    err->severity = builder->severity;
    err->code = builder->code;
    err->message = builder->message;
    err->spans = builder->spans;
    err->notes = builder->notes;
    err->suggestion = builder->suggestion;
    err->next = NULL;

    /* Transfer ownership, don't free internal pointers */
    free(builder);
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

static void print_span_underline(const ErrorSpan *span, const char *color)
{
    if (span == NULL || span->source_lines == NULL)
    {
        return;
    }

    int line_num = span->location.start_line;
    const char *ptr = span->source_lines;
    const char *line_start = ptr;

    /* Handle single or multi-line spans */
    while (*ptr != '\0')
    {
        if (*ptr == '\n' || *(ptr + 1) == '\0')
        {
            /* Print line with line number */
            int line_len = (*ptr == '\n') ? (ptr - line_start) : (ptr - line_start + 1);
            fprintf(stderr, "\033[1;34m%4d |\033[0m ", line_num);
            fwrite(line_start, 1, line_len, stderr);
            fprintf(stderr, "\n");

            /* Print underline */
            fprintf(stderr, "     \033[1;34m|\033[0m ");

            int start_col = (line_num == span->location.start_line) ? span->location.start_column - 1 : 0;
            int end_col = (line_num == span->location.end_line) ? span->location.end_column : line_len;

            for (int i = 0; i < start_col; i++)
            {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "%s", color);
            for (int i = start_col; i < end_col; i++)
            {
                fprintf(stderr, "^");
            }
            fprintf(stderr, "\033[0m");

            /* Print label if present and on last line */
            if (span->label != NULL && (line_num == span->location.end_line || *(ptr + 1) == '\0'))
            {
                fprintf(stderr, " %s%s\033[0m", color, span->label);
            }
            fprintf(stderr, "\n");

            line_start = ptr + 1;
            line_num++;
        }
        ptr++;
    }
}

void error_print(const Error *err)
{
    if (err == NULL)
    {
        return;
    }

    /* Severity and error type */
    const char *severity_name;
    const char *severity_color;

    switch (err->severity)
    {
    case SEVERITY_WARNING:
        severity_name = "warning";
        severity_color = "\033[1;33m"; /* Yellow */
        break;
    case SEVERITY_NOTE:
        severity_name = "note";
        severity_color = "\033[1;36m"; /* Cyan */
        break;
    case SEVERITY_ERROR:
    default:
        severity_name = "error";
        severity_color = "\033[1;31m"; /* Red */
        break;
    }

    const char *error_type = (err->type == ERROR_PARSE) ? "parse" : "runtime";

    /* Error header with code if present */
    if (err->code != NULL)
    {
        fprintf(stderr, "%s%s[%s][%s]:\033[0m \033[1m%s\033[0m\n",
                severity_color, severity_name, err->code, error_type, err->message);
    }
    else
    {
        fprintf(stderr, "%s%s[%s]:\033[0m \033[1m%s\033[0m\n",
                severity_color, severity_name, error_type, err->message);
    }

    /* Primary span (first span) */
    if (err->spans != NULL)
    {
        const ErrorSpan *primary = err->spans;

        /* Location header */
        if (primary->location.filename != NULL)
        {
            fprintf(stderr, "  \033[1;34m-->\033[0m %s:%d:%d\n",
                    primary->location.filename,
                    primary->location.start_line,
                    primary->location.start_column);
        }
        else
        {
            fprintf(stderr, "  \033[1;34m-->\033[0m line %d:%d\n",
                    primary->location.start_line,
                    primary->location.start_column);
        }

        fprintf(stderr, "     \033[1;34m|\033[0m\n");

        /* Print primary span */
        print_span_underline(primary, severity_color);

        /* Print additional spans */
        const ErrorSpan *span = primary->next;
        while (span != NULL)
        {
            fprintf(stderr, "     \033[1;34m|\033[0m\n");
            print_span_underline(span, "\033[1;36m"); /* Cyan for secondary */
            span = span->next;
        }
    }

    /* Print notes */
    const ErrorNote *note = err->notes;
    while (note != NULL)
    {
        fprintf(stderr, "     \033[1;34m|\033[0m\n");
        fprintf(stderr, "     \033[1;34m= \033[1mnote:\033[0m %s\n", note->message);
        note = note->next;
    }

    /* Print suggestion/help */
    if (err->suggestion != NULL)
    {
        fprintf(stderr, "     \033[1;34m|\033[0m\n");
        fprintf(stderr, "     \033[1;34m= \033[1;32mhelp:\033[0m %s\n", err->suggestion);
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

        /* Free message and code */
        free(list->message);
        free(list->code);
        free(list->suggestion);

        /* Free spans */
        ErrorSpan *span = list->spans;
        while (span != NULL)
        {
            ErrorSpan *next_span = span->next;
            free((char *)span->location.filename);
            free(span->label);
            free((char *)span->source_lines);
            free(span);
            span = next_span;
        }

        /* Free notes */
        ErrorNote *note = list->notes;
        while (note != NULL)
        {
            ErrorNote *next_note = note->next;
            free(note->message);
            free(note);
            note = next_note;
        }

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

char *error_get_source_range(const char *source, int start_line, int end_line)
{
    if (source == NULL || start_line < 1 || end_line < start_line)
    {
        return NULL;
    }

    int current_line = 1;
    const char *range_start = source;
    const char *ptr = source;

    /* Find start line */
    while (*ptr != '\0' && current_line < start_line)
    {
        if (*ptr == '\n')
        {
            current_line++;
            range_start = ptr + 1;
        }
        ptr++;
    }

    if (current_line != start_line)
    {
        return NULL;
    }

    /* Find end of end_line */
    ptr = range_start;
    while (*ptr != '\0' && current_line < end_line)
    {
        if (*ptr == '\n')
        {
            current_line++;
        }
        ptr++;
    }

    /* Go to end of current line */
    while (*ptr != '\0' && *ptr != '\n')
    {
        ptr++;
    }

    /* Copy the range */
    int len = ptr - range_start;
    char *result = malloc(len + 1);
    if (result == NULL)
    {
        return NULL;
    }

    strncpy(result, range_start, len);
    result[len] = '\0';

    return result;
}
