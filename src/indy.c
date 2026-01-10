/*
 * indy.c
 * invokedynamic infrastructure for Python operations
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

#include "indy.h"
#include "codegen.h"

void indy_cache_init(indy_cache_t *cache)
{
    if (!cache) {
        return;
    }

    cache->bsm_getattr = -1;
    cache->bsm_setattr = -1;
    cache->bsm_delattr = -1;
    cache->bsm_call = -1;
    cache->bsm_call_method = -1;
    cache->bsm_getitem = -1;
    cache->bsm_setitem = -1;
    cache->bsm_delitem = -1;
    cache->bsm_binop = -1;
    cache->bsm_unaryop = -1;
    cache->bsm_compare = -1;
    cache->bsm_contains = -1;
    cache->bsm_iter = -1;
    cache->bsm_next = -1;
    cache->bsm_builtin = -1;
}

const char *indy_get_descriptor(py_indy_op_t op, int argc)
{
    (void)argc;

    switch (op) {
        /* Attribute access: (PyObject)PyObject or (PyObject, PyObject)V */
        case PY_INDY_GETATTR:
            return "(" DESC_OBJECT ")" DESC_OBJECT;
        case PY_INDY_SETATTR:
            return "(" DESC_OBJECT DESC_OBJECT ")V";
        case PY_INDY_DELATTR:
            return "(" DESC_OBJECT ")V";

        /* Call: (PyObject, PyObject[])PyObject */
        case PY_INDY_CALL:
        case PY_INDY_CALL_METHOD:
            return "(" DESC_OBJECT DESC_OBJECT_ARR ")" DESC_OBJECT;

        /* Subscript: same as attribute but with PyObject key */
        case PY_INDY_GETITEM:
            return "(" DESC_OBJECT DESC_OBJECT ")" DESC_OBJECT;
        case PY_INDY_SETITEM:
            return "(" DESC_OBJECT DESC_OBJECT DESC_OBJECT ")V";
        case PY_INDY_DELITEM:
            return "(" DESC_OBJECT DESC_OBJECT ")V";

        /* Binary operations: (PyObject, PyObject)PyObject */
        case PY_INDY_ADD:
        case PY_INDY_SUB:
        case PY_INDY_MUL:
        case PY_INDY_MATMUL:
        case PY_INDY_TRUEDIV:
        case PY_INDY_FLOORDIV:
        case PY_INDY_MOD:
        case PY_INDY_POW:
        case PY_INDY_LSHIFT:
        case PY_INDY_RSHIFT:
        case PY_INDY_AND:
        case PY_INDY_OR:
        case PY_INDY_XOR:
        case PY_INDY_IADD:
        case PY_INDY_ISUB:
        case PY_INDY_IMUL:
        case PY_INDY_IMATMUL:
        case PY_INDY_ITRUEDIV:
        case PY_INDY_IFLOORDIV:
        case PY_INDY_IMOD:
        case PY_INDY_IPOW:
        case PY_INDY_ILSHIFT:
        case PY_INDY_IRSHIFT:
        case PY_INDY_IAND:
        case PY_INDY_IOR:
        case PY_INDY_IXOR:
            return "(" DESC_OBJECT DESC_OBJECT ")" DESC_OBJECT;

        /* Unary operations: (PyObject)PyObject */
        case PY_INDY_NEG:
        case PY_INDY_POS:
        case PY_INDY_INVERT:
        case PY_INDY_NOT:
            return "(" DESC_OBJECT ")" DESC_OBJECT;

        /* Comparisons: (PyObject, PyObject)PyObject (returns Python bool) */
        case PY_INDY_LT:
        case PY_INDY_LE:
        case PY_INDY_EQ:
        case PY_INDY_NE:
        case PY_INDY_GT:
        case PY_INDY_GE:
        case PY_INDY_IS:
        case PY_INDY_IS_NOT:
        case PY_INDY_CONTAINS:
        case PY_INDY_NOT_CONTAINS:
            return "(" DESC_OBJECT DESC_OBJECT ")" DESC_OBJECT;

        /* Iteration */
        case PY_INDY_ITER:
        case PY_INDY_NEXT:
            return "(" DESC_OBJECT ")" DESC_OBJECT;

        /* Builtins */
        case PY_INDY_BOOL:
            /* Returns int for use with ifeq/ifne */
            return "(" DESC_OBJECT ")I";
        case PY_INDY_LEN:
        case PY_INDY_REPR:
        case PY_INDY_STR:
        case PY_INDY_HASH:
            return "(" DESC_OBJECT ")" DESC_OBJECT;

        default:
            return "(" DESC_OBJECT ")" DESC_OBJECT;
    }
}

const char *indy_binop_name(py_indy_op_t op)
{
    switch (op) {
        case PY_INDY_ADD:       return "__add__";
        case PY_INDY_SUB:       return "__sub__";
        case PY_INDY_MUL:       return "__mul__";
        case PY_INDY_MATMUL:    return "__matmul__";
        case PY_INDY_TRUEDIV:   return "__truediv__";
        case PY_INDY_FLOORDIV:  return "__floordiv__";
        case PY_INDY_MOD:       return "__mod__";
        case PY_INDY_POW:       return "__pow__";
        case PY_INDY_LSHIFT:    return "__lshift__";
        case PY_INDY_RSHIFT:    return "__rshift__";
        case PY_INDY_AND:       return "__and__";
        case PY_INDY_OR:        return "__or__";
        case PY_INDY_XOR:       return "__xor__";
        case PY_INDY_IADD:      return "__iadd__";
        case PY_INDY_ISUB:      return "__isub__";
        case PY_INDY_IMUL:      return "__imul__";
        case PY_INDY_IMATMUL:   return "__imatmul__";
        case PY_INDY_ITRUEDIV:  return "__itruediv__";
        case PY_INDY_IFLOORDIV: return "__ifloordiv__";
        case PY_INDY_IMOD:      return "__imod__";
        case PY_INDY_IPOW:      return "__ipow__";
        case PY_INDY_ILSHIFT:   return "__ilshift__";
        case PY_INDY_IRSHIFT:   return "__irshift__";
        case PY_INDY_IAND:      return "__iand__";
        case PY_INDY_IOR:       return "__ior__";
        case PY_INDY_IXOR:      return "__ixor__";
        default:                return "__unknown__";
    }
}

const char *indy_cmpop_name(py_indy_op_t op)
{
    switch (op) {
        case PY_INDY_LT:           return "__lt__";
        case PY_INDY_LE:           return "__le__";
        case PY_INDY_EQ:           return "__eq__";
        case PY_INDY_NE:           return "__ne__";
        case PY_INDY_GT:           return "__gt__";
        case PY_INDY_GE:           return "__ge__";
        case PY_INDY_IS:           return "is";
        case PY_INDY_IS_NOT:       return "is_not";
        case PY_INDY_CONTAINS:     return "__contains__";
        case PY_INDY_NOT_CONTAINS: return "not_contains";
        default:                   return "__unknown__";
    }
}

/**
 * Register a bootstrap method for a given operation category.
 */
static int16_t ensure_bootstrap_method(class_writer_t *cw,
                                        const char *bsm_name, int16_t *cache_slot)
{
    if (*cache_slot >= 0) {
        return *cache_slot;
    }

    const_pool_t *cp = class_writer_get_cp(cw);

    /* Create method handle for PyBootstrap.bsm_name */
    /* Bootstrap method signature: (Lookup, String, MethodType, ...)CallSite */
    const char *bsm_desc = "(Ljava/lang/invoke/MethodHandles$Lookup;"
                           "Ljava/lang/String;"
                           "Ljava/lang/invoke/MethodType;)"
                           "Ljava/lang/invoke/CallSite;";

    uint16_t method_ref = cp_add_methodref(cp, LRT_BOOTSTRAP, bsm_name, bsm_desc);
    uint16_t method_handle = cp_add_method_handle(cp, REF_invokeStatic, method_ref);

    *cache_slot = class_writer_add_bootstrap_method(cw, method_handle, NULL, 0);
    return *cache_slot;
}

uint16_t indy_emit_operation(class_writer_t *cw, bytebuf_t *code,
                              indy_cache_t *cache, py_indy_op_t op,
                              const char *name, int argc)
{
    if (!cw || !code || !cache) {
        return 0;
    }

    const_pool_t *cp = class_writer_get_cp(cw);
    int16_t bsm_idx = -1;
    const char *call_name = name ? name : "";

    /* Determine which bootstrap method to use */
    switch (op) {
        case PY_INDY_GETATTR:
            bsm_idx = ensure_bootstrap_method(cw, "getattr", &cache->bsm_getattr);
            break;

        case PY_INDY_SETATTR:
            bsm_idx = ensure_bootstrap_method(cw, "setattr", &cache->bsm_setattr);
            break;

        case PY_INDY_DELATTR:
            bsm_idx = ensure_bootstrap_method(cw, "delattr", &cache->bsm_delattr);
            break;

        case PY_INDY_CALL:
            bsm_idx = ensure_bootstrap_method(cw, "call", &cache->bsm_call);
            call_name = "__call__";
            break;

        case PY_INDY_CALL_METHOD:
            bsm_idx = ensure_bootstrap_method(cw, "callMethod", &cache->bsm_call_method);
            call_name = "__call__";
            break;

        case PY_INDY_GETITEM:
            bsm_idx = ensure_bootstrap_method(cw, "getitem", &cache->bsm_getitem);
            call_name = "__getitem__";
            break;

        case PY_INDY_SETITEM:
            bsm_idx = ensure_bootstrap_method(cw, "setitem", &cache->bsm_setitem);
            call_name = "__setitem__";
            break;

        case PY_INDY_DELITEM:
            bsm_idx = ensure_bootstrap_method(cw, "delitem", &cache->bsm_delitem);
            call_name = "__delitem__";
            break;

        case PY_INDY_ADD:
        case PY_INDY_SUB:
        case PY_INDY_MUL:
        case PY_INDY_MATMUL:
        case PY_INDY_TRUEDIV:
        case PY_INDY_FLOORDIV:
        case PY_INDY_MOD:
        case PY_INDY_POW:
        case PY_INDY_LSHIFT:
        case PY_INDY_RSHIFT:
        case PY_INDY_AND:
        case PY_INDY_OR:
        case PY_INDY_XOR:
        case PY_INDY_IADD:
        case PY_INDY_ISUB:
        case PY_INDY_IMUL:
        case PY_INDY_IMATMUL:
        case PY_INDY_ITRUEDIV:
        case PY_INDY_IFLOORDIV:
        case PY_INDY_IMOD:
        case PY_INDY_IPOW:
        case PY_INDY_ILSHIFT:
        case PY_INDY_IRSHIFT:
        case PY_INDY_IAND:
        case PY_INDY_IOR:
        case PY_INDY_IXOR:
            bsm_idx = ensure_bootstrap_method(cw, "binop", &cache->bsm_binop);
            call_name = indy_binop_name(op);
            break;

        case PY_INDY_NEG:
            bsm_idx = ensure_bootstrap_method(cw, "unaryop", &cache->bsm_unaryop);
            call_name = "__neg__";
            break;

        case PY_INDY_POS:
            bsm_idx = ensure_bootstrap_method(cw, "unaryop", &cache->bsm_unaryop);
            call_name = "__pos__";
            break;

        case PY_INDY_INVERT:
            bsm_idx = ensure_bootstrap_method(cw, "unaryop", &cache->bsm_unaryop);
            call_name = "__invert__";
            break;

        case PY_INDY_NOT:
            bsm_idx = ensure_bootstrap_method(cw, "unaryop", &cache->bsm_unaryop);
            call_name = "__not__";
            break;

        case PY_INDY_LT:
        case PY_INDY_LE:
        case PY_INDY_EQ:
        case PY_INDY_NE:
        case PY_INDY_GT:
        case PY_INDY_GE:
        case PY_INDY_IS:
        case PY_INDY_IS_NOT:
            bsm_idx = ensure_bootstrap_method(cw, "compare", &cache->bsm_compare);
            call_name = indy_cmpop_name(op);
            break;

        case PY_INDY_CONTAINS:
        case PY_INDY_NOT_CONTAINS:
            bsm_idx = ensure_bootstrap_method(cw, "contains", &cache->bsm_contains);
            call_name = indy_cmpop_name(op);
            break;

        case PY_INDY_ITER:
            bsm_idx = ensure_bootstrap_method(cw, "iter", &cache->bsm_iter);
            call_name = "__iter__";
            break;

        case PY_INDY_NEXT:
            bsm_idx = ensure_bootstrap_method(cw, "next", &cache->bsm_next);
            call_name = "__next__";
            break;

        case PY_INDY_BOOL:
            bsm_idx = ensure_bootstrap_method(cw, "builtin", &cache->bsm_builtin);
            call_name = "__bool__";
            break;

        case PY_INDY_LEN:
            bsm_idx = ensure_bootstrap_method(cw, "builtin", &cache->bsm_builtin);
            call_name = "__len__";
            break;

        case PY_INDY_REPR:
            bsm_idx = ensure_bootstrap_method(cw, "builtin", &cache->bsm_builtin);
            call_name = "__repr__";
            break;

        case PY_INDY_STR:
            bsm_idx = ensure_bootstrap_method(cw, "builtin", &cache->bsm_builtin);
            call_name = "__str__";
            break;

        case PY_INDY_HASH:
            bsm_idx = ensure_bootstrap_method(cw, "builtin", &cache->bsm_builtin);
            call_name = "__hash__";
            break;

        default:
            return 0;
    }

    if (bsm_idx < 0) {
        return 0;
    }

    /* Create NameAndType for the call site */
    const char *desc = indy_get_descriptor(op, argc);
    uint16_t nat_idx = cp_add_name_and_type(cp, call_name, desc);

    /* Create InvokeDynamic constant pool entry */
    uint16_t indy_idx = cp_add_invoke_dynamic(cp, bsm_idx, nat_idx);

    /* Emit invokedynamic instruction */
    bytebuf_write_u8(code, OP_INVOKEDYNAMIC);
    bytebuf_write_u16(code, indy_idx);
    bytebuf_write_u8(code, 0);  /* Reserved bytes */
    bytebuf_write_u8(code, 0);

    return indy_idx;
}

