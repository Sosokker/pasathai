export PATH := C:\w64devkit\bin:$(PATH)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc
SRCS = src/lexer.c src/parser.c src/ast.c src/evaluator.c src/object.c src/gc.c src/error.c src/main.c
OBJS = $(SRCS:.c=.o)
TARGET = pasathai

.PHONY: all clean test test-all test-quick test-basic help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

help:
	@echo "Pasathai Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build the project"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make test     - Run all tests"
	@echo "  make test-quick - Run quick smoke tests"
	@echo "  make test-basic - Run basic test only"
	@echo "  make help     - Show this help message"

# Test targets
test: $(TARGET)
	@echo "Running all tests..."
	@$(TARGET) tests/test_comments.thai
	@$(TARGET) tests/test_strings.thai
	@$(TARGET) tests/test_integers.thai
	@$(TARGET) tests/test_booleans.thai
	@$(TARGET) tests/test_conditionals.thai
	@$(TARGET) tests/test_while_loops.thai
	@$(TARGET) tests/test_functions.thai
	@$(TARGET) tests/test_scoping.thai
	@$(TARGET) tests/test_builtin_functions.thai
	@$(TARGET) tests/test_expressions.thai
	@$(TARGET) tests/test_edge_cases.thai
	@echo "All tests completed!"

test-quick: $(TARGET)
	@$(TARGET) tests/test.thai
	@$(TARGET) tests/test_strings.thai

test-basic: $(TARGET)
	@$(TARGET) tests/test.thai
