import java.lang.invoke.*;

/**
 * $BS - Bootstrap methods for invokedynamic call sites.
 * 
 * These methods are called by the JVM to link invokedynamic call sites.
 * They return CallSite objects that provide the actual method handles
 * to invoke for Python operations.
 */
public final class $BS {
    
    private static final MethodHandles.Lookup LOOKUP = MethodHandles.lookup();
    
    // Method type for most operations: ($O, $O) -> $O
    private static final MethodType BINARY_TYPE = MethodType.methodType($O.class, $O.class, $O.class);
    
    // Method type for unary: ($O) -> $O
    private static final MethodType UNARY_TYPE = MethodType.methodType($O.class, $O.class);
    
    // Method type for bool conversion: ($O) -> boolean
    private static final MethodType BOOL_TYPE = MethodType.methodType(boolean.class, $O.class);
    
    /**
     * Bootstrap method for binary operations (__add__, __sub__, etc.)
     */
    public static CallSite binop(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            // Look up the method on $O class
            mh = LOOKUP.findVirtual($O.class, name, MethodType.methodType($O.class, $O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap binop: " + name, e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for unary operations (__neg__, __pos__, etc.)
     */
    public static CallSite unaryop(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, name, MethodType.methodType($O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap unaryop: " + name, e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for comparison operations (__lt__, __eq__, etc.)
     */
    public static CallSite compare(MethodHandles.Lookup lookup, String name, MethodType type) {
        // Comparisons are just binary operations returning $O (which is a $B)
        return binop(lookup, name, type);
    }
    
    /**
     * Bootstrap method for boolean conversion (for if/while conditions).
     */
    public static CallSite bool(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__bool__", MethodType.methodType(boolean.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap bool", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for built-in protocol methods (__bool__, __len__, __repr__, __str__, __hash__).
     * These are all unary operations that take an object and return something.
     */
    public static CallSite builtin(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            // name is the dunder method name like __bool__, __len__, etc.
            if ("__bool__".equals(name)) {
                // __bool__ returns boolean, but codegen expects int for ifeq/ifne
                // We use a wrapper that converts boolean to int
                mh = LOOKUP.findStatic($BS.class, "boolToInt",
                        MethodType.methodType(int.class, $O.class));
            } else {
                Class<?> returnType;
                switch (name) {
                    case "__len__":
                    case "__hash__":
                        returnType = $I.class;
                        break;
                    case "__repr__":
                    case "__str__":
                        returnType = $S.class;
                        break;
                    default:
                        returnType = $O.class;
                }
                mh = LOOKUP.findVirtual($O.class, name, MethodType.methodType(returnType));
            }
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap builtin: " + name, e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Helper to convert Python __bool__ (which returns boolean) to int for JVM branching.
     */
    public static int boolToInt($O obj) {
        return obj.__bool__() ? 1 : 0;
    }
    
    /**
     * Bootstrap method for attribute access (__getattr__).
     */
    public static CallSite getattr(MethodHandles.Lookup lookup, String name, MethodType type) {
        // name is the attribute name
        // type is ($O) -> $O
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__getattr__", 
                    MethodType.methodType($O.class, String.class));
            mh = MethodHandles.insertArguments(mh, 1, name);
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap getattr: " + name, e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for attribute assignment (__setattr__).
     */
    public static CallSite setattr(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__setattr__",
                    MethodType.methodType(void.class, String.class, $O.class));
            mh = MethodHandles.insertArguments(mh, 1, name);
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap setattr: " + name, e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for subscript access (__getitem__).
     */
    public static CallSite getitem(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__getitem__",
                    MethodType.methodType($O.class, $O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap getitem", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for subscript assignment (__setitem__).
     */
    public static CallSite setitem(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__setitem__",
                    MethodType.methodType(void.class, $O.class, $O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap setitem", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for attribute deletion (__delattr__).
     */
    public static CallSite delattr(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findStatic($BS.class, "invokeDelattr",
                    MethodType.methodType(void.class, $O.class, String.class));
            mh = MethodHandles.insertArguments(mh, 1, name);
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap delattr: " + name, e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Helper for delattr - calls __delattr__ on the object.
     */
    public static void invokeDelattr($O obj, String name) {
        if (obj instanceof $Inst) {
            (($Inst) obj).__delattr__(name);
        } else if (obj instanceof $D) {
            (($D) obj).__delitem__($S.of(name));
        } else {
            throw new $X("AttributeError", "'" + obj.getClass().getSimpleName() + 
                         "' object has no attribute '" + name + "'");
        }
    }
    
    /**
     * Bootstrap method for subscript deletion (__delitem__).
     */
    public static CallSite delitem(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__delitem__",
                    MethodType.methodType(void.class, $O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap delitem", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for function calls (__call__).
     * The call site type is ($O, [$O) -> $O where the first arg is the callable.
     */
    public static CallSite call(MethodHandles.Lookup lookup, String name, MethodType type) {
        // We use a simple approach: invoke __call__ with the args array
        // type will be ($O, $O[]) -> $O  or similar with varargs
        MethodHandle mh;
        try {
            // Try MethodHandle first (for compiled functions)
            mh = MethodHandles.filterReturnValue(
                LOOKUP.findStatic($BS.class, "invokeCallable",
                    MethodType.methodType($O.class, $O.class, $O[].class)),
                MethodHandles.identity($O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap call", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Helper method to invoke a callable object.
     */
    public static $O invokeCallable($O callable, $O[] args) {
        // Use __call__ which handles $MH (wrapped MethodHandle) and other callables
        return callable.__call__(args);
    }
    
    /**
     * Bootstrap method for iteration (__iter__).
     */
    public static CallSite iter(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__iter__",
                    MethodType.methodType($O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap iter", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Bootstrap method for iterator next (__next__).
     * The codegen expects null return on iterator exhaustion, so we catch StopIteration.
     */
    public static CallSite next(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            // Use a wrapper that catches StopIteration and returns null
            mh = LOOKUP.findStatic($BS.class, "safeNext",
                    MethodType.methodType($O.class, $O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap next", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Wrapper for __next__ that catches StopIteration and returns null.
     */
    public static $O safeNext($O iter) {
        try {
            return iter.__next__();
        } catch ($X e) {
            if (e.isStopIteration()) {
                return null;
            }
            throw e;
        }
    }
    
    /**
     * Bootstrap method for membership test (__contains__).
     */
    public static CallSite contains(MethodHandles.Lookup lookup, String name, MethodType type) {
        MethodHandle mh;
        try {
            mh = LOOKUP.findVirtual($O.class, "__contains__",
                    MethodType.methodType($B.class, $O.class));
        } catch (Exception e) {
            throw new RuntimeException("Failed to bootstrap contains", e);
        }
        return new ConstantCallSite(mh.asType(type));
    }
    
    /**
     * Check if an exception matches a type (for except clause type checking).
     * 
     * @param exc The exception that was caught
     * @param excType The exception type to check against (a $Cls or tuple of $Cls)
     * @return true if the exception matches the type
     */
    public static boolean exceptionMatches($X exc, $O excType) {
        if (excType instanceof $Cls) {
            $Cls cls = ($Cls) excType;
            
            // First try Java instanceof check if we have proper subclass
            if (cls.javaClass != null && cls.javaClass.isInstance(exc)) {
                return true;
            }
            
            // Fallback: check by type name with hierarchy awareness
            return typeMatchesByName(exc.type, cls.name);
        }
        
        if (excType instanceof $T) {
            // Tuple of exception types
            for ($O item : (($T) excType).items) {
                if (exceptionMatches(exc, item)) {
                    return true;
                }
            }
            return false;
        }
        
        // Unknown type, don't match
        return false;
    }
    
    /**
     * Check if an exception type name matches a target type name, considering hierarchy.
     */
    private static boolean typeMatchesByName(String excType, String targetType) {
        if (excType.equals(targetType)) return true;
        
        // Check hierarchy - these are the known parent relationships
        // This handles old-style exceptions created via $X(typeName, message)
        switch (excType) {
            case "ZeroDivisionError":
            case "OverflowError":
            case "FloatingPointError":
                if (targetType.equals("ArithmeticError") || targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "ArithmeticError":
                if (targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "IndexError":
            case "KeyError":
                if (targetType.equals("LookupError") || targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "LookupError":
                if (targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "ModuleNotFoundError":
                if (targetType.equals("ImportError") || targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "UnboundLocalError":
                if (targetType.equals("NameError") || targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "FileNotFoundError":
            case "FileExistsError":
            case "PermissionError":
            case "IsADirectoryError":
            case "NotADirectoryError":
                if (targetType.equals("OSError") || targetType.equals("IOError") || targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "NotImplementedError":
            case "RecursionError":
                if (targetType.equals("RuntimeError") || targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "UnicodeError":
                if (targetType.equals("ValueError") || targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "TypeError":
            case "ValueError":
            case "AttributeError":
            case "NameError":
            case "ImportError":
            case "RuntimeError":
            case "OSError":
            case "IOError":
            case "AssertionError":
            case "EOFError":
            case "SyntaxError":
            case "StopIteration":
            case "StopAsyncIteration":
                if (targetType.equals("Exception") || targetType.equals("BaseException")) return true;
                break;
            case "SystemExit":
            case "KeyboardInterrupt":
            case "GeneratorExit":
                if (targetType.equals("BaseException")) return true;
                break;
            case "Exception":
                if (targetType.equals("BaseException")) return true;
                break;
        }
        return false;
    }
    
    /**
     * Raise a Python exception.
     * Handles both $XO wrappers (from exception class calls) and raw values.
     * Returns the exception to throw (caller uses athrow).
     * 
     * @param value The value to raise - can be $XO, $Cls (exception class), or any value
     * @return The exception to throw
     */
    public static $X raiseException($O value) {
        if (value instanceof $XO) {
            // Unwrap and return the Java exception
            return (($XO) value).exception;
        }
        
        if (value instanceof $Cls) {
            // Bare exception class: raise TypeError -> TypeError()
            $Cls cls = ($Cls) value;
            if (cls.javaClass != null && $X.class.isAssignableFrom(cls.javaClass)) {
                return $X.create(cls.name, "");
            }
        }
        
        // Default: convert to string and wrap in generic Exception
        String message = value.__str__().value;
        return new $X.Exception(message);
    }
    
    /**
     * Raise a Python exception with cause (for raise X from Y).
     * Returns the exception to throw (caller uses athrow).
     * 
     * @param value The exception to raise
     * @param cause The cause exception
     * @return The exception to throw
     */
    public static $X raiseExceptionFrom($O value, $O cause) {
        $X exc;
        
        if (value instanceof $XO) {
            exc = (($XO) value).exception;
        } else if (value instanceof $Cls) {
            $Cls cls = ($Cls) value;
            if (cls.javaClass != null && $X.class.isAssignableFrom(cls.javaClass)) {
                exc = $X.create(cls.name, "");
            } else {
                exc = new $X.Exception(value.__str__().value);
            }
        } else {
            exc = new $X.Exception(value.__str__().value);
        }
        
        // Set the cause if it's an exception
        if (cause instanceof $XO) {
            exc.initCause((($XO) cause).exception);
        }
        
        return exc;
    }
}
