/**
 * $X - PyException, base class for all Python exceptions.
 * 
 * Extends RuntimeException so it can be thrown without declaration.
 */
public class $X extends RuntimeException {
    
    public final String type;
    public final String message;
    
    public $X(String type, String message) {
        super(type + ": " + message);
        this.type = type;
        this.message = message;
    }
    
    public $X(String type) {
        this(type, "");
    }
    
    /**
     * Check if this is a StopIteration exception.
     */
    public boolean isStopIteration() {
        return "StopIteration".equals(type);
    }
    
    /**
     * Get the exception as a Python object (for except clause binding).
     */
    public $O asPyObject() {
        return new $XO(this);
    }
}

/**
 * Wrapper to make $X usable as a Python object.
 */
class $XO extends $O {
    final $X exception;
    
    $XO($X exception) {
        this.exception = exception;
    }
    
    @Override
    public $S __repr__() {
        return $S.of(exception.type + "('" + exception.message + "')");
    }
    
    @Override
    public $S __str__() {
        return $S.of(exception.message);
    }
    
    @Override
    public $O __getattr__(String name) {
        if ("args".equals(name)) {
            return $T.of($S.of(exception.message));
        }
        return super.__getattr__(name);
    }
}
