/**
 * $Super - Python super() proxy object.
 * 
 * Provides access to methods from a parent class.
 * super() returns a proxy that delegates attribute access to the parent class.
 */
public final class $Super extends $O {
    
    private final $Cls type;      // The class to start search from
    private final $O obj;         // The instance (or class for unbound)
    private final $Cls objType;   // The type of obj
    
    /**
     * Create a super proxy.
     * @param type The class whose parent to access
     * @param obj The instance to bind methods to
     */
    public $Super($Cls type, $O obj) {
        this.type = type;
        this.obj = obj;
        
        // Determine the type of obj
        if (obj instanceof $Inst) {
            this.objType = (($Inst) obj).type;
        } else if (obj instanceof $Cls) {
            this.objType = ($Cls) obj;
        } else {
            this.objType = null;
        }
    }
    
    /**
     * Create an unbound super proxy (just the type).
     */
    public $Super($Cls type) {
        this(type, null);
    }
    
    @Override
    public $O __getattr__(String name) {
        // Search in MRO starting from parent of 'type'
        // For simple single inheritance, just look in bases
        if (type.bases.length == 0) {
            throw new $X("AttributeError", "super: no parent class");
        }
        
        // Try each base class
        for ($Cls base : type.bases) {
            if (base.hasAttr(name)) {
                $O attr = base.getAttr(name);
                
                // If it's a method and we have an instance, bind it
                if (attr instanceof $MH && obj != null) {
                    return new $BM(obj, name, ($MH) attr);
                }
                
                return attr;
            }
        }
        
        throw new $X("AttributeError", "super object has no attribute '" + name + "'");
    }
    
    @Override
    public $S __repr__() {
        if (obj != null) {
            return $S.of("<super: <class '" + type.name + "'>, <" + 
                        (objType != null ? objType.name : "?") + " object>>");
        }
        return $S.of("<super: <class '" + type.name + "'>, NULL>");
    }
    
    @Override
    public boolean __bool__() {
        return true;
    }
}
