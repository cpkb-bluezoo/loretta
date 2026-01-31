/**
 * $T - PyTuple, immutable sequence.
 */
public final class $T extends $O {
    
    /** Empty tuple singleton. */
    public static final $T EMPTY = new $T(new $O[0]);
    
    public final $O[] items;
    
    private $T($O[] items) {
        this.items = items;
    }
    
    public static $T of($O... items) {
        if (items.length == 0) return EMPTY;
        return new $T(items.clone());
    }
    
    @Override
    public boolean __bool__() {
        return items.length > 0;
    }
    
    @Override
    public $S __repr__() {
        StringBuilder sb = new StringBuilder();
        sb.append('(');
        for (int i = 0; i < items.length; i++) {
            if (i > 0) sb.append(", ");
            sb.append(items[i].__repr__().value);
        }
        if (items.length == 1) sb.append(',');  // (x,) for single-element tuple
        sb.append(')');
        return $S.of(sb.toString());
    }
    
    @Override
    public $I __hash__() {
        int hash = 1;
        for ($O item : items) {
            hash = 31 * hash + item.__hash__().hashCode();
        }
        return $I.of(hash);
    }
    
    @Override
    public $I __len__() {
        return $I.of(items.length);
    }
    
    @Override
    public $O __eq__($O other) {
        if (!(other instanceof $T)) return $B.FALSE;
        $T o = ($T) other;
        if (items.length != o.items.length) return $B.FALSE;
        for (int i = 0; i < items.length; i++) {
            $O eq = items[i].__eq__(o.items[i]);
            if (eq instanceof $B && !((($B)eq).boolValue)) {
                return $B.FALSE;
            }
        }
        return $B.TRUE;
    }
    
    @Override
    public $O __lt__($O other) {
        if (!(other instanceof $T)) {
            throw new $X("TypeError", "'<' not supported between tuple and " + other.getClass().getSimpleName());
        }
        $T o = ($T) other;
        int len = Math.min(items.length, o.items.length);
        for (int i = 0; i < len; i++) {
            $O lt = items[i].__lt__(o.items[i]);
            $O eq = items[i].__eq__(o.items[i]);
            if (lt instanceof $B && (($B)lt).boolValue) return $B.TRUE;
            if (eq instanceof $B && !(($B)eq).boolValue) return $B.FALSE;
        }
        return $B.of(items.length < o.items.length);
    }
    
    @Override
    public $O __getitem__($O key) {
        if (key instanceof $I) {
            $I i = ($I) key;
            if (i.big != null) throw new $X("IndexError", "tuple index out of range");
            int idx = (int) i.value;
            if (idx < 0) idx += items.length;
            if (idx < 0 || idx >= items.length) {
                throw new $X("IndexError", "tuple index out of range");
            }
            return items[idx];
        }
        if (key instanceof $SL) {
            $SL sl = ($SL) key;
            $T indices = sl.indices($I.of(items.length));
            int start = (int)(($I)indices.items[0]).value;
            int stop = (int)(($I)indices.items[1]).value;
            int step = (int)(($I)indices.items[2]).value;
            
            java.util.List<$O> result = new java.util.ArrayList<>();
            if (step > 0) {
                for (int i = start; i < stop; i += step) {
                    result.add(items[i]);
                }
            } else {
                for (int i = start; i > stop; i += step) {
                    result.add(items[i]);
                }
            }
            return of(result.toArray(new $O[0]));
        }
        throw new $X("TypeError", "tuple indices must be integers or slices");
    }
    
    @Override
    public $B __contains__($O item) {
        for ($O x : items) {
            $O eq = x.__eq__(item);
            if (eq instanceof $B && (($B)eq).boolValue) {
                return $B.TRUE;
            }
        }
        return $B.FALSE;
    }
    
    @Override
    public $O __add__($O other) {
        if (other instanceof $T) {
            $T o = ($T) other;
            $O[] combined = new $O[items.length + o.items.length];
            System.arraycopy(items, 0, combined, 0, items.length);
            System.arraycopy(o.items, 0, combined, items.length, o.items.length);
            return of(combined);
        }
        throw new $X("TypeError", "can only concatenate tuple to tuple");
    }
    
    @Override
    public $O __mul__($O other) {
        if (other instanceof $I) {
            $I i = ($I) other;
            if (i.big != null) throw new $X("OverflowError", "repeated tuple is too long");
            int n = (int) i.value;
            if (n <= 0) return EMPTY;
            $O[] result = new $O[items.length * n];
            for (int j = 0; j < n; j++) {
                System.arraycopy(items, 0, result, j * items.length, items.length);
            }
            return of(result);
        }
        throw new $X("TypeError", "can't multiply sequence by non-int");
    }
    
    @Override
    public $O __iter__() {
        return new $TI(this);
    }
    
    // Tuple methods
    
    public $I index($O item) {
        for (int i = 0; i < items.length; i++) {
            $O eq = items[i].__eq__(item);
            if (eq instanceof $B && (($B)eq).boolValue) {
                return $I.of(i);
            }
        }
        throw new $X("ValueError", "tuple.index(x): x not in tuple");
    }
    
    public $I count($O item) {
        int count = 0;
        for ($O x : items) {
            $O eq = x.__eq__(item);
            if (eq instanceof $B && (($B)eq).boolValue) {
                count++;
            }
        }
        return $I.of(count);
    }
}

/**
 * Tuple iterator.
 */
class $TI extends $O {
    private final $T tuple;
    private int index;
    
    $TI($T tuple) {
        this.tuple = tuple;
        this.index = 0;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (index >= tuple.items.length) {
            throw new $X.StopIteration();
        }
        return tuple.items[index++];
    }
}
