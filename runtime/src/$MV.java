/**
 * $MV - memoryview object.
 * 
 * Provides a view into a bytes-like object without copying.
 * Supports slicing and indexing.
 */
public final class $MV extends $O {
    
    private final byte[] buffer;
    private final int offset;
    private final int length;
    private final boolean readonly;
    
    /**
     * Create a memoryview wrapping bytes.
     */
    public $MV($BY bytes) {
        this.buffer = bytes.data;
        this.offset = 0;
        this.length = bytes.data.length;
        this.readonly = true;  // bytes are immutable
    }
    
    /**
     * Create a memoryview slice.
     */
    private $MV(byte[] buffer, int offset, int length, boolean readonly) {
        this.buffer = buffer;
        this.offset = offset;
        this.length = length;
        this.readonly = readonly;
    }
    
    @Override
    public $I __len__() {
        return $I.of(length);
    }
    
    @Override
    public $O __getitem__($O key) {
        if (key instanceof $I) {
            int idx = (int)(($I) key).value;
            if (idx < 0) idx += length;
            if (idx < 0 || idx >= length) {
                throw new $X("IndexError", "index out of range");
            }
            // Return the byte value as an integer
            return $I.of(buffer[offset + idx] & 0xFF);
        }
        
        if (key instanceof $SL) {
            $SL slice = ($SL) key;
            // Get slice parameters
            int start = slice.start != null ? (int)(($I)slice.start).value : 0;
            int stop = slice.stop != null ? (int)(($I)slice.stop).value : length;
            int step = slice.step != null ? (int)(($I)slice.step).value : 1;
            
            // Normalize negative indices
            if (start < 0) start += length;
            if (stop < 0) stop += length;
            if (start < 0) start = 0;
            if (stop > length) stop = length;
            if (stop < start) stop = start;
            
            if (step == 1) {
                // Contiguous slice - can return a view
                return new $MV(buffer, offset + start, stop - start, readonly);
            } else {
                // Non-contiguous - must copy
                int newLen = 0;
                for (int i = start; i < stop; i += step) newLen++;
                byte[] newBuf = new byte[newLen];
                int j = 0;
                for (int i = start; i < stop; i += step) {
                    newBuf[j++] = buffer[offset + i];
                }
                return new $MV(newBuf, 0, newLen, readonly);
            }
        }
        
        throw new $X("TypeError", "memoryview indices must be integers or slices");
    }
    
    @Override
    public void __setitem__($O key, $O value) {
        if (readonly) {
            throw new $X("TypeError", "cannot modify read-only memory");
        }
        
        if (key instanceof $I) {
            int idx = (int)(($I) key).value;
            if (idx < 0) idx += length;
            if (idx < 0 || idx >= length) {
                throw new $X("IndexError", "index out of range");
            }
            if (!(value instanceof $I)) {
                throw new $X("TypeError", "memoryview assignment must be an int");
            }
            int v = (int)(($I) value).value;
            if (v < 0 || v > 255) {
                throw new $X("ValueError", "byte must be in range(0, 256)");
            }
            buffer[offset + idx] = (byte) v;
            return;
        }
        
        throw new $X("TypeError", "memoryview indices must be integers");
    }
    
    /**
     * Convert to bytes.
     */
    public $BY tobytes() {
        byte[] copy = new byte[length];
        System.arraycopy(buffer, offset, copy, 0, length);
        return $BY.of(copy);
    }
    
    /**
     * Convert to list of ints.
     */
    public $L tolist() {
        $L list = new $L();
        for (int i = 0; i < length; i++) {
            list.append($I.of(buffer[offset + i] & 0xFF));
        }
        return list;
    }
    
    /**
     * Release the buffer.
     */
    public void release() {
        // No-op in Java (GC handles it)
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "readonly": return $B.of(readonly);
            case "nbytes": return $I.of(length);
            case "itemsize": return $I.of(1);
            case "ndim": return $I.of(1);
            case "shape": return $T.of($I.of(length));
            case "strides": return $T.of($I.of(1));
            case "tobytes": return new $O() {
                @Override
                public $O __call__($O... args) {
                    return tobytes();
                }
            };
            case "tolist": return new $O() {
                @Override
                public $O __call__($O... args) {
                    return tolist();
                }
            };
            case "release": return new $O() {
                @Override
                public $O __call__($O... args) {
                    release();
                    return $N.INSTANCE;
                }
            };
            default:
                throw new $X("AttributeError", "memoryview has no attribute '" + name + "'");
        }
    }
    
    @Override
    public $O __iter__() {
        return new $MVI(this);
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<memory at 0x" + Integer.toHexString(System.identityHashCode(this)) + ">");
    }
    
    @Override
    public boolean __bool__() {
        return length > 0;
    }
    
    @Override
    public $O __eq__($O other) {
        if (other instanceof $MV) {
            $MV o = ($MV) other;
            if (length != o.length) return $B.FALSE;
            for (int i = 0; i < length; i++) {
                if (buffer[offset + i] != o.buffer[o.offset + i]) {
                    return $B.FALSE;
                }
            }
            return $B.TRUE;
        }
        if (other instanceof $BY) {
            $BY o = ($BY) other;
            if (length != o.data.length) return $B.FALSE;
            for (int i = 0; i < length; i++) {
                if (buffer[offset + i] != o.data[i]) {
                    return $B.FALSE;
                }
            }
            return $B.TRUE;
        }
        return $B.FALSE;
    }
}

/**
 * Memoryview iterator.
 */
class $MVI extends $O {
    private final $MV mv;
    private int idx;
    
    $MVI($MV mv) {
        this.mv = mv;
        this.idx = 0;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (idx >= (int)mv.__len__().value) {
            throw new $X("StopIteration", "");
        }
        return mv.__getitem__($I.of(idx++));
    }
}
