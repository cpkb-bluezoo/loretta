import java.util.Scanner;
import java.util.HashMap;
import java.util.Map;

/**
 * $G - Global/builtin function lookup.
 * 
 * Provides access to Python built-in functions, user-defined globals, and modules.
 */
public final class $G {
    
    private static final Scanner scanner = new Scanner(System.in);
    
    /** User-defined global variables */
    private static final Map<String, $O> globals = new HashMap<>();
    
    /** Loaded modules registry */
    private static final Map<String, $Mod> modules = new HashMap<>();
    
    /**
     * Set a global variable.
     */
    public static void setGlobal(String name, $O value) {
        globals.put(name, value);
    }
    
    /**
     * Get a global variable, returning null if not found.
     */
    public static $O getGlobal(String name) {
        return globals.get(name);
    }
    
    /**
     * Import a module by name.
     * First checks if already loaded, then attempts to load the Java class.
     */
    public static $Mod importModule(String name) {
        // Check if already loaded
        $Mod existing = modules.get(name);
        if (existing != null) {
            return existing;
        }
        
        // Convert Python module name to Java class name
        // foo.bar -> foo$bar (using $ as separator in class name)
        String className = name.replace(".", "$");
        
        try {
            // Try to load the class
            Class<?> clazz = Class.forName(className);
            $Mod mod = new $Mod(name, clazz);
            modules.put(name, mod);
            
            // Save current globals, clear them, run module, collect module globals
            Map<String, $O> savedGlobals = new HashMap<>(globals);
            globals.clear();
            
            mod.initialize();
            
            // Copy all globals created by module to the module object
            for (Map.Entry<String, $O> entry : globals.entrySet()) {
                mod.setAttr(entry.getKey(), entry.getValue());
            }
            
            // Restore original globals
            globals.clear();
            globals.putAll(savedGlobals);
            
            return mod;
        } catch (ClassNotFoundException e) {
            throw new $X("ModuleNotFoundError: No module named '" + name + "'");
        }
    }
    
    /**
     * Get a module from the registry without loading.
     */
    public static $Mod getModule(String name) {
        $Mod mod = modules.get(name);
        if (mod == null) {
            throw new $X("ModuleNotFoundError: No module named '" + name + "'");
        }
        return mod;
    }
    
    /**
     * Register a module (used by module's own initialization).
     */
    public static void registerModule(String name, $Mod mod) {
        modules.put(name, mod);
    }
    
    /**
     * Check if a module is loaded.
     */
    public static boolean hasModule(String name) {
        return modules.containsKey(name);
    }
    
    /**
     * Look up a builtin function by name.
     * Also checks user-defined globals.
     */
    public static $O builtin(String name) {
        // First check user-defined globals
        $O global = globals.get(name);
        if (global != null) {
            return global;
        }
        
        // Then check builtins
        switch (name) {
            case "print": return PRINT;
            case "input": return INPUT;
            case "len": return LEN;
            case "range": return RANGE;
            case "int": return INT;
            case "float": return FLOAT;
            case "str": return STR;
            case "bool": return BOOL;
            case "list": return LIST;
            case "tuple": return TUPLE;
            case "dict": return DICT;
            case "type": return TYPE;
            case "isinstance": return ISINSTANCE;
            case "abs": return ABS;
            case "min": return MIN;
            case "max": return MAX;
            case "sum": return SUM;
            case "repr": return REPR;
            case "ord": return ORD;
            case "chr": return CHR;
            case "hex": return HEX;
            case "bin": return BIN;
            case "oct": return OCT;
            case "hash": return HASH;
            case "id": return ID;
            case "iter": return ITER;
            case "next": return NEXT;
            case "sorted": return SORTED;
            case "reversed": return REVERSED;
            case "enumerate": return ENUMERATE;
            case "zip": return ZIP;
            case "map": return MAP;
            case "filter": return FILTER;
            case "all": return ALL;
            case "any": return ANY;
            case "callable": return CALLABLE;
            case "getattr": return GETATTR;
            case "setattr": return SETATTR;
            case "hasattr": return HASATTR;
            case "round": return ROUND;
            case "pow": return POW;
            case "divmod": return DIVMOD;
            case "set": return SET;
            case "frozenset": return FROZENSET;
            case "bytes": return BYTES;
            case "bytearray": return BYTEARRAY;
            case "complex": return COMPLEX;
            case "open": return OPEN;
            case "format": return FORMAT;
            case "ascii": return ASCII;
            case "slice": return SLICE;
            case "object": return OBJECT;
            case "dir": return DIR;
            case "vars": return VARS;
            case "globals": return GLOBALS;
            case "locals": return LOCALS;
            case "issubclass": return ISSUBCLASS;
            case "delattr": return DELATTR;
            case "__import__": return __IMPORT__;
            // Constants
            case "True": return $B.TRUE;
            case "False": return $B.FALSE;
            case "None": return $N.INSTANCE;
            default:
                throw new $X("NameError", "name '" + name + "' is not defined");
        }
    }
    
    // Built-in function objects
    
    public static final $O PRINT = new $O() {
        @Override
        public $O __call__($O... args) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < args.length; i++) {
                if (i > 0) sb.append(' ');
                sb.append(args[i].__str__().value);
            }
            System.out.println(sb.toString());
            return $N.INSTANCE;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function print>"); }
    };
    
    public static final $O INPUT = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length > 0) {
                System.out.print(args[0].__str__().value);
            }
            return $S.of(scanner.nextLine());
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function input>"); }
    };
    
    public static final $O LEN = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "len() takes exactly one argument");
            return args[0].__len__();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function len>"); }
    };
    
    public static final $O RANGE = new $O() {
        @Override
        public $O __call__($O... args) {
            long start, stop, step;
            if (args.length == 1) {
                start = 0;
                stop = toInt(args[0]);
                step = 1;
            } else if (args.length == 2) {
                start = toInt(args[0]);
                stop = toInt(args[1]);
                step = 1;
            } else if (args.length == 3) {
                start = toInt(args[0]);
                stop = toInt(args[1]);
                step = toInt(args[2]);
                if (step == 0) throw new $X("ValueError", "range() arg 3 must not be zero");
            } else {
                throw new $X("TypeError", "range expected 1-3 arguments");
            }
            return new $R(start, stop, step);
        }
        private long toInt($O o) {
            if (o instanceof $I) {
                $I i = ($I) o;
                if (i.big != null) throw new $X("OverflowError", "range() integer too large");
                return i.value;
            }
            throw new $X("TypeError", "range() integer expected");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'range'>"); }
    };
    
    public static final $O INT = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $I.of(0);
            if (args.length > 1) throw new $X("TypeError", "int() takes at most 1 argument");
            $O arg = args[0];
            if (arg instanceof $I) return arg;
            if (arg instanceof $F) return $I.of((long)(($F)arg).value);
            if (arg instanceof $B) return $I.of((($B)arg).boolValue ? 1 : 0);
            if (arg instanceof $S) return $I.of((($S)arg).value);
            throw new $X("TypeError", "int() argument must be a string or number");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'int'>"); }
    };
    
    public static final $O FLOAT = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $F.of(0.0);
            if (args.length > 1) throw new $X("TypeError", "float() takes at most 1 argument");
            $O arg = args[0];
            if (arg instanceof $F) return arg;
            if (arg instanceof $I) {
                $I i = ($I) arg;
                return $F.of(i.big != null ? i.big.doubleValue() : i.value);
            }
            if (arg instanceof $S) return $F.of((($S)arg).value);
            throw new $X("TypeError", "float() argument must be a string or number");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'float'>"); }
    };
    
    public static final $O STR = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $S.of("");
            if (args.length > 1) throw new $X("TypeError", "str() takes at most 1 argument");
            return args[0].__str__();
        }
        @Override
        public $S __repr__() { return $S.of("<class 'str'>"); }
    };
    
    public static final $O BOOL = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $B.FALSE;
            if (args.length > 1) throw new $X("TypeError", "bool() takes at most 1 argument");
            return $B.of(args[0].__bool__());
        }
        @Override
        public $S __repr__() { return $S.of("<class 'bool'>"); }
    };
    
    public static final $O LIST = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return new $L();
            if (args.length > 1) throw new $X("TypeError", "list() takes at most 1 argument");
            $O iterable = args[0];
            $L result = new $L();
            $O iter = iterable.__iter__();
            while (true) {
                try {
                    result.items.add(iter.__next__());
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            return result;
        }
        @Override
        public $S __repr__() { return $S.of("<class 'list'>"); }
    };
    
    public static final $O TUPLE = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $T.EMPTY;
            if (args.length > 1) throw new $X("TypeError", "tuple() takes at most 1 argument");
            $O iterable = args[0];
            java.util.List<$O> items = new java.util.ArrayList<>();
            $O iter = iterable.__iter__();
            while (true) {
                try {
                    items.add(iter.__next__());
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            return $T.of(items.toArray(new $O[0]));
        }
        @Override
        public $S __repr__() { return $S.of("<class 'tuple'>"); }
    };
    
    public static final $O DICT = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return new $D();
            throw new $X("TypeError", "dict() not fully implemented");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'dict'>"); }
    };
    
    public static final $O TYPE = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "type() takes 1 argument");
            $O obj = args[0];
            if (obj instanceof $I) return $S.of("<class 'int'>");
            if (obj instanceof $F) return $S.of("<class 'float'>");
            if (obj instanceof $S) return $S.of("<class 'str'>");
            if (obj instanceof $B) return $S.of("<class 'bool'>");
            if (obj instanceof $N) return $S.of("<class 'NoneType'>");
            if (obj instanceof $L) return $S.of("<class 'list'>");
            if (obj instanceof $T) return $S.of("<class 'tuple'>");
            if (obj instanceof $D) return $S.of("<class 'dict'>");
            return $S.of("<class 'object'>");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'type'>"); }
    };
    
    public static final $O ISINSTANCE = new $O() {
        @Override
        public $O __call__($O... args) {
            // Simplified - just check type name
            if (args.length != 2) throw new $X("TypeError", "isinstance() takes 2 arguments");
            return $B.FALSE; // TODO: proper implementation
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function isinstance>"); }
    };
    
    public static final $O ABS = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "abs() takes exactly one argument");
            $O arg = args[0];
            if (arg instanceof $I) {
                $I i = ($I) arg;
                if (i.big != null) return $I.of(i.big.abs());
                return $I.of(Math.abs(i.value));
            }
            if (arg instanceof $F) return $F.of(Math.abs((($F)arg).value));
            throw new $X("TypeError", "bad operand type for abs()");
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function abs>"); }
    };
    
    public static final $O MIN = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) throw new $X("TypeError", "min expected 1 argument");
            $O result = args[0];
            for (int i = 1; i < args.length; i++) {
                $O cmp = args[i].__lt__(result);
                if (cmp instanceof $B && (($B)cmp).boolValue) {
                    result = args[i];
                }
            }
            return result;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function min>"); }
    };
    
    public static final $O MAX = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) throw new $X("TypeError", "max expected 1 argument");
            $O result = args[0];
            for (int i = 1; i < args.length; i++) {
                $O cmp = args[i].__gt__(result);
                if (cmp instanceof $B && (($B)cmp).boolValue) {
                    result = args[i];
                }
            }
            return result;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function max>"); }
    };
    
    public static final $O SUM = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) throw new $X("TypeError", "sum() takes at least 1 argument");
            $O iterable = args[0];
            $O result = args.length > 1 ? args[1] : $I.of(0);
            $O iter = iterable.__iter__();
            while (true) {
                try {
                    result = result.__add__(iter.__next__());
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            return result;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function sum>"); }
    };
    
    public static final $O REPR = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "repr() takes exactly one argument");
            return args[0].__repr__();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function repr>"); }
    };
    
    public static final $O ORD = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "ord() takes exactly one argument");
            if (!(args[0] instanceof $S)) throw new $X("TypeError", "ord() expected string of length 1");
            String s = (($S)args[0]).value;
            if (s.length() != 1) throw new $X("TypeError", "ord() expected string of length 1");
            return $I.of(s.charAt(0));
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function ord>"); }
    };
    
    public static final $O CHR = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "chr() takes exactly one argument");
            if (!(args[0] instanceof $I)) throw new $X("TypeError", "chr() expected int");
            long v = (($I)args[0]).value;
            if (v < 0 || v > 0x10FFFF) throw new $X("ValueError", "chr() arg not in range(0x110000)");
            return $S.of(String.valueOf((char)v));
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function chr>"); }
    };
    
    public static final $O HEX = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "hex() takes exactly one argument");
            if (!(args[0] instanceof $I)) throw new $X("TypeError", "hex() expected int");
            $I i = ($I) args[0];
            String s = i.big != null ? i.big.toString(16) : Long.toHexString(i.value);
            return $S.of((i.value < 0 || (i.big != null && i.big.signum() < 0)) ? "-0x" + s.substring(1) : "0x" + s);
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function hex>"); }
    };
    
    public static final $O BIN = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "bin() takes exactly one argument");
            if (!(args[0] instanceof $I)) throw new $X("TypeError", "bin() expected int");
            $I i = ($I) args[0];
            String s = i.big != null ? i.big.toString(2) : Long.toBinaryString(i.value);
            return $S.of((i.value < 0 || (i.big != null && i.big.signum() < 0)) ? "-0b" + s.substring(1) : "0b" + s);
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function bin>"); }
    };
    
    public static final $O OCT = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "oct() takes exactly one argument");
            if (!(args[0] instanceof $I)) throw new $X("TypeError", "oct() expected int");
            $I i = ($I) args[0];
            String s = i.big != null ? i.big.toString(8) : Long.toOctalString(i.value);
            return $S.of((i.value < 0 || (i.big != null && i.big.signum() < 0)) ? "-0o" + s.substring(1) : "0o" + s);
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function oct>"); }
    };
    
    public static final $O HASH = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "hash() takes exactly one argument");
            return args[0].__hash__();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function hash>"); }
    };
    
    public static final $O ID = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "id() takes exactly one argument");
            return $I.of(System.identityHashCode(args[0]));
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function id>"); }
    };
    
    public static final $O ITER = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "iter() takes exactly one argument");
            return args[0].__iter__();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function iter>"); }
    };
    
    public static final $O NEXT = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1 || args.length > 2) throw new $X("TypeError", "next() takes 1 or 2 arguments");
            try {
                return args[0].__next__();
            } catch ($X e) {
                if (e.isStopIteration() && args.length == 2) {
                    return args[1];  // default value
                }
                throw e;
            }
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function next>"); }
    };
    
    public static final $O SORTED = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "sorted() takes exactly one argument");
            // Simple bubble sort for now
            $L list = ($L) LIST.__call__(args[0]);
            java.util.List<$O> items = list.items;
            for (int i = 0; i < items.size() - 1; i++) {
                for (int j = 0; j < items.size() - i - 1; j++) {
                    $O cmp = items.get(j).__gt__(items.get(j + 1));
                    if (cmp instanceof $B && (($B)cmp).boolValue) {
                        $O temp = items.get(j);
                        items.set(j, items.get(j + 1));
                        items.set(j + 1, temp);
                    }
                }
            }
            return list;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function sorted>"); }
    };
    
    public static final $O REVERSED = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "reversed() takes exactly one argument");
            $L list = ($L) LIST.__call__(args[0]);
            list.reverse();
            return list.__iter__();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function reversed>"); }
    };
    
    public static final $O ENUMERATE = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1) throw new $X("TypeError", "enumerate() takes at least 1 argument");
            long start = args.length > 1 ? (($I)args[1]).value : 0;
            return new $EN(args[0].__iter__(), start);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'enumerate'>"); }
    };
    
    public static final $O ZIP = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $T.EMPTY.__iter__();
            $O[] iters = new $O[args.length];
            for (int i = 0; i < args.length; i++) {
                iters[i] = args[i].__iter__();
            }
            return new $ZI(iters);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'zip'>"); }
    };
    
    public static final $O MAP = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 2) throw new $X("TypeError", "map() requires at least 2 arguments");
            $O func = args[0];
            $O[] iters = new $O[args.length - 1];
            for (int i = 1; i < args.length; i++) {
                iters[i - 1] = args[i].__iter__();
            }
            return new $MI(func, iters);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'map'>"); }
    };
    
    public static final $O FILTER = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 2) throw new $X("TypeError", "filter() takes exactly 2 arguments");
            return new $FI(args[0], args[1].__iter__());
        }
        @Override
        public $S __repr__() { return $S.of("<class 'filter'>"); }
    };
    
    public static final $O ALL = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "all() takes exactly one argument");
            $O iter = args[0].__iter__();
            while (true) {
                try {
                    if (!iter.__next__().__bool__()) return $B.FALSE;
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            return $B.TRUE;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function all>"); }
    };
    
    public static final $O ANY = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "any() takes exactly one argument");
            $O iter = args[0].__iter__();
            while (true) {
                try {
                    if (iter.__next__().__bool__()) return $B.TRUE;
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            return $B.FALSE;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function any>"); }
    };
    
    public static final $O CALLABLE = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "callable() takes exactly one argument");
            // Check if it has __call__ that doesn't throw
            try {
                args[0].getClass().getMethod("__call__", $O[].class);
                return $B.TRUE;
            } catch (NoSuchMethodException e) {
                return $B.FALSE;
            }
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function callable>"); }
    };
    
    public static final $O GETATTR = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 2 || args.length > 3) throw new $X("TypeError", "getattr() takes 2 or 3 arguments");
            try {
                return args[0].__getattr__((($S)args[1]).value);
            } catch ($X e) {
                if (args.length == 3) return args[2];
                throw e;
            }
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function getattr>"); }
    };
    
    public static final $O SETATTR = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 3) throw new $X("TypeError", "setattr() takes exactly 3 arguments");
            args[0].__setattr__((($S)args[1]).value, args[2]);
            return $N.INSTANCE;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function setattr>"); }
    };
    
    public static final $O HASATTR = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 2) throw new $X("TypeError", "hasattr() takes exactly 2 arguments");
            try {
                args[0].__getattr__((($S)args[1]).value);
                return $B.TRUE;
            } catch ($X e) {
                return $B.FALSE;
            }
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function hasattr>"); }
    };
    
    public static final $O ROUND = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1 || args.length > 2) throw new $X("TypeError", "round() takes 1 or 2 arguments");
            $O num = args[0];
            int ndigits = args.length > 1 ? (int)(($I)args[1]).value : 0;
            if (num instanceof $F) {
                double v = (($F)num).value;
                double factor = Math.pow(10, ndigits);
                return $F.of(Math.round(v * factor) / factor);
            }
            if (num instanceof $I) {
                if (ndigits >= 0) return num;
                // For negative ndigits, round to nearest 10^(-ndigits)
                long factor = (long)Math.pow(10, -ndigits);
                return $I.of(Math.round((double)(($I)num).value / factor) * factor);
            }
            throw new $X("TypeError", "type has no __round__ method");
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function round>"); }
    };
    
    public static final $O POW = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 2 || args.length > 3) throw new $X("TypeError", "pow() takes 2 or 3 arguments");
            if (args.length == 3) {
                // pow(base, exp, mod)
                if (!(args[0] instanceof $I) || !(args[1] instanceof $I) || !(args[2] instanceof $I)) {
                    throw new $X("TypeError", "pow() 3rd argument not allowed unless all arguments are integers");
                }
                return $I.of((($I)args[0]).toBigInteger().modPow(
                    (($I)args[1]).toBigInteger(), (($I)args[2]).toBigInteger()));
            }
            return args[0].__pow__(args[1]);
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function pow>"); }
    };
    
    public static final $O DIVMOD = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 2) throw new $X("TypeError", "divmod() takes exactly 2 arguments");
            return $T.of(args[0].__floordiv__(args[1]), args[0].__mod__(args[1]));
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function divmod>"); }
    };
    
    public static final $O SET = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return new $ST();
            if (args.length > 1) throw new $X("TypeError", "set() takes at most 1 argument");
            return $ST.fromIterable(args[0]);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'set'>"); }
    };
    
    public static final $O FROZENSET = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $FS.EMPTY;
            if (args.length > 1) throw new $X("TypeError", "frozenset() takes at most 1 argument");
            return $FS.fromIterable(args[0]);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'frozenset'>"); }
    };
    
    public static final $O BYTES = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $BY.of(new byte[0]);
            if (args.length == 1) {
                $O arg = args[0];
                if (arg instanceof $S) {
                    throw new $X("TypeError", "string argument without encoding");
                }
                if (arg instanceof $I) {
                    int size = (int)(($I)arg).value;
                    return $BY.of(new byte[size]);
                }
                if (arg instanceof $BY) return arg;
                // Iterable of ints
                java.util.List<Byte> bytes = new java.util.ArrayList<>();
                $O iter = arg.__iter__();
                while (true) {
                    try {
                        $O item = iter.__next__();
                        if (!(item instanceof $I)) throw new $X("TypeError", "bytes element must be int");
                        int v = (int)(($I)item).value;
                        if (v < 0 || v > 255) throw new $X("ValueError", "bytes must be in range(0, 256)");
                        bytes.add((byte)v);
                    } catch ($X e) {
                        if (e.isStopIteration()) break;
                        throw e;
                    }
                }
                byte[] arr = new byte[bytes.size()];
                for (int i = 0; i < bytes.size(); i++) arr[i] = bytes.get(i);
                return $BY.of(arr);
            }
            if (args.length >= 2 && args[0] instanceof $S) {
                String s = (($S)args[0]).value;
                String enc = args.length > 1 ? (($S)args[1]).value : "utf-8";
                try {
                    return $BY.of(s.getBytes(enc));
                } catch (java.io.UnsupportedEncodingException e) {
                    throw new $X("LookupError", "unknown encoding: " + enc);
                }
            }
            throw new $X("TypeError", "bytes() takes at most 3 arguments");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'bytes'>"); }
    };
    
    public static final $O BYTEARRAY = new $O() {
        @Override
        public $O __call__($O... args) {
            // For now, bytearray is same as bytes (simplified)
            return BYTES.__call__(args);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'bytearray'>"); }
    };
    
    public static final $O COMPLEX = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) return $C.of(0.0, 0.0);
            if (args.length == 1) {
                $O arg = args[0];
                if (arg instanceof $C) return arg;
                if (arg instanceof $I) {
                    $I i = ($I) arg;
                    return $C.of(i.big != null ? i.big.doubleValue() : i.value, 0.0);
                }
                if (arg instanceof $F) return $C.of((($F)arg).value, 0.0);
                if (arg instanceof $S) {
                    String s = (($S)arg).value.trim().replace(" ", "");
                    // Simple parsing: a+bj or bj
                    try {
                        if (s.endsWith("j") || s.endsWith("J")) {
                            s = s.substring(0, s.length() - 1);
                            int plusIdx = s.lastIndexOf('+');
                            int minusIdx = s.lastIndexOf('-');
                            int idx = Math.max(plusIdx, minusIdx);
                            if (idx <= 0) {
                                return $C.of(0.0, s.isEmpty() ? 1.0 : Double.parseDouble(s));
                            }
                            return $C.of(Double.parseDouble(s.substring(0, idx)),
                                        Double.parseDouble(s.substring(idx)));
                        }
                        return $C.of(Double.parseDouble(s), 0.0);
                    } catch (NumberFormatException e) {
                        throw new $X("ValueError", "complex() arg is malformed string");
                    }
                }
                throw new $X("TypeError", "complex() first argument must be string or number");
            }
            if (args.length == 2) {
                double real = 0, imag = 0;
                if (args[0] instanceof $I) real = (($I)args[0]).big != null ? (($I)args[0]).big.doubleValue() : (($I)args[0]).value;
                else if (args[0] instanceof $F) real = (($F)args[0]).value;
                else throw new $X("TypeError", "complex() argument must be number");
                
                if (args[1] instanceof $I) imag = (($I)args[1]).big != null ? (($I)args[1]).big.doubleValue() : (($I)args[1]).value;
                else if (args[1] instanceof $F) imag = (($F)args[1]).value;
                else throw new $X("TypeError", "complex() argument must be number");
                
                return $C.of(real, imag);
            }
            throw new $X("TypeError", "complex() takes at most 2 arguments");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'complex'>"); }
    };
    
    public static final $O OPEN = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1) throw new $X("TypeError", "open() requires at least 1 argument");
            String filename = (($S)args[0]).value;
            String mode = args.length > 1 ? (($S)args[1]).value : "r";
            try {
                return new $File(filename, mode);
            } catch (java.io.IOException e) {
                throw new $X("FileNotFoundError", e.getMessage());
            }
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function open>"); }
    };
    
    public static final $O FORMAT = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1) throw new $X("TypeError", "format() takes at least 1 argument");
            if (args.length == 1) return args[0].__str__();
            // With format spec (simplified)
            String spec = (($S)args[1]).value;
            if (spec.isEmpty()) return args[0].__str__();
            // Basic formatting
            $O val = args[0];
            if (val instanceof $I || val instanceof $F) {
                try {
                    return $S.of(String.format("%" + spec, 
                        val instanceof $I ? (($I)val).value : (($F)val).value));
                } catch (Exception e) {
                    return val.__str__();
                }
            }
            return val.__str__();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function format>"); }
    };
    
    public static final $O ASCII = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "ascii() takes exactly one argument");
            String s = args[0].__repr__().value;
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < s.length(); i++) {
                char c = s.charAt(i);
                if (c < 128) {
                    sb.append(c);
                } else {
                    sb.append(String.format("\\u%04x", (int)c));
                }
            }
            return $S.of(sb.toString());
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function ascii>"); }
    };
    
    public static final $O SLICE = new $O() {
        @Override
        public $O __call__($O... args) {
            // slice(stop) or slice(start, stop) or slice(start, stop, step)
            $O start = $N.INSTANCE, stop = $N.INSTANCE, step = $N.INSTANCE;
            if (args.length == 1) {
                stop = args[0];
            } else if (args.length == 2) {
                start = args[0];
                stop = args[1];
            } else if (args.length == 3) {
                start = args[0];
                stop = args[1];
                step = args[2];
            }
            return new $SL(start, stop, step);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'slice'>"); }
    };
    
    public static final $O OBJECT = new $O() {
        @Override
        public $O __call__($O... args) {
            return new $O();
        }
        @Override
        public $S __repr__() { return $S.of("<class 'object'>"); }
    };
    
    public static final $O DIR = new $O() {
        @Override
        public $O __call__($O... args) {
            // Simplified: return empty list (proper implementation would use reflection)
            return new $L();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function dir>"); }
    };
    
    public static final $O VARS = new $O() {
        @Override
        public $O __call__($O... args) {
            // Simplified: return empty dict
            return new $D();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function vars>"); }
    };
    
    public static final $O GLOBALS = new $O() {
        @Override
        public $O __call__($O... args) {
            return new $D();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function globals>"); }
    };
    
    public static final $O LOCALS = new $O() {
        @Override
        public $O __call__($O... args) {
            return new $D();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function locals>"); }
    };
    
    public static final $O ISSUBCLASS = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 2) throw new $X("TypeError", "issubclass() takes 2 arguments");
            // Simplified implementation - always false for now
            return $B.FALSE;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function issubclass>"); }
    };
    
    public static final $O DELATTR = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 2) throw new $X("TypeError", "delattr() takes exactly 2 arguments");
            // Simplified - raise AttributeError
            throw new $X("AttributeError", "cannot delete attribute");
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function delattr>"); }
    };
    
    public static final $O __IMPORT__ = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1) throw new $X("TypeError", "__import__() takes at least 1 argument");
            String name = (($S)args[0]).value;
            return importModule(name);
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function __import__>"); }
    };
}

/**
 * Range object.
 */
class $R extends $O {
    final long start, stop, step;
    
    $R(long start, long stop, long step) {
        this.start = start;
        this.stop = stop;
        this.step = step;
    }
    
    @Override
    public $S __repr__() {
        if (step == 1) {
            return $S.of("range(" + start + ", " + stop + ")");
        }
        return $S.of("range(" + start + ", " + stop + ", " + step + ")");
    }
    
    @Override
    public $I __len__() {
        if (step > 0 && start < stop) {
            return $I.of((stop - start + step - 1) / step);
        } else if (step < 0 && start > stop) {
            return $I.of((start - stop - step - 1) / (-step));
        }
        return $I.of(0);
    }
    
    @Override
    public $O __iter__() {
        return new $RI(this);
    }
    
    @Override
    public $O __getitem__($O key) {
        if (key instanceof $I) {
            long idx = (($I)key).value;
            long len = __len__().value;
            if (idx < 0) idx += len;
            if (idx < 0 || idx >= len) {
                throw new $X("IndexError", "range object index out of range");
            }
            return $I.of(start + idx * step);
        }
        throw new $X("TypeError", "range indices must be integers");
    }
    
    @Override
    public $B __contains__($O item) {
        if (!(item instanceof $I)) return $B.FALSE;
        long v = (($I)item).value;
        if (step > 0) {
            if (v < start || v >= stop) return $B.FALSE;
        } else {
            if (v > start || v <= stop) return $B.FALSE;
        }
        return $B.of((v - start) % step == 0);
    }
}

/**
 * Range iterator.
 */
class $RI extends $O {
    private final $R range;
    private long current;
    
    $RI($R range) {
        this.range = range;
        this.current = range.start;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (range.step > 0 && current >= range.stop) {
            throw new $X("StopIteration", "");
        }
        if (range.step < 0 && current <= range.stop) {
            throw new $X("StopIteration", "");
        }
        long result = current;
        current += range.step;
        return $I.of(result);
    }
}

/**
 * Enumerate iterator.
 */
class $EN extends $O {
    private final $O iter;
    private long index;
    
    $EN($O iter, long start) {
        this.iter = iter;
        this.index = start;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        $O item = iter.__next__();
        return $T.of($I.of(index++), item);
    }
}

/**
 * Zip iterator.
 */
class $ZI extends $O {
    private final $O[] iters;
    
    $ZI($O[] iters) {
        this.iters = iters;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        $O[] items = new $O[iters.length];
        for (int i = 0; i < iters.length; i++) {
            items[i] = iters[i].__next__();
        }
        return $T.of(items);
    }
}

/**
 * Map iterator.
 */
class $MI extends $O {
    private final $O func;
    private final $O[] iters;
    
    $MI($O func, $O[] iters) {
        this.func = func;
        this.iters = iters;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        $O[] args = new $O[iters.length];
        for (int i = 0; i < iters.length; i++) {
            args[i] = iters[i].__next__();
        }
        return func.__call__(args);
    }
}

/**
 * Filter iterator.
 */
class $FI extends $O {
    private final $O func;
    private final $O iter;
    
    $FI($O func, $O iter) {
        this.func = func;
        this.iter = iter;
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        while (true) {
            $O item = iter.__next__();
            boolean keep;
            if (func == $N.INSTANCE) {
                keep = item.__bool__();
            } else {
                keep = func.__call__(item).__bool__();
            }
            if (keep) return item;
        }
    }
}
