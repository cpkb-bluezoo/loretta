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
    $(SRCDIR)/semantic.c \
    $(SRCDIR)/constpool.c \
    $(SRCDIR)/classwriter.c \
    $(SRCDIR)/stackmap.c \
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
$(SRCDIR)/semantic.o: $(SRCDIR)/semantic.c $(SRCDIR)/loretta.h $(SRCDIR)/util.h
$(SRCDIR)/constpool.o: $(SRCDIR)/constpool.c $(SRCDIR)/constpool.h $(SRCDIR)/util.h
$(SRCDIR)/classwriter.o: $(SRCDIR)/classwriter.c $(SRCDIR)/classwriter.h $(SRCDIR)/constpool.h $(SRCDIR)/util.h
$(SRCDIR)/stackmap.o: $(SRCDIR)/stackmap.c $(SRCDIR)/stackmap.h $(SRCDIR)/constpool.h $(SRCDIR)/util.h
$(SRCDIR)/indy.o: $(SRCDIR)/indy.c $(SRCDIR)/indy.h $(SRCDIR)/constpool.h $(SRCDIR)/util.h
$(SRCDIR)/codegen.o: $(SRCDIR)/codegen.c $(SRCDIR)/codegen.h $(SRCDIR)/loretta.h $(SRCDIR)/constpool.h $(SRCDIR)/classwriter.h $(SRCDIR)/stackmap.h $(SRCDIR)/indy.h $(SRCDIR)/util.h
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

# Build runtime library if needed
runtime/loretta.jar:
	$(MAKE) -C runtime

# Build and run all tests
test: loretta runtime/loretta.jar
	@./test/run_tests.sh

# Run tests with verbose output (shows failures)
test-verbose: loretta runtime/loretta.jar
	@./test/run_tests.sh -v

# Run a single test by name (e.g., make test-one TEST=hello)
test-one: loretta runtime/loretta.jar
	@if [ -z "$(TEST)" ]; then \
		echo "Usage: make test-one TEST=<name>"; \
		echo "Example: make test-one TEST=hello"; \
		exit 1; \
	fi
	@./test/run_tests.sh $(TEST)

# Run a single test with verbose output
test-one-verbose: loretta runtime/loretta.jar
	@if [ -z "$(TEST)" ]; then \
		echo "Usage: make test-one-verbose TEST=<name>"; \
		exit 1; \
	fi
	@./test/run_tests.sh -v $(TEST)

# Clean test build directory
test-clean:
	$(RM) -r $(TEST_BUILD)

# Release build (optimized, no debug symbols)
release:
	$(MAKE) DEBUG=0 clean all

.PHONY: all clean install uninstall test test-verbose test-one test-one-verbose test-clean release
