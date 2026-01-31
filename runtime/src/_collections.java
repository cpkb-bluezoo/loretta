import java.util.ArrayDeque;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * _collections - C-extension replacement for collections module.
 * 
 * Provides deque and defaultdict implementations.
 */
public class _collections {
    
    /**
     * deque - Double-ended queue.
     * Supports fast appends and pops from both ends.
     */
    public static class deque extends $O implements Iterable<$O> {
        private ArrayDeque<$O> data;
        private int maxlen;
        
        public deque() {
            this.data = new ArrayDeque<>();
            this.maxlen = -1;  // No limit
        }
        
        public deque($O iterable) {
            this();
            if (iterable != null && !(iterable instanceof $N)) {
                extend(iterable);
            }
        }
        
        public deque($O iterable, $O maxlen) {
            this();
            if (maxlen != null && maxlen instanceof $I) {
                this.maxlen = (int)(($I)maxlen).value;
                if (this.maxlen < 0) {
                    throw new $X.ValueError("maxlen must be non-negative");
                }
            }
            if (iterable != null && !(iterable instanceof $N)) {
                extend(iterable);
            }
        }
        
        /**
         * Append to right end.
         */
        public void append($O x) {
            if (maxlen >= 0 && data.size() >= maxlen) {
                data.pollFirst();
            }
            data.addLast(x);
        }
        
        /**
         * Append to left end.
         */
        public void appendleft($O x) {
            if (maxlen >= 0 && data.size() >= maxlen) {
                data.pollLast();
            }
            data.addFirst(x);
        }
        
        /**
         * Remove and return from right end.
         */
        public $O pop() {
            if (data.isEmpty()) {
                throw new $X.IndexError("pop from an empty deque");
            }
            return data.removeLast();
        }
        
        /**
         * Remove and return from left end.
         */
        public $O popleft() {
            if (data.isEmpty()) {
                throw new $X.IndexError("pop from an empty deque");
            }
            return data.removeFirst();
        }
        
        /**
         * Extend by appending elements from iterable (right end).
         */
        public void extend($O iterable) {
            $O iter = iterable.__iter__();
            while (true) {
                try {
                    append(iter.__next__());
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
        }
        
        /**
         * Extend by appending elements from iterable (left end).
         */
        public void extendleft($O iterable) {
            $O iter = iterable.__iter__();
            while (true) {
                try {
                    appendleft(iter.__next__());
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
        }
        
        /**
         * Rotate deque n steps to the right. If n is negative, rotate left.
         */
        public void rotate($O n) {
            int steps = n instanceof $I ? (int)(($I)n).value : 0;
            if (data.isEmpty()) return;
            
            steps = steps % data.size();
            if (steps == 0) return;
            
            if (steps > 0) {
                // Rotate right
                for (int i = 0; i < steps; i++) {
                    data.addFirst(data.removeLast());
                }
            } else {
                // Rotate left
                for (int i = 0; i < -steps; i++) {
                    data.addLast(data.removeFirst());
                }
            }
        }
        
        /**
         * Remove all elements.
         */
        public void clear() {
            data.clear();
        }
        
        /**
         * Return shallow copy.
         */
        public deque copy() {
            deque result = new deque();
            result.data = new ArrayDeque<>(this.data);
            result.maxlen = this.maxlen;
            return result;
        }
        
        /**
         * Count occurrences of value.
         */
        public $I count($O x) {
            int count = 0;
            for ($O item : data) {
                if (item.__eq__(x).__bool__()) {
                    count++;
                }
            }
            return $I.of(count);
        }
        
        /**
         * Return index of first occurrence.
         */
        public $I index($O x) {
            int idx = 0;
            for ($O item : data) {
                if (item.__eq__(x).__bool__()) {
                    return $I.of(idx);
                }
                idx++;
            }
            throw new $X.ValueError("value not in deque");
        }
        
        /**
         * Insert at position.
         */
        public void insert($O idx, $O x) {
            int i = (int)(($I)idx).value;
            if (maxlen >= 0 && data.size() >= maxlen) {
                throw new $X.IndexError("deque already at its maximum size");
            }
            
            // Convert to list, insert, convert back
            $O[] arr = data.toArray(new $O[0]);
            if (i < 0) i = Math.max(0, arr.length + i);
            if (i > arr.length) i = arr.length;
            
            data.clear();
            for (int j = 0; j < i; j++) {
                data.addLast(arr[j]);
            }
            data.addLast(x);
            for (int j = i; j < arr.length; j++) {
                data.addLast(arr[j]);
            }
        }
        
        /**
         * Remove first occurrence of value.
         */
        public void remove($O x) {
            Iterator<$O> iter = data.iterator();
            while (iter.hasNext()) {
                if (iter.next().__eq__(x).__bool__()) {
                    iter.remove();
                    return;
                }
            }
            throw new $X.ValueError("deque.remove(x): x not in deque");
        }
        
        /**
         * Reverse in place.
         */
        public void reverse() {
            $O[] arr = data.toArray(new $O[0]);
            data.clear();
            for (int i = arr.length - 1; i >= 0; i--) {
                data.addLast(arr[i]);
            }
        }
        
        @Override
        public $I __len__() {
            return $I.of(data.size());
        }
        
        @Override
        public $O __getitem__($O key) {
            if (!(key instanceof $I)) {
                throw new $X.TypeError("deque indices must be integers");
            }
            int idx = (int)(($I)key).value;
            int size = data.size();
            if (idx < 0) idx += size;
            if (idx < 0 || idx >= size) {
                throw new $X.IndexError("deque index out of range");
            }
            
            // ArrayDeque doesn't support random access, so iterate
            int i = 0;
            for ($O item : data) {
                if (i == idx) return item;
                i++;
            }
            throw new $X.IndexError("deque index out of range");
        }
        
        @Override
        public void __setitem__($O key, $O value) {
            if (!(key instanceof $I)) {
                throw new $X.TypeError("deque indices must be integers");
            }
            int idx = (int)(($I)key).value;
            int size = data.size();
            if (idx < 0) idx += size;
            if (idx < 0 || idx >= size) {
                throw new $X.IndexError("deque index out of range");
            }
            
            // Need to rebuild - ArrayDeque doesn't support set
            $O[] arr = data.toArray(new $O[0]);
            arr[idx] = value;
            data.clear();
            for ($O item : arr) {
                data.addLast(item);
            }
        }
        
        @Override
        public void __delitem__($O key) {
            if (!(key instanceof $I)) {
                throw new $X.TypeError("deque indices must be integers");
            }
            int idx = (int)(($I)key).value;
            int size = data.size();
            if (idx < 0) idx += size;
            if (idx < 0 || idx >= size) {
                throw new $X.IndexError("deque index out of range");
            }
            
            // Rebuild without the item
            $O[] arr = data.toArray(new $O[0]);
            data.clear();
            for (int i = 0; i < arr.length; i++) {
                if (i != idx) data.addLast(arr[i]);
            }
        }
        
        @Override
        public $B __contains__($O item) {
            for ($O x : data) {
                if (x.__eq__(item).__bool__()) {
                    return $B.TRUE;
                }
            }
            return $B.FALSE;
        }
        
        @Override
        public $O __iter__() {
            return new deque_iterator(data.iterator());
        }
        
        @Override
        public Iterator<$O> iterator() {
            return data.iterator();
        }
        
        @Override
        public boolean __bool__() {
            return !data.isEmpty();
        }
        
        @Override
        public $S __repr__() {
            StringBuilder sb = new StringBuilder("deque([");
            boolean first = true;
            for ($O item : data) {
                if (!first) sb.append(", ");
                sb.append(item.__repr__().value);
                first = false;
            }
            sb.append("]");
            if (maxlen >= 0) {
                sb.append(", maxlen=").append(maxlen);
            }
            sb.append(")");
            return $S.of(sb.toString());
        }
        
        @Override
        public $O __getattr__(String name) {
            final deque self = this;
            switch (name) {
                case "maxlen": return maxlen >= 0 ? $I.of(maxlen) : $N.INSTANCE;
                case "append": return new $O() {
                    @Override public $O __call__($O... args) { self.append(args[0]); return $N.INSTANCE; }
                };
                case "appendleft": return new $O() {
                    @Override public $O __call__($O... args) { self.appendleft(args[0]); return $N.INSTANCE; }
                };
                case "pop": return new $O() {
                    @Override public $O __call__($O... args) { return self.pop(); }
                };
                case "popleft": return new $O() {
                    @Override public $O __call__($O... args) { return self.popleft(); }
                };
                case "extend": return new $O() {
                    @Override public $O __call__($O... args) { self.extend(args[0]); return $N.INSTANCE; }
                };
                case "extendleft": return new $O() {
                    @Override public $O __call__($O... args) { self.extendleft(args[0]); return $N.INSTANCE; }
                };
                case "rotate": return new $O() {
                    @Override public $O __call__($O... args) { self.rotate(args.length > 0 ? args[0] : $I.of(1)); return $N.INSTANCE; }
                };
                case "clear": return new $O() {
                    @Override public $O __call__($O... args) { self.clear(); return $N.INSTANCE; }
                };
                case "copy": return new $O() {
                    @Override public $O __call__($O... args) { return self.copy(); }
                };
                case "count": return new $O() {
                    @Override public $O __call__($O... args) { return self.count(args[0]); }
                };
                case "index": return new $O() {
                    @Override public $O __call__($O... args) { return self.index(args[0]); }
                };
                case "insert": return new $O() {
                    @Override public $O __call__($O... args) { self.insert(args[0], args[1]); return $N.INSTANCE; }
                };
                case "remove": return new $O() {
                    @Override public $O __call__($O... args) { self.remove(args[0]); return $N.INSTANCE; }
                };
                case "reverse": return new $O() {
                    @Override public $O __call__($O... args) { self.reverse(); return $N.INSTANCE; }
                };
                default: return super.__getattr__(name);
            }
        }
    }
    
    /**
     * Iterator for deque.
     */
    static class deque_iterator extends $O {
        private Iterator<$O> iter;
        
        deque_iterator(Iterator<$O> iter) {
            this.iter = iter;
        }
        
        @Override
        public $O __iter__() {
            return this;
        }
        
        @Override
        public $O __next__() {
            if (!iter.hasNext()) {
                throw new $X.StopIteration();
            }
            return iter.next();
        }
    }
    
    /**
     * defaultdict - Dict-like class with default factory.
     * Uses composition since $D is final.
     */
    public static class defaultdict extends $O {
        private $O default_factory;
        private $D dict;
        
        public defaultdict() {
            this.default_factory = $N.INSTANCE;
            this.dict = new $D();
        }
        
        public defaultdict($O default_factory) {
            this.default_factory = default_factory != null ? default_factory : $N.INSTANCE;
            this.dict = new $D();
        }
        
        public defaultdict($O default_factory, $O items) {
            this.default_factory = default_factory != null ? default_factory : $N.INSTANCE;
            this.dict = new $D();
            if (items != null && items instanceof $D) {
                // Copy items from dict
                $D d = ($D)items;
                for ($O key : d.keys().items) {
                    dict.__setitem__(key, d.__getitem__(key));
                }
            }
        }
        
        @Override
        public $O __getitem__($O key) {
            $O value = dict.get(key);
            if (value != null) {
                return value;
            }
            
            // Key not found - call factory if available
            if (default_factory != null && !(default_factory instanceof $N)) {
                $O newValue = default_factory.__call__();
                dict.__setitem__(key, newValue);
                return newValue;
            }
            
            throw new $X.KeyError(key);
        }
        
        @Override
        public void __setitem__($O key, $O value) {
            dict.__setitem__(key, value);
        }
        
        @Override
        public void __delitem__($O key) {
            dict.__delitem__(key);
        }
        
        @Override
        public $B __contains__($O key) {
            return dict.__contains__(key);
        }
        
        @Override
        public $I __len__() {
            return dict.__len__();
        }
        
        @Override
        public $O __iter__() {
            return dict.__iter__();
        }
        
        /**
         * Get without calling factory.
         */
        public $O get($O key) {
            return dict.get(key);
        }
        
        public $O get($O key, $O defaultVal) {
            return dict.get(key, defaultVal);
        }
        
        public $L keys() {
            return dict.keys();
        }
        
        public $L values() {
            return dict.values();
        }
        
        public $L items() {
            return dict.items();
        }
        
        public void update($O other) {
            dict.update(other);
        }
        
        public $O pop($O key) {
            return dict.pop(key);
        }
        
        public $O pop($O key, $O defaultVal) {
            return dict.pop(key, defaultVal);
        }
        
        public void clear() {
            dict.clear();
        }
        
        @Override
        public $O __getattr__(String name) {
            final defaultdict self = this;
            switch (name) {
                case "default_factory": return default_factory;
                case "get": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 1 ? self.get(args[0], args[1]) : self.get(args[0]); 
                    }
                };
                case "keys": return new $O() {
                    @Override public $O __call__($O... args) { return self.keys(); }
                };
                case "values": return new $O() {
                    @Override public $O __call__($O... args) { return self.values(); }
                };
                case "items": return new $O() {
                    @Override public $O __call__($O... args) { return self.items(); }
                };
                case "update": return new $O() {
                    @Override public $O __call__($O... args) { self.update(args[0]); return $N.INSTANCE; }
                };
                case "pop": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 1 ? self.pop(args[0], args[1]) : self.pop(args[0]); 
                    }
                };
                case "clear": return new $O() {
                    @Override public $O __call__($O... args) { self.clear(); return $N.INSTANCE; }
                };
                case "copy": return new $O() {
                    @Override public $O __call__($O... args) { return self.copy(); }
                };
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            String factory = default_factory instanceof $N ? "None" : default_factory.__repr__().value;
            return $S.of("defaultdict(" + factory + ", " + dict.__repr__().value + ")");
        }
        
        @Override
        public boolean __bool__() {
            return dict.__bool__();
        }
        
        /**
         * Return missing value (for subclassing).
         */
        public $O __missing__($O key) {
            if (default_factory == null || default_factory instanceof $N) {
                throw new $X.KeyError(key);
            }
            $O value = default_factory.__call__();
            dict.__setitem__(key, value);
            return value;
        }
        
        /**
         * Copy.
         */
        public defaultdict copy() {
            defaultdict result = new defaultdict(default_factory);
            for ($O key : dict.keys().items) {
                result.dict.__setitem__(key, dict.__getitem__(key));
            }
            return result;
        }
    }
    
    /**
     * Counter - Dict-like class for counting hashable objects.
     * Uses composition since $D is final.
     */
    public static class Counter extends $O {
        private $D dict;
        
        public Counter() {
            this.dict = new $D();
        }
        
        public Counter($O iterable) {
            this.dict = new $D();
            if (iterable != null && !(iterable instanceof $N)) {
                update(iterable);
            }
        }
        
        /**
         * Return count for element (0 if missing).
         */
        @Override
        public $O __getitem__($O key) {
            $O value = dict.get(key);
            return value != null ? value : $I.of(0);
        }
        
        @Override
        public void __setitem__($O key, $O value) {
            dict.__setitem__(key, value);
        }
        
        @Override
        public void __delitem__($O key) {
            dict.__delitem__(key);
        }
        
        @Override
        public $B __contains__($O key) {
            return dict.__contains__(key);
        }
        
        @Override
        public $I __len__() {
            return dict.__len__();
        }
        
        @Override
        public $O __iter__() {
            return dict.__iter__();
        }
        
        public $L keys() {
            return dict.keys();
        }
        
        public $L values() {
            return dict.values();
        }
        
        public $L items() {
            return dict.items();
        }
        
        /**
         * Update counts from iterable.
         */
        public void update($O iterable) {
            if (iterable instanceof $D) {
                // Update from dict
                $D d = ($D)iterable;
                for ($O key : d.keys().items) {
                    $O count = d.__getitem__(key);
                    $O existing = __getitem__(key);
                    __setitem__(key, existing.__add__(count));
                }
            } else {
                // Count items from iterable
                $O iter = iterable.__iter__();
                while (true) {
                    try {
                        $O item = iter.__next__();
                        $O count = __getitem__(item);
                        __setitem__(item, count.__add__($I.of(1)));
                    } catch ($X e) {
                        if (e.isStopIteration()) break;
                        throw e;
                    }
                }
            }
        }
        
        /**
         * Return list of (element, count) pairs, most common first.
         */
        public $L most_common() {
            return most_common($N.INSTANCE);
        }
        
        public $L most_common($O n) {
            // Get all items and sort by count descending
            $L itemsList = new $L();
            for ($O key : dict.keys().items) {
                itemsList.append($T.of(key, __getitem__(key)));
            }
            
            // Sort by count (descending)
            itemsList.items.sort((a, b) -> {
                $I countA = ($I)(($T)a).items[1];
                $I countB = ($I)(($T)b).items[1];
                return Long.compare(countB.value, countA.value);
            });
            
            // Limit if n specified
            if (n instanceof $I) {
                int limit = (int)(($I)n).value;
                while (itemsList.items.size() > limit) {
                    itemsList.items.remove(itemsList.items.size() - 1);
                }
            }
            
            return itemsList;
        }
        
        /**
         * List elements with positive counts.
         */
        public $O elements() {
            // Return a generator-like iterator
            return new Counter_elements_iterator(this);
        }
        
        /**
         * Subtract counts.
         */
        public void subtract($O iterable) {
            if (iterable instanceof $D) {
                $D d = ($D)iterable;
                for ($O key : d.keys().items) {
                    $O count = d.__getitem__(key);
                    $O existing = __getitem__(key);
                    __setitem__(key, existing.__sub__(count));
                }
            } else {
                $O iter = iterable.__iter__();
                while (true) {
                    try {
                        $O item = iter.__next__();
                        $O count = __getitem__(item);
                        __setitem__(item, count.__sub__($I.of(1)));
                    } catch ($X e) {
                        if (e.isStopIteration()) break;
                        throw e;
                    }
                }
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("Counter(" + dict.__repr__().value + ")");
        }
        
        @Override
        public boolean __bool__() {
            return dict.__bool__();
        }
    }
    
    /**
     * Iterator for Counter.elements().
     */
    static class Counter_elements_iterator extends $O {
        private Counter counter;
        private Iterator<$O> keyIter;
        private $O currentKey;
        private int remaining;
        
        Counter_elements_iterator(Counter counter) {
            this.counter = counter;
            this.keyIter = counter.keys().items.iterator();
            this.remaining = 0;
        }
        
        @Override
        public $O __iter__() {
            return this;
        }
        
        @Override
        public $O __next__() {
            while (remaining <= 0) {
                if (!keyIter.hasNext()) {
                    throw new $X.StopIteration();
                }
                currentKey = keyIter.next();
                $O count = counter.__getitem__(currentKey);
                remaining = count instanceof $I ? (int)(($I)count).value : 0;
            }
            remaining--;
            return currentKey;
        }
    }
    
    /**
     * Factory function for deque.
     */
    public static $O deque_new($O... args) {
        if (args.length == 0) {
            return new deque();
        } else if (args.length == 1) {
            return new deque(args[0]);
        } else {
            return new deque(args[0], args[1]);
        }
    }
    
    /**
     * Factory function for defaultdict.
     */
    public static $O defaultdict_new($O... args) {
        if (args.length == 0) {
            return new defaultdict();
        } else if (args.length == 1) {
            return new defaultdict(args[0]);
        } else {
            return new defaultdict(args[0], args[1]);
        }
    }
    
    /**
     * Factory function for Counter.
     */
    public static $O Counter_new($O... args) {
        if (args.length == 0) {
            return new Counter();
        } else {
            return new Counter(args[0]);
        }
    }
}
