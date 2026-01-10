# Makefile for loretta - A Python 3 to JVM bytecode compiler
#
# Build options:
#   make              - Debug build (with -g)
#   make release      - Optimized build (with -O2, no debug symbols)
#   make DEBUG=0      - Same as release

CC ?= cc
DEBUG ?= 1

ifeq ($(DEBUG),1)
CFLAGS = -Wall -Wextra -g -std=c99 -pedantic
else
CFLAGS = -Wall -Wextra -O2 -std=c99 -pedantic -DNDEBUG
endif

# Platform-specific flags
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
# macOS
LDFLAGS ?=
else ifeq ($(UNAME),Linux)
# Linux
LDFLAGS ?=
endif

LIBS =

# Source directory
SRCDIR = src

# Source files
SOURCES = \
    $(SRCDIR)/util.c \
    $(SRCDIR)/lexer.c \
    $(SRCDIR)/parser.c \
    $(SRCDIR)/analyze.c \
    $(SRCDIR)/constpool.c \
    $(SRCDIR)/classwriter.c \
    $(SRCDIR)/indy.c \
    $(SRCDIR)/codegen.c \
    $(SRCDIR)/loretta.c

OBJECTS = $(SOURCES:.c=.o)

# Main target
all: loretta

loretta: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
$(SRCDIR)/util.o: $(SRCDIR)/util.c $(SRCDIR)/util.h
$(SRCDIR)/lexer.o: $(SRCDIR)/lexer.c $(SRCDIR)/loretta.h $(SRCDIR)/util.h
$(SRCDIR)/parser.o: $(SRCDIR)/parser.c $(SRCDIR)/loretta.h $(SRCDIR)/util.h
$(SRCDIR)/analyze.o: $(SRCDIR)/analyze.c $(SRCDIR)/loretta.h $(SRCDIR)/util.h
$(SRCDIR)/constpool.o: $(SRCDIR)/constpool.c $(SRCDIR)/constpool.h $(SRCDIR)/util.h
$(SRCDIR)/classwriter.o: $(SRCDIR)/classwriter.c $(SRCDIR)/classwriter.h $(SRCDIR)/constpool.h $(SRCDIR)/util.h
$(SRCDIR)/indy.o: $(SRCDIR)/indy.c $(SRCDIR)/indy.h $(SRCDIR)/constpool.h $(SRCDIR)/util.h
$(SRCDIR)/codegen.o: $(SRCDIR)/codegen.c $(SRCDIR)/codegen.h $(SRCDIR)/loretta.h $(SRCDIR)/constpool.h $(SRCDIR)/classwriter.h $(SRCDIR)/indy.h $(SRCDIR)/util.h
$(SRCDIR)/loretta.o: $(SRCDIR)/loretta.c $(SRCDIR)/loretta.h $(SRCDIR)/util.h $(SRCDIR)/codegen.h

# Clean up
clean:
	$(RM) loretta $(OBJECTS)
	$(RM) -r *.dSYM

# Install (adjust PREFIX as needed)
PREFIX ?= /usr/local
install: loretta
	install -d $(PREFIX)/bin
	install -m 755 loretta $(PREFIX)/bin/loretta

# Uninstall
uninstall:
	$(RM) $(PREFIX)/bin/loretta

# Test directories
TEST_SRC = test
TEST_BUILD = test/build

# Build and run all tests
test: loretta
	@echo "=== Loretta Test Suite ==="
	@echo ""
	@mkdir -p $(TEST_BUILD)
	@echo "Tests not yet implemented"

# Run a single test with verbose output
test-one: loretta
	@mkdir -p $(TEST_BUILD)
	@if [ -z "$(TEST)" ]; then \
		echo "Usage: make test-one TEST=<TestName>"; \
		exit 1; \
	fi
	@echo "Compiling $(TEST)..."
	./loretta -v -d $(TEST_BUILD) $(TEST_SRC)/$(TEST).py

# Clean test build directory
test-clean:
	$(RM) -r $(TEST_BUILD)

# Release build (optimized, no debug symbols)
release:
	$(MAKE) DEBUG=0 clean all

.PHONY: all clean install uninstall test test-one test-clean release
