/**
 * $Prop - property descriptor.
 * 
 * Creates a property with optional getter, setter, and deleter.
 * 
 * Usage:
 *   @property
 *   def x(self): return self._x
 *   
 *   @x.setter
 *   def x(self, value): self._x = value
 */
public final class $Prop extends $O {
    
    private $O fget;    // Getter function
    private $O fset;    // Setter function
    private $O fdel;    // Deleter function
    private String doc; // Docstring
    
    /**
     * Create a property with getter only.
     */
    public $Prop($O fget) {
        this.fget = fget;
        this.fset = null;
        this.fdel = null;
        this.doc = null;
    }
    
    /**
     * Create a property with getter and setter.
     */
    public $Prop($O fget, $O fset) {
        this.fget = fget;
        this.fset = fset;
        this.fdel = null;
        this.doc = null;
    }
    
    /**
     * Create a property with getter, setter, and deleter.
     */
    public $Prop($O fget, $O fset, $O fdel) {
        this.fget = fget;
        this.fset = fset;
        this.fdel = fdel;
        this.doc = null;
    }
    
    /**
     * Create a property with all components.
     */
    public $Prop($O fget, $O fset, $O fdel, String doc) {
        this.fget = fget;
        this.fset = fset;
        this.fdel = fdel;
        this.doc = doc;
    }
    
    /**
     * Get the value (calls fget with instance).
     */
    public $O get($O instance) {
        if (fget == null) {
            throw new $X("AttributeError", "unreadable attribute");
        }
        return fget.__call__(instance);
    }
    
    /**
     * Set the value (calls fset with instance and value).
     */
    public void set($O instance, $O value) {
        if (fset == null) {
            throw new $X("AttributeError", "can't set attribute");
        }
        fset.__call__(instance, value);
    }
    
    /**
     * Delete the value (calls fdel with instance).
     */
    public void delete($O instance) {
        if (fdel == null) {
            throw new $X("AttributeError", "can't delete attribute");
        }
        fdel.__call__(instance);
    }
    
    /**
     * Create a new property with a setter.
     * Used for @property.setter decorator.
     */
    public $Prop setter($O fset) {
        return new $Prop(this.fget, fset, this.fdel, this.doc);
    }
    
    /**
     * Create a new property with a deleter.
     * Used for @property.deleter decorator.
     */
    public $Prop deleter($O fdel) {
        return new $Prop(this.fget, this.fset, fdel, this.doc);
    }
    
    /**
     * Create a new property with a getter (for chaining).
     */
    public $Prop getter($O fget) {
        return new $Prop(fget, this.fset, this.fdel, this.doc);
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "fget": return fget != null ? fget : $N.INSTANCE;
            case "fset": return fset != null ? fset : $N.INSTANCE;
            case "fdel": return fdel != null ? fdel : $N.INSTANCE;
            case "setter": return new $O() {
                @Override
                public $O __call__($O... args) {
                    if (args.length != 1) throw new $X("TypeError", "setter takes 1 argument");
                    return setter(args[0]);
                }
            };
            case "deleter": return new $O() {
                @Override
                public $O __call__($O... args) {
                    if (args.length != 1) throw new $X("TypeError", "deleter takes 1 argument");
                    return deleter(args[0]);
                }
            };
            case "getter": return new $O() {
                @Override
                public $O __call__($O... args) {
                    if (args.length != 1) throw new $X("TypeError", "getter takes 1 argument");
                    return getter(args[0]);
                }
            };
            default:
                throw new $X("AttributeError", "property has no attribute '" + name + "'");
        }
    }
    
    @Override
    public $O __call__($O... args) {
        // When used as decorator: @property
        if (args.length == 1) {
            // property(fget)
            return new $Prop(args[0]);
        }
        throw new $X("TypeError", "property() takes 1 positional argument");
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<property object>");
    }
    
    @Override
    public boolean __bool__() {
        return true;
    }
    
    /**
     * Factory method.
     */
    public static $Prop of($O fget) {
        return new $Prop(fget);
    }
    
    public static $Prop of($O fget, $O fset) {
        return new $Prop(fget, fset);
    }
    
    public static $Prop of($O fget, $O fset, $O fdel) {
        return new $Prop(fget, fset, fdel);
    }
}
