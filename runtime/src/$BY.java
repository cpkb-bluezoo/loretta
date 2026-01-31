import java.util.Arrays;

/**
 * $BY - PyBytes for immutable byte sequences.
 */
public final class $BY extends $O {
    
    public final byte[] data;
    
    private $BY(byte[] data) {
        this.data = data;
    }
    
    public static $BY of(byte[] data) {
        return new $BY(data.clone());
    }
    
    public static $BY of(String s) {
        return new $BY(s.getBytes(java.nio.charset.StandardCharsets.UTF_8));
    }
    
    public static $BY of(int... values) {
        byte[] data = new byte[values.length];
        for (int i = 0; i < values.length; i++) {
            if (values[i] < 0 || values[i] > 255) {
                throw new $X("ValueError", "bytes must be in range(0, 256)");
            }
            data[i] = (byte) values[i];
        }
        return new $BY(data);
    }
    
    @Override
    public boolean __bool__() {
        return data.length > 0;
    }
    
    @Override
    public $S __repr__() {
        StringBuilder sb = new StringBuilder();
        sb.append("b'");
        for (byte b : data) {
            int v = b & 0xFF;
            if (v == '\\') sb.append("\\\\");
            else if (v == '\'') sb.append("\\'");
            else if (v == '\n') sb.append("\\n");
            else if (v == '\r') sb.append("\\r");
            else if (v == '\t') sb.append("\\t");
            else if (v >= 32 && v < 127) sb.append((char) v);
            else sb.append(String.format("\\x%02x", v));
        }
        sb.append("'");
        return $S.of(sb.toString());
    }
    
    @Override
    public $I __hash__() {
        return $I.of(Arrays.hashCode(data));
    }
    
    @Override
    public $I __len__() {
        return $I.of(data.length);
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $BY) {
            return $B.of(Arrays.equals(data, (($BY)other).data));
        }
        return $B.FALSE;
    }
    
    @Override
    public $O __lt__($O other) {
        if (other instanceof $BY) {
            return $B.of(Arrays.compare(data, (($BY)other).data) < 0);
        }
        throw new $X("TypeError", "'<' not supported between bytes and " + other.getClass().getSimpleName());
    }
    
    @Override
    public $O __le__($O other) {
        if (other instanceof $BY) {
            return $B.of(Arrays.compare(data, (($BY)other).data) <= 0);
        }
        throw new $X("TypeError", "'<=' not supported");
    }
    
    @Override
    public $O __gt__($O other) {
        if (other instanceof $BY) {
            return $B.of(Arrays.compare(data, (($BY)other).data) > 0);
        }
        throw new $X("TypeError", "'>' not supported");
    }
    
    @Override
    public $O __ge__($O other) {
        if (other instanceof $BY) {
            return $B.of(Arrays.compare(data, (($BY)other).data) >= 0);
        }
        throw new $X("TypeError", "'>=' not supported");
    }
    
    @Override
    public $O __getitem__($O key) {
        if (key instanceof $I) {
            $I i = ($I) key;
            if (i.big != null) throw new $X("IndexError", "bytes index out of range");
            int idx = (int) i.value;
            if (idx < 0) idx += data.length;
            if (idx < 0 || idx >= data.length) {
                throw new $X("IndexError", "bytes index out of range");
            }
            return $I.of(data[idx] & 0xFF);
        }
        if (key instanceof $SL) {
            $SL sl = ($SL) key;
            $T indices = sl.indices($I.of(data.length));
            int start = (int)(($I)indices.items[0]).value;
            int stop = (int)(($I)indices.items[1]).value;
            int step = (int)(($I)indices.items[2]).value;
            
            java.util.List<Byte> result = new java.util.ArrayList<>();
            if (step > 0) {
                for (int i = start; i < stop; i += step) {
                    result.add(data[i]);
                }
            } else {
                for (int i = start; i > stop; i += step) {
                    result.add(data[i]);
                }
            }
            byte[] newData = new byte[result.size()];
            for (int i = 0; i < result.size(); i++) {
                newData[i] = result.get(i);
            }
            return new $BY(newData);
        }
        throw new $X("TypeError", "bytes indices must be integers or slices");
    }
    
    @Override
    public $B __contains__($O item) {
        if (item instanceof $I) {
            int v = (int)(($I)item).value;
            if (v < 0 || v > 255) return $B.FALSE;
            byte b = (byte) v;
            for (byte d : data) {
                if (d == b) return $B.TRUE;
            }
            return $B.FALSE;
        }
        if (item instanceof $BY) {
            byte[] sub = (($BY)item).data;
            outer:
            for (int i = 0; i <= data.length - sub.length; i++) {
                for (int j = 0; j < sub.length; j++) {
                    if (data[i + j] != sub[j]) continue outer;
                }
                return $B.TRUE;
            }
            return $B.FALSE;
        }
        throw new $X("TypeError", "a bytes-like object is required");
    }
    
    @Override
    public $O __add__($O other) {
        if (other instanceof $BY) {
            byte[] o = (($BY)other).data;
            byte[] result = new byte[data.length + o.length];
            System.arraycopy(data, 0, result, 0, data.length);
            System.arraycopy(o, 0, result, data.length, o.length);
            return new $BY(result);
        }
        throw new $X("TypeError", "can't concat bytes to " + other.getClass().getSimpleName());
    }
    
    @Override
    public $O __mul__($O other) {
        if (other instanceof $I) {
            $I i = ($I) other;
            if (i.big != null) throw new $X("OverflowError", "repeated bytes is too long");
            int n = (int) i.value;
            if (n <= 0) return new $BY(new byte[0]);
            byte[] result = new byte[data.length * n];
            for (int j = 0; j < n; j++) {
                System.arraycopy(data, 0, result, j * data.length, data.length);
            }
            return new $BY(result);
        }
        throw new $X("TypeError", "can't multiply sequence by non-int");
    }
    
    @Override
    public $O __iter__() {
        return new $BYI(this);
    }
    
    // Bytes methods
    
    public $S decode() {
        return $S.of(new String(data, java.nio.charset.StandardCharsets.UTF_8));
    }
    
    public $S decode($O encoding) {
        String enc = (($S)encoding).value;
        try {
            return $S.of(new String(data, enc));
        } catch (java.io.UnsupportedEncodingException e) {
            throw new $X("LookupError", "unknown encoding: " + enc);
        }
    }
    
    public $I find($O sub) {
        if (!(sub instanceof $BY)) throw new $X("TypeError", "expected bytes");
        byte[] s = (($BY)sub).data;
        outer:
        for (int i = 0; i <= data.length - s.length; i++) {
            for (int j = 0; j < s.length; j++) {
                if (data[i + j] != s[j]) continue outer;
            }
            return $I.of(i);
        }
        return $I.of(-1);
    }
    
    public $BY upper() {
        byte[] result = new byte[data.length];
        for (int i = 0; i < data.length; i++) {
            byte b = data[i];
            if (b >= 'a' && b <= 'z') {
                result[i] = (byte)(b - 32);
            } else {
                result[i] = b;
            }
        }
        return new $BY(result);
    }
    
    public $BY lower() {
        byte[] result = new byte[data.length];
        for (int i = 0; i < data.length; i++) {
            byte b = data[i];
            if (b >= 'A' && b <= 'Z') {
                result[i] = (byte)(b + 32);
            } else {
                result[i] = b;
            }
        }
        return new $BY(result);
    }
    
    public $B startswith($O prefix) {
        if (!(prefix instanceof $BY)) throw new $X("TypeError", "expected bytes");
        byte[] p = (($BY)prefix).data;
        if (p.length > data.length) return $B.FALSE;
        for (int i = 0; i < p.length; i++) {
            if (data[i] != p[i]) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $B endswith($O suffix) {
        if (!(suffix instanceof $BY)) throw new $X("TypeError", "expected bytes");
        byte[] s = (($BY)suffix).data;
        if (s.length > data.length) return $B.FALSE;
        int offset = data.length - s.length;
        for (int i = 0; i < s.length; i++) {
            if (data[offset + i] != s[i]) return $B.FALSE;
        }
        return $B.TRUE;
    }
    
    public $S hex() {
        StringBuilder sb = new StringBuilder();
        for (byte b : data) {
            sb.append(String.format("%02x", b & 0xFF));
        }
        return $S.of(sb.toString());
    }
}

/**
 * Bytes iterator.
 */
class $BYI extends $O {
    private final $BY bytes;
    private int index;
    
    $BYI($BY bytes) {
        this.bytes = bytes;
        this.index = 0;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (index >= bytes.data.length) {
            throw new $X.StopIteration();
        }
        return $I.of(bytes.data[index++] & 0xFF);
    }
}
