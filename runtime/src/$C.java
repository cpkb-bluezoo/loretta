/**
 * $C - PyComplex for complex numbers.
 */
public final class $C extends $O {
    
    public final double real;
    public final double imag;
    
    public $C(double real, double imag) {
        this.real = real;
        this.imag = imag;
    }
    
    public static $C of(double real, double imag) {
        return new $C(real, imag);
    }
    
    public static $C of(double real) {
        return new $C(real, 0.0);
    }
    
    @Override
    public boolean __bool__() {
        return real != 0.0 || imag != 0.0;
    }
    
    @Override
    public $S __repr__() {
        if (real == 0.0) {
            return $S.of(imag + "j");
        }
        if (imag >= 0) {
            return $S.of("(" + real + "+" + imag + "j)");
        }
        return $S.of("(" + real + imag + "j)");
    }
    
    @Override
    public $I __hash__() {
        return $I.of(Double.hashCode(real) ^ Double.hashCode(imag));
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $C) {
            $C o = ($C) other;
            return $B.of(real == o.real && imag == o.imag);
        }
        if (other instanceof $I || other instanceof $F) {
            double r = other instanceof $I ? 
                ((($I)other).big != null ? (($I)other).big.doubleValue() : (($I)other).value) :
                (($F)other).value;
            return $B.of(real == r && imag == 0.0);
        }
        return $B.FALSE;
    }
    
    @Override
    public $O __add__($O other) {
        if (other instanceof $C) {
            $C o = ($C) other;
            return of(real + o.real, imag + o.imag);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            double r = i.big != null ? i.big.doubleValue() : i.value;
            return of(real + r, imag);
        }
        if (other instanceof $F) {
            return of(real + (($F)other).value, imag);
        }
        throw new $X("TypeError", "unsupported operand type(s) for +");
    }
    
    @Override
    public $O __sub__($O other) {
        if (other instanceof $C) {
            $C o = ($C) other;
            return of(real - o.real, imag - o.imag);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            double r = i.big != null ? i.big.doubleValue() : i.value;
            return of(real - r, imag);
        }
        if (other instanceof $F) {
            return of(real - (($F)other).value, imag);
        }
        throw new $X("TypeError", "unsupported operand type(s) for -");
    }
    
    @Override
    public $O __mul__($O other) {
        if (other instanceof $C) {
            $C o = ($C) other;
            // (a+bi)(c+di) = (ac-bd) + (ad+bc)i
            return of(real * o.real - imag * o.imag, real * o.imag + imag * o.real);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            double r = i.big != null ? i.big.doubleValue() : i.value;
            return of(real * r, imag * r);
        }
        if (other instanceof $F) {
            double r = (($F)other).value;
            return of(real * r, imag * r);
        }
        throw new $X("TypeError", "unsupported operand type(s) for *");
    }
    
    @Override
    public $O __truediv__($O other) {
        if (other instanceof $C) {
            $C o = ($C) other;
            // (a+bi)/(c+di) = ((ac+bd) + (bc-ad)i) / (c^2+d^2)
            double denom = o.real * o.real + o.imag * o.imag;
            if (denom == 0) throw new $X("ZeroDivisionError", "complex division by zero");
            return of((real * o.real + imag * o.imag) / denom,
                      (imag * o.real - real * o.imag) / denom);
        }
        if (other instanceof $I) {
            $I i = ($I) other;
            double r = i.big != null ? i.big.doubleValue() : i.value;
            if (r == 0) throw new $X("ZeroDivisionError", "complex division by zero");
            return of(real / r, imag / r);
        }
        if (other instanceof $F) {
            double r = (($F)other).value;
            if (r == 0) throw new $X("ZeroDivisionError", "complex division by zero");
            return of(real / r, imag / r);
        }
        throw new $X("TypeError", "unsupported operand type(s) for /");
    }
    
    @Override
    public $O __neg__() {
        return of(-real, -imag);
    }
    
    @Override
    public $O __pos__() {
        return this;
    }
    
    @Override
    public $O __pow__($O other) {
        if (other instanceof $I) {
            $I i = ($I) other;
            if (i.big != null || i.value > Integer.MAX_VALUE) {
                throw new $X("ValueError", "exponent too large");
            }
            int n = (int) i.value;
            if (n == 0) return of(1.0, 0.0);
            if (n == 1) return this;
            if (n < 0) {
                // 1 / z^(-n)
                $C inv = ($C) of(1.0, 0.0).__truediv__(this);
                return inv.__pow__($I.of(-n));
            }
            // Use repeated squaring
            $C result = of(1.0, 0.0);
            $C base = this;
            while (n > 0) {
                if ((n & 1) == 1) {
                    result = ($C) result.__mul__(base);
                }
                base = ($C) base.__mul__(base);
                n >>= 1;
            }
            return result;
        }
        // For float exponents, use polar form
        if (other instanceof $F) {
            double exp = (($F)other).value;
            double r = Math.sqrt(real * real + imag * imag);
            double theta = Math.atan2(imag, real);
            double newR = Math.pow(r, exp);
            double newTheta = theta * exp;
            return of(newR * Math.cos(newTheta), newR * Math.sin(newTheta));
        }
        throw new $X("TypeError", "unsupported operand type(s) for **");
    }
    
    // Complex-specific methods
    
    public $C conjugate() {
        return of(real, -imag);
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "real": return $F.of(real);
            case "imag": return $F.of(imag);
            default: return super.__getattr__(name);
        }
    }
}
