#ifndef ERROR_H
#define ERROR_H

typedef enum
{
    ERROR_PARSE,
    ERROR_RUNTIME,
} ErrorType;

typedef enum
{
    SEVERITY_ERROR,
    SEVERITY_WARNING,
    SEVERITY_NOTE,
} ErrorSeverity;

/* Source location range */
typedef struct SourceLocation
{
    const char *filename;
    int start_line;
    int start_column;
    int end_line;
    int end_column;
} SourceLocation;

/* Labeled span for highlighting code sections */
typedef struct ErrorSpan
{
    SourceLocation location;
    char *label;              /* Optional label for this span */
    const char *source_lines; /* Source text for this span */
    struct ErrorSpan *next;   /* Linked list of spans */
} ErrorSpan;

/* Additional note attached to error */
typedef struct ErrorNote
{
    char *message;
    struct ErrorNote *next;
} ErrorNote;

typedef struct Error
{
    ErrorType type;
    ErrorSeverity severity;
    char *code;         /* Error code like "E0001" */
    char *message;      /* Main error message */
    ErrorSpan *spans;   /* Primary and additional spans */
    ErrorNote *notes;   /* Additional notes */
    char *suggestion;   /* Optional suggestion/help text */
    struct Error *next; /* Linked list for multiple errors */
} Error;

/* Create a new error (legacy, for backward compatibility) */
Error *error_new(ErrorType type, const char *message, const char *filename,
                 int line, int column, const char *source_line);

/* Error builder API */
typedef struct ErrorBuilder ErrorBuilder;

ErrorBuilder *error_builder_new(ErrorType type, const char *code, const char *message);
void error_builder_set_severity(ErrorBuilder *builder, ErrorSeverity severity);
void error_builder_add_span(ErrorBuilder *builder, SourceLocation loc,
                            const char *source, const char *label);
void error_builder_add_note(ErrorBuilder *builder, const char *note);
void error_builder_set_suggestion(ErrorBuilder *builder, const char *suggestion);
Error *error_builder_build(ErrorBuilder *builder);

/* Add error to list */
void error_append(Error **list, Error *err);

/* Print error in Rust-style format */
void error_print(const Error *err);

/* Print all errors in list */
void error_print_all(const Error *list);

/* Free error list */
void error_free_all(Error *list);

/* Helper: get source line from input by line number */
char *error_get_source_line(const char *input, int line);

/* Helper: extract source lines for a location range */
char *error_get_source_range(const char *source, int start_line, int end_line);

#endif /* ERROR_H */
