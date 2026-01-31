/**
 * $Cls - Python class object.
 * 
 * Represents a Python class at runtime. Stores:
 * - Class name
 * - Base classes (for inheritance)
 * - Methods and class attributes as a dictionary
 * 
 * Calling a class creates a new instance.
 */
public final class $Cls extends $O {
    
    public final String name;
    public final $Cls[] bases;
    public final $D attrs;  // Methods and class attributes
    
    /**
     * Create a new class.
     */
    public $Cls(String name, $Cls[] bases) {
        this.name = name;
        this.bases = bases != null ? bases : new $Cls[0];
        this.attrs = new $D();
    }
    
    /**
     * Create a class with no bases.
     */
    public $Cls(String name) {
        this(name, null);
    }
    
    /**
     * Add a method or class attribute.
     */
    public void setAttr(String name, $O value) {
        attrs.__setitem__($S.of(name), value);
    }
    
    /**
     * Get a raw attribute without descriptor processing.
     * Used internally to check descriptor types.
     */
    public $O getRawAttr(String name) {
        $S key = $S.of(name);
        if (attrs.__contains__(key).__bool__()) {
            return attrs.__getitem__(key);
        }
        
        // Check base classes
        for ($Cls base : bases) {
            try {
                return base.getRawAttr(name);
            } catch ($X e) {
                // Continue to next base
            }
        }
        
        throw new $X("AttributeError", "type object '" + this.name + "' has no attribute '" + name + "'");
    }
    
    /**
     * Get a method or class attribute with descriptor processing.
     */
    public $O getAttr(String name) {
        $O attr = getRawAttr(name);
        
        // Handle staticmethod - return unwrapped function
        if (attr instanceof $SM) {
            return (($SM) attr).getFunc();
        }
        
        // Handle classmethod - bind to this class
        if (attr instanceof $CM) {
            return (($CM) attr).bind(this);
        }
        
        return attr;
    }
    
    /**
     * Check if class has an attribute.
     */
    public boolean hasAttr(String name) {
        $S key = $S.of(name);
        if (attrs.__contains__(key).__bool__()) {
            return true;
        }
        for ($Cls base : bases) {
            if (base.hasAttr(name)) {
                return true;
            }
        }
        return false;
    }
    
    @Override
    public $O __call__($O... args) {
        // Create new instance
        $Inst inst = new $Inst(this);
        
        // Call __init__ if present
        if (hasAttr("__init__")) {
            $O init = getAttr("__init__");
            // Prepend self to args
            $O[] initArgs = new $O[args.length + 1];
            initArgs[0] = inst;
            System.arraycopy(args, 0, initArgs, 1, args.length);
            init.__call__(initArgs);
        }
        
        return inst;
    }
    
    @Override
    public $O __getattr__(String name) {
        // Return unbound method or class attribute
        return getAttr(name);
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<class '" + name + "'>");
    }
    
    @Override
    public $S __str__() {
        return __repr__();
    }
    
    @Override
    public boolean __bool__() {
        return true;
    }
    
    /**
     * Factory for creating a class with bases.
     */
    public static $Cls of(String name, $O[] bases) {
        $Cls[] clsBases = new $Cls[bases.length];
        for (int i = 0; i < bases.length; i++) {
            if (bases[i] instanceof $Cls) {
                clsBases[i] = ($Cls) bases[i];
            } else {
                throw new $X("TypeError", "bases must be classes");
            }
        }
        return new $Cls(name, clsBases);
    }
    
    /**
     * Factory for creating a class with no bases.
     */
    public static $Cls of(String name) {
        return new $Cls(name);
    }
}
