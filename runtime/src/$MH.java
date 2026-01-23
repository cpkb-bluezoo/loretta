import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;

/**
 * $MH - Wrapper for MethodHandle to make it a Python object.
 * 
 * This allows compiled Python functions (which are static methods
 * accessed via MethodHandle) to be treated as first-class Python objects.
 * 
 * For closures, the closure array is bound as the first argument.
 */
public final class $MH extends $O {
    
    public final MethodHandle handle;
    public final String name;
    public final $O[] closure;  // Captured variables (null if no closure)
    
    private $MH(MethodHandle handle, String name, $O[] closure) {
        this.handle = handle;
        this.name = name;
        this.closure = closure;
    }
    
    public $MH(MethodHandle handle) {
        this(handle, "<function>", null);
    }
    
    public $MH(MethodHandle handle, String name) {
        this(handle, name, null);
    }
    
    /**
     * Factory method for functions without closures.
     */
    public static $MH of(MethodHandle handle) {
        return new $MH(handle, "<function>", null);
    }
    
    /**
     * Factory method for functions with closures.
     * The closure array will be passed as the first argument when called.
     */
    public static $MH withClosure(MethodHandle handle, $O[] closure) {
        return new $MH(handle, "<closure>", closure);
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<function " + name + ">");
    }
    
    @Override
    public $O __call__($O... args) {
        try {
            if (closure != null) {
                // Prepend closure array to arguments
                return invokeWithClosure(args);
            } else {
                return invokeWithoutClosure(args);
            }
        } catch (Throwable t) {
            if (t instanceof $X) throw ($X) t;
            if (t instanceof RuntimeException) throw (RuntimeException) t;
            throw new $X("RuntimeError", t.getMessage());
        }
    }
    
    private $O invokeWithoutClosure($O[] args) throws Throwable {
        if (args.length == 0) {
            return ($O) handle.invoke();
        } else if (args.length == 1) {
            return ($O) handle.invoke(args[0]);
        } else if (args.length == 2) {
            return ($O) handle.invoke(args[0], args[1]);
        } else if (args.length == 3) {
            return ($O) handle.invoke(args[0], args[1], args[2]);
        } else if (args.length == 4) {
            return ($O) handle.invoke(args[0], args[1], args[2], args[3]);
        } else if (args.length == 5) {
            return ($O) handle.invoke(args[0], args[1], args[2], args[3], args[4]);
        } else if (args.length == 6) {
            return ($O) handle.invoke(args[0], args[1], args[2], args[3], args[4], args[5]);
        } else if (args.length == 7) {
            return ($O) handle.invoke(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        } else if (args.length == 8) {
            return ($O) handle.invoke(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        } else {
            Object[] objArgs = new Object[args.length];
            System.arraycopy(args, 0, objArgs, 0, args.length);
            return ($O) handle.invokeWithArguments(objArgs);
        }
    }
    
    private $O invokeWithClosure($O[] args) throws Throwable {
        // First argument is the closure array, then the user arguments
        int totalArgs = 1 + args.length;
        
        if (totalArgs == 1) {
            return ($O) handle.invoke(closure);
        } else if (totalArgs == 2) {
            return ($O) handle.invoke(closure, args[0]);
        } else if (totalArgs == 3) {
            return ($O) handle.invoke(closure, args[0], args[1]);
        } else if (totalArgs == 4) {
            return ($O) handle.invoke(closure, args[0], args[1], args[2]);
        } else if (totalArgs == 5) {
            return ($O) handle.invoke(closure, args[0], args[1], args[2], args[3]);
        } else if (totalArgs == 6) {
            return ($O) handle.invoke(closure, args[0], args[1], args[2], args[3], args[4]);
        } else if (totalArgs == 7) {
            return ($O) handle.invoke(closure, args[0], args[1], args[2], args[3], args[4], args[5]);
        } else if (totalArgs == 8) {
            return ($O) handle.invoke(closure, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        } else {
            Object[] objArgs = new Object[totalArgs];
            objArgs[0] = closure;
            System.arraycopy(args, 0, objArgs, 1, args.length);
            return ($O) handle.invokeWithArguments(objArgs);
        }
    }
}
