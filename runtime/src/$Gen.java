/**
 * $Gen - Generator object for Python generator functions.
 * 
 * Generator functions (functions containing yield) return a generator object
 * when called. The generator implements the iterator protocol (__iter__, __next__).
 * 
 * For simple generators, we use eager evaluation into a list and iterate over it.
 * For complex generators with state, subclasses override the step() method.
 */
public class $Gen extends $O {
    
    /** State values */
    protected static final int STATE_START = 0;
    protected static final int STATE_DONE = -1;
    
    /** Current state (yield point) */
    protected int state = STATE_START;
    
    /** Yielded values (for simple eager evaluation) */
    protected $L values;
    protected $O valuesIter;
    
    /** Last yielded value (for send support later) */
    protected $O lastValue;
    
    /**
     * Default constructor for subclasses implementing step().
     */
    protected $Gen() {
        // Subclasses will override step()
    }
    
    /**
     * Constructor for eager-evaluated generators.
     * @param values Pre-computed list of values to yield
     */
    public $Gen($L values) {
        this.values = values;
        this.valuesIter = values.__iter__();
    }
    
    /**
     * Step the generator to the next yield point.
     * Subclasses override this for state machine behavior.
     * @return The value to yield, or null if done
     */
    protected $O step() {
        // Default implementation for eager evaluation
        if (valuesIter != null) {
            try {
                return valuesIter.__next__();
            } catch ($X e) {
                if (e.isStopIteration()) {
                    state = STATE_DONE;
                    return null;
                }
                throw e;
            }
        }
        state = STATE_DONE;
        return null;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (state == STATE_DONE) {
            throw new $X("StopIteration", "");
        }
        
        $O value = step();
        if (value == null && state == STATE_DONE) {
            throw new $X("StopIteration", "");
        }
        
        lastValue = value;
        return value;
    }
    
    /**
     * Send a value into the generator (PEP 342).
     * For now, this just advances and ignores the value.
     */
    public $O send($O value) {
        return __next__();
    }
    
    /**
     * Throw an exception into the generator.
     */
    public $O throwGen($X exc) {
        state = STATE_DONE;
        throw exc;
    }
    
    /**
     * Close the generator.
     */
    public void close() {
        state = STATE_DONE;
    }
    
    @Override
    public boolean __bool__() {
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
     * Factory for simple generators with pre-computed values.
     */
    public static $Gen of($O... values) {
        $L list = new $L();
        for ($O v : values) {
            list.append(v);
        }
        return new $Gen(list);
    }
}
