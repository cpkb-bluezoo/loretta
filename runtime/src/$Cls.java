import java.util.ArrayList;
import java.util.List;

/**
 * $Cls - Python class object.
 * 
 * Represents a Python class at runtime. Stores:
 * - Class name
 * - Base classes (for inheritance)
 * - Methods and class attributes as a dictionary
 * - MRO (Method Resolution Order) computed using C3 linearization
 * 
 * Calling a class creates a new instance.
 */
public final class $Cls extends $O {
    
    public final String name;
    public final $Cls[] bases;
    public final $D attrs;  // Methods and class attributes
    public final $Cls[] mro;  // Method Resolution Order (C3 linearization)
    
    /** For built-in types: the corresponding Java class */
    public Class<?> javaClass;
    
    /** Slot names for __slots__ support (null means use dict) */
    public String[] slots;
    
    /**
     * Create a new class.
     */
    public $Cls(String name, $Cls[] bases) {
        this.name = name;
        this.bases = bases != null ? bases : new $Cls[0];
        this.attrs = new $D();
        this.javaClass = null;
        this.slots = null;  // No slots by default
        this.mro = computeMRO();
    }
    
    /**
     * Create a class with no bases.
     */
    public $Cls(String name) {
        this(name, null);
    }
    
    /**
     * Compute the Method Resolution Order using C3 linearization.
     * 
     * C3 linearization ensures:
     * 1. Local precedence order (children before parents, left-to-right)
     * 2. Monotonicity (if A precedes B in one class, it does in subclasses)
     * 
     * Algorithm: L[C] = C + merge(L[B1], L[B2], ..., L[Bn], [B1, B2, ..., Bn])
     * where merge repeatedly selects the first head that doesn't appear in any tail.
     */
    private $Cls[] computeMRO() {
        List<$Cls> result = new ArrayList<>();
        result.add(this);
        
        if (bases.length == 0) {
            // No bases - MRO is just this class
            return result.toArray(new $Cls[0]);
        }
        
        // Build the list of lists to merge:
        // - MRO of each base class
        // - The list of bases themselves
        List<List<$Cls>> toMerge = new ArrayList<>();
        for ($Cls base : bases) {
            List<$Cls> baseMRO = new ArrayList<>();
            for ($Cls c : base.mro) {
                baseMRO.add(c);
            }
            toMerge.add(baseMRO);
        }
        // Add the list of direct bases
        List<$Cls> basesList = new ArrayList<>();
        for ($Cls base : bases) {
            basesList.add(base);
        }
        toMerge.add(basesList);
        
        // C3 merge algorithm
        while (true) {
            // Remove empty lists
            toMerge.removeIf(List::isEmpty);
            
            if (toMerge.isEmpty()) {
                break;
            }
            
            // Find a good head: first element of some list that doesn't
            // appear in the tail of any other list
            $Cls candidate = null;
            for (List<$Cls> lst : toMerge) {
                $Cls head = lst.get(0);
                boolean inTail = false;
                
                // Check if head appears in tail of any list
                for (List<$Cls> other : toMerge) {
                    for (int i = 1; i < other.size(); i++) {
                        if (other.get(i) == head) {
                            inTail = true;
                            break;
                        }
                    }
                    if (inTail) break;
                }
                
                if (!inTail) {
                    candidate = head;
                    break;
                }
            }
            
            if (candidate == null) {
                // No valid candidate - inconsistent MRO
                throw new $X("TypeError", 
                    "Cannot create a consistent method resolution order (MRO) for bases");
            }
            
            // Add candidate to result
            result.add(candidate);
            
            // Remove candidate from all lists
            for (List<$Cls> lst : toMerge) {
                lst.remove(candidate);
            }
        }
        
        return result.toArray(new $Cls[0]);
    }
    
    /**
     * Set __slots__ for this class.
     * @param slotNames Array of allowed attribute names
     */
    public void setSlots(String[] slotNames) {
        this.slots = slotNames;
    }
    
    /**
     * Check if this class uses __slots__.
     */
    public boolean hasSlots() {
        return slots != null;
    }
    
    /**
     * Get the index of a slot by name, or -1 if not found.
     */
    public int getSlotIndex(String name) {
        if (slots == null) return -1;
        for (int i = 0; i < slots.length; i++) {
            if (slots[i].equals(name)) return i;
        }
        return -1;
    }
    
    /**
     * Add a method or class attribute.
     */
    public void setAttr(String name, $O value) {
        attrs.__setitem__($S.of(name), value);
    }
    
    /**
     * Get a raw attribute without descriptor processing.
     * Uses MRO (Method Resolution Order) for lookup.
     */
    public $O getRawAttr(String name) {
        $S key = $S.of(name);
        
        // Walk the MRO to find the attribute
        for ($Cls cls : mro) {
            if (cls.attrs.__contains__(key).__bool__()) {
                return cls.attrs.__getitem__(key);
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
     * Uses MRO (Method Resolution Order) for lookup.
     */
    public boolean hasAttr(String name) {
        $S key = $S.of(name);
        // Walk the MRO to find the attribute
        for ($Cls cls : mro) {
            if (cls.attrs.__contains__(key).__bool__()) {
                return true;
            }
        }
        return false;
    }
    
    @Override
    public $O __call__($O... args) {
        // Special handling for exception classes
        if (javaClass != null && $X.class.isAssignableFrom(javaClass)) {
            return createException(args);
        }
        
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
    
    /**
     * Create an exception instance from this exception class.
     */
    private $O createException($O[] args) {
        String message = "";
        if (args.length > 0 && args[0] != null) {
            message = args[0].__str__().value;
        }
        
        // Use $X.create to get the right exception type
        $X exc = $X.create(name, message);
        return exc.asPyObject();
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
