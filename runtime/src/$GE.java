/**
 * $GE - Generator Expression iterator.
 * 
 * Implements lazy evaluation for generator expressions like:
 *   (x*2 for x in nums)
 *   (x for x in nums if x > 0)
 * 
 * Takes:
 *   - source: the iterable to iterate over
 *   - mapper: function to transform each element (the expression)
 *   - filters: optional filter functions (the if conditions)
 */
public final class $GE extends $O {
    
    private $O sourceIter;      // Iterator from source
    private final $MH mapper;   // Element expression as callable
    private final $MH[] filters; // Filter conditions as callables
    private $O nextValue;       // Prefetched next value (null if exhausted)
    private boolean exhausted;
    
    /**
     * Create a generator expression.
     * @param source The iterable to iterate over
     * @param mapper The element expression (callable taking one arg)
     * @param filters Array of filter functions (may be empty)
     */
    public $GE($O source, $MH mapper, $MH[] filters) {
        this.mapper = mapper;
        this.filters = filters != null ? filters : new $MH[0];
        this.exhausted = false;
        
        // Get iterator from source
        this.sourceIter = source.__iter__();
        
        // Prefetch first value
        advance();
    }
    
    /**
     * Convenience constructor with no filters.
     */
    public $GE($O source, $MH mapper) {
        this(source, mapper, new $MH[0]);
    }
    
    /**
     * Advance to the next valid element (that passes all filters).
     */
    private void advance() {
        if (exhausted) {
            return;
        }
        
        while (true) {
            // Get next from source iterator
            $O item;
            try {
                item = sourceIter.__next__();
            } catch ($X e) {
                if ("StopIteration".equals(e.type)) {
                    exhausted = true;
                    nextValue = null;
                    return;
                }
                throw e;
            }
            
            // Handle null return (alternate way of signaling end)
            if (item == null) {
                exhausted = true;
                nextValue = null;
                return;
            }
            
            // Check all filters
            boolean passesFilters = true;
            for ($MH filter : filters) {
                $O result = filter.__call__(item);
                if (!result.__bool__()) {
                    passesFilters = false;
                    break;
                }
            }
            
            if (passesFilters) {
                // Apply mapper and save
                nextValue = mapper.__call__(item);
                return;
            }
            // Otherwise, continue to next item
        }
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (exhausted || nextValue == null) {
            throw new $X.StopIteration();
        }
        
        $O result = nextValue;
        advance();
        return result;
    }
    
    @Override
    public boolean __bool__() {
        // Generator is truthy (always, like Python)
        return true;
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<generator object>");
    }
    
    @Override
    public $S __str__() {
        return __repr__();
    }
    
    /**
     * Factory method for simple generator (no filters).
     */
    public static $GE of($O source, $MH mapper) {
        return new $GE(source, mapper);
    }
    
    /**
     * Factory method with single filter.
     */
    public static $GE of($O source, $MH mapper, $MH filter) {
        return new $GE(source, mapper, new $MH[] { filter });
    }
    
    /**
     * Factory method with multiple filters.
     */
    public static $GE of($O source, $MH mapper, $MH[] filters) {
        return new $GE(source, mapper, filters);
    }
}
