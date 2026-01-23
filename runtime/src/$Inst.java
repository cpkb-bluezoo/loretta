/**
 * $Inst - Python instance object.
 * 
 * Represents an instance of a Python class. Stores:
 * - Reference to its class ($Cls)
 * - Instance attributes as a dictionary
 * 
 * Attribute lookup checks instance attrs first, then class methods.
 */
public final class $Inst extends $O {
    
    public final $Cls type;
    public final $D attrs;  // Instance attributes
    
    /**
     * Create a new instance of a class.
     */
    public $Inst($Cls type) {
        this.type = type;
        this.attrs = new $D();
    }
    
    @Override
    public $O __getattr__(String name) {
        $S key = $S.of(name);
        
        // First check instance attributes
        if (attrs.__contains__(key).__bool__()) {
            return attrs.__getitem__(key);
        }
        
        // Then check class for methods/class attrs
        if (type.hasAttr(name)) {
            $O classAttr = type.getAttr(name);
            
            // If it's a callable (method), bind it to this instance
            if (classAttr instanceof $MH) {
                return new $BM(this, name, ($MH) classAttr);
            }
            
            // Otherwise return as-is (class variable)
            return classAttr;
        }
        
        throw new $X("AttributeError", "'" + type.name + "' object has no attribute '" + name + "'");
    }
    
    @Override
    public void __setattr__(String name, $O value) {
        attrs.__setitem__($S.of(name), value);
    }
    
    public void __delattr__(String name) {
        $S key = $S.of(name);
        if (attrs.__contains__(key).__bool__()) {
            attrs.__delitem__(key);
        } else {
            throw new $X("AttributeError", "'" + type.name + "' object has no attribute '" + name + "'");
        }
    }
    
    @Override
    public $S __repr__() {
        // Check for custom __repr__
        if (type.hasAttr("__repr__")) {
            $O repr = type.getAttr("__repr__");
            if (repr instanceof $MH) {
                return ($S) repr.__call__(this);
            }
        }
        return $S.of("<" + type.name + " object>");
    }
    
    @Override
    public $S __str__() {
        // Check for custom __str__
        if (type.hasAttr("__str__")) {
            $O str = type.getAttr("__str__");
            if (str instanceof $MH) {
                $O result = str.__call__(this);
                if (result instanceof $S) {
                    return ($S) result;
                }
                return result.__str__();
            }
        }
        return __repr__();
    }
    
    @Override
    public boolean __bool__() {
        // Check for custom __bool__
        if (type.hasAttr("__bool__")) {
            $O boolMethod = type.getAttr("__bool__");
            if (boolMethod instanceof $MH) {
                $O result = boolMethod.__call__(this);
                return result.__bool__();
            }
        }
        // Check for __len__
        if (type.hasAttr("__len__")) {
            $O lenMethod = type.getAttr("__len__");
            if (lenMethod instanceof $MH) {
                $O result = lenMethod.__call__(this);
                if (result instanceof $I) {
                    return (($I) result).value != 0;
                }
            }
        }
        // Default: instances are truthy
        return true;
    }
    
    @Override
    public $I __len__() {
        if (type.hasAttr("__len__")) {
            $O lenMethod = type.getAttr("__len__");
            if (lenMethod instanceof $MH) {
                $O result = lenMethod.__call__(this);
                if (result instanceof $I) {
                    return ($I) result;
                }
            }
        }
        throw new $X("TypeError", "object of type '" + type.name + "' has no len()");
    }
    
    @Override
    public $O __iter__() {
        if (type.hasAttr("__iter__")) {
            $O iterMethod = type.getAttr("__iter__");
            if (iterMethod instanceof $MH) {
                return iterMethod.__call__(this);
            }
        }
        throw new $X("TypeError", "'" + type.name + "' object is not iterable");
    }
    
    @Override
    public $O __next__() {
        if (type.hasAttr("__next__")) {
            $O nextMethod = type.getAttr("__next__");
            if (nextMethod instanceof $MH) {
                return nextMethod.__call__(this);
            }
        }
        throw new $X("TypeError", "'" + type.name + "' object is not an iterator");
    }
    
    @Override
    public $O __getitem__($O key) {
        if (type.hasAttr("__getitem__")) {
            $O method = type.getAttr("__getitem__");
            if (method instanceof $MH) {
                return method.__call__(this, key);
            }
        }
        throw new $X("TypeError", "'" + type.name + "' object is not subscriptable");
    }
    
    @Override
    public void __setitem__($O key, $O value) {
        if (type.hasAttr("__setitem__")) {
            $O method = type.getAttr("__setitem__");
            if (method instanceof $MH) {
                method.__call__(this, key, value);
                return;
            }
        }
        throw new $X("TypeError", "'" + type.name + "' object does not support item assignment");
    }
    
    @Override
    public void __delitem__($O key) {
        if (type.hasAttr("__delitem__")) {
            $O method = type.getAttr("__delitem__");
            if (method instanceof $MH) {
                method.__call__(this, key);
                return;
            }
        }
        throw new $X("TypeError", "'" + type.name + "' object does not support item deletion");
    }
    
    @Override
    public $B __contains__($O item) {
        if (type.hasAttr("__contains__")) {
            $O method = type.getAttr("__contains__");
            if (method instanceof $MH) {
                $O result = method.__call__(this, item);
                return $B.of(result.__bool__());
            }
        }
        // Default: iterate and check
        return super.__contains__(item);
    }
    
    @Override
    public $O __call__($O... args) {
        if (type.hasAttr("__call__")) {
            $O method = type.getAttr("__call__");
            if (method instanceof $MH) {
                $O[] callArgs = new $O[args.length + 1];
                callArgs[0] = this;
                System.arraycopy(args, 0, callArgs, 1, args.length);
                return method.__call__(callArgs);
            }
        }
        throw new $X("TypeError", "'" + type.name + "' object is not callable");
    }
    
    // Arithmetic operations - delegate to class methods
    @Override
    public $O __add__($O other) {
        if (type.hasAttr("__add__")) {
            $O method = type.getAttr("__add__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        return super.__add__(other);
    }
    
    @Override
    public $O __sub__($O other) {
        if (type.hasAttr("__sub__")) {
            $O method = type.getAttr("__sub__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        return super.__sub__(other);
    }
    
    @Override
    public $O __mul__($O other) {
        if (type.hasAttr("__mul__")) {
            $O method = type.getAttr("__mul__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        return super.__mul__(other);
    }
    
    @Override
    public $O __eq__($O other) {
        if (type.hasAttr("__eq__")) {
            $O method = type.getAttr("__eq__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        // Default: identity comparison
        return $B.of(this == other);
    }
    
    @Override
    public $O __ne__($O other) {
        if (type.hasAttr("__ne__")) {
            $O method = type.getAttr("__ne__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        // Default: negation of __eq__
        return $B.of(!__eq__(other).__bool__());
    }
    
    @Override
    public $O __lt__($O other) {
        if (type.hasAttr("__lt__")) {
            $O method = type.getAttr("__lt__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        return super.__lt__(other);
    }
    
    @Override
    public $O __le__($O other) {
        if (type.hasAttr("__le__")) {
            $O method = type.getAttr("__le__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        return super.__le__(other);
    }
    
    @Override
    public $O __gt__($O other) {
        if (type.hasAttr("__gt__")) {
            $O method = type.getAttr("__gt__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        return super.__gt__(other);
    }
    
    @Override
    public $O __ge__($O other) {
        if (type.hasAttr("__ge__")) {
            $O method = type.getAttr("__ge__");
            if (method instanceof $MH) {
                return method.__call__(this, other);
            }
        }
        return super.__ge__(other);
    }
    
    @Override
    public $I __hash__() {
        if (type.hasAttr("__hash__")) {
            $O method = type.getAttr("__hash__");
            if (method instanceof $MH) {
                $O result = method.__call__(this);
                if (result instanceof $I) {
                    return ($I) result;
                }
            }
        }
        // Default: use identity hash
        return $I.of(System.identityHashCode(this));
    }
}
