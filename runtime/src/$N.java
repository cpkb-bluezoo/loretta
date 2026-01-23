/**
 * $N - PyNone, the None singleton.
 */
public final class $N extends $O {
    
    /** The singleton None instance. */
    public static final $N INSTANCE = new $N();
    
    private $N() {}
    
    @Override
    public boolean __bool__() {
        return false;
    }
    
    @Override
    public $S __repr__() {
        return $S.of("None");
    }
    
    @Override
    public $I __hash__() {
        return $I.of(0);
    }
    
    @Override
    public $O __eq__($O other) {
        return $B.of(other == INSTANCE);
    }
    
    @Override
    public $O __ne__($O other) {
        return $B.of(other != INSTANCE);
    }
}
