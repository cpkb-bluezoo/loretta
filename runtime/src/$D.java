import java.util.LinkedHashMap;
import java.util.Map;

/**
 * $D - PyDict, dictionary (insertion-ordered).
 */
public final class $D extends $O {
    
    // Use LinkedHashMap to maintain insertion order (Python 3.7+)
    public final Map<$O, $O> map;
    
    public $D() {
        this.map = new LinkedHashMap<>();
    }
    
    public static $D of() {
        return new $D();
    }
    
    public static $D of($O... pairs) {
        $D dict = new $D();
        for (int i = 0; i < pairs.length; i += 2) {
            dict.map.put(pairs[i], pairs[i + 1]);
        }
        return dict;
    }
    
    // Overload for compiler-generated code (separate keys/values arrays)
    public static $D of($O[] keys, $O[] values) {
        $D dict = new $D();
        for (int i = 0; i < keys.length; i++) {
            dict.map.put(keys[i], values[i]);
        }
        return dict;
    }
    
    @Override
    public boolean __bool__() {
        return !map.isEmpty();
    }
    
    @Override
    public $S __repr__() {
        StringBuilder sb = new StringBuilder();
        sb.append('{');
        boolean first = true;
        for (Map.Entry<$O, $O> e : map.entrySet()) {
            if (!first) sb.append(", ");
            first = false;
            sb.append(e.getKey().__repr__().value);
            sb.append(": ");
            sb.append(e.getValue().__repr__().value);
        }
        sb.append('}');
        return $S.of(sb.toString());
    }
    
    @Override
    public $I __len__() {
        return $I.of(map.size());
    }
    
    @Override
    public $O __eq__($O other) {
        if (!(other instanceof $D)) return $B.FALSE;
        $D o = ($D) other;
        if (map.size() != o.map.size()) return $B.FALSE;
        for (Map.Entry<$O, $O> e : map.entrySet()) {
            $O oVal = o.get(e.getKey());
            if (oVal == null) return $B.FALSE;
            $O eq = e.getValue().__eq__(oVal);
            if (eq instanceof $B && !((($B)eq).boolValue)) {
                return $B.FALSE;
            }
        }
        return $B.TRUE;
    }
    
    private $O findKey($O key) {
        // Dict lookup uses __eq__, not identity
        for ($O k : map.keySet()) {
            $O eq = k.__eq__(key);
            if (eq instanceof $B && (($B)eq).boolValue) {
                return k;
            }
        }
        return null;
    }
    
    public $O get($O key) {
        $O k = findKey(key);
        return k != null ? map.get(k) : null;
    }
    
    @Override
    public $O __getitem__($O key) {
        $O value = get(key);
        if (value == null) {
            throw new $X("KeyError", key.__repr__().value);
        }
        return value;
    }
    
    @Override
    public void __setitem__($O key, $O value) {
        $O k = findKey(key);
        if (k != null) {
            map.put(k, value);
        } else {
            map.put(key, value);
        }
    }
    
    @Override
    public void __delitem__($O key) {
        $O k = findKey(key);
        if (k == null) {
            throw new $X("KeyError", key.__repr__().value);
        }
        map.remove(k);
    }
    
    @Override
    public $B __contains__($O key) {
        return $B.of(findKey(key) != null);
    }
    
    @Override
    public $O __iter__() {
        return keys().__iter__();
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "keys":
            case "values":
            case "items":
            case "get":
            case "pop":
            case "setdefault":
            case "update":
            case "clear":
            case "copy":
                return new $BM(this, name);
            default:
                return super.__getattr__(name);
        }
    }
    
    // Dict methods
    
    public $L keys() {
        return $L.of(map.keySet().toArray(new $O[0]));
    }
    
    public $L values() {
        return $L.of(map.values().toArray(new $O[0]));
    }
    
    public $L items() {
        $L list = new $L();
        for (Map.Entry<$O, $O> e : map.entrySet()) {
            list.items.add($T.of(e.getKey(), e.getValue()));
        }
        return list;
    }
    
    public $O get($O key, $O defaultVal) {
        $O value = get(key);
        return value != null ? value : defaultVal;
    }
    
    public $O setdefault($O key, $O defaultVal) {
        $O value = get(key);
        if (value != null) return value;
        __setitem__(key, defaultVal);
        return defaultVal;
    }
    
    public $O pop($O key) {
        $O k = findKey(key);
        if (k == null) {
            throw new $X("KeyError", key.__repr__().value);
        }
        return map.remove(k);
    }
    
    public $O pop($O key, $O defaultVal) {
        $O k = findKey(key);
        if (k == null) return defaultVal;
        return map.remove(k);
    }
    
    public void clear() {
        map.clear();
    }
    
    public void update($O other) {
        if (other instanceof $D) {
            map.putAll((($D)other).map);
        } else {
            throw new $X("TypeError", "argument must be dict");
        }
    }
    
    public $D copy() {
        $D result = new $D();
        result.map.putAll(map);
        return result;
    }
}
