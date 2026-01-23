import java.util.LinkedHashSet;
import java.util.Set;

/**
 * $ST - PySet for mutable sets.
 */
public final class $ST extends $O {
    
    public final Set<$O> items;
    
    public $ST() {
        this.items = new LinkedHashSet<>();
    }
    
    public static $ST of($O... elements) {
        $ST set = new $ST();
        for ($O e : elements) {
            set.add(e);
        }
        return set;
    }
    
    // Find matching element by __eq__
    private $O findKey($O item) {
        for ($O k : items) {
            $O eq = k.__eq__(item);
            if (eq instanceof $B && (($B)eq).boolValue) {
                return k;
            }
        }
        return null;
    }
    
    @Override
    public boolean __bool__() {
        return !items.isEmpty();
    }
    
    @Override
    public $S __repr__() {
        if (items.isEmpty()) {
            return $S.of("set()");
        }
        StringBuilder sb = new StringBuilder();
        sb.append('{');
        boolean first = true;
        for ($O item : items) {
            if (!first) sb.append(", ");
            first = false;
            sb.append(item.__repr__().value);
        }
        sb.append('}');
        return $S.of(sb.toString());
    }
    
    @Override
    public $I __len__() {
        return $I.of(items.size());
    }
    
    @Override
    public $O __eq__($O other) {
        if (!(other instanceof $ST)) return $B.FALSE;
        $ST o = ($ST) other;
        if (items.size() != o.items.size()) return $B.FALSE;
        for ($O item : items) {
            if (o.findKey(item) == null) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    @Override
    public $B __contains__($O item) {
        return $B.of(findKey(item) != null);
    }
    
    @Override
    public $O __iter__() {
        return new $STI(this);
    }
    
    // Set operations
    
    @Override
    public $O __sub__($O other) {
        if (other instanceof $ST) {
            $ST result = new $ST();
            for ($O item : items) {
                if ((($ST)other).findKey(item) == null) {
                    result.items.add(item);
                }
            }
            return result;
        }
        throw new $X("TypeError", "unsupported operand type(s) for -");
    }
    
    @Override
    public $O __and__($O other) {
        if (other instanceof $ST) {
            $ST result = new $ST();
            for ($O item : items) {
                if ((($ST)other).findKey(item) != null) {
                    result.items.add(item);
                }
            }
            return result;
        }
        throw new $X("TypeError", "unsupported operand type(s) for &");
    }
    
    @Override
    public $O __or__($O other) {
        if (other instanceof $ST) {
            $ST result = new $ST();
            result.items.addAll(items);
            for ($O item : (($ST)other).items) {
                if (result.findKey(item) == null) {
                    result.items.add(item);
                }
            }
            return result;
        }
        throw new $X("TypeError", "unsupported operand type(s) for |");
    }
    
    @Override
    public $O __xor__($O other) {
        if (other instanceof $ST) {
            $ST o = ($ST) other;
            $ST result = new $ST();
            for ($O item : items) {
                if (o.findKey(item) == null) {
                    result.items.add(item);
                }
            }
            for ($O item : o.items) {
                if (findKey(item) == null) {
                    result.items.add(item);
                }
            }
            return result;
        }
        throw new $X("TypeError", "unsupported operand type(s) for ^");
    }
    
    // Set methods
    
    public void add($O item) {
        $O existing = findKey(item);
        if (existing == null) {
            items.add(item);
        }
    }
    
    public void remove($O item) {
        $O existing = findKey(item);
        if (existing == null) {
            throw new $X("KeyError", item.__repr__().value);
        }
        items.remove(existing);
    }
    
    public void discard($O item) {
        $O existing = findKey(item);
        if (existing != null) {
            items.remove(existing);
        }
    }
    
    public $O pop() {
        if (items.isEmpty()) {
            throw new $X("KeyError", "pop from an empty set");
        }
        $O first = items.iterator().next();
        items.remove(first);
        return first;
    }
    
    public void clear() {
        items.clear();
    }
    
    public void update($O other) {
        if (other instanceof $ST) {
            for ($O item : (($ST)other).items) {
                add(item);
            }
        } else {
            $O iter = other.__iter__();
            while (true) {
                try {
                    add(iter.__next__());
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
        }
    }
    
    public $ST copy() {
        $ST result = new $ST();
        result.items.addAll(items);
        return result;
    }
    
    public $ST union($O other) {
        return ($ST) __or__(other instanceof $ST ? other : $ST.fromIterable(other));
    }
    
    public $ST intersection($O other) {
        return ($ST) __and__(other instanceof $ST ? other : $ST.fromIterable(other));
    }
    
    public $ST difference($O other) {
        return ($ST) __sub__(other instanceof $ST ? other : $ST.fromIterable(other));
    }
    
    public $ST symmetric_difference($O other) {
        return ($ST) __xor__(other instanceof $ST ? other : $ST.fromIterable(other));
    }
    
    public $B issubset($O other) {
        $ST o = other instanceof $ST ? ($ST)other : $ST.fromIterable(other);
        for ($O item : items) {
            if (o.findKey(item) == null) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $B issuperset($O other) {
        $ST o = other instanceof $ST ? ($ST)other : $ST.fromIterable(other);
        for ($O item : o.items) {
            if (findKey(item) == null) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $B isdisjoint($O other) {
        $ST o = other instanceof $ST ? ($ST)other : $ST.fromIterable(other);
        for ($O item : items) {
            if (o.findKey(item) != null) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public static $ST fromIterable($O iterable) {
        $ST result = new $ST();
        $O iter = iterable.__iter__();
        while (true) {
            try {
                result.add(iter.__next__());
            } catch ($X e) {
                if (e.isStopIteration()) break;
                throw e;
            }
        }
        return result;
    }
}

/**
 * Set iterator.
 */
class $STI extends $O {
    private final java.util.Iterator<$O> iter;
    
    $STI($ST set) {
        this.iter = set.items.iterator();
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (!iter.hasNext()) {
            throw new $X("StopIteration", "");
        }
        return iter.next();
    }
}
