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
Python Source → Lexer → Parser → AST → Analyzer → Codegen → .class file
```

1. **Lexer** (`lexer.c`) - Tokenizes Python source using feedforward style
   - Handles Python's significant indentation (INDENT/DEDENT tokens)
   - Implicit line continuation inside brackets
   - All Python 3 token types

2. **Parser** (`parser.c`) - Recursive descent parser producing AST
   - Python 3 grammar (statements and expressions)
   - AST nodes mirror Python's official AST specification

3. **Analyzer** (`analyze.c`) - Semantic analysis
   - Scope and symbol table construction
   - Name resolution
   - Free/cell variable detection for closures

4. **Code Generator** (`codegen.c`) - AST to JVM bytecode
   - Uses `invokedynamic` for all dynamic operations
   - Class file generation via `classwriter.c`
   - Constant pool management via `constpool.c`

### invokedynamic Strategy

Python's dynamic nature is handled via `invokedynamic` call sites:

| Operation | Bootstrap Method | Description |
|-----------|-----------------|-------------|
| `obj.attr` | `getattr` | Attribute lookup |
| `obj.attr = x` | `setattr` | Attribute assignment |
| `f(args)` | `call` | Function/method call |
| `a + b` | `binop` | Binary operations |
| `a < b` | `compare` | Comparisons |
| `x[i]` | `getitem` | Subscript access |
| `iter(x)` | `iter` | Iterator protocol |

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
├── analyze.c        # Semantic analysis
├── constpool.h/c    # JVM constant pool builder
├── classwriter.h/c  # Class file writer
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
```

## Runtime Library

The Loretta runtime library (to be implemented in Java) uses ultra-short
class names for compact bytecode. The `$` prefix marks these as internal:

| Class | Purpose |
|-------|---------|
| `$O`  | PyObject - base class for all Python values |
| `$I`  | PyInt - arbitrary precision integers |
| `$S`  | PyStr - Unicode strings |
| `$F`  | PyFloat - floating point numbers |
| `$B`  | PyBool - boolean values |
| `$N`  | PyNone - the None singleton |
| `$L`  | PyList - mutable lists |
| `$T`  | PyTuple - immutable tuples |
| `$D`  | PyDict - dictionaries |
| `$BS` | Bootstrap methods for invokedynamic |

This naming scheme reduces bytecode size significantly compared to using
full package names like `org.example.runtime.PyObject`.

## Status

Loretta is in early development. See the [TODO](TODO) file for detailed
development status and planned work.

## License

Loretta is free software, licensed under the GNU General Public License
version 3 or later. See COPYING for details.

## Author

Chris Burdess <dog@bluezoo.org>
