* **General**

  * Follow C99; do not use compiler-specific extensions unless already used in the project.
  * No undefined behavior by design; if behavior is implementation-defined, document it.
  * New code must compile without warnings under the project’s default flags (e.g., `-Wall -Wextra -Werror`).

* **Files and Includes**

  * One logical module per `.c`/`.h` pair.
  * Public APIs go in headers; internal functions stay `static` in `.c` files.
  * Use include guards or `#pragma once` consistently (follow existing convention).
  * “Include what you use”: every source file includes the headers it directly needs.
  * Order includes: first standard library headers, then third-party, then project headers.

    * Example:

      * `#include <stdio.h>`
      * `#include <stdlib.h>`
      * `#include "project.h"`
      * `#include "module.h"`

* **Indentation and Layout**

  * Use 4 spaces per indent level; no hard tabs.
  * One statement per line.
  * One declaration per line.
  * Keep lines within the project’s line length limit (default: 100 characters if unspecified).
  * Place opening brace on the same line for control statements and function definitions:

    * `if (cond) {`
    * `while (cond) {`
    * `int func(void) {`
  * Always use braces for `if`, `else`, `for`, `while`, and `do` blocks, even for single statements.

* **Whitespace**

  * Single space after keywords: `if (x)`, `for (i = 0; i < n; i++)`.
  * Single space around binary operators: `a + b`, `x == 0`, `ptr != NULL`.
  * No space after unary operators: `!flag`, `*ptr`, `&var`.
  * No trailing spaces at end of lines.
  * Blank lines separate logical blocks of code; avoid multiple consecutive blank lines.

* **Naming**

  * Functions: `snake_case`, verb-first, descriptive.

    * Examples: `read_config`, `init_buffer`, `compute_checksum`.
  * Variables: `snake_case`, meaningful and specific.

    * Examples: `buffer_size`, `index`, `error_code`.
  * Struct/enum/types: follow repository convention; if none:

    * `typedef` types end with `_t`, e.g., `buffer_t`, `config_t`.
  * Macros and constants: `ALL_CAPS_WITH_UNDERSCORES`.

    * Examples: `MAX_BUFFER_SIZE`, `DEFAULT_PORT`.
  * Global variables (discouraged): prefix with module name, e.g., `log_level`, `config_state`; avoid if possible.

* **Functions**

  * Use explicit return types; never rely on default `int`.
  * Function prototypes in headers must match definitions exactly.
  * Prefer `void` for no-argument functions: `int init_system(void);`.
  * Keep functions short and focused on a single responsibility.
  * Order in `.c` file:

    * Public functions first, then `static` (internal) helpers.
  * Do not nest function definitions (not allowed in standard C).
  * Do not rely on implicit `int` or implicit `extern`.

* **Control Flow**

  * Avoid `goto` unless used for structured cleanup (single exit label, e.g., `out:`).
  * Single `return` at the end is preferred but not mandatory; do not sacrifice clarity.
  * `switch` statements:

    * Always include a `default` case (even if it just logs or asserts).
    * Always use `break` or explicit `fallthrough` comments when falling through is intended.
  * Do not use assignment in conditions without explicit parentheses:

    * Disallow: `if (x = foo())`.
    * Allowed: `if ((x = foo()) != 0)` only when intentional and clear.

* **Types and Casting**

  * Use fixed-width integer types (`int32_t`, `uint64_t`, etc.) where size matters.
  * Minimize casts; when required, use explicit C-style casts.
  * Avoid casting away `const` unless strictly necessary and well-documented.
  * Do not assume sizes: use `sizeof(type)` or `sizeof(*ptr)`; never hard-code sizes.

* **Pointers and NULL**

  * Always initialize pointers; set unused pointers to `NULL`.
  * Compare pointers against `NULL` explicitly: `if (ptr == NULL)` and `if (ptr != NULL)`.
  * After `free(ptr)`, set `ptr = NULL;`.
  * Do not dereference pointers without checking for `NULL` unless provably safe.

* **Memory Management**

  * Use `malloc`, `calloc`, `realloc`, and `free` consistently from `<stdlib.h>`.
  * Always check the result of memory allocation before use.
  * Use `sizeof(*ptr)` instead of repeating the type:

    * `ptr = malloc(count * sizeof(*ptr));`
  * The owner of dynamically allocated memory is clearly defined and documented in comments.
  * No memory leaks: every `malloc`/`calloc`/`realloc` must be paired with a valid `free` in all paths.

* **Error Handling**

  * Use consistent return conventions; default:

    * `0` for success, non-zero (often negative) for error.
  * Check and handle all error codes from:

    * Memory allocations, I/O calls, system/library functions.
  * Do not silently ignore failures; if failure is intentionally ignored, comment why.
  * Prefer propagating errors up the call stack over calling `exit()` inside library-like code.

* **Preprocessor and Macros**

  * Use macros sparingly; prefer `static inline` functions for type safety.
  * Macro names must be uppercase; arguments must be fully parenthesized.

    * Example:

      * `#define SQUARE(x) ((x) * (x))`
  * Avoid macros that evaluate arguments multiple times with side effects.
  * Use `#ifdef`/`#if` in a structured and minimal way; avoid deep nesting.

* **Comments and Documentation**

  * Use `/* ... */` for block comments; `//` only if project already uses C99-style line comments consistently.
  * Comment “why,” not “what the code obviously does.”
  * Public functions must have a brief comment describing:

    * Purpose, parameters, return value, error conditions, ownership rules.
  * Update or remove comments when behavior changes; no stale comments.

* **Const-Correctness**

  * Use `const` for:

    * Pointers to data that should not be modified: `const char *str`.
    * Local variables that are not reassigned.
  * Apply `const` at the most restrictive level possible.

* **Global State**

  * Avoid new global mutable state unless strictly necessary.
  * If global state is required:

    * Keep it `static` and local to a `.c` file when possible.
    * Document usage and thread-safety assumptions.

* **Thread Safety (if applicable in project)**

  * Do not introduce shared mutable state without proper synchronization.
  * Do not assume functions are thread-safe without explicit design or documentation.
  * If a function is not thread-safe, document that clearly.

* **Style Changes Scope**

  * Do not reformat or restyle unrelated code in the same change.
  * When fixing style, limit changes to the lines directly involved in the task.


