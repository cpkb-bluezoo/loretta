import java.lang.reflect.Method;

/**
 * $BM - Bound method wrapper.
 * When you access obj.method, this wraps the object and method name
 * so that calling it invokes the method on the object.
 * 
 * Supports two modes:
 * 1. Reflection-based: for built-in types (uses methodName to find Java method)
 * 2. Direct: for user-defined classes (uses $MH directly)
 */
public final class $BM extends $O {
    
    public final $O self;
    public final String methodName;
    private final $MH directMethod;  // For user-defined methods
    private Method cachedMethod;
    
    /**
     * Create a bound method for built-in types (reflection-based).
     */
    public $BM($O self, String methodName) {
        this.self = self;
        this.methodName = methodName;
        this.directMethod = null;
    }
    
    /**
     * Create a bound method for user-defined classes (direct $MH).
     */
    public $BM($O self, String methodName, $MH method) {
        this.self = self;
        this.methodName = methodName;
        this.directMethod = method;
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<bound method " + methodName + " of " + self.__repr__().value + ">");
    }
    
    @Override
    public $O __call__($O... args) {
        // If we have a direct method handle, use it
        if (directMethod != null) {
            // Prepend self to args
            $O[] fullArgs = new $O[args.length + 1];
            fullArgs[0] = self;
            System.arraycopy(args, 0, fullArgs, 1, args.length);
            return directMethod.__call__(fullArgs);
        }
        
        // Otherwise, use reflection for built-in types
        try {
            Class<?> clazz = self.getClass();
            
            // Try to find a matching method
            Method method = findMethod(clazz, methodName, args.length);
            if (method == null) {
                throw new $X("TypeError", "'" + methodName + "' takes different number of arguments");
            }
            
            // Build argument array with proper types
            Object[] javaArgs = convertArgs(method, args);
            
            // Invoke the method
            Object result = method.invoke(self, javaArgs);
            
            // Convert result to $O
            return wrapResult(result);
            
        } catch ($X e) {
            throw e;
        } catch (Exception e) {
            throw new $X("TypeError", "failed to call method '" + methodName + "': " + e.getMessage());
        }
    }
    
    private Method findMethod(Class<?> clazz, String name, int argCount) {
        // First try exact match from cache
        if (cachedMethod != null && cachedMethod.getParameterCount() == argCount) {
            return cachedMethod;
        }
        
        // Search for method with matching name and arg count
        for (Method m : clazz.getMethods()) {
            if (m.getName().equals(name)) {
                int paramCount = m.getParameterCount();
                // Check for exact match or varargs
                if (paramCount == argCount) {
                    cachedMethod = m;
                    return m;
                }
                // Check for varargs (single $O[] parameter)
                if (paramCount == 1 && m.getParameterTypes()[0] == $O[].class) {
                    cachedMethod = m;
                    return m;
                }
            }
        }
        
        // Try with no args if argCount is 0
        if (argCount == 0) {
            try {
                Method m = clazz.getMethod(name);
                cachedMethod = m;
                return m;
            } catch (NoSuchMethodException e) {
                // Continue searching
            }
        }
        
        return null;
    }
    
    private Object[] convertArgs(Method method, $O[] args) {
        Class<?>[] paramTypes = method.getParameterTypes();
        
        // Handle varargs case
        if (paramTypes.length == 1 && paramTypes[0] == $O[].class) {
            return new Object[] { args };
        }
        
        // Handle no-args case
        if (paramTypes.length == 0) {
            return new Object[0];
        }
        
        // Convert each argument
        Object[] result = new Object[args.length];
        for (int i = 0; i < args.length; i++) {
            result[i] = convertArg(args[i], paramTypes[i]);
        }
        return result;
    }
    
    private Object convertArg($O arg, Class<?> targetType) {
        if (targetType == $O.class) {
            return arg;
        }
        if (targetType == String.class && arg instanceof $S) {
            return (($S) arg).value;
        }
        if (targetType == int.class || targetType == Integer.class) {
            if (arg instanceof $I) {
                return (int) (($I) arg).value;
            }
        }
        if (targetType == long.class || targetType == Long.class) {
            if (arg instanceof $I) {
                return (($I) arg).value;
            }
        }
        // Default: pass as-is (might fail if types don't match)
        return arg;
    }
    
    private $O wrapResult(Object result) {
        if (result == null) {
            return $N.INSTANCE;
        }
        if (result instanceof $O) {
            return ($O) result;
        }
        if (result instanceof String) {
            return $S.of((String) result);
        }
        if (result instanceof Integer) {
            return $I.of((Integer) result);
        }
        if (result instanceof Long) {
            return $I.of((Long) result);
        }
        if (result instanceof Double) {
            return $F.of((Double) result);
        }
        if (result instanceof Boolean) {
            return $B.of((Boolean) result);
        }
        // For void methods
        return $N.INSTANCE;
    }
}
