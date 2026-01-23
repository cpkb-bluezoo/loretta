/**
 * $SL - Slice object for slicing operations.
 */
public final class $SL extends $O {
    
    public final $O start;
    public final $O stop;
    public final $O step;
    
    public $SL($O start, $O stop, $O step) {
        this.start = start;
        this.stop = stop;
        this.step = step;
    }
    
    @Override
    public $S __repr__() {
        return $S.of("slice(" + start.__repr__().value + ", " + 
                     stop.__repr__().value + ", " + step.__repr__().value + ")");
    }
    
    @Override
    public $I __hash__() {
        throw new $X("TypeError", "unhashable type: 'slice'");
    }
    
    @Override
    public $O __eq__($O other) {
        if (!(other instanceof $SL)) return $B.FALSE;
        $SL o = ($SL) other;
        $O eq1 = start.__eq__(o.start);
        $O eq2 = stop.__eq__(o.stop);
        $O eq3 = step.__eq__(o.step);
        boolean b1 = eq1 instanceof $B && (($B)eq1).boolValue;
        boolean b2 = eq2 instanceof $B && (($B)eq2).boolValue;
        boolean b3 = eq3 instanceof $B && (($B)eq3).boolValue;
        return $B.of(b1 && b2 && b3);
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "start": return start;
            case "stop": return stop;
            case "step": return step;
            default: return super.__getattr__(name);
        }
    }
    
    /**
     * Compute indices for a sequence of given length.
     * Returns (start, stop, step) tuple for range iteration.
     */
    public $T indices($O length) {
        if (!(length instanceof $I)) {
            throw new $X("TypeError", "slice indices must be integers");
        }
        long len = (($I)length).value;
        
        long stepVal = 1;
        if (step != $N.INSTANCE) {
            if (!(step instanceof $I)) throw new $X("TypeError", "slice step must be int");
            stepVal = (($I)step).value;
            if (stepVal == 0) throw new $X("ValueError", "slice step cannot be zero");
        }
        
        long startVal, stopVal;
        
        if (stepVal > 0) {
            // Forward iteration
            if (start == $N.INSTANCE) {
                startVal = 0;
            } else {
                startVal = (($I)start).value;
                if (startVal < 0) startVal = Math.max(0, len + startVal);
                else startVal = Math.min(len, startVal);
            }
            
            if (stop == $N.INSTANCE) {
                stopVal = len;
            } else {
                stopVal = (($I)stop).value;
                if (stopVal < 0) stopVal = Math.max(0, len + stopVal);
                else stopVal = Math.min(len, stopVal);
            }
        } else {
            // Backward iteration
            if (start == $N.INSTANCE) {
                startVal = len - 1;
            } else {
                startVal = (($I)start).value;
                if (startVal < 0) startVal = Math.max(-1, len + startVal);
                else startVal = Math.min(len - 1, startVal);
            }
            
            if (stop == $N.INSTANCE) {
                stopVal = -1;
            } else {
                stopVal = (($I)stop).value;
                if (stopVal < 0) stopVal = Math.max(-1, len + stopVal);
                else stopVal = Math.min(len - 1, stopVal);
            }
        }
        
        return $T.of($I.of(startVal), $I.of(stopVal), $I.of(stepVal));
    }
}
