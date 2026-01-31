/**
 * $SM - staticmethod descriptor.
 * 
 * Wraps a function so that it's not bound to an instance when accessed.
 * Static methods don't receive 'self' as the first argument.
 * 
 * Usage: @staticmethod decorator
 */
public final class $SM extends $O {
    
    private final $O func;
    
    public $SM($O func) {
        this.func = func;
    }
    
    /**
     * Get the wrapped function (no binding).
     */
    public $O getFunc() {
        return func;
    }
    
    @Override
    public $O __call__($O... args) {
        // When called directly, just call the underlying function
        return func.__call__(args);
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<staticmethod object>");
    }
    
    @Override
    public boolean __bool__() {
        return true;
    }
    
    /**
     * Factory method.
     */
    public static $SM of($O func) {
        return new $SM(func);
    }
}
