/**
 * $CM - classmethod descriptor.
 * 
 * Wraps a function so that when accessed on an instance or class,
 * the first argument is the class (not the instance).
 * 
 * Usage: @classmethod decorator
 */
public final class $CM extends $O {
    
    private final $O func;
    
    public $CM($O func) {
        this.func = func;
    }
    
    /**
     * Get the wrapped function.
     */
    public $O getFunc() {
        return func;
    }
    
    /**
     * Bind to a class and return a bound method.
     */
    public $O bind($Cls cls) {
        return new $BCM(cls, func);
    }
    
    @Override
    public $O __call__($O... args) {
        // When called directly without a class context, raise error
        throw new $X("TypeError", "classmethod requires a class");
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<classmethod object>");
    }
    
    @Override
    public boolean __bool__() {
        return true;
    }
    
    /**
     * Factory method.
     */
    public static $CM of($O func) {
        return new $CM(func);
    }
}

/**
 * Bound class method - a classmethod bound to a specific class.
 */
class $BCM extends $O {
    
    private final $Cls cls;
    private final $O func;
    
    $BCM($Cls cls, $O func) {
        this.cls = cls;
        this.func = func;
    }
    
    @Override
    public $O __call__($O... args) {
        // Prepend the class to the arguments
        $O[] newArgs = new $O[args.length + 1];
        newArgs[0] = cls;
        System.arraycopy(args, 0, newArgs, 1, args.length);
        return func.__call__(newArgs);
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<bound method of " + cls.name + ">");
    }
}
