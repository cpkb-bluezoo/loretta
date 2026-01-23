import java.util.LinkedHashSet;
import java.util.Set;

/**
 * $FS - PyFrozenSet for immutable sets.
 */
public final class $FS extends $O {
    
    /** Empty frozenset singleton. */
    public static final $FS EMPTY = new $FS();
    
    final Set<$O> items;  // Package-private for iterator access
    private int cachedHash = 0;
    
    private $FS() {
        this.items = new LinkedHashSet<>();
    }
    
    private $FS(Set<$O> items) {
        this.items = items;
    }
    
    public static $FS of($O... elements) {
        if (elements.length == 0) return EMPTY;
        Set<$O> set = new LinkedHashSet<>();
        for ($O e : elements) {
            // Check for duplicates using __eq__
            boolean found = false;
            for ($O k : set) {
                $O eq = k.__eq__(e);
                if (eq instanceof $B && (($B)eq).boolValue) {
                    found = true;
                    break;
                }
            }
            if (!found) set.add(e);
        }
        return new $FS(set);
    }
    
    public static $FS fromIterable($O iterable) {
        Set<$O> set = new LinkedHashSet<>();
        $O iter = iterable.__iter__();
        while (true) {
            try {
                $O item = iter.__next__();
                // Check for duplicates
                boolean found = false;
                for ($O k : set) {
                    $O eq = k.__eq__(item);
                    if (eq instanceof $B && (($B)eq).boolValue) {
                        found = true;
                        break;
                    }
                }
                if (!found) set.add(item);
            } catch ($X e) {
                if (e.isStopIteration()) break;
                throw e;
            }
        }
        if (set.isEmpty()) return EMPTY;
        return new $FS(set);
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
            return $S.of("frozenset()");
        }
        StringBuilder sb = new StringBuilder();
        sb.append("frozenset({");
        boolean first = true;
        for ($O item : items) {
            if (!first) sb.append(", ");
            first = false;
            sb.append(item.__repr__().value);
        }
        sb.append("})");
        return $S.of(sb.toString());
    }
    
    @Override
    public $I __hash__() {
        if (cachedHash == 0) {
            int hash = 0;
            for ($O item : items) {
                hash ^= item.__hash__().hashCode();
            }
            cachedHash = hash == 0 ? 1 : hash;
        }
        return $I.of(cachedHash);
    }
    
    @Override
    public $I __len__() {
        return $I.of(items.size());
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $FS) {
            $FS o = ($FS) other;
            if (items.size() != o.items.size()) return $B.FALSE;
            for ($O item : items) {
                if (o.findKey(item) == null) return $B.FALSE;
            }
            return $B.TRUE;
        }
        if (other instanceof $ST) {
            $ST o = ($ST) other;
            if (items.size() != o.items.size()) return $B.FALSE;
            for ($O item : items) {
                if (!o.__contains__(item).boolValue) return $B.FALSE;
            }
            return $B.TRUE;
        }
        return $B.FALSE;
    }
    
    @Override
    public $B __contains__($O item) {
        return $B.of(findKey(item) != null);
    }
    
    @Override
    public $O __iter__() {
        return new $FSI(this);
    }
    
    // Set operations (return frozenset)
    
    @Override
    public $O __sub__($O other) {
        Set<$O> result = new LinkedHashSet<>();
        Set<$O> otherSet = getOtherSet(other);
        for ($O item : items) {
            if (!containsIn(otherSet, item)) {
                result.add(item);
            }
        }
        return result.isEmpty() ? EMPTY : new $FS(result);
    }
    
    @Override
    public $O __and__($O other) {
        Set<$O> result = new LinkedHashSet<>();
        Set<$O> otherSet = getOtherSet(other);
        for ($O item : items) {
            if (containsIn(otherSet, item)) {
                result.add(item);
            }
        }
        return result.isEmpty() ? EMPTY : new $FS(result);
    }
    
    @Override
    public $O __or__($O other) {
        Set<$O> result = new LinkedHashSet<>(items);
        Set<$O> otherSet = getOtherSet(other);
        for ($O item : otherSet) {
            if (!containsIn(result, item)) {
                result.add(item);
            }
        }
        return new $FS(result);
    }
    
    @Override
    public $O __xor__($O other) {
        Set<$O> result = new LinkedHashSet<>();
        Set<$O> otherSet = getOtherSet(other);
        for ($O item : items) {
            if (!containsIn(otherSet, item)) {
                result.add(item);
            }
        }
        for ($O item : otherSet) {
            if (findKey(item) == null) {
                result.add(item);
            }
        }
        return result.isEmpty() ? EMPTY : new $FS(result);
    }
    
    private Set<$O> getOtherSet($O other) {
        if (other instanceof $FS) return (($FS)other).items;
        if (other instanceof $ST) return (($ST)other).items;
        // Convert iterable to set
        Set<$O> set = new LinkedHashSet<>();
        $O iter = other.__iter__();
        while (true) {
            try {
                set.add(iter.__next__());
            } catch ($X e) {
                if (e.isStopIteration()) break;
                throw e;
            }
        }
        return set;
    }
    
    private boolean containsIn(Set<$O> set, $O item) {
        for ($O k : set) {
            $O eq = k.__eq__(item);
            if (eq instanceof $B && (($B)eq).boolValue) {
                return true;
            }
        }
        return false;
    }
    
    // Frozenset methods
    
    public $FS copy() {
        return this;  // Immutable, so return self
    }
    
    public $FS union($O other) {
        return ($FS) __or__(other);
    }
    
    public $FS intersection($O other) {
        return ($FS) __and__(other);
    }
    
    public $FS difference($O other) {
        return ($FS) __sub__(other);
    }
    
    public $FS symmetric_difference($O other) {
        return ($FS) __xor__(other);
    }
    
    public $B issubset($O other) {
        Set<$O> otherSet = getOtherSet(other);
        for ($O item : items) {
            if (!containsIn(otherSet, item)) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $B issuperset($O other) {
        Set<$O> otherSet = getOtherSet(other);
        for ($O item : otherSet) {
            if (findKey(item) == null) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $B isdisjoint($O other) {
        Set<$O> otherSet = getOtherSet(other);
        for ($O item : items) {
            if (containsIn(otherSet, item)) return $B.FALSE;
        }
        return $B.TRUE;
    }
}

/**
 * FrozenSet iterator.
 */
class $FSI extends $O {
    private final java.util.Iterator<$O> iter;
    
    $FSI($FS set) {
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
