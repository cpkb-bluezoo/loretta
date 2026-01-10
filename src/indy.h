/*
 * indy.h
 * invokedynamic infrastructure for Python semantics
 * Copyright (C) 2026 Chris Burdess <dog@bluezoo.org>
 *
 * This file is part of loretta.
 *
 * loretta is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * loretta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef INDY_H
#define INDY_H

#include <stdint.h>
#include "util.h"
#include "constpool.h"
#include "classwriter.h"

/*
 * Python dynamic operations implemented via invokedynamic
 *
 * The loretta compiler uses invokedynamic for all Python operations that
 * require runtime type dispatch. Bootstrap methods in the loretta runtime
 * ($BS class) implement Python's lookup semantics with inline caching for
 * performance.
 *
 * Runtime classes use short names for compact bytecode:
 *   $O  - PyObject      $I  - PyInt        $S  - PyStr
 *   $F  - PyFloat       $B  - PyBool       $N  - PyNone
 *   $L  - PyList        $T  - PyTuple      $D  - PyDict
 *   $X  - PyException   $BS - Bootstrap methods
 *
 * Call site types:
 *   GETATTR     - obj.attr          → PyBootstrap.getattr
 *   SETATTR     - obj.attr = value  → PyBootstrap.setattr
 *   DELATTR     - del obj.attr      → PyBootstrap.delattr
 *   CALL        - func(args)        → PyBootstrap.call
 *   GETITEM     - obj[key]          → PyBootstrap.getitem
 *   SETITEM     - obj[key] = value  → PyBootstrap.setitem
 *   DELITEM     - del obj[key]      → PyBootstrap.delitem
 *   BINOP_*     - a + b, a - b, ... → PyBootstrap.binop
 *   UNARYOP_*   - -a, ~a, not a     → PyBootstrap.unaryop
 *   COMPARE_*   - a < b, a == b     → PyBootstrap.compare
 *   CONTAINS    - a in b            → PyBootstrap.contains
 *   ITER        - iter(obj)         → PyBootstrap.iter
 *   NEXT        - next(iter)        → PyBootstrap.next
 */

/* ========================================================================
 * Python invokedynamic call site types
 * ======================================================================== */

typedef enum py_indy_op
{
    /* Attribute access */
    PY_INDY_GETATTR,
    PY_INDY_SETATTR,
    PY_INDY_DELATTR,

    /* Call operations */
    PY_INDY_CALL,                   /* Function call */
    PY_INDY_CALL_METHOD,            /* Method call (obj.method(args)) */

    /* Subscript operations */
    PY_INDY_GETITEM,
    PY_INDY_SETITEM,
    PY_INDY_DELITEM,

    /* Binary operations */
    PY_INDY_ADD,
    PY_INDY_SUB,
    PY_INDY_MUL,
    PY_INDY_MATMUL,
    PY_INDY_TRUEDIV,
    PY_INDY_FLOORDIV,
    PY_INDY_MOD,
    PY_INDY_POW,
    PY_INDY_LSHIFT,
    PY_INDY_RSHIFT,
    PY_INDY_AND,
    PY_INDY_OR,
    PY_INDY_XOR,

    /* In-place binary operations */
    PY_INDY_IADD,
    PY_INDY_ISUB,
    PY_INDY_IMUL,
    PY_INDY_IMATMUL,
    PY_INDY_ITRUEDIV,
    PY_INDY_IFLOORDIV,
    PY_INDY_IMOD,
    PY_INDY_IPOW,
    PY_INDY_ILSHIFT,
    PY_INDY_IRSHIFT,
    PY_INDY_IAND,
    PY_INDY_IOR,
    PY_INDY_IXOR,

    /* Unary operations */
    PY_INDY_NEG,                    /* -x */
    PY_INDY_POS,                    /* +x */
    PY_INDY_INVERT,                 /* ~x */
    PY_INDY_NOT,                    /* not x (returns Python bool) */

    /* Comparison operations */
    PY_INDY_LT,
    PY_INDY_LE,
    PY_INDY_EQ,
    PY_INDY_NE,
    PY_INDY_GT,
    PY_INDY_GE,
    PY_INDY_IS,
    PY_INDY_IS_NOT,

    /* Containment */
    PY_INDY_CONTAINS,               /* in */
    PY_INDY_NOT_CONTAINS,           /* not in */

    /* Iteration */
    PY_INDY_ITER,                   /* Get iterator */
    PY_INDY_NEXT,                   /* Get next item */

    /* Type operations */
    PY_INDY_BOOL,                   /* Convert to Python bool */
    PY_INDY_LEN,                    /* len(x) */
    PY_INDY_REPR,                   /* repr(x) */
    PY_INDY_STR,                    /* str(x) */
    PY_INDY_HASH                    /* hash(x) */
} py_indy_op_t;

/* ========================================================================
 * Bootstrap method cache
 *
 * Each class tracks which bootstrap methods have been registered so we
 * don't create duplicates.
 * ======================================================================== */

typedef struct indy_cache
{
    /* Bootstrap method indices for each operation type */
    /* -1 means not yet registered */
    int16_t bsm_getattr;
    int16_t bsm_setattr;
    int16_t bsm_delattr;
    int16_t bsm_call;
    int16_t bsm_call_method;
    int16_t bsm_getitem;
    int16_t bsm_setitem;
    int16_t bsm_delitem;
    int16_t bsm_binop;
    int16_t bsm_unaryop;
    int16_t bsm_compare;
    int16_t bsm_contains;
    int16_t bsm_iter;
    int16_t bsm_next;
    int16_t bsm_builtin;            /* For bool, len, repr, str, hash */
} indy_cache_t;

/**
 * Initialize an indy cache (set all indices to -1).
 */
void indy_cache_init(indy_cache_t *cache);

/* ========================================================================
 * invokedynamic call site generation
 * ======================================================================== */

/**
 * Generate an invokedynamic instruction for a Python operation.
 *
 * @param cw        Class writer
 * @param code      Code buffer to write instruction to
 * @param cache     Bootstrap method cache for this class
 * @param op        Python operation type
 * @param name      Operation-specific name (attr name for GETATTR, etc.)
 * @param argc      For CALL operations: number of positional arguments
 *
 * @return CP index of the InvokeDynamic entry, or 0 on error
 *
 * The generated call site signature depends on the operation:
 *   GETATTR:     (PyObject)PyObject
 *   SETATTR:     (PyObject, PyObject)V
 *   CALL:        (PyObject, PyObject[])PyObject   [varargs]
 *   BINOP:       (PyObject, PyObject)PyObject
 *   COMPARE:     (PyObject, PyObject)PyObject
 *   etc.
 */
uint16_t indy_emit_operation(class_writer_t *cw, bytebuf_t *code,
                              indy_cache_t *cache, py_indy_op_t op,
                              const char *name, int argc);

/**
 * Get the method descriptor for a Python operation.
 */
const char *indy_get_descriptor(py_indy_op_t op, int argc);

/**
 * Get the operation name string for a binary operation.
 */
const char *indy_binop_name(py_indy_op_t op);

/**
 * Get the operation name string for a comparison operation.
 */
const char *indy_cmpop_name(py_indy_op_t op);

/* ========================================================================
 * Runtime class names (short form for compact bytecode)
 *
 * The loretta runtime uses ultra-short class names to minimize bytecode
 * size. The $ prefix marks these as compiler-internal and avoids conflicts.
 * ======================================================================== */

/* Core Python object - base class for all Python values */
#define LRT_OBJECT      "$O"

/* Python type/class objects */
#define LRT_TYPE        "$Y"

/* Singleton types */
#define LRT_NONE        "$N"
#define LRT_BOOL        "$B"

/* Numeric types */
#define LRT_INT         "$I"
#define LRT_FLOAT       "$F"
#define LRT_COMPLEX     "$C"

/* Sequence types */
#define LRT_STR         "$S"
#define LRT_BYTES       "$BY"
#define LRT_LIST        "$L"
#define LRT_TUPLE       "$T"

/* Mapping and set types */
#define LRT_DICT        "$D"
#define LRT_SET         "$ST"
#define LRT_FROZENSET   "$FS"

/* Callable types */
#define LRT_FUNCTION    "$FN"
#define LRT_CODE        "$CO"
#define LRT_FRAME       "$FR"

/* Bootstrap methods class (for invokedynamic) */
#define LRT_BOOTSTRAP   "$BS"

/* Exception types */
#define LRT_EXCEPTION   "$X"

/* Iterator types */
#define LRT_ITERATOR    "$IT"
#define LRT_GENERATOR   "$GN"

/* Common descriptors */
#define DESC_OBJECT     "L" LRT_OBJECT ";"
#define DESC_OBJECT_ARR "[" DESC_OBJECT
#define DESC_INT        "L" LRT_INT ";"
#define DESC_STR        "L" LRT_STR ";"
#define DESC_BOOL       "L" LRT_BOOL ";"
#define DESC_LIST       "L" LRT_LIST ";"
#define DESC_DICT       "L" LRT_DICT ";"
#define DESC_TUPLE      "L" LRT_TUPLE ";"
#define DESC_NONE       "L" LRT_NONE ";"

#endif /* INDY_H */

