# Chibicc Makefile - Source files to be compiled directly with kernel
# This Makefile is kept for reference; actual compilation is handled by the kernel Makefile

CC ?= gcc
CFLAGS ?= -std=c11 -g -fno-common -Wall -Wno-switch -ffreestanding

# Source files (exclude main.c - to be compiled directly with kernel)
SRCS = codegen.c hashmap.c parse.c preprocess.c strings.c tokenize.c type.c unicode.c
OBJS = $(SRCS:.c=.o)

# Compile object files (for reference/testing)
%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "[CLEAN] chibicc"
	@rm -f $(OBJS)
	@rm -f test/*.o test/*.exe test/*.s

.PHONY: clean
