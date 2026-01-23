/**
 * $B - PyBool for boolean values.
 */
public final class $B extends $I {
    
    /** The True singleton. */
    public static final $B TRUE = new $B(true);
    
    /** The False singleton. */
    public static final $B FALSE = new $B(false);
    
    public final boolean boolValue;
    
    private $B(boolean value) {
        super(value ? 1 : 0);
        this.boolValue = value;
    }
    
    /**
     * Factory method.
     */
    public static $B of(boolean value) {
        return value ? TRUE : FALSE;
    }
    
    @Override
    public boolean __bool__() {
        return boolValue;
    }
    
    @Override
    public $S __repr__() {
        return $S.of(boolValue ? "True" : "False");
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $B) {
            return of(boolValue == (($B)other).boolValue);
        }
        // Bool is a subclass of int in Python
        return super.__eq__(other);
    }
    
    // Inherit all arithmetic from $I since bool is a subclass of int in Python
    // Note: True + True = 2, not True
    
    @Override
    public $O __and__($O other) {
        if (other instanceof $B) {
            return of(boolValue && (($B)other).boolValue);
        }
        return super.__and__(other);
    }
    
    @Override
    public $O __or__($O other) {
        if (other instanceof $B) {
            return of(boolValue || (($B)other).boolValue);
        }
        return super.__or__(other);
    }
    
    @Override
    public $O __xor__($O other) {
        if (other instanceof $B) {
            return of(boolValue ^ (($B)other).boolValue);
        }
        return super.__xor__(other);
    }
}
