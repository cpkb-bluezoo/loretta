import java.util.ArrayList;
import java.util.List;

/**
 * $L - PyList, mutable sequence.
 */
public final class $L extends $O {
    
    public final List<$O> items;
    
    public $L() {
        this.items = new ArrayList<>();
    }
    
    public $L(List<$O> items) {
        this.items = items;
    }
    
    public static $L of($O... items) {
        $L list = new $L();
        for ($O item : items) {
            list.items.add(item);
        }
        return list;
    }
    
    @Override
    public boolean __bool__() {
        return !items.isEmpty();
    }
    
    @Override
    public $S __repr__() {
        StringBuilder sb = new StringBuilder();
        sb.append('[');
        for (int i = 0; i < items.size(); i++) {
            if (i > 0) sb.append(", ");
            sb.append(items.get(i).__repr__().value);
        }
        sb.append(']');
        return $S.of(sb.toString());
    }
    
    @Override
    public $I __len__() {
        return $I.of(items.size());
    }
    
    @Override
    public $O __eq__($O other) {
        if (!(other instanceof $L)) return $B.FALSE;
        $L o = ($L) other;
        if (items.size() != o.items.size()) return $B.FALSE;
        for (int i = 0; i < items.size(); i++) {
            $O eq = items.get(i).__eq__(o.items.get(i));
            if (eq instanceof $B && !((($B)eq).boolValue)) {
                return $B.FALSE;
            }
        }
        return $B.TRUE;
    }
    
    @Override
    public $O __getitem__($O key) {
        if (key instanceof $I) {
            $I i = ($I) key;
            if (i.big != null) throw new $X("IndexError", "list index out of range");
            int idx = (int) i.value;
            if (idx < 0) idx += items.size();
            if (idx < 0 || idx >= items.size()) {
                throw new $X("IndexError", "list index out of range");
            }
            return items.get(idx);
        }
        if (key instanceof $SL) {
            $SL sl = ($SL) key;
            $T indices = sl.indices($I.of(items.size()));
            int start = (int)(($I)indices.items[0]).value;
            int stop = (int)(($I)indices.items[1]).value;
            int step = (int)(($I)indices.items[2]).value;
            
            $L result = new $L();
            if (step > 0) {
                for (int i = start; i < stop; i += step) {
                    result.items.add(items.get(i));
                }
            } else {
                for (int i = start; i > stop; i += step) {
                    result.items.add(items.get(i));
                }
            }
            return result;
        }
        throw new $X("TypeError", "list indices must be integers or slices");
    }
    
    @Override
    public void __setitem__($O key, $O value) {
        if (key instanceof $I) {
            $I i = ($I) key;
            if (i.big != null) throw new $X("IndexError", "list assignment index out of range");
            int idx = (int) i.value;
            if (idx < 0) idx += items.size();
            if (idx < 0 || idx >= items.size()) {
                throw new $X("IndexError", "list assignment index out of range");
            }
            items.set(idx, value);
            return;
        }
        if (key instanceof $SL) {
            // Slice assignment: list[start:stop] = iterable
            $SL sl = ($SL) key;
            $T indices = sl.indices($I.of(items.size()));
            int start = (int)(($I)indices.items[0]).value;
            int stop = (int)(($I)indices.items[1]).value;
            int step = (int)(($I)indices.items[2]).value;
            
            // Collect new values
            java.util.List<$O> newItems = new java.util.ArrayList<>();
            $O iter = value.__iter__();
            while (true) {
                try {
                    newItems.add(iter.__next__());
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            
            if (step == 1) {
                // Simple slice: remove old, insert new
                for (int i = stop - 1; i >= start; i--) {
                    if (i < items.size()) items.remove(i);
                }
                for (int i = 0; i < newItems.size(); i++) {
                    items.add(start + i, newItems.get(i));
                }
            } else {
                // Extended slice: must have same length
                int count = 0;
                if (step > 0) {
                    for (int i = start; i < stop; i += step) count++;
                } else {
                    for (int i = start; i > stop; i += step) count++;
                }
                if (count != newItems.size()) {
                    throw new $X("ValueError", "attempt to assign sequence of size " + 
                                 newItems.size() + " to extended slice of size " + count);
                }
                int idx = 0;
                if (step > 0) {
                    for (int i = start; i < stop; i += step) {
                        items.set(i, newItems.get(idx++));
                    }
                } else {
                    for (int i = start; i > stop; i += step) {
                        items.set(i, newItems.get(idx++));
                    }
                }
            }
            return;
        }
        throw new $X("TypeError", "list indices must be integers or slices");
    }
    
    @Override
    public void __delitem__($O key) {
        if (key instanceof $I) {
            $I i = ($I) key;
            if (i.big != null) throw new $X("IndexError", "list index out of range");
            int idx = (int) i.value;
            if (idx < 0) idx += items.size();
            if (idx < 0 || idx >= items.size()) {
                throw new $X("IndexError", "list index out of range");
            }
            items.remove(idx);
            return;
        }
        if (key instanceof $SL) {
            $SL sl = ($SL) key;
            $T indices = sl.indices($I.of(items.size()));
            int start = (int)(($I)indices.items[0]).value;
            int stop = (int)(($I)indices.items[1]).value;
            int step = (int)(($I)indices.items[2]).value;
            
            // Collect indices to remove
            java.util.List<Integer> toRemove = new java.util.ArrayList<>();
            if (step > 0) {
                for (int i = start; i < stop; i += step) toRemove.add(i);
            } else {
                for (int i = start; i > stop; i += step) toRemove.add(i);
            }
            // Remove in reverse order to maintain indices
            java.util.Collections.sort(toRemove, java.util.Collections.reverseOrder());
            for (int idx : toRemove) {
                items.remove(idx);
            }
            return;
        }
        throw new $X("TypeError", "list indices must be integers or slices");
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
        if (other instanceof $L) {
            $L result = new $L();
            result.items.addAll(items);
            result.items.addAll((($L)other).items);
            return result;
        }
        throw new $X("TypeError", "can only concatenate list to list");
    }
    
    @Override
    public $O __mul__($O other) {
        if (other instanceof $I) {
            $I i = ($I) other;
            if (i.big != null) throw new $X("OverflowError", "repeated list is too long");
            int n = (int) i.value;
            $L result = new $L();
            for (int j = 0; j < n; j++) {
                result.items.addAll(items);
            }
            return result;
        }
        throw new $X("TypeError", "can't multiply sequence by non-int");
    }
    
    @Override
    public $O __iter__() {
        return new $LI(this);
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "append":
            case "extend":
            case "insert":
            case "pop":
            case "remove":
            case "clear":
            case "index":
            case "count":
            case "reverse":
            case "copy":
            case "sort":
                return new $BM(this, name);
            default:
                return super.__getattr__(name);
        }
    }
    
    // List methods
    
    public void append($O item) {
        items.add(item);
    }
    
    public void extend($O iterable) {
        if (iterable instanceof $L) {
            items.addAll((($L)iterable).items);
        } else if (iterable instanceof $T) {
            for ($O item : (($T)iterable).items) {
                items.add(item);
            }
        } else {
            // Try iterator protocol for any other iterable
            try {
                $O iter = iterable.__iter__();
                while (true) {
                    try {
                        $O item = iter.__next__();
                        items.add(item);
                    } catch ($X e) {
                        if (e.isStopIteration()) {
                            break;
                        }
                        throw e;
                    }
                }
            } catch (UnsupportedOperationException e) {
                throw new $X("TypeError", "object is not iterable");
            }
        }
    }
    
    public void insert($O index, $O item) {
        if (!(index instanceof $I)) {
            throw new $X("TypeError", "index must be integer");
        }
        int idx = (int)(($I)index).value;
        if (idx < 0) idx += items.size();
        if (idx < 0) idx = 0;
        if (idx > items.size()) idx = items.size();
        items.add(idx, item);
    }
    
    public $O pop() {
        if (items.isEmpty()) {
            throw new $X("IndexError", "pop from empty list");
        }
        return items.remove(items.size() - 1);
    }
    
    public $O pop($O index) {
        if (!(index instanceof $I)) {
            throw new $X("TypeError", "index must be integer");
        }
        int idx = (int)(($I)index).value;
        if (idx < 0) idx += items.size();
        if (idx < 0 || idx >= items.size()) {
            throw new $X("IndexError", "pop index out of range");
        }
        return items.remove(idx);
    }
    
    public void remove($O item) {
        for (int i = 0; i < items.size(); i++) {
            $O eq = items.get(i).__eq__(item);
            if (eq instanceof $B && (($B)eq).boolValue) {
                items.remove(i);
                return;
            }
        }
        throw new $X("ValueError", "list.remove(x): x not in list");
    }
    
    public void clear() {
        items.clear();
    }
    
    public $I index($O item) {
        for (int i = 0; i < items.size(); i++) {
            $O eq = items.get(i).__eq__(item);
            if (eq instanceof $B && (($B)eq).boolValue) {
                return $I.of(i);
            }
        }
        throw new $X("ValueError", "is not in list");
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
    
    public void reverse() {
        java.util.Collections.reverse(items);
    }
    
    public $L copy() {
        return $L.of(items.toArray(new $O[0]));
    }
}

/**
 * List iterator.
 */
class $LI extends $O {
    private final $L list;
    private int index;
    
    $LI($L list) {
        this.list = list;
        this.index = 0;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (index >= list.items.size()) {
            throw new $X("StopIteration", "");
        }
        return list.items.get(index++);
    }
}
