import java.math.BigInteger;

/**
 * $I - PyInt for arbitrary precision integers.
 * 
 * Uses long for small values, BigInteger for large ones.
 * Not final because $B extends it (bool is a subclass of int in Python).
 */
public class $I extends $O {
    
    // Common small integers are cached
    private static final $I[] CACHE = new $I[256 + 128];
    static {
        for (int i = 0; i < CACHE.length; i++) {
            CACHE[i] = new $I(i - 128);
        }
    }
    
    public final long value;
    public final BigInteger big;  // null if fits in long
    
    $I(long value) {
        this.value = value;
        this.big = null;
    }
    
    private $I(BigInteger big) {
        this.big = big;
        this.value = 0;  // unused when big != null
    }
    
    /**
     * Factory method for creating integers.
     */
    public static $I of(long value) {
        if (value >= -128 && value < 256) {
            return CACHE[(int)(value + 128)];
        }
        return new $I(value);
    }
    
    public static $I of(BigInteger big) {
        // Try to fit in long
        if (big.bitLength() < 64) {
            return of(big.longValue());
        }
        return new $I(big);
    }
    
    public static $I of(String s) {
        return of(new BigInteger(s));
    }
    
    private boolean isBig() {
        return big != null;
    }
    
    public BigInteger toBigInteger() {
        return isBig() ? big : BigInteger.valueOf(value);
    }
    
    @Override
    public boolean __bool__() {
        return isBig() ? !big.equals(BigInteger.ZERO) : value != 0;
    }
    
    @Override
    public $S __repr__() {
        return $S.of(isBig() ? big.toString() : String.valueOf(value));
    }
    
    @Override
    public $I __hash__() {
        return this;
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return $B.of(toBigInteger().equals(o.toBigInteger()));
            }
            return $B.of(value == o.value);
        }
        if (other instanceof $F) {
            return $B.of((double)(isBig() ? big.doubleValue() : value) == (($F)other).value);
        }
        return $B.FALSE;
    }
    
    @Override
    public $O __lt__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return $B.of(toBigInteger().compareTo(o.toBigInteger()) < 0);
            }
            return $B.of(value < o.value);
        }
        if (other instanceof $F) {
            return $B.of((isBig() ? big.doubleValue() : value) < (($F)other).value);
        }
        throw new $X("TypeError", "'<' not supported between instances");
    }
    
    @Override
    public $O __le__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return $B.of(toBigInteger().compareTo(o.toBigInteger()) <= 0);
            }
            return $B.of(value <= o.value);
        }
        if (other instanceof $F) {
            return $B.of((isBig() ? big.doubleValue() : value) <= (($F)other).value);
        }
        throw new $X("TypeError", "'<=' not supported between instances");
    }
    
    @Override
    public $O __gt__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return $B.of(toBigInteger().compareTo(o.toBigInteger()) > 0);
            }
            return $B.of(value > o.value);
        }
        if (other instanceof $F) {
            return $B.of((isBig() ? big.doubleValue() : value) > (($F)other).value);
        }
        throw new $X("TypeError", "'>' not supported between instances");
    }
    
    @Override
    public $O __ge__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return $B.of(toBigInteger().compareTo(o.toBigInteger()) >= 0);
            }
            return $B.of(value >= o.value);
        }
        if (other instanceof $F) {
            return $B.of((isBig() ? big.doubleValue() : value) >= (($F)other).value);
        }
        throw new $X("TypeError", "'>=' not supported between instances");
    }
    
    @Override
    public $O __add__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return of(toBigInteger().add(o.toBigInteger()));
            }
            // Check for overflow
            long r = value + o.value;
            if (((value ^ r) & (o.value ^ r)) < 0) {
                return of(toBigInteger().add(o.toBigInteger()));
            }
            return of(r);
        }
        if (other instanceof $F) {
            return $F.of((isBig() ? big.doubleValue() : value) + (($F)other).value);
        }
        throw new $X("TypeError", "unsupported operand type(s) for +");
    }
    
    @Override
    public $O __sub__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return of(toBigInteger().subtract(o.toBigInteger()));
            }
            long r = value - o.value;
            if (((value ^ o.value) & (value ^ r)) < 0) {
                return of(toBigInteger().subtract(o.toBigInteger()));
            }
            return of(r);
        }
        if (other instanceof $F) {
            return $F.of((isBig() ? big.doubleValue() : value) - (($F)other).value);
        }
        throw new $X("TypeError", "unsupported operand type(s) for -");
    }
    
    @Override
    public $O __mul__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return of(toBigInteger().multiply(o.toBigInteger()));
            }
            // Check for overflow
            long r = value * o.value;
            if (value != 0 && r / value != o.value) {
                return of(toBigInteger().multiply(o.toBigInteger()));
            }
            return of(r);
        }
        if (other instanceof $F) {
            return $F.of((isBig() ? big.doubleValue() : value) * (($F)other).value);
        }
        if (other instanceof $S) {
            return (($S)other).__mul__(this);
        }
        throw new $X("TypeError", "unsupported operand type(s) for *");
    }
    
    @Override
    public $O __truediv__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            double d = o.isBig() ? o.big.doubleValue() : o.value;
            if (d == 0) throw new $X("ZeroDivisionError", "division by zero");
            return $F.of((isBig() ? big.doubleValue() : value) / d);
        }
        if (other instanceof $F) {
            if ((($F)other).value == 0) throw new $X("ZeroDivisionError", "division by zero");
            return $F.of((isBig() ? big.doubleValue() : value) / (($F)other).value);
        }
        throw new $X("TypeError", "unsupported operand type(s) for /");
    }
    
    @Override
    public $O __floordiv__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                BigInteger ob = o.toBigInteger();
                if (ob.equals(BigInteger.ZERO)) throw new $X("ZeroDivisionError", "division by zero");
                return of(toBigInteger().divide(ob));
            }
            if (o.value == 0) throw new $X("ZeroDivisionError", "division by zero");
            return of(Math.floorDiv(value, o.value));
        }
        if (other instanceof $F) {
            if ((($F)other).value == 0) throw new $X("ZeroDivisionError", "division by zero");
            return $F.of(Math.floor((isBig() ? big.doubleValue() : value) / (($F)other).value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for //");
    }
    
    @Override
    public $O __mod__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                BigInteger ob = o.toBigInteger();
                if (ob.equals(BigInteger.ZERO)) throw new $X("ZeroDivisionError", "modulo by zero");
                return of(toBigInteger().mod(ob.abs()));
            }
            if (o.value == 0) throw new $X("ZeroDivisionError", "modulo by zero");
            return of(Math.floorMod(value, o.value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for %");
    }
    
    @Override
    public $O __pow__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (o.isBig() || (o.value > Integer.MAX_VALUE)) {
                throw new $X("ValueError", "exponent too large");
            }
            int exp = (int) o.value;
            if (exp < 0) {
                return $F.of(Math.pow(isBig() ? big.doubleValue() : value, exp));
            }
            return of(toBigInteger().pow(exp));
        }
        if (other instanceof $F) {
            return $F.of(Math.pow(isBig() ? big.doubleValue() : value, (($F)other).value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for **");
    }
    
    @Override
    public $O __neg__() {
        if (isBig()) {
            return of(big.negate());
        }
        if (value == Long.MIN_VALUE) {
            return of(BigInteger.valueOf(value).negate());
        }
        return of(-value);
    }
    
    @Override
    public $O __pos__() {
        return this;
    }
    
    @Override
    public $O __invert__() {
        if (isBig()) {
            return of(big.not());
        }
        return of(~value);
    }
    
    @Override
    public $O __and__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return of(toBigInteger().and(o.toBigInteger()));
            }
            return of(value & o.value);
        }
        throw new $X("TypeError", "unsupported operand type(s) for &");
    }
    
    @Override
    public $O __or__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return of(toBigInteger().or(o.toBigInteger()));
            }
            return of(value | o.value);
        }
        throw new $X("TypeError", "unsupported operand type(s) for |");
    }
    
    @Override
    public $O __xor__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (isBig() || o.isBig()) {
                return of(toBigInteger().xor(o.toBigInteger()));
            }
            return of(value ^ o.value);
        }
        throw new $X("TypeError", "unsupported operand type(s) for ^");
    }
    
    @Override
    public $O __lshift__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (o.isBig() || o.value > Integer.MAX_VALUE || o.value < 0) {
                throw new $X("ValueError", "shift count out of range");
            }
            return of(toBigInteger().shiftLeft((int)o.value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for <<");
    }
    
    @Override
    public $O __rshift__($O other) {
        if (other instanceof $I) {
            $I o = ($I) other;
            if (o.isBig() || o.value > Integer.MAX_VALUE || o.value < 0) {
                throw new $X("ValueError", "shift count out of range");
            }
            return of(toBigInteger().shiftRight((int)o.value));
        }
        throw new $X("TypeError", "unsupported operand type(s) for >>");
    }
}
