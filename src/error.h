#ifndef ERROR_H
#define ERROR_H

typedef enum
{
    ERROR_PARSE,
    ERROR_RUNTIME,
} ErrorType;

typedef struct Error
{
    ErrorType type;
    char *message;
    char *filename;
    int line;
    int column;
    char *source_line;  /* The actual source line for context */
    struct Error *next; /* Linked list for multiple errors */
} Error;

/* Create a new error */
Error *error_new(ErrorType type, const char *message, const char *filename,
                 int line, int column, const char *source_line);

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

#endif /* ERROR_H */
