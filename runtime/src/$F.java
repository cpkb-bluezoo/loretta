/**
 * $F - PyFloat for double-precision floating point numbers.
 */
public final class $F extends $O {
    
    public final double value;
    
    private $F(double value) {
        this.value = value;
    }
    
    public static $F of(double value) {
        return new $F(value);
    }
    
    public static $F of(String s) {
        return of(Double.parseDouble(s));
    }
    
    @Override
    public boolean __bool__() {
        return value != 0.0;
    }
    
    @Override
    public $S __repr__() {
        if (Double.isInfinite(value)) {
            return $S.of(value > 0 ? "inf" : "-inf");
        }
        if (Double.isNaN(value)) {
            return $S.of("nan");
        }
        String s = String.valueOf(value);
        // Python shows integers as X.0
        if (s.indexOf('.') < 0 && s.indexOf('e') < 0 && s.indexOf('E') < 0) {
            s = s + ".0";
        }
        return $S.of(s);
    }
    
    @Override
    public $I __hash__() {
        return $I.of(Double.hashCode(value));
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $F) {
            return $B.of(value == (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return $B.of(value == (i.big != null ? i.big.doubleValue() : i.value));
        }
        return $B.FALSE;
    }
    
    @Override
    public $O __lt__($O other) {
        if (other instanceof $F) {
            return $B.of(value < (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return $B.of(value < (i.big != null ? i.big.doubleValue() : i.value));
        }
        throw new $X("TypeError", "'<' not supported between instances");
    }
    
    @Override
    public $O __le__($O other) {
        if (other instanceof $F) {
            return $B.of(value <= (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return $B.of(value <= (i.big != null ? i.big.doubleValue() : i.value));
        }
        throw new $X("TypeError", "'<=' not supported between instances");
    }
    
    @Override
    public $O __gt__($O other) {
        if (other instanceof $F) {
            return $B.of(value > (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return $B.of(value > (i.big != null ? i.big.doubleValue() : i.value));
        }
        throw new $X("TypeError", "'>' not supported between instances");
    }
    
    @Override
    public $O __ge__($O other) {
        if (other instanceof $F) {
            return $B.of(value >= (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return $B.of(value >= (i.big != null ? i.big.doubleValue() : i.value));
        }
        throw new $X("TypeError", "'>=' not supported between instances");
    }
    
    @Override
    public $O __add__($O other) {
        if (other instanceof $F) {
            return of(value + (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return of(value + (i.big != null ? i.big.doubleValue() : i.value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for +");
    }
    
    @Override
    public $O __sub__($O other) {
        if (other instanceof $F) {
            return of(value - (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return of(value - (i.big != null ? i.big.doubleValue() : i.value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for -");
    }
    
    @Override
    public $O __mul__($O other) {
        if (other instanceof $F) {
            return of(value * (($F)other).value);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            return of(value * (i.big != null ? i.big.doubleValue() : i.value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for *");
    }
    
    @Override
    public $O __truediv__($O other) {
        double d;
        if (other instanceof $F) {
            d = (($F)other).value;
        } else if (other instanceof $I) {
            $I i = ($I) other;
            d = i.big != null ? i.big.doubleValue() : i.value;
        } else {
            throw new $X("TypeError", "unsupported operand type(s) for /");
        }
        if (d == 0) throw new $X("ZeroDivisionError", "float division by zero");
        return of(value / d);
    }
    
    @Override
    public $O __floordiv__($O other) {
        double d;
        if (other instanceof $F) {
            d = (($F)other).value;
        } else if (other instanceof $I) {
            $I i = ($I) other;
            d = i.big != null ? i.big.doubleValue() : i.value;
        } else {
            throw new $X("TypeError", "unsupported operand type(s) for //");
        }
        if (d == 0) throw new $X("ZeroDivisionError", "float floor division by zero");
        return of(Math.floor(value / d));
    }
    
    @Override
    public $O __mod__($O other) {
        double d;
        if (other instanceof $F) {
            d = (($F)other).value;
        } else if (other instanceof $I) {
            $I i = ($I) other;
            d = i.big != null ? i.big.doubleValue() : i.value;
        } else {
            throw new $X("TypeError", "unsupported operand type(s) for %");
        }
        if (d == 0) throw new $X("ZeroDivisionError", "float modulo by zero");
        return of(value - Math.floor(value / d) * d);
    }
    
    @Override
    public $O __pow__($O other) {
        double exp;
        if (other instanceof $F) {
            exp = (($F)other).value;
        } else if (other instanceof $I) {
            $I i = ($I) other;
            exp = i.big != null ? i.big.doubleValue() : i.value;
        } else {
            throw new $X("TypeError", "unsupported operand type(s) for **");
        }
        return of(Math.pow(value, exp));
    }
    
    @Override
    public $O __neg__() {
        return of(-value);
    }
    
    @Override
    public $O __pos__() {
        return this;
    }
}
