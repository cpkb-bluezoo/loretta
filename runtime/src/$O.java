/**
 * $O - PyObject base class for all Python values in Loretta.
 * 
 * All Python objects extend this class. It provides the core protocol
 * methods that Python objects support.
 */
public class $O {
    
    /**
     * Convert to boolean for truthiness testing.
     * Default: objects are truthy unless overridden.
     */
    public boolean __bool__() {
        return true;
    }
    
    /**
     * Get string representation (repr).
     */
    public $S __repr__() {
        return $S.of("<object>");
    }
    
    /**
     * Get string representation (str).
     * Default implementation calls __repr__.
     */
    public $S __str__() {
        return __repr__();
    }
    
    /**
     * Hash value for use in dicts/sets.
     */
    public $I __hash__() {
        return $I.of(System.identityHashCode(this));
    }
    
    /**
     * Equality comparison.
     */
    public $O __eq__($O other) {
        return $B.of(this == other);
    }
    
    /**
     * Inequality comparison.
     */
    public $O __ne__($O other) {
        return $B.of(this != other);
    }
    
    /**
     * Less than - default raises TypeError.
     */
    public $O __lt__($O other) {
        throw new $X("TypeError", "'<' not supported");
    }
    
    /**
     * Less than or equal - default raises TypeError.
     */
    public $O __le__($O other) {
        throw new $X("TypeError", "'<=' not supported");
    }
    
    /**
     * Greater than - default raises TypeError.
     */
    public $O __gt__($O other) {
        throw new $X("TypeError", "'>' not supported");
    }
    
    /**
     * Greater than or equal - default raises TypeError.
     */
    public $O __ge__($O other) {
        throw new $X("TypeError", "'>=' not supported");
    }
    
    // Arithmetic operations - default implementations raise TypeError
    
    public $O __add__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for +");
    }
    
    public $O __sub__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for -");
    }
    
    public $O __mul__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for *");
    }
    
    public $O __truediv__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for /");
    }
    
    public $O __floordiv__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for //");
    }
    
    public $O __mod__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for %");
    }
    
    public $O __pow__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for **");
    }
    
    public $O __neg__() {
        throw new $X("TypeError", "bad operand type for unary -");
    }
    
    public $O __pos__() {
        throw new $X("TypeError", "bad operand type for unary +");
    }
    
    public $O __invert__() {
        throw new $X("TypeError", "bad operand type for unary ~");
    }
    
    // Bitwise operations
    
    public $O __and__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for &");
    }
    
    public $O __or__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for |");
    }
    
    public $O __xor__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for ^");
    }
    
    public $O __lshift__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for <<");
    }
    
    public $O __rshift__($O other) {
        throw new $X("TypeError", "unsupported operand type(s) for >>");
    }
    
    // Container operations
    
    public $I __len__() {
        throw new $X("TypeError", "object has no len()");
    }
    
    public $O __getitem__($O key) {
        throw new $X("TypeError", "object is not subscriptable");
    }
    
    public void __setitem__($O key, $O value) {
        throw new $X("TypeError", "object does not support item assignment");
    }
    
    public void __delitem__($O key) {
        throw new $X("TypeError", "object does not support item deletion");
    }
    
    public $B __contains__($O item) {
        throw new $X("TypeError", "argument of type is not iterable");
    }
    
    // Iteration
    
    public $O __iter__() {
        throw new $X("TypeError", "object is not iterable");
    }
    
    public $O __next__() {
        throw new $X("TypeError", "object is not an iterator");
    }
    
    // Attribute access
    
    public $O __getattr__(String name) {
        throw new $X("AttributeError", "object has no attribute '" + name + "'");
    }
    
    public void __setattr__(String name, $O value) {
        throw new $X("AttributeError", "object has no attribute '" + name + "'");
    }
    
    // Call protocol
    
    public $O __call__($O... args) {
        throw new $X("TypeError", "object is not callable");
    }
    
    /**
     * Java toString for debugging.
     */
    @Override
    public String toString() {
        return __str__().value;
    }
}
