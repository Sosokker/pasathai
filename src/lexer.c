#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "error.h"

static void read_char(Lexer *l)
{
    if (l->read_position >= (int)strlen(l->input))
    {
        l->ch = 0;
        l->position = l->read_position;
        return;
    }

    unsigned char *str = (unsigned char *)l->input;
    uint32_t code_point = 0;
    int len = 0;

    if (str[l->read_position] < 0x80)
    {
        code_point = str[l->read_position];
        len = 1;
    }
    else if ((str[l->read_position] & 0xE0) == 0xC0)
    {
        code_point = ((str[l->read_position] & 0x1F) << 6) |
                     (str[l->read_position + 1] & 0x3F);
        len = 2;
    }
    else if ((str[l->read_position] & 0xF0) == 0xE0)
    {
        code_point = ((str[l->read_position] & 0x0F) << 12) |
                     ((str[l->read_position + 1] & 0x3F) << 6) |
                     (str[l->read_position + 2] & 0x3F);
        len = 3;
    }
    else if ((str[l->read_position] & 0xF8) == 0xF0)
    {
        code_point = ((str[l->read_position] & 0x07) << 18) |
                     ((str[l->read_position + 1] & 0x3F) << 12) |
                     ((str[l->read_position + 2] & 0x3F) << 6) |
                     (str[l->read_position + 3] & 0x3F);
        len = 4;
    }
    else
    {
        // Invalid UTF-8
        code_point = 0;
        len = 1;
    }

    l->ch = code_point;
    l->position = l->read_position;
    l->read_position += len;

    /* Track line/column */
    if (code_point == '\n')
    {
        l->line++;
        l->column = 0;
    }
    else
    {
        l->column++;
    }
}

static char peek_char(Lexer *l)
{
    if (l->read_position >= (int)strlen(l->input))
    {
        return 0;
    }
    else
    {
        return l->input[l->read_position];
    }
}

static void skip_whitespace(Lexer *l)
{
    while (isspace(l->ch))
    {
        read_char(l);
    }
}

static void skip_comment(Lexer *l)
{
    if (l->ch == '#')
    {
        /* Skip until end of line or end of input */
        while (l->ch != '\n' && l->ch != 0)
        {
            read_char(l);
        }
    }
}

static int is_thai_char(uint32_t ch)
{
    return (ch >= 0x0E00 && ch <= 0x0E7F);
}

static char *read_identifier(Lexer *l)
{
    int position = l->position;
    while (isalpha(l->ch) || l->ch == '_' || is_thai_char(l->ch))
    {
        read_char(l);
    }
    int length = l->position - position;
    char *identifier = malloc(length + 1);
    strncpy(identifier, &l->input[position], length);
    identifier[length] = '\0';
    return identifier;
}

static char *read_number(Lexer *l)
{
    int position = l->position;
    while (isdigit(l->ch))
    {
        read_char(l);
    }
    int length = l->position - position;
    char *number = malloc(length + 1);
    strncpy(number, &l->input[position], length);
    number[length] = '\0';
    return number;
}

static char *read_string(Lexer *l)
{
    /* Skip opening quote */
    int position = l->position + 1;
    read_char(l);

    /* Read until closing quote or EOF */
    while (l->ch != '"' && l->ch != 0)
    {
        /* Handle escape sequences */
        if (l->ch == '\\')
        {
            read_char(l); /* Skip backslash */
            read_char(l); /* Skip escaped char */
        }
        else
        {
            read_char(l);
        }
    }

    int length = l->position - position;
    char *str = malloc(length + 1);

    /* Copy and process escape sequences */
    int j = 0;
    for (int i = 0; i < length; i++)
    {
        if (l->input[position + i] == '\\' && i + 1 < length)
        {
            i++; /* Skip backslash */
            char next = l->input[position + i];
            switch (next)
            {
            case 'n':
                str[j++] = '\n';
                break;
            case 't':
                str[j++] = '\t';
                break;
            case 'r':
                str[j++] = '\r';
                break;
            case '\\':
                str[j++] = '\\';
                break;
            case '"':
                str[j++] = '"';
                break;
            default:
                str[j++] = next;
                break;
            }
        }
        else
        {
            str[j++] = l->input[position + i];
        }
    }
    str[j] = '\0';

    return str;
}

static TokenType lookup_ident(char *ident)
{
    if (strcmp(ident, "ให้") == 0)
        return TOKEN_LET;
    if (strcmp(ident, "ฟังก์ชัน") == 0)
        return TOKEN_FUNCTION;
    if (strcmp(ident, "จริง") == 0)
        return TOKEN_TRUE;
    if (strcmp(ident, "เท็จ") == 0)
        return TOKEN_FALSE;
    if (strcmp(ident, "ถ้า") == 0)
        return TOKEN_IF;
    if (strcmp(ident, "ไม่งั้น") == 0)
        return TOKEN_ELSE;
    if (strcmp(ident, "คืนค่า") == 0)
        return TOKEN_RETURN;
    if (strcmp(ident, "ขณะที่") == 0)
        return TOKEN_WHILE;
    if (strcmp(ident, "ว่างเปล่า") == 0)
        return TOKEN_NULL;
    if (strcmp(ident, "สำหรับ") == 0)
        return TOKEN_FOR;
    if (strcmp(ident, "จาก") == 0)
        return TOKEN_FROM;
    if (strcmp(ident, "ถึง") == 0)
        return TOKEN_TO;
    if (strcmp(ident, "ก่อนถึง") == 0)
        return TOKEN_BEFORE_TO;
    return TOKEN_IDENT;
}

Lexer *new_lexer(const char *input)
{
    Lexer *l = malloc(sizeof(Lexer));
    l->input = input;
    l->position = 0;
    l->read_position = 0;
    l->ch = 0;
    l->line = 1;
    l->column = 0;
    l->errors = NULL;
    l->filename = NULL;
    read_char(l);
    return l;
}

void lexer_set_filename(Lexer *l, const char *filename)
{
    if (l != NULL)
    {
        l->filename = filename;
    }
}

int lexer_has_errors(Lexer *l)
{
    return l != NULL && l->errors != NULL;
}

void lexer_print_errors(Lexer *l)
{
    if (l != NULL && l->errors != NULL)
    {
        error_print_all(l->errors);
    }
}

void next_token(Lexer *l, Token *tok)
{
    /* Skip whitespace and comments until we find real content */
    while (1)
    {
        skip_whitespace(l);
        if (l->ch == '#')
        {
            skip_comment(l);
        }
        else
        {
            break;
        }
    }

    /* Store current position for token */
    tok->line = l->line;
    tok->column = l->column;

    switch (l->ch)
    {
    case '=':
        if (peek_char(l) == '=')
        {
            read_char(l);
            tok->type = TOKEN_EQ;
            tok->literal = "==";
        }
        else
        {
            tok->type = TOKEN_ASSIGN;
            tok->literal = "=";
        }
        break;
    case '+':
        tok->type = TOKEN_PLUS;
        tok->literal = "+";
        break;
    case '-':
        tok->type = TOKEN_MINUS;
        tok->literal = "-";
        break;
    case '!':
        if (peek_char(l) == '=')
        {
            read_char(l);
            tok->type = TOKEN_NOT_EQ;
            tok->literal = "!=";
        }
        else
        {
            tok->type = TOKEN_BANG;
            tok->literal = "!";
        }
        break;
    case '/':
        tok->type = TOKEN_SLASH;
        tok->literal = "/";
        break;
    case '*':
        tok->type = TOKEN_ASTERISK;
        tok->literal = "*";
        break;
    case '%':
        tok->type = TOKEN_MODULO;
        tok->literal = "%";
        break;
    case '<':
        tok->type = TOKEN_LT;
        tok->literal = "<";
        break;
    case '>':
        tok->type = TOKEN_GT;
        tok->literal = ">";
        break;
    case ';':
        tok->type = TOKEN_SEMICOLON;
        tok->literal = ";";
        break;
    case ',':
        tok->type = TOKEN_COMMA;
        tok->literal = ",";
        break;
    case '(':
        tok->type = TOKEN_LPAREN;
        tok->literal = "(";
        break;
    case ')':
        tok->type = TOKEN_RPAREN;
        tok->literal = ")";
        break;
    case '{':
        tok->type = TOKEN_LBRACE;
        tok->literal = "{";
        break;
    case '}':
        tok->type = TOKEN_RBRACE;
        tok->literal = "}";
        break;
    case '[':
        tok->type = TOKEN_LBRACKET;
        tok->literal = "[";
        break;
    case ']':
        tok->type = TOKEN_RBRACKET;
        tok->literal = "]";
        break;
    case '"':
        tok->type = TOKEN_STRING;
        tok->literal = read_string(l);
        read_char(l); /* Skip closing quote */
        return;
    case 0:
        tok->type = TOKEN_EOF;
        tok->literal = "";
        break;
    default:
        if (isalpha(l->ch) || l->ch == '_' || is_thai_char(l->ch))
        {
            tok->literal = read_identifier(l);
            tok->type = lookup_ident(tok->literal);
            return;
        }
        else if (isdigit(l->ch))
        {
            tok->type = TOKEN_INT;
            tok->literal = read_number(l);
            return;
        }
        else
        {
            /* Generate error for illegal character */
            tok->type = TOKEN_ILLEGAL;

            char ch_str[16] = {0};
            if (l->ch < 128 && isprint(l->ch))
            {
                snprintf(ch_str, sizeof(ch_str), "%c", (char)l->ch);
            }
            else
            {
                snprintf(ch_str, sizeof(ch_str), "U+%04X", l->ch);
            }
            tok->literal = strdup(ch_str);

            /* Build lexer error */
            char message[128];
            snprintf(message, sizeof(message), "unexpected character: '%s'", ch_str);

            ErrorBuilder *builder = error_builder_new(ERROR_PARSE, "E100", message);
            if (builder != NULL)
            {
                char *source_line = error_get_source_line(l->input, l->line);

                SourceLocation loc = {
                    .filename = l->filename,
                    .start_line = l->line,
                    .start_column = l->column,
                    .end_line = l->line,
                    .end_column = l->column};

                error_builder_add_span(builder, loc, source_line, "illegal character");
                error_builder_set_suggestion(builder, "remove this character or check for encoding issues");

                Error *err = error_builder_build(builder);
                error_append(&l->errors, err);

                if (source_line != NULL)
                {
                    free(source_line);
                }
            }
        }
    }

    read_char(l);
}
