/**
 * $S - PyStr for Unicode strings.
 */
public final class $S extends $O {
    
    public final String value;
    
    private $S(String value) {
        this.value = value;
    }
    
    public static $S of(String value) {
        return new $S(value);
    }
    
    @Override
    public boolean __bool__() {
        return !value.isEmpty();
    }
    
    @Override
    public $S __repr__() {
        StringBuilder sb = new StringBuilder();
        sb.append('\'');
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            switch (c) {
                case '\\': sb.append("\\\\"); break;
                case '\'': sb.append("\\'"); break;
                case '\n': sb.append("\\n"); break;
                case '\r': sb.append("\\r"); break;
                case '\t': sb.append("\\t"); break;
                default:
                    if (c < 32 || c > 126) {
                        sb.append(String.format("\\x%02x", (int)c));
                    } else {
                        sb.append(c);
                    }
            }
        }
        sb.append('\'');
        return $S.of(sb.toString());
    }
    
    @Override
    public $S __str__() {
        return this;
    }
    
    @Override
    public $I __hash__() {
        return $I.of(value.hashCode());
    }
    
    @Override
    public $I __len__() {
        return $I.of(value.length());
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $S) {
            return $B.of(value.equals((($S)other).value));
        }
        return $B.FALSE;
    }
    
    @Override
    public $O __lt__($O other) {
        if (other instanceof $S) {
            return $B.of(value.compareTo((($S)other).value) < 0);
        }
        throw new $X("TypeError", "'<' not supported between str and " + other.getClass().getSimpleName());
    }
    
    @Override
    public $O __le__($O other) {
        if (other instanceof $S) {
            return $B.of(value.compareTo((($S)other).value) <= 0);
        }
        throw new $X("TypeError", "'<=' not supported between str and " + other.getClass().getSimpleName());
    }
    
    @Override
    public $O __gt__($O other) {
        if (other instanceof $S) {
            return $B.of(value.compareTo((($S)other).value) > 0);
        }
        throw new $X("TypeError", "'>' not supported between str and " + other.getClass().getSimpleName());
    }
    
    @Override
    public $O __ge__($O other) {
        if (other instanceof $S) {
            return $B.of(value.compareTo((($S)other).value) >= 0);
        }
        throw new $X("TypeError", "'>=' not supported between str and " + other.getClass().getSimpleName());
    }
    
    @Override
    public $O __add__($O other) {
        if (other instanceof $S) {
            return of(value + (($S)other).value);
        }
        throw new $X("TypeError", "can only concatenate str to str");
    }
    
    @Override
    public $O __mul__($O other) {
        if (other instanceof $I) {
            $I i = ($I) other;
            if (i.big != null) throw new $X("OverflowError", "repeated string is too long");
            int n = (int) i.value;
            if (n <= 0) return of("");
            StringBuilder sb = new StringBuilder(value.length() * n);
            for (int j = 0; j < n; j++) {
                sb.append(value);
            }
            return of(sb.toString());
        }
        throw new $X("TypeError", "can't multiply sequence by non-int");
    }
    
    @Override
    public $O __getitem__($O key) {
        if (key instanceof $I) {
            $I i = ($I) key;
            if (i.big != null) throw new $X("IndexError", "string index out of range");
            int idx = (int) i.value;
            if (idx < 0) idx += value.length();
            if (idx < 0 || idx >= value.length()) {
                throw new $X("IndexError", "string index out of range");
            }
            return of(String.valueOf(value.charAt(idx)));
        }
        if (key instanceof $SL) {
            $SL sl = ($SL) key;
            $T indices = sl.indices($I.of(value.length()));
            int start = (int)(($I)indices.items[0]).value;
            int stop = (int)(($I)indices.items[1]).value;
            int step = (int)(($I)indices.items[2]).value;
            
            StringBuilder sb = new StringBuilder();
            if (step > 0) {
                for (int i = start; i < stop; i += step) {
                    sb.append(value.charAt(i));
                }
            } else {
                for (int i = start; i > stop; i += step) {
                    sb.append(value.charAt(i));
                }
            }
            return of(sb.toString());
        }
        throw new $X("TypeError", "string indices must be integers or slices");
    }
    
    @Override
    public $B __contains__($O item) {
        if (item instanceof $S) {
            return $B.of(value.contains((($S)item).value));
        }
        throw new $X("TypeError", "'in <string>' requires string as left operand");
    }
    
    @Override
    public $O __iter__() {
        return new $SI(this);
    }
    
    @Override
    public $O __getattr__(String name) {
        // Return bound method for known string methods
        switch (name) {
            case "upper":
            case "lower":
            case "strip":
            case "lstrip":
            case "rstrip":
            case "split":
            case "join":
            case "startswith":
            case "endswith":
            case "find":
            case "replace":
            case "isdigit":
            case "isalpha":
            case "isalnum":
            case "capitalize":
            case "title":
            case "swapcase":
            case "center":
            case "ljust":
            case "rjust":
            case "zfill":
            case "count":
            case "index":
            case "encode":
            case "format":
                return new $BM(this, name);
            default:
                return super.__getattr__(name);
        }
    }
    
    // String methods
    
    public $S upper() {
        return of(value.toUpperCase());
    }
    
    public $S lower() {
        return of(value.toLowerCase());
    }
    
    public $S strip() {
        return of(value.strip());
    }
    
    public $S lstrip() {
        return of(value.stripLeading());
    }
    
    public $S rstrip() {
        return of(value.stripTrailing());
    }
    
    public $L split() {
        return split($S.of(" "));
    }
    
    public $L split($O sep) {
        if (!(sep instanceof $S)) {
            throw new $X("TypeError", "must be str, not " + sep.getClass().getSimpleName());
        }
        String[] parts = value.split(java.util.regex.Pattern.quote((($S)sep).value), -1);
        $O[] items = new $O[parts.length];
        for (int i = 0; i < parts.length; i++) {
            items[i] = of(parts[i]);
        }
        return $L.of(items);
    }
    
    public $S join($O iterable) {
        if (iterable instanceof $L) {
            $L list = ($L) iterable;
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < list.items.size(); i++) {
                if (i > 0) sb.append(value);
                $O item = list.items.get(i);
                if (!(item instanceof $S)) {
                    throw new $X("TypeError", "sequence item: expected str instance");
                }
                sb.append((($S)item).value);
            }
            return of(sb.toString());
        }
        if (iterable instanceof $T) {
            $T tuple = ($T) iterable;
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < tuple.items.length; i++) {
                if (i > 0) sb.append(value);
                $O item = tuple.items[i];
                if (!(item instanceof $S)) {
                    throw new $X("TypeError", "sequence item: expected str instance");
                }
                sb.append((($S)item).value);
            }
            return of(sb.toString());
        }
        throw new $X("TypeError", "can only join an iterable");
    }
    
    public $B startswith($O prefix) {
        if (prefix instanceof $S) {
            return $B.of(value.startsWith((($S)prefix).value));
        }
        throw new $X("TypeError", "startswith arg must be str");
    }
    
    public $B endswith($O suffix) {
        if (suffix instanceof $S) {
            return $B.of(value.endsWith((($S)suffix).value));
        }
        throw new $X("TypeError", "endswith arg must be str");
    }
    
    public $I find($O sub) {
        if (sub instanceof $S) {
            return $I.of(value.indexOf((($S)sub).value));
        }
        throw new $X("TypeError", "find arg must be str");
    }
    
    public $S replace($O old, $O newStr) {
        if (old instanceof $S && newStr instanceof $S) {
            return of(value.replace((($S)old).value, (($S)newStr).value));
        }
        throw new $X("TypeError", "replace args must be str");
    }
    
    public $B isdigit() {
        if (value.isEmpty()) return $B.FALSE;
        for (int i = 0; i < value.length(); i++) {
            if (!Character.isDigit(value.charAt(i))) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $B isalpha() {
        if (value.isEmpty()) return $B.FALSE;
        for (int i = 0; i < value.length(); i++) {
            if (!Character.isLetter(value.charAt(i))) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $B isalnum() {
        if (value.isEmpty()) return $B.FALSE;
        for (int i = 0; i < value.length(); i++) {
            if (!Character.isLetterOrDigit(value.charAt(i))) return $B.FALSE;
        }
        return $B.TRUE;
    }
}

/**
 * String iterator.
 */
class $SI extends $O {
    private final $S str;
    private int index;
    
    $SI($S str) {
        this.str = str;
        this.index = 0;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (index >= str.value.length()) {
            throw new $X("StopIteration", "");
        }
        return $S.of(String.valueOf(str.value.charAt(index++)));
    }
}
