/**
 * $X - Python exception hierarchy.
 * 
 * This is BaseException, the root of all Python exceptions.
 * Extends RuntimeException so exceptions can be thrown without declaration.
 * 
 * Hierarchy (subset of Python's full hierarchy):
 * 
 * BaseException ($X)
 * ├── SystemExit
 * ├── KeyboardInterrupt
 * ├── GeneratorExit
 * └── Exception
 *     ├── StopIteration
 *     ├── StopAsyncIteration
 *     ├── ArithmeticError
 *     │   └── ZeroDivisionError
 *     ├── AssertionError
 *     ├── AttributeError
 *     ├── EOFError
 *     ├── ImportError
 *     │   └── ModuleNotFoundError
 *     ├── LookupError
 *     │   ├── IndexError
 *     │   └── KeyError
 *     ├── NameError
 *     │   └── UnboundLocalError
 *     ├── OSError
 *     │   ├── FileExistsError
 *     │   ├── FileNotFoundError
 *     │   ├── IsADirectoryError
 *     │   ├── NotADirectoryError
 *     │   └── PermissionError
 *     ├── RuntimeError
 *     │   ├── NotImplementedError
 *     │   └── RecursionError
 *     ├── SyntaxError
 *     ├── TypeError
 *     ├── ValueError
 *     │   └── UnicodeError
 *     └── Warning
 */
public class $X extends RuntimeException {
    
    /** Exception type name (for backward compatibility) */
    public final String type;
    
    /** Exception message */
    public final String message;
    
    /** Exception arguments (tuple) */
    public $T args;
    
    public $X(String message) {
        super(getTypeName() + (message.isEmpty() ? "" : ": " + message));
        this.type = getTypeName();
        this.message = message;
        this.args = $T.of($S.of(message));
    }
    
    public $X() {
        this("");
    }
    
    /**
     * Backward compatibility constructor - creates the appropriate exception subclass.
     * Use specific exception classes directly when possible.
     */
    public $X(String type, String message) {
        super(type + (message.isEmpty() ? "" : ": " + message));
        this.type = type;
        this.message = message;
        this.args = $T.of($S.of(message));
    }
    
    /**
     * Factory method to create typed exception by name.
     * Prefer using specific exception classes directly.
     */
    public static $X create(String type, String message) {
        switch (type) {
            case "TypeError": return new TypeError(message);
            case "ValueError": return new ValueError(message);
            case "AttributeError": return new AttributeError(message);
            case "NameError": return new NameError(message);
            case "IndexError": return new IndexError(message);
            case "KeyError": return new KeyError(message);
            case "RuntimeError": return new RuntimeError(message);
            case "NotImplementedError": return new NotImplementedError(message);
            case "StopIteration": return new StopIteration();
            case "ZeroDivisionError": return new ZeroDivisionError(message);
            case "OverflowError": return new OverflowError(message);
            case "AssertionError": return new AssertionError(message);
            case "ImportError": return new ImportError(message);
            case "ModuleNotFoundError": return new ModuleNotFoundError(message);
            case "OSError": return new OSError(message);
            case "IOError": return new OSError(message);
            case "FileNotFoundError": return new FileNotFoundError(message);
            case "PermissionError": return new PermissionError(message);
            case "EOFError": return new EOFError(message);
            case "SyntaxError": return new SyntaxError(message);
            case "Exception": return new Exception(message);
            case "BaseException": return new $X(message);
            default: return new $X(type, message);
        }
    }
    
    /**
     * Get the Python type name for this exception.
     * Subclasses override this.
     */
    protected static String getTypeName() {
        return "BaseException";
    }
    
    /**
     * Get the type name for this instance.
     */
    public String typeName() {
        return getTypeName();
    }
    
    /**
     * Check if this is a StopIteration exception.
     */
    public boolean isStopIteration() {
        return this instanceof StopIteration;
    }
    
    /**
     * Get the exception as a Python object (for except clause binding).
     */
    public $O asPyObject() {
        return new $XO(this);
    }
    
    /**
     * Get the exception class object for isinstance checks.
     */
    public static $Cls getExceptionClass() {
        return ExceptionClasses.BaseException;
    }
    
    // ========================================================================
    // Direct subclasses of BaseException
    // ========================================================================
    
    public static class SystemExit extends $X {
        public final $O code;
        
        public SystemExit($O code) {
            super(code != null ? code.__str__().value : "");
            this.code = code;
        }
        
        public SystemExit() {
            this($N.INSTANCE);
        }
        
        protected static String getTypeName() { return "SystemExit"; }
        @Override public String typeName() { return "SystemExit"; }
    }
    
    public static class KeyboardInterrupt extends $X {
        public KeyboardInterrupt(String message) { super(message); }
        public KeyboardInterrupt() { super(); }
        protected static String getTypeName() { return "KeyboardInterrupt"; }
        @Override public String typeName() { return "KeyboardInterrupt"; }
    }
    
    public static class GeneratorExit extends $X {
        public GeneratorExit(String message) { super(message); }
        public GeneratorExit() { super(); }
        protected static String getTypeName() { return "GeneratorExit"; }
        @Override public String typeName() { return "GeneratorExit"; }
    }
    
    // ========================================================================
    // Exception - base class for most exceptions
    // ========================================================================
    
    public static class Exception extends $X {
        public Exception(String message) { super(message); }
        public Exception() { super(); }
        protected static String getTypeName() { return "Exception"; }
        @Override public String typeName() { return "Exception"; }
    }
    
    // ========================================================================
    // StopIteration and StopAsyncIteration
    // ========================================================================
    
    public static class StopIteration extends Exception {
        public $O value;
        
        public StopIteration($O value) {
            super("");
            this.value = value;
        }
        
        public StopIteration() {
            this($N.INSTANCE);
        }
        
        protected static String getTypeName() { return "StopIteration"; }
        @Override public String typeName() { return "StopIteration"; }
    }
    
    public static class StopAsyncIteration extends Exception {
        public StopAsyncIteration(String message) { super(message); }
        public StopAsyncIteration() { super(); }
        protected static String getTypeName() { return "StopAsyncIteration"; }
        @Override public String typeName() { return "StopAsyncIteration"; }
    }
    
    // ========================================================================
    // ArithmeticError hierarchy
    // ========================================================================
    
    public static class ArithmeticError extends Exception {
        public ArithmeticError(String message) { super(message); }
        public ArithmeticError() { super(); }
        protected static String getTypeName() { return "ArithmeticError"; }
        @Override public String typeName() { return "ArithmeticError"; }
    }
    
    public static class ZeroDivisionError extends ArithmeticError {
        public ZeroDivisionError(String message) { super(message); }
        public ZeroDivisionError() { super("division by zero"); }
        protected static String getTypeName() { return "ZeroDivisionError"; }
        @Override public String typeName() { return "ZeroDivisionError"; }
    }
    
    public static class OverflowError extends ArithmeticError {
        public OverflowError(String message) { super(message); }
        public OverflowError() { super(); }
        protected static String getTypeName() { return "OverflowError"; }
        @Override public String typeName() { return "OverflowError"; }
    }
    
    // ========================================================================
    // Common exceptions
    // ========================================================================
    
    public static class AssertionError extends Exception {
        public AssertionError(String message) { super(message); }
        public AssertionError() { super(); }
        protected static String getTypeName() { return "AssertionError"; }
        @Override public String typeName() { return "AssertionError"; }
    }
    
    public static class AttributeError extends Exception {
        public AttributeError(String message) { super(message); }
        public AttributeError() { super(); }
        protected static String getTypeName() { return "AttributeError"; }
        @Override public String typeName() { return "AttributeError"; }
    }
    
    public static class EOFError extends Exception {
        public EOFError(String message) { super(message); }
        public EOFError() { super(); }
        protected static String getTypeName() { return "EOFError"; }
        @Override public String typeName() { return "EOFError"; }
    }
    
    public static class TypeError extends Exception {
        public TypeError(String message) { super(message); }
        public TypeError() { super(); }
        protected static String getTypeName() { return "TypeError"; }
        @Override public String typeName() { return "TypeError"; }
    }
    
    public static class ValueError extends Exception {
        public ValueError(String message) { super(message); }
        public ValueError() { super(); }
        protected static String getTypeName() { return "ValueError"; }
        @Override public String typeName() { return "ValueError"; }
    }
    
    public static class UnicodeError extends ValueError {
        public UnicodeError(String message) { super(message); }
        public UnicodeError() { super(); }
        protected static String getTypeName() { return "UnicodeError"; }
        @Override public String typeName() { return "UnicodeError"; }
    }
    
    // ========================================================================
    // ImportError hierarchy
    // ========================================================================
    
    public static class ImportError extends Exception {
        public String name;  // module name that failed to import
        public String path;  // path to the module
        
        public ImportError(String message) { 
            super(message); 
        }
        
        public ImportError(String message, String name, String path) {
            super(message);
            this.name = name;
            this.path = path;
        }
        
        public ImportError() { super(); }
        protected static String getTypeName() { return "ImportError"; }
        @Override public String typeName() { return "ImportError"; }
    }
    
    public static class ModuleNotFoundError extends ImportError {
        public ModuleNotFoundError(String message) { super(message); }
        public ModuleNotFoundError() { super(); }
        protected static String getTypeName() { return "ModuleNotFoundError"; }
        @Override public String typeName() { return "ModuleNotFoundError"; }
    }
    
    // ========================================================================
    // LookupError hierarchy
    // ========================================================================
    
    public static class LookupError extends Exception {
        public LookupError(String message) { super(message); }
        public LookupError() { super(); }
        protected static String getTypeName() { return "LookupError"; }
        @Override public String typeName() { return "LookupError"; }
    }
    
    public static class IndexError extends LookupError {
        public IndexError(String message) { super(message); }
        public IndexError() { super("list index out of range"); }
        protected static String getTypeName() { return "IndexError"; }
        @Override public String typeName() { return "IndexError"; }
    }
    
    public static class KeyError extends LookupError {
        public $O key;
        
        public KeyError($O key) {
            super(key.__repr__().value);
            this.key = key;
        }
        
        public KeyError(String message) { 
            super(message);
            this.key = $S.of(message);
        }
        
        public KeyError() { super(); }
        protected static String getTypeName() { return "KeyError"; }
        @Override public String typeName() { return "KeyError"; }
    }
    
    // ========================================================================
    // NameError hierarchy
    // ========================================================================
    
    public static class NameError extends Exception {
        public String name;
        
        public NameError(String message) { 
            super(message);
        }
        
        public NameError(String message, String name) {
            super(message);
            this.name = name;
        }
        
        public NameError() { super(); }
        protected static String getTypeName() { return "NameError"; }
        @Override public String typeName() { return "NameError"; }
    }
    
    public static class UnboundLocalError extends NameError {
        public UnboundLocalError(String message) { super(message); }
        public UnboundLocalError() { super(); }
        protected static String getTypeName() { return "UnboundLocalError"; }
        @Override public String typeName() { return "UnboundLocalError"; }
    }
    
    // ========================================================================
    // OSError hierarchy
    // ========================================================================
    
    public static class OSError extends Exception {
        public int errno;
        public String strerror;
        public String filename;
        public String filename2;
        
        public OSError(String message) { super(message); }
        
        public OSError(int errno, String strerror) {
            super("[Errno " + errno + "] " + strerror);
            this.errno = errno;
            this.strerror = strerror;
        }
        
        public OSError(int errno, String strerror, String filename) {
            super("[Errno " + errno + "] " + strerror + ": '" + filename + "'");
            this.errno = errno;
            this.strerror = strerror;
            this.filename = filename;
        }
        
        public OSError() { super(); }
        protected static String getTypeName() { return "OSError"; }
        @Override public String typeName() { return "OSError"; }
    }
    
    public static class FileExistsError extends OSError {
        public FileExistsError(String message) { super(message); }
        public FileExistsError() { super(); }
        protected static String getTypeName() { return "FileExistsError"; }
        @Override public String typeName() { return "FileExistsError"; }
    }
    
    public static class FileNotFoundError extends OSError {
        public FileNotFoundError(String message) { super(message); }
        public FileNotFoundError() { super(); }
        protected static String getTypeName() { return "FileNotFoundError"; }
        @Override public String typeName() { return "FileNotFoundError"; }
    }
    
    public static class IsADirectoryError extends OSError {
        public IsADirectoryError(String message) { super(message); }
        public IsADirectoryError() { super(); }
        protected static String getTypeName() { return "IsADirectoryError"; }
        @Override public String typeName() { return "IsADirectoryError"; }
    }
    
    public static class NotADirectoryError extends OSError {
        public NotADirectoryError(String message) { super(message); }
        public NotADirectoryError() { super(); }
        protected static String getTypeName() { return "NotADirectoryError"; }
        @Override public String typeName() { return "NotADirectoryError"; }
    }
    
    public static class PermissionError extends OSError {
        public PermissionError(String message) { super(message); }
        public PermissionError() { super(); }
        protected static String getTypeName() { return "PermissionError"; }
        @Override public String typeName() { return "PermissionError"; }
    }
    
    // ========================================================================
    // RuntimeError hierarchy
    // ========================================================================
    
    public static class RuntimeError extends Exception {
        public RuntimeError(String message) { super(message); }
        public RuntimeError() { super(); }
        protected static String getTypeName() { return "RuntimeError"; }
        @Override public String typeName() { return "RuntimeError"; }
    }
    
    public static class NotImplementedError extends RuntimeError {
        public NotImplementedError(String message) { super(message); }
        public NotImplementedError() { super(); }
        protected static String getTypeName() { return "NotImplementedError"; }
        @Override public String typeName() { return "NotImplementedError"; }
    }
    
    public static class RecursionError extends RuntimeError {
        public RecursionError(String message) { super(message); }
        public RecursionError() { super("maximum recursion depth exceeded"); }
        protected static String getTypeName() { return "RecursionError"; }
        @Override public String typeName() { return "RecursionError"; }
    }
    
    // ========================================================================
    // SyntaxError
    // ========================================================================
    
    public static class SyntaxError extends Exception {
        public String filename;
        public int lineno;
        public int offset;
        public String text;
        
        public SyntaxError(String message) { super(message); }
        
        public SyntaxError(String message, String filename, int lineno, int offset, String text) {
            super(message);
            this.filename = filename;
            this.lineno = lineno;
            this.offset = offset;
            this.text = text;
        }
        
        public SyntaxError() { super(); }
        protected static String getTypeName() { return "SyntaxError"; }
        @Override public String typeName() { return "SyntaxError"; }
    }
    
    // ========================================================================
    // Warning (base class for warnings)
    // ========================================================================
    
    public static class Warning extends Exception {
        public Warning(String message) { super(message); }
        public Warning() { super(); }
        protected static String getTypeName() { return "Warning"; }
        @Override public String typeName() { return "Warning"; }
    }
    
    public static class DeprecationWarning extends Warning {
        public DeprecationWarning(String message) { super(message); }
        public DeprecationWarning() { super(); }
        protected static String getTypeName() { return "DeprecationWarning"; }
        @Override public String typeName() { return "DeprecationWarning"; }
    }
    
    public static class UserWarning extends Warning {
        public UserWarning(String message) { super(message); }
        public UserWarning() { super(); }
        protected static String getTypeName() { return "UserWarning"; }
        @Override public String typeName() { return "UserWarning"; }
    }
}

/**
 * Wrapper to make $X usable as a Python object.
 */
class $XO extends $O {
    /** The underlying Java exception */
    public final $X exception;
    
    $XO($X exception) {
        this.exception = exception;
    }
    
    /**
     * Get the underlying exception for throwing.
     */
    public $X getException() {
        return exception;
    }
    
    @Override
    public $S __repr__() {
        return $S.of(exception.typeName() + "('" + exception.message + "')");
    }
    
    @Override
    public $S __str__() {
        return $S.of(exception.message);
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "args":
                return exception.args;
            case "__class__":
                return ExceptionClasses.forException(exception);
            default:
                return super.__getattr__(name);
        }
    }
}

/**
 * Exception class objects for isinstance checks.
 */
class ExceptionClasses {
    // Singleton class objects for each exception type
    public static final $Cls BaseException = makeExceptionClass("BaseException", null, $X.class);
    public static final $Cls SystemExit = makeExceptionClass("SystemExit", BaseException, $X.SystemExit.class);
    public static final $Cls KeyboardInterrupt = makeExceptionClass("KeyboardInterrupt", BaseException, $X.KeyboardInterrupt.class);
    public static final $Cls GeneratorExit = makeExceptionClass("GeneratorExit", BaseException, $X.GeneratorExit.class);
    public static final $Cls Exception = makeExceptionClass("Exception", BaseException, $X.Exception.class);
    
    public static final $Cls StopIteration = makeExceptionClass("StopIteration", Exception, $X.StopIteration.class);
    public static final $Cls StopAsyncIteration = makeExceptionClass("StopAsyncIteration", Exception, $X.StopAsyncIteration.class);
    
    public static final $Cls ArithmeticError = makeExceptionClass("ArithmeticError", Exception, $X.ArithmeticError.class);
    public static final $Cls ZeroDivisionError = makeExceptionClass("ZeroDivisionError", ArithmeticError, $X.ZeroDivisionError.class);
    public static final $Cls OverflowError = makeExceptionClass("OverflowError", ArithmeticError, $X.OverflowError.class);
    
    public static final $Cls AssertionError = makeExceptionClass("AssertionError", Exception, $X.AssertionError.class);
    public static final $Cls AttributeError = makeExceptionClass("AttributeError", Exception, $X.AttributeError.class);
    public static final $Cls EOFError = makeExceptionClass("EOFError", Exception, $X.EOFError.class);
    public static final $Cls TypeError = makeExceptionClass("TypeError", Exception, $X.TypeError.class);
    public static final $Cls ValueError = makeExceptionClass("ValueError", Exception, $X.ValueError.class);
    public static final $Cls UnicodeError = makeExceptionClass("UnicodeError", ValueError, $X.UnicodeError.class);
    
    public static final $Cls ImportError = makeExceptionClass("ImportError", Exception, $X.ImportError.class);
    public static final $Cls ModuleNotFoundError = makeExceptionClass("ModuleNotFoundError", ImportError, $X.ModuleNotFoundError.class);
    
    public static final $Cls LookupError = makeExceptionClass("LookupError", Exception, $X.LookupError.class);
    public static final $Cls IndexError = makeExceptionClass("IndexError", LookupError, $X.IndexError.class);
    public static final $Cls KeyError = makeExceptionClass("KeyError", LookupError, $X.KeyError.class);
    
    public static final $Cls NameError = makeExceptionClass("NameError", Exception, $X.NameError.class);
    public static final $Cls UnboundLocalError = makeExceptionClass("UnboundLocalError", NameError, $X.UnboundLocalError.class);
    
    public static final $Cls OSError = makeExceptionClass("OSError", Exception, $X.OSError.class);
    public static final $Cls FileExistsError = makeExceptionClass("FileExistsError", OSError, $X.FileExistsError.class);
    public static final $Cls FileNotFoundError = makeExceptionClass("FileNotFoundError", OSError, $X.FileNotFoundError.class);
    public static final $Cls IsADirectoryError = makeExceptionClass("IsADirectoryError", OSError, $X.IsADirectoryError.class);
    public static final $Cls NotADirectoryError = makeExceptionClass("NotADirectoryError", OSError, $X.NotADirectoryError.class);
    public static final $Cls PermissionError = makeExceptionClass("PermissionError", OSError, $X.PermissionError.class);
    
    public static final $Cls RuntimeError = makeExceptionClass("RuntimeError", Exception, $X.RuntimeError.class);
    public static final $Cls NotImplementedError = makeExceptionClass("NotImplementedError", RuntimeError, $X.NotImplementedError.class);
    public static final $Cls RecursionError = makeExceptionClass("RecursionError", RuntimeError, $X.RecursionError.class);
    
    public static final $Cls SyntaxError = makeExceptionClass("SyntaxError", Exception, $X.SyntaxError.class);
    
    public static final $Cls Warning = makeExceptionClass("Warning", Exception, $X.Warning.class);
    public static final $Cls DeprecationWarning = makeExceptionClass("DeprecationWarning", Warning, $X.DeprecationWarning.class);
    public static final $Cls UserWarning = makeExceptionClass("UserWarning", Warning, $X.UserWarning.class);
    
    private static $Cls makeExceptionClass(String name, $Cls base, Class<?> javaClass) {
        $Cls cls = base != null ? $Cls.of(name, new $O[]{base}) : $Cls.of(name);
        cls.javaClass = javaClass;
        return cls;
    }
    
    /**
     * Get the class object for an exception instance.
     */
    public static $Cls forException($X e) {
        if (e instanceof $X.SystemExit) return SystemExit;
        if (e instanceof $X.KeyboardInterrupt) return KeyboardInterrupt;
        if (e instanceof $X.GeneratorExit) return GeneratorExit;
        if (e instanceof $X.StopIteration) return StopIteration;
        if (e instanceof $X.StopAsyncIteration) return StopAsyncIteration;
        if (e instanceof $X.ZeroDivisionError) return ZeroDivisionError;
        if (e instanceof $X.OverflowError) return OverflowError;
        if (e instanceof $X.ArithmeticError) return ArithmeticError;
        if (e instanceof $X.AssertionError) return AssertionError;
        if (e instanceof $X.AttributeError) return AttributeError;
        if (e instanceof $X.EOFError) return EOFError;
        if (e instanceof $X.TypeError) return TypeError;
        if (e instanceof $X.UnicodeError) return UnicodeError;
        if (e instanceof $X.ValueError) return ValueError;
        if (e instanceof $X.ModuleNotFoundError) return ModuleNotFoundError;
        if (e instanceof $X.ImportError) return ImportError;
        if (e instanceof $X.IndexError) return IndexError;
        if (e instanceof $X.KeyError) return KeyError;
        if (e instanceof $X.LookupError) return LookupError;
        if (e instanceof $X.UnboundLocalError) return UnboundLocalError;
        if (e instanceof $X.NameError) return NameError;
        if (e instanceof $X.FileExistsError) return FileExistsError;
        if (e instanceof $X.FileNotFoundError) return FileNotFoundError;
        if (e instanceof $X.IsADirectoryError) return IsADirectoryError;
        if (e instanceof $X.NotADirectoryError) return NotADirectoryError;
        if (e instanceof $X.PermissionError) return PermissionError;
        if (e instanceof $X.OSError) return OSError;
        if (e instanceof $X.NotImplementedError) return NotImplementedError;
        if (e instanceof $X.RecursionError) return RecursionError;
        if (e instanceof $X.RuntimeError) return RuntimeError;
        if (e instanceof $X.SyntaxError) return SyntaxError;
        if (e instanceof $X.DeprecationWarning) return DeprecationWarning;
        if (e instanceof $X.UserWarning) return UserWarning;
        if (e instanceof $X.Warning) return Warning;
        if (e instanceof $X.Exception) return Exception;
        return BaseException;
    }
    
    /**
     * Get exception class by name.
     */
    public static $Cls byName(String name) {
        switch (name) {
            case "BaseException": return BaseException;
            case "SystemExit": return SystemExit;
            case "KeyboardInterrupt": return KeyboardInterrupt;
            case "GeneratorExit": return GeneratorExit;
            case "Exception": return Exception;
            case "StopIteration": return StopIteration;
            case "StopAsyncIteration": return StopAsyncIteration;
            case "ArithmeticError": return ArithmeticError;
            case "ZeroDivisionError": return ZeroDivisionError;
            case "OverflowError": return OverflowError;
            case "AssertionError": return AssertionError;
            case "AttributeError": return AttributeError;
            case "EOFError": return EOFError;
            case "TypeError": return TypeError;
            case "ValueError": return ValueError;
            case "UnicodeError": return UnicodeError;
            case "ImportError": return ImportError;
            case "ModuleNotFoundError": return ModuleNotFoundError;
            case "LookupError": return LookupError;
            case "IndexError": return IndexError;
            case "KeyError": return KeyError;
            case "NameError": return NameError;
            case "UnboundLocalError": return UnboundLocalError;
            case "OSError": return OSError;
            case "IOError": return OSError;  // Alias
            case "FileExistsError": return FileExistsError;
            case "FileNotFoundError": return FileNotFoundError;
            case "IsADirectoryError": return IsADirectoryError;
            case "NotADirectoryError": return NotADirectoryError;
            case "PermissionError": return PermissionError;
            case "RuntimeError": return RuntimeError;
            case "NotImplementedError": return NotImplementedError;
            case "RecursionError": return RecursionError;
            case "SyntaxError": return SyntaxError;
            case "Warning": return Warning;
            case "DeprecationWarning": return DeprecationWarning;
            case "UserWarning": return UserWarning;
            default: return null;
        }
    }
}
