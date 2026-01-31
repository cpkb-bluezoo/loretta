# Loretta

Loretta is a Python 3 to JVM bytecode compiler written in C.

## Overview

Loretta compiles Python 3 source code directly to Java bytecode class files,
enabling Python programs to run on the Java Virtual Machine. The compiler
uses `invokedynamic` extensively to handle Python's dynamic semantics while
maintaining reasonable performance through inline caching.

## Architecture

### Compilation Pipeline

```
Python Source → Lexer → Parser → AST → Semantic → Codegen → .class file
```

1. **Lexer** (`lexer.c`) - Tokenizes Python source using feedforward style
   - Handles Python's significant indentation (INDENT/DEDENT tokens)
   - Implicit line continuation inside brackets
   - All Python 3 token types

2. **Parser** (`parser.c`) - Recursive descent parser producing AST
   - Full Python 3 grammar (statements and expressions)
   - AST nodes mirror Python's official AST specification
   - Pratt parsing for operator precedence

3. **Semantic Analyzer** (`semantic.c`) - Semantic analysis
   - Scope and symbol table construction
   - Name resolution
   - Free/cell variable detection for closures

4. **Code Generator** (`codegen.c`) - AST to JVM bytecode
   - Uses `invokedynamic` for all dynamic operations
   - Class file generation via `classwriter.c`
   - Constant pool management via `constpool.c`
   - StackMapTable generation for bytecode verification

### invokedynamic Strategy

Python's dynamic nature is handled via `invokedynamic` call sites:

| Operation | Bootstrap Method | Description |
|-----------|-----------------|-------------|
| `obj.attr` | `getattr` | Attribute lookup |
| `obj.attr = x` | `setattr` | Attribute assignment |
| `del obj.attr` | `delattr` | Attribute deletion |
| `f(args)` | `call` | Function/method call |
| `a + b` | `binop` | Binary operations |
| `a < b` | `compare` | Comparisons |
| `x[i]` | `getitem` | Subscript access |
| `x[i] = v` | `setitem` | Subscript assignment |
| `del x[i]` | `delitem` | Subscript deletion |
| `iter(x)` | `iter` | Iterator protocol |
| `bool(x)` | `bool` | Truth value testing |

Each call site is linked at runtime by bootstrap methods in the Loretta
runtime library, which implement Python's lookup semantics with inline
caching for hot paths.

### Source Files

```
src/
├── loretta.h        # Main header (types, tokens, AST)
├── loretta.c        # Entry point, CLI
├── util.h/c         # Data structures (slist, hashtable, string, bytebuf)
├── lexer.c          # Python tokenizer
├── parser.c         # Recursive descent parser
├── semantic.c       # Semantic analysis
├── constpool.h/c    # JVM constant pool builder
├── classwriter.h/c  # Class file writer
├── stackmap.h/c     # StackMapTable generation
├── indy.h/c         # invokedynamic infrastructure
└── codegen.h/c      # Code generation
```

## Building

```bash
make            # Debug build
make release    # Optimized build
make clean      # Clean build artifacts
```

Requirements:
- C99 compiler (gcc, clang)
- make

To build the runtime library:

```bash
cd runtime
make            # Builds loretta.jar
```

Requirements:
- JDK 11 or later

## Usage

```bash
./loretta [options] <source files>

Options:
  -d <dir>       Output directory for class files
  -v, --verbose  Verbose output
  -g             Generate debug information (default)
  -version       Print version and exit
  -help          Print this help and exit

Example:
  ./loretta -d build hello.py
  java -cp runtime/loretta.jar:build hello
```

## Testing

The test suite compiles Python test files and runs them on the JVM:

```bash
make test              # Run all tests
make test-verbose      # Run with verbose output (show failures)
make test-one TEST=hello  # Run a specific test
```

Tests are in `test/*.py` and compiled to `test/build/*.class`.
See `test/run_tests.sh` for the test runner and skip list.

## Runtime Library

The Loretta runtime library (`runtime/loretta.jar`) implements Python types
and semantics in Java. Ultra-short class names keep bytecode compact:

### Core Types

| Class | Purpose |
|-------|---------|
| `$O`  | PyObject - base class for all Python values |
| `$I`  | PyInt - arbitrary precision integers |
| `$F`  | PyFloat - floating point numbers |
| `$S`  | PyStr - Unicode strings |
| `$B`  | PyBool - boolean values |
| `$N`  | PyNone - the None singleton |
| `$L`  | PyList - mutable lists |
| `$T`  | PyTuple - immutable tuples |
| `$D`  | PyDict - dictionaries |
| `$ST` | PySet - mutable sets |
| `$FS` | PyFrozenSet - immutable sets |
| `$BY` | PyBytes - byte sequences |
| `$C`  | PyComplex - complex numbers |

### Functions and Classes

| Class | Purpose |
|-------|---------|
| `$MH` | PyMethodHandle - callable wrapper |
| `$BM` | PyBoundMethod - bound method object |
| `$Cls` | PyClass - user-defined class object |
| `$Inst` | PyInstance - class instance |
| `$Mod` | PyModule - imported module |

### Infrastructure

| Class | Purpose |
|-------|---------|
| `$G`  | Global/builtin function registry |
| `$BS` | Bootstrap methods for invokedynamic |
| `$X`  | PyException - exception base class |
| `$SL` | PySlice - slice objects |
| `$GE` | PyGeneratorExpression - lazy iterator |
| `$File` | File objects (context manager) |

## Supported Python Features

### Expressions
- Arithmetic, comparison, boolean, bitwise operators
- Comparison chaining (`a < b < c`)
- Conditional expressions (`x if cond else y`)
- Lambda expressions
- List/dict/set/generator comprehensions
- Slicing (`a[1:2:3]`)
- Attribute access and method calls
- `*args` and `**kwargs`

### Statements
- Function definitions (with defaults, `*args`, `**kwargs`)
- Class definitions (with inheritance)
- Control flow (`if`, `while`, `for`, `break`, `continue`)
- Exception handling (`try`/`except`/`finally`, `raise`)
- Context managers (`with`)
- Import system (`import`, `from ... import`)
- `global` and `nonlocal` declarations
- `assert`, `del`, `pass`

### Types
- Integers (arbitrary precision)
- Floats, complex numbers
- Strings (with f-strings)
- Lists, tuples, dicts, sets
- User-defined classes

## Known Limitations

- **Multiple inheritance**: Not yet supported
- **Metaclasses**: Not supported
- **Async/await**: Parsed but not code-generated
- **Standard library**: Only built-in functions, no stdlib modules

## Status

Loretta is functional for many Python programs. See the [TODO](TODO) file
for detailed development status and planned work.

## License

Loretta is free software, licensed under the GNU General Public License
version 3 or later. See COPYING for details.

## Author

Chris Burdess <dog@bluezoo.org>
