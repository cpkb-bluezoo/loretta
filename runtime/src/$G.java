import java.util.Scanner;
import java.util.HashMap;
import java.util.Map;
import java.io.File;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.List;

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
    
    /** Current package being imported (for relative imports) */
    private static String currentPackage = "";
    
    /** CPython stdlib path (auto-detected or from env var) */
    private static String cpythonStdlibPath = null;
    
    /** Loretta compiler path */
    private static String lorettaCompilerPath = null;
    
    /** Cache directory for compiled stdlib modules */
    private static File stdlibCacheDir = null;
    
    static {
        // Register built-in modules
        registerBuiltinModules();
    }
    
    /**
     * Register built-in modules like sys, builtins.
     */
    private static void registerBuiltinModules() {
        // Create sys module
        $Mod sysModule = new $Mod("sys");
        sysModule.setAttr("path", $Sys.path);
        sysModule.setAttr("modules", $Sys.modules);
        sysModule.setAttr("argv", $Sys.argv);
        sysModule.setAttr("version", $Sys.version);
        sysModule.setAttr("version_info", $Sys.version_info);
        sysModule.setAttr("platform", $Sys.platform);
        sysModule.setAttr("stdin", $Sys.stdin);
        sysModule.setAttr("stdout", $Sys.stdout);
        sysModule.setAttr("stderr", $Sys.stderr);
        sysModule.setAttr("maxsize", $Sys.maxsize);
        sysModule.setAttr("meta_path", $Sys.meta_path);
        sysModule.setAttr("path_hooks", $Sys.path_hooks);
        sysModule.setAttr("path_importer_cache", $Sys.path_importer_cache);
        sysModule.setAttr("executable", $Sys.executable);
        sysModule.setAttr("prefix", $Sys.prefix);
        modules.put("sys", sysModule);
        
        // Create builtins module (references to built-in functions)
        $Mod builtinsModule = new $Mod("builtins");
        builtinsModule.setAttr("print", PRINT);
        builtinsModule.setAttr("len", LEN);
        builtinsModule.setAttr("range", RANGE);
        builtinsModule.setAttr("int", INT);
        builtinsModule.setAttr("float", FLOAT);
        builtinsModule.setAttr("str", STR);
        builtinsModule.setAttr("bool", BOOL);
        builtinsModule.setAttr("list", LIST);
        builtinsModule.setAttr("dict", DICT);
        builtinsModule.setAttr("tuple", TUPLE);
        builtinsModule.setAttr("set", SET);
        builtinsModule.setAttr("type", TYPE);
        builtinsModule.setAttr("isinstance", ISINSTANCE);
        builtinsModule.setAttr("hasattr", HASATTR);
        builtinsModule.setAttr("getattr", GETATTR);
        builtinsModule.setAttr("setattr", SETATTR);
        builtinsModule.setAttr("None", $N.INSTANCE);
        builtinsModule.setAttr("True", $B.TRUE);
        builtinsModule.setAttr("False", $B.FALSE);
        
        // Exception classes
        builtinsModule.setAttr("BaseException", ExceptionClasses.BaseException);
        builtinsModule.setAttr("Exception", ExceptionClasses.Exception);
        builtinsModule.setAttr("SystemExit", ExceptionClasses.SystemExit);
        builtinsModule.setAttr("KeyboardInterrupt", ExceptionClasses.KeyboardInterrupt);
        builtinsModule.setAttr("GeneratorExit", ExceptionClasses.GeneratorExit);
        builtinsModule.setAttr("StopIteration", ExceptionClasses.StopIteration);
        builtinsModule.setAttr("StopAsyncIteration", ExceptionClasses.StopAsyncIteration);
        builtinsModule.setAttr("ArithmeticError", ExceptionClasses.ArithmeticError);
        builtinsModule.setAttr("ZeroDivisionError", ExceptionClasses.ZeroDivisionError);
        builtinsModule.setAttr("OverflowError", ExceptionClasses.OverflowError);
        builtinsModule.setAttr("AssertionError", ExceptionClasses.AssertionError);
        builtinsModule.setAttr("AttributeError", ExceptionClasses.AttributeError);
        builtinsModule.setAttr("EOFError", ExceptionClasses.EOFError);
        builtinsModule.setAttr("TypeError", ExceptionClasses.TypeError);
        builtinsModule.setAttr("ValueError", ExceptionClasses.ValueError);
        builtinsModule.setAttr("UnicodeError", ExceptionClasses.UnicodeError);
        builtinsModule.setAttr("ImportError", ExceptionClasses.ImportError);
        builtinsModule.setAttr("ModuleNotFoundError", ExceptionClasses.ModuleNotFoundError);
        builtinsModule.setAttr("LookupError", ExceptionClasses.LookupError);
        builtinsModule.setAttr("IndexError", ExceptionClasses.IndexError);
        builtinsModule.setAttr("KeyError", ExceptionClasses.KeyError);
        builtinsModule.setAttr("NameError", ExceptionClasses.NameError);
        builtinsModule.setAttr("UnboundLocalError", ExceptionClasses.UnboundLocalError);
        builtinsModule.setAttr("OSError", ExceptionClasses.OSError);
        builtinsModule.setAttr("IOError", ExceptionClasses.OSError);  // Alias
        builtinsModule.setAttr("FileExistsError", ExceptionClasses.FileExistsError);
        builtinsModule.setAttr("FileNotFoundError", ExceptionClasses.FileNotFoundError);
        builtinsModule.setAttr("IsADirectoryError", ExceptionClasses.IsADirectoryError);
        builtinsModule.setAttr("NotADirectoryError", ExceptionClasses.NotADirectoryError);
        builtinsModule.setAttr("PermissionError", ExceptionClasses.PermissionError);
        builtinsModule.setAttr("RuntimeError", ExceptionClasses.RuntimeError);
        builtinsModule.setAttr("NotImplementedError", ExceptionClasses.NotImplementedError);
        builtinsModule.setAttr("RecursionError", ExceptionClasses.RecursionError);
        builtinsModule.setAttr("SyntaxError", ExceptionClasses.SyntaxError);
        builtinsModule.setAttr("Warning", ExceptionClasses.Warning);
        builtinsModule.setAttr("DeprecationWarning", ExceptionClasses.DeprecationWarning);
        builtinsModule.setAttr("UserWarning", ExceptionClasses.UserWarning);
        
        modules.put("builtins", builtinsModule);
        
        // Register C-extension replacement modules
        registerCExtModules();
    }
    
    /**
     * Register C-extension replacement modules.
     * These are Java implementations of modules that are normally C extensions in CPython.
     */
    private static void registerCExtModules() {
        // _posix / posix / nt - OS operations
        $Mod posixMod = new $Mod("posix");
        posixMod.setAttr("name", _posix.name);
        posixMod.setAttr("sep", _posix.sep);
        posixMod.setAttr("altsep", _posix.altsep != null ? _posix.altsep : $N.INSTANCE);
        posixMod.setAttr("pathsep", _posix.pathsep);
        posixMod.setAttr("linesep", _posix.linesep);
        posixMod.setAttr("curdir", _posix.curdir);
        posixMod.setAttr("pardir", _posix.pardir);
        posixMod.setAttr("extsep", _posix.extsep);
        posixMod.setAttr("devnull", _posix.devnull);
        posixMod.setAttr("environ", _posix.environ);
        posixMod.setAttr("F_OK", $I.of(_posix.F_OK));
        posixMod.setAttr("R_OK", $I.of(_posix.R_OK));
        posixMod.setAttr("W_OK", $I.of(_posix.W_OK));
        posixMod.setAttr("X_OK", $I.of(_posix.X_OK));
        
        // Register functions as method handles
        posixMod.setAttr("getcwd", new $O() {
            @Override public $O __call__($O... args) { return _posix.getcwd(); }
            @Override public $S __repr__() { return $S.of("<built-in function getcwd>"); }
        });
        posixMod.setAttr("chdir", new $O() {
            @Override public $O __call__($O... args) { _posix.chdir(args[0]); return $N.INSTANCE; }
            @Override public $S __repr__() { return $S.of("<built-in function chdir>"); }
        });
        posixMod.setAttr("listdir", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 0 ? _posix.listdir(args[0]) : _posix.listdir(); 
            }
            @Override public $S __repr__() { return $S.of("<built-in function listdir>"); }
        });
        posixMod.setAttr("mkdir", new $O() {
            @Override public $O __call__($O... args) { 
                if (args.length > 1) _posix.mkdir(args[0], args[1]);
                else _posix.mkdir(args[0]); 
                return $N.INSTANCE; 
            }
            @Override public $S __repr__() { return $S.of("<built-in function mkdir>"); }
        });
        posixMod.setAttr("makedirs", new $O() {
            @Override public $O __call__($O... args) { 
                if (args.length > 2) _posix.makedirs(args[0], args[1], args[2]);
                else if (args.length > 1) _posix.makedirs(args[0], args[1]);
                else _posix.makedirs(args[0]); 
                return $N.INSTANCE; 
            }
            @Override public $S __repr__() { return $S.of("<built-in function makedirs>"); }
        });
        posixMod.setAttr("remove", new $O() {
            @Override public $O __call__($O... args) { _posix.remove(args[0]); return $N.INSTANCE; }
            @Override public $S __repr__() { return $S.of("<built-in function remove>"); }
        });
        posixMod.setAttr("unlink", new $O() {
            @Override public $O __call__($O... args) { _posix.unlink(args[0]); return $N.INSTANCE; }
            @Override public $S __repr__() { return $S.of("<built-in function unlink>"); }
        });
        posixMod.setAttr("rmdir", new $O() {
            @Override public $O __call__($O... args) { _posix.rmdir(args[0]); return $N.INSTANCE; }
            @Override public $S __repr__() { return $S.of("<built-in function rmdir>"); }
        });
        posixMod.setAttr("rename", new $O() {
            @Override public $O __call__($O... args) { _posix.rename(args[0], args[1]); return $N.INSTANCE; }
            @Override public $S __repr__() { return $S.of("<built-in function rename>"); }
        });
        posixMod.setAttr("replace", new $O() {
            @Override public $O __call__($O... args) { _posix.replace(args[0], args[1]); return $N.INSTANCE; }
            @Override public $S __repr__() { return $S.of("<built-in function replace>"); }
        });
        posixMod.setAttr("stat", new $O() {
            @Override public $O __call__($O... args) { return _posix.stat(args[0]); }
            @Override public $S __repr__() { return $S.of("<built-in function stat>"); }
        });
        posixMod.setAttr("lstat", new $O() {
            @Override public $O __call__($O... args) { return _posix.lstat(args[0]); }
            @Override public $S __repr__() { return $S.of("<built-in function lstat>"); }
        });
        posixMod.setAttr("access", new $O() {
            @Override public $O __call__($O... args) { return _posix.access(args[0], args[1]); }
            @Override public $S __repr__() { return $S.of("<built-in function access>"); }
        });
        posixMod.setAttr("getenv", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 1 ? _posix.getenv(args[0], args[1]) : _posix.getenv(args[0]); 
            }
            @Override public $S __repr__() { return $S.of("<built-in function getenv>"); }
        });
        posixMod.setAttr("walk", new $O() {
            @Override public $O __call__($O... args) { 
                if (args.length > 3) return _posix.walk(args[0], args[1], args[2], args[3]);
                return _posix.walk(args[0]); 
            }
            @Override public $S __repr__() { return $S.of("<built-in function walk>"); }
        });
        
        modules.put("posix", posixMod);
        modules.put("_posix", posixMod);
        // On Windows this would be "nt", but we use posix for simplicity
        modules.put("nt", posixMod);
        
        // _collections - deque, defaultdict, Counter
        $Mod collectionsMod = new $Mod("_collections");
        collectionsMod.setAttr("deque", new $O() {
            @Override public $O __call__($O... args) { return _collections.deque_new(args); }
            @Override public $S __repr__() { return $S.of("<class 'collections.deque'>"); }
        });
        collectionsMod.setAttr("defaultdict", new $O() {
            @Override public $O __call__($O... args) { return _collections.defaultdict_new(args); }
            @Override public $S __repr__() { return $S.of("<class 'collections.defaultdict'>"); }
        });
        collectionsMod.setAttr("Counter", new $O() {
            @Override public $O __call__($O... args) { return _collections.Counter_new(args); }
            @Override public $S __repr__() { return $S.of("<class 'collections.Counter'>"); }
        });
        modules.put("_collections", collectionsMod);
        
        // _sre - regex engine
        $Mod sreMod = new $Mod("_sre");
        // Flag constants
        sreMod.setAttr("IGNORECASE", $I.of(_sre.IGNORECASE));
        sreMod.setAttr("I", $I.of(_sre.I));
        sreMod.setAttr("LOCALE", $I.of(_sre.LOCALE));
        sreMod.setAttr("L", $I.of(_sre.L));
        sreMod.setAttr("MULTILINE", $I.of(_sre.MULTILINE));
        sreMod.setAttr("M", $I.of(_sre.M));
        sreMod.setAttr("DOTALL", $I.of(_sre.DOTALL));
        sreMod.setAttr("S", $I.of(_sre.S));
        sreMod.setAttr("UNICODE", $I.of(_sre.UNICODE));
        sreMod.setAttr("U", $I.of(_sre.U));
        sreMod.setAttr("VERBOSE", $I.of(_sre.VERBOSE));
        sreMod.setAttr("X", $I.of(_sre.X));
        sreMod.setAttr("ASCII", $I.of(_sre.ASCII));
        sreMod.setAttr("A", $I.of(_sre.A));
        
        // Functions
        sreMod.setAttr("compile", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 1 ? _sre.compile(args[0], args[1]) : _sre.compile(args[0]); 
            }
            @Override public $S __repr__() { return $S.of("<function compile>"); }
        });
        sreMod.setAttr("search", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 2 ? _sre.search(args[0], args[1], args[2]) : _sre.search(args[0], args[1]); 
            }
            @Override public $S __repr__() { return $S.of("<function search>"); }
        });
        sreMod.setAttr("match", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 2 ? _sre.match(args[0], args[1], args[2]) : _sre.match(args[0], args[1]); 
            }
            @Override public $S __repr__() { return $S.of("<function match>"); }
        });
        sreMod.setAttr("fullmatch", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 2 ? _sre.fullmatch(args[0], args[1], args[2]) : _sre.fullmatch(args[0], args[1]); 
            }
            @Override public $S __repr__() { return $S.of("<function fullmatch>"); }
        });
        sreMod.setAttr("findall", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 2 ? _sre.findall(args[0], args[1], args[2]) : _sre.findall(args[0], args[1]); 
            }
            @Override public $S __repr__() { return $S.of("<function findall>"); }
        });
        sreMod.setAttr("finditer", new $O() {
            @Override public $O __call__($O... args) { 
                return args.length > 2 ? _sre.finditer(args[0], args[1], args[2]) : _sre.finditer(args[0], args[1]); 
            }
            @Override public $S __repr__() { return $S.of("<function finditer>"); }
        });
        sreMod.setAttr("sub", new $O() {
            @Override public $O __call__($O... args) { 
                if (args.length > 4) return _sre.sub(args[0], args[1], args[2], args[3], args[4]);
                if (args.length > 3) return _sre.sub(args[0], args[1], args[2], args[3]);
                return _sre.sub(args[0], args[1], args[2]); 
            }
            @Override public $S __repr__() { return $S.of("<function sub>"); }
        });
        sreMod.setAttr("split", new $O() {
            @Override public $O __call__($O... args) { 
                if (args.length > 3) return _sre.split(args[0], args[1], args[2], args[3]);
                if (args.length > 2) return _sre.split(args[0], args[1], args[2]);
                return _sre.split(args[0], args[1]); 
            }
            @Override public $S __repr__() { return $S.of("<function split>"); }
        });
        sreMod.setAttr("escape", new $O() {
            @Override public $O __call__($O... args) { return _sre.escape(args[0]); }
            @Override public $S __repr__() { return $S.of("<function escape>"); }
        });
        
        modules.put("_sre", sreMod);
        modules.put("re", sreMod);  // Also register as 're' for direct import
        
        // _io - I/O module
        $Mod ioMod = new $Mod("_io");
        ioMod.setAttr("DEFAULT_BUFFER_SIZE", $I.of(_io.DEFAULT_BUFFER_SIZE));
        ioMod.setAttr("SEEK_SET", $I.of(_io.SEEK_SET));
        ioMod.setAttr("SEEK_CUR", $I.of(_io.SEEK_CUR));
        ioMod.setAttr("SEEK_END", $I.of(_io.SEEK_END));
        
        // I/O classes
        ioMod.setAttr("FileIO", new $O() {
            @Override public $O __call__($O... args) {
                return new _io.FileIO(args[0],
                    args.length > 1 ? args[1] : $S.of("r"),
                    args.length > 2 ? args[2] : $B.TRUE,
                    args.length > 3 ? args[3] : $N.INSTANCE);
            }
            @Override public $S __repr__() { return $S.of("<class '_io.FileIO'>"); }
        });
        ioMod.setAttr("BufferedReader", new $O() {
            @Override public $O __call__($O... args) {
                _io.RawIOBase raw = (_io.RawIOBase)args[0];
                int size = args.length > 1 && args[1] instanceof $I ? (int)(($I)args[1]).value : _io.DEFAULT_BUFFER_SIZE;
                return new _io.BufferedReader(raw, size);
            }
            @Override public $S __repr__() { return $S.of("<class '_io.BufferedReader'>"); }
        });
        ioMod.setAttr("BufferedWriter", new $O() {
            @Override public $O __call__($O... args) {
                _io.RawIOBase raw = (_io.RawIOBase)args[0];
                int size = args.length > 1 && args[1] instanceof $I ? (int)(($I)args[1]).value : _io.DEFAULT_BUFFER_SIZE;
                return new _io.BufferedWriter(raw, size);
            }
            @Override public $S __repr__() { return $S.of("<class '_io.BufferedWriter'>"); }
        });
        ioMod.setAttr("TextIOWrapper", new $O() {
            @Override public $O __call__($O... args) {
                _io.BufferedIOBase buf = (_io.BufferedIOBase)args[0];
                return new _io.TextIOWrapper(buf,
                    args.length > 1 ? args[1] : $N.INSTANCE,
                    args.length > 2 ? args[2] : $N.INSTANCE,
                    args.length > 3 ? args[3] : $N.INSTANCE,
                    args.length > 4 ? args[4] : $B.FALSE);
            }
            @Override public $S __repr__() { return $S.of("<class '_io.TextIOWrapper'>"); }
        });
        ioMod.setAttr("BytesIO", new $O() {
            @Override public $O __call__($O... args) {
                return args.length > 0 ? new _io.BytesIO(args[0]) : new _io.BytesIO();
            }
            @Override public $S __repr__() { return $S.of("<class '_io.BytesIO'>"); }
        });
        ioMod.setAttr("StringIO", new $O() {
            @Override public $O __call__($O... args) {
                return args.length > 0 ? new _io.StringIO(args[0]) : new _io.StringIO();
            }
            @Override public $S __repr__() { return $S.of("<class '_io.StringIO'>"); }
        });
        ioMod.setAttr("open", new $O() {
            @Override public $O __call__($O... args) {
                return _io.open(args[0],
                    args.length > 1 ? args[1] : $S.of("r"),
                    args.length > 2 ? args[2] : $I.of(-1),
                    args.length > 3 ? args[3] : $N.INSTANCE,
                    args.length > 4 ? args[4] : $N.INSTANCE,
                    args.length > 5 ? args[5] : $N.INSTANCE,
                    args.length > 6 ? args[6] : $B.TRUE,
                    args.length > 7 ? args[7] : $N.INSTANCE);
            }
            @Override public $S __repr__() { return $S.of("<built-in function open>"); }
        });
        
        modules.put("_io", ioMod);
        modules.put("io", ioMod);  // Also register as 'io' for direct import
        
        // _socket - Network sockets
        $Mod socketMod = new $Mod("socket");
        // Address families
        socketMod.setAttr("AF_INET", $I.of(_socket.AF_INET));
        socketMod.setAttr("AF_INET6", $I.of(_socket.AF_INET6));
        socketMod.setAttr("AF_UNIX", $I.of(_socket.AF_UNIX));
        socketMod.setAttr("AF_UNSPEC", $I.of(_socket.AF_UNSPEC));
        // Socket types
        socketMod.setAttr("SOCK_STREAM", $I.of(_socket.SOCK_STREAM));
        socketMod.setAttr("SOCK_DGRAM", $I.of(_socket.SOCK_DGRAM));
        socketMod.setAttr("SOCK_RAW", $I.of(_socket.SOCK_RAW));
        // Protocols
        socketMod.setAttr("IPPROTO_TCP", $I.of(_socket.IPPROTO_TCP));
        socketMod.setAttr("IPPROTO_UDP", $I.of(_socket.IPPROTO_UDP));
        socketMod.setAttr("IPPROTO_IP", $I.of(_socket.IPPROTO_IP));
        // Socket options
        socketMod.setAttr("SOL_SOCKET", $I.of(_socket.SOL_SOCKET));
        socketMod.setAttr("SO_REUSEADDR", $I.of(_socket.SO_REUSEADDR));
        socketMod.setAttr("SO_KEEPALIVE", $I.of(_socket.SO_KEEPALIVE));
        socketMod.setAttr("SO_BROADCAST", $I.of(_socket.SO_BROADCAST));
        socketMod.setAttr("SO_RCVBUF", $I.of(_socket.SO_RCVBUF));
        socketMod.setAttr("SO_SNDBUF", $I.of(_socket.SO_SNDBUF));
        socketMod.setAttr("TCP_NODELAY", $I.of(_socket.TCP_NODELAY));
        // Shutdown
        socketMod.setAttr("SHUT_RD", $I.of(_socket.SHUT_RD));
        socketMod.setAttr("SHUT_WR", $I.of(_socket.SHUT_WR));
        socketMod.setAttr("SHUT_RDWR", $I.of(_socket.SHUT_RDWR));
        // Special addresses
        socketMod.setAttr("INADDR_ANY", _socket.INADDR_ANY);
        socketMod.setAttr("INADDR_BROADCAST", _socket.INADDR_BROADCAST);
        socketMod.setAttr("INADDR_LOOPBACK", _socket.INADDR_LOOPBACK);
        
        // socket class
        socketMod.setAttr("socket", new $O() {
            @Override public $O __call__($O... args) {
                return new _socket.socket(
                    args.length > 0 ? args[0] : $I.of(_socket.AF_INET),
                    args.length > 1 ? args[1] : $I.of(_socket.SOCK_STREAM),
                    args.length > 2 ? args[2] : $I.of(0)
                );
            }
            @Override public $S __repr__() { return $S.of("<class 'socket.socket'>"); }
        });
        
        // Helper functions
        socketMod.setAttr("gethostname", new $O() {
            @Override public $O __call__($O... args) { return _socket.gethostname(); }
        });
        socketMod.setAttr("gethostbyname", new $O() {
            @Override public $O __call__($O... args) { return _socket.gethostbyname(args[0]); }
        });
        socketMod.setAttr("getaddrinfo", new $O() {
            @Override public $O __call__($O... args) {
                return _socket.getaddrinfo(
                    args[0], args[1],
                    args.length > 2 ? args[2] : $I.of(_socket.AF_UNSPEC),
                    args.length > 3 ? args[3] : $I.of(0),
                    args.length > 4 ? args[4] : $I.of(0),
                    args.length > 5 ? args[5] : $I.of(0)
                );
            }
        });
        socketMod.setAttr("inet_aton", new $O() {
            @Override public $O __call__($O... args) { return _socket.inet_aton(args[0]); }
        });
        socketMod.setAttr("inet_ntoa", new $O() {
            @Override public $O __call__($O... args) { return _socket.inet_ntoa(args[0]); }
        });
        socketMod.setAttr("create_connection", new $O() {
            @Override public $O __call__($O... args) {
                return _socket.create_connection(args[0], args.length > 1 ? args[1] : $N.INSTANCE);
            }
        });
        
        modules.put("_socket", socketMod);
        modules.put("socket", socketMod);  // Also register as 'socket' for direct import
    }
    
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
     * First checks if already loaded, then searches sys.path for the module.
     */
    public static $Mod importModule(String name) {
        return importModule(name, null, 0);
    }
    
    /**
     * Import a module with package context (for relative imports).
     * @param name Module name (absolute or relative)
     * @param packageName Package context for relative imports (can be null)
     * @param level Number of parent packages to go up (0 = absolute, 1 = current package, etc.)
     */
    public static $Mod importModule(String name, String packageName, int level) {
        String fullName = name;
        
        // Handle relative imports
        if (level > 0) {
            if (packageName == null || packageName.isEmpty()) {
                throw new $X("ImportError", "attempted relative import with no known parent package");
            }
            
            // Split package into parts
            String[] parts = packageName.split("\\.");
            if (level > parts.length) {
                throw new $X("ImportError", "attempted relative import beyond top-level package");
            }
            
            // Build the base package (going up 'level' levels)
            StringBuilder base = new StringBuilder();
            for (int i = 0; i < parts.length - level + 1; i++) {
                if (i > 0) base.append(".");
                base.append(parts[i]);
            }
            
            // Append the name if provided
            if (name != null && !name.isEmpty()) {
                if (base.length() > 0) {
                    base.append(".");
                }
                base.append(name);
            }
            fullName = base.toString();
        }
        
        // Check if already loaded
        $Mod existing = modules.get(fullName);
        if (existing != null) {
            return existing;
        }
        
        // For dotted names, ensure parent packages are loaded first
        int lastDot = fullName.lastIndexOf('.');
        if (lastDot > 0) {
            String parentName = fullName.substring(0, lastDot);
            if (!modules.containsKey(parentName)) {
                importModule(parentName, null, 0);
            }
        }
        
        // Try to find and load the module
        Class<?> clazz = findModuleClass(fullName);
        if (clazz == null) {
            throw new $X("ModuleNotFoundError", "No module named '" + fullName + "'");
        }
        
        $Mod mod = new $Mod(fullName, clazz);
        modules.put(fullName, mod);
        
        // Save current globals and package context
        Map<String, $O> savedGlobals = new HashMap<>(globals);
        String savedPackage = currentPackage;
        globals.clear();
        
        // Set current package for any nested imports
        currentPackage = mod.name.contains(".") ? 
            mod.name.substring(0, mod.name.lastIndexOf('.')) : "";
        
        // Initialize the module
        mod.initialize();
        
        // Copy all globals created by module to the module object
        for (Map.Entry<String, $O> entry : globals.entrySet()) {
            mod.setAttr(entry.getKey(), entry.getValue());
        }
        
        // Restore original globals and package context
        globals.clear();
        globals.putAll(savedGlobals);
        currentPackage = savedPackage;
        
        return mod;
    }
    
    /**
     * Import all public names from a module into globals.
     * Implements 'from module import *'.
     */
    public static void importStar($Mod mod) {
        $L publicNames = mod.getPublicNames();
        for ($O nameObj : publicNames.items) {
            if (nameObj instanceof $S) {
                String name = (($S)nameObj).value;
                try {
                    $O value = mod.getAttr(name);
                    globals.put(name, value);
                } catch ($X e) {
                    // Skip if attribute not found (shouldn't happen)
                }
            }
        }
    }
    
    /**
     * Find a module class by searching sys.path.
     */
    private static Class<?> findModuleClass(String name) {
        // Convert Python module name to class name
        // foo.bar -> foo$bar (using $ as separator, or try foo/bar as package)
        String className = name.replace(".", "$");
        
        // First try direct class loading (for modules on classpath)
        try {
            return Class.forName(className);
        } catch (ClassNotFoundException e) {
            // Continue to search sys.path
        }
        
        // Also try the dotted name directly (some class loaders support it)
        try {
            return Class.forName(name);
        } catch (ClassNotFoundException e) {
            // Continue
        }
        
        // Search sys.path for .class files
        String classFileName = className + ".class";
        String packagePath = name.replace(".", "/");
        
        for ($O pathObj : $Sys.path.items) {
            if (!(pathObj instanceof $S)) continue;
            String pathStr = (($S)pathObj).value;
            
            File dir = new File(pathStr);
            if (!dir.isDirectory()) {
                // Could be a JAR file - skip for now
                continue;
            }
            
            // Try as a simple class file
            File classFile = new File(dir, classFileName);
            if (classFile.exists()) {
                try {
                    URL[] urls = { dir.toURI().toURL() };
                    URLClassLoader loader = new URLClassLoader(urls, $G.class.getClassLoader());
                    return loader.loadClass(className);
                } catch (Exception e) {
                    // Continue searching
                }
            }
            
            // Try as a package (directory with __init__.py compiled to __init__.class)
            File packageDir = new File(dir, packagePath);
            if (packageDir.isDirectory()) {
                File initFile = new File(packageDir, "__init__.class");
                if (initFile.exists()) {
                    try {
                        URL[] urls = { dir.toURI().toURL() };
                        URLClassLoader loader = new URLClassLoader(urls, $G.class.getClassLoader());
                        String initClassName = name.replace(".", "/") + "/__init__";
                        initClassName = initClassName.replace("/", "$");
                        return loader.loadClass(initClassName);
                    } catch (Exception e) {
                        // Continue searching
                    }
                }
            }
        }
        
        // Try to compile from CPython stdlib
        Class<?> stdlibClass = compileStdlibModule(name);
        if (stdlibClass != null) {
            return stdlibClass;
        }
        
        return null;
    }
    
    /**
     * Compile a module from CPython stdlib on demand.
     * @param name Module name (e.g., "os", "os.path", "collections")
     * @return The loaded class, or null if not found/compilable
     */
    private static Class<?> compileStdlibModule(String name) {
        String stdlibPath = getCPythonStdlib();
        if (stdlibPath == null) {
            return null;
        }
        
        String compiler = getLorettaCompiler();
        if (compiler == null) {
            return null;
        }
        
        File cacheDir = getStdlibCacheDir();
        String className = name.replace(".", "$");
        
        // Check if already compiled in cache
        File cachedClass = new File(cacheDir, className + ".class");
        if (cachedClass.exists()) {
            try {
                URL[] urls = { cacheDir.toURI().toURL() };
                URLClassLoader loader = new URLClassLoader(urls, $G.class.getClassLoader());
                return loader.loadClass(className);
            } catch (Exception e) {
                // Fall through to recompile
            }
        }
        
        // Find the .py source file
        String modulePath = name.replace(".", "/");
        File sourceFile = null;
        
        // Try as module.py
        File simpleSource = new File(stdlibPath, modulePath + ".py");
        if (simpleSource.exists()) {
            sourceFile = simpleSource;
        } else {
            // Try as package/__init__.py
            File packageInit = new File(stdlibPath, modulePath + "/__init__.py");
            if (packageInit.exists()) {
                sourceFile = packageInit;
            }
        }
        
        if (sourceFile == null) {
            return null;
        }
        
        // Compile the module
        try {
            ProcessBuilder pb = new ProcessBuilder(
                compiler,
                "-o", cacheDir.getAbsolutePath(),
                sourceFile.getAbsolutePath()
            );
            pb.redirectErrorStream(true);
            Process p = pb.start();
            
            // Read output (for debugging)
            BufferedReader reader = new BufferedReader(new InputStreamReader(p.getInputStream()));
            String line;
            while ((line = reader.readLine()) != null) {
                // Could log this for debugging
            }
            
            int exitCode = p.waitFor();
            if (exitCode != 0) {
                return null;
            }
            
            // Load the compiled class
            URL[] urls = { cacheDir.toURI().toURL() };
            URLClassLoader loader = new URLClassLoader(urls, $G.class.getClassLoader());
            
            // For packages, the class name might be different
            if (sourceFile.getName().equals("__init__.py")) {
                // Package init - try different class name patterns
                String initClassName = name + "$__init__";
                try {
                    return loader.loadClass(initClassName.replace(".", "$"));
                } catch (ClassNotFoundException e) {
                    // Try without __init__
                    return loader.loadClass(className);
                }
            }
            
            return loader.loadClass(className);
        } catch (Exception e) {
            return null;
        }
    }
    
    /**
     * Get the current package context (for relative imports).
     */
    public static String getCurrentPackage() {
        return currentPackage;
    }
    
    /**
     * Set the current package context.
     */
    public static void setCurrentPackage(String pkg) {
        currentPackage = pkg != null ? pkg : "";
    }
    
    /**
     * Get the CPython stdlib path, auto-detecting if needed.
     * @return Path to CPython's Lib directory, or null if not found
     */
    public static String getCPythonStdlib() {
        if (cpythonStdlibPath != null) {
            return cpythonStdlibPath;
        }
        
        // First check environment variable
        String envPath = System.getenv("CPYTHON_STDLIB_PATH");
        if (envPath != null && new File(envPath).isDirectory()) {
            cpythonStdlibPath = envPath;
            return cpythonStdlibPath;
        }
        
        // Auto-detect from python3
        try {
            ProcessBuilder pb = new ProcessBuilder("python3", "-c",
                "import sys; print(sys.prefix); print('.'.join(map(str, sys.version_info[:2])))");
            pb.redirectErrorStream(true);
            Process p = pb.start();
            BufferedReader reader = new BufferedReader(new InputStreamReader(p.getInputStream()));
            String prefix = reader.readLine();
            String version = reader.readLine();
            p.waitFor();
            
            if (prefix != null && version != null) {
                // Try common locations for stdlib
                String[] candidates = {
                    prefix + "/lib/python" + version,
                    prefix + "/Lib",  // Windows
                    "/usr/lib/python" + version,
                    "/usr/local/lib/python" + version
                };
                
                for (String candidate : candidates) {
                    File dir = new File(candidate);
                    if (dir.isDirectory() && new File(dir, "os.py").exists()) {
                        cpythonStdlibPath = candidate;
                        return cpythonStdlibPath;
                    }
                }
            }
        } catch (Exception e) {
            // Fall through to return null
        }
        
        return null;
    }
    
    /**
     * Get the Loretta compiler path.
     */
    public static String getLorettaCompiler() {
        if (lorettaCompilerPath != null) {
            return lorettaCompilerPath;
        }
        
        // Check environment variable
        String envPath = System.getenv("LORETTA_COMPILER");
        if (envPath != null && new File(envPath).canExecute()) {
            lorettaCompilerPath = envPath;
            return lorettaCompilerPath;
        }
        
        // Try to find relative to the runtime JAR location
        try {
            String jarPath = $G.class.getProtectionDomain().getCodeSource().getLocation().getPath();
            File jarFile = new File(jarPath);
            // Assume structure: runtime/loretta.jar, compiler is at ../loretta
            File compilerFile = new File(jarFile.getParentFile().getParentFile(), "loretta");
            if (compilerFile.canExecute()) {
                lorettaCompilerPath = compilerFile.getAbsolutePath();
                return lorettaCompilerPath;
            }
        } catch (Exception e) {
            // Fall through
        }
        
        // Try PATH
        String[] pathDirs = System.getenv("PATH").split(File.pathSeparator);
        for (String dir : pathDirs) {
            File compiler = new File(dir, "loretta");
            if (compiler.canExecute()) {
                lorettaCompilerPath = compiler.getAbsolutePath();
                return lorettaCompilerPath;
            }
        }
        
        return null;
    }
    
    /**
     * Get the stdlib cache directory, creating if needed.
     */
    public static File getStdlibCacheDir() {
        if (stdlibCacheDir != null) {
            return stdlibCacheDir;
        }
        
        String tmpDir = System.getProperty("java.io.tmpdir");
        stdlibCacheDir = new File(tmpDir, "loretta_stdlib_cache");
        if (!stdlibCacheDir.exists()) {
            stdlibCacheDir.mkdirs();
        }
        
        // Add to sys.path if not already there
        String cachePath = stdlibCacheDir.getAbsolutePath();
        for ($O p : $Sys.path.items) {
            if (p instanceof $S && (($S)p).value.equals(cachePath)) {
                return stdlibCacheDir;
            }
        }
        $Sys.path.insert($I.of(0), $S.of(cachePath));
        
        return stdlibCacheDir;
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
            // Class-related builtins
            case "super": return SUPER;
            case "staticmethod": return STATICMETHOD;
            case "classmethod": return CLASSMETHOD;
            case "property": return PROPERTY;
            // Memory and eval builtins
            case "memoryview": return MEMORYVIEW;
            case "exec": return EXEC;
            case "eval": return EVAL;
            case "compile": return COMPILE;
            // Async builtins
            case "async_gather": return ASYNC_GATHER;
            case "async_sleep": return ASYNC_SLEEP;
            case "async_run": return ASYNC_RUN;
            case "async_thread": return ASYNC_THREAD;
            // Constants
            case "True": return $B.TRUE;
            case "False": return $B.FALSE;
            case "None": return $N.INSTANCE;
            // Exception classes
            case "BaseException": return ExceptionClasses.BaseException;
            case "Exception": return ExceptionClasses.Exception;
            case "SystemExit": return ExceptionClasses.SystemExit;
            case "KeyboardInterrupt": return ExceptionClasses.KeyboardInterrupt;
            case "GeneratorExit": return ExceptionClasses.GeneratorExit;
            case "StopIteration": return ExceptionClasses.StopIteration;
            case "StopAsyncIteration": return ExceptionClasses.StopAsyncIteration;
            case "ArithmeticError": return ExceptionClasses.ArithmeticError;
            case "ZeroDivisionError": return ExceptionClasses.ZeroDivisionError;
            case "OverflowError": return ExceptionClasses.OverflowError;
            case "AssertionError": return ExceptionClasses.AssertionError;
            case "AttributeError": return ExceptionClasses.AttributeError;
            case "EOFError": return ExceptionClasses.EOFError;
            case "TypeError": return ExceptionClasses.TypeError;
            case "ValueError": return ExceptionClasses.ValueError;
            case "UnicodeError": return ExceptionClasses.UnicodeError;
            case "ImportError": return ExceptionClasses.ImportError;
            case "ModuleNotFoundError": return ExceptionClasses.ModuleNotFoundError;
            case "LookupError": return ExceptionClasses.LookupError;
            case "IndexError": return ExceptionClasses.IndexError;
            case "KeyError": return ExceptionClasses.KeyError;
            case "NameError": return ExceptionClasses.NameError;
            case "UnboundLocalError": return ExceptionClasses.UnboundLocalError;
            case "OSError": return ExceptionClasses.OSError;
            case "IOError": return ExceptionClasses.OSError;
            case "FileExistsError": return ExceptionClasses.FileExistsError;
            case "FileNotFoundError": return ExceptionClasses.FileNotFoundError;
            case "IsADirectoryError": return ExceptionClasses.IsADirectoryError;
            case "NotADirectoryError": return ExceptionClasses.NotADirectoryError;
            case "PermissionError": return ExceptionClasses.PermissionError;
            case "RuntimeError": return ExceptionClasses.RuntimeError;
            case "NotImplementedError": return ExceptionClasses.NotImplementedError;
            case "RecursionError": return ExceptionClasses.RecursionError;
            case "SyntaxError": return ExceptionClasses.SyntaxError;
            case "Warning": return ExceptionClasses.Warning;
            case "DeprecationWarning": return ExceptionClasses.DeprecationWarning;
            case "UserWarning": return ExceptionClasses.UserWarning;
            default:
                throw new $X.NameError("name '" + name + "' is not defined");
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
            if (args.length != 2) throw new $X.TypeError("isinstance() takes 2 arguments");
            $O obj = args[0];
            $O classinfo = args[1];
            
            // Handle tuple of types
            if (classinfo instanceof $T) {
                for ($O cls : (($T)classinfo).items) {
                    if (isInstanceOf(obj, cls)) {
                        return $B.TRUE;
                    }
                }
                return $B.FALSE;
            }
            
            return $B.of(isInstanceOf(obj, classinfo));
        }
        
        private boolean isInstanceOf($O obj, $O cls) {
            // Check exception types via $XO wrapper
            if (obj instanceof $XO && cls instanceof $Cls) {
                $Cls excCls = ($Cls) cls;
                $X exc = (($XO) obj).exception;
                
                // Use Java's instanceof via the javaClass field
                if (excCls.javaClass != null) {
                    return excCls.javaClass.isInstance(exc);
                }
            }
            
            // Check built-in types
            if (cls instanceof $Cls) {
                $Cls pyCls = ($Cls) cls;
                String name = pyCls.name;
                
                // Built-in type checks
                if ("int".equals(name)) return obj instanceof $I;
                if ("float".equals(name)) return obj instanceof $F;
                if ("str".equals(name)) return obj instanceof $S;
                if ("bool".equals(name)) return obj instanceof $B;
                if ("list".equals(name)) return obj instanceof $L;
                if ("tuple".equals(name)) return obj instanceof $T;
                if ("dict".equals(name)) return obj instanceof $D;
                if ("NoneType".equals(name)) return obj instanceof $N;
                if ("bytes".equals(name)) return obj instanceof $BY;
                if ("set".equals(name)) return obj instanceof $ST;
                
                // User-defined class check
                if (obj instanceof $Inst) {
                    $Inst inst = ($Inst) obj;
                    return isSubclass(inst.type, pyCls);
                }
            }
            
            return false;
        }
        
        private boolean isSubclass($Cls sub, $Cls sup) {
            if (sub == sup) return true;
            for ($Cls base : sub.bases) {
                if (isSubclass(base, sup)) return true;
            }
            return false;
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
            if (args.length < 1) throw new $X.TypeError("open() requires at least 1 argument");
            return _io.open(args[0],
                args.length > 1 ? args[1] : $S.of("r"),
                args.length > 2 ? args[2] : $I.of(-1),
                args.length > 3 ? args[3] : $N.INSTANCE,
                args.length > 4 ? args[4] : $N.INSTANCE,
                args.length > 5 ? args[5] : $N.INSTANCE,
                args.length > 6 ? args[6] : $B.TRUE,
                args.length > 7 ? args[7] : $N.INSTANCE);
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
    
    // Async builtins (using virtual threads)
    
    public static final $O ASYNC_GATHER = new $O() {
        @Override
        public $O __call__($O... args) {
            // Gather multiple coroutines/callables and run them concurrently
            $L coroutines = new $L();
            for ($O arg : args) {
                coroutines.append(arg);
            }
            return $Async.gather(coroutines);
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function async_gather>"); }
    };
    
    public static final $O ASYNC_SLEEP = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1) throw new $X("TypeError", "async_sleep() takes 1 argument");
            double seconds;
            if (args[0] instanceof $I) {
                seconds = (double)(($I)args[0]).value;
            } else if (args[0] instanceof $F) {
                seconds = (($F)args[0]).value;
            } else {
                throw new $X("TypeError", "sleep time must be a number");
            }
            $Async.sleep(seconds);
            return $N.INSTANCE;
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function async_sleep>"); }
    };
    
    public static final $O ASYNC_RUN = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length < 1) throw new $X("TypeError", "async_run() takes at least 1 argument");
            if (!(args[0] instanceof $MH)) {
                throw new $X("TypeError", "async_run() argument must be a coroutine");
            }
            $MH coroutine = ($MH)args[0];
            $O[] remaining = new $O[args.length - 1];
            System.arraycopy(args, 1, remaining, 0, remaining.length);
            
            $Future future = $Async.run(coroutine, remaining);
            return future.get();  // Block until complete
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function async_run>"); }
    };
    
    public static final $O ASYNC_THREAD = new $O() {
        @Override
        public $O __call__($O... args) {
            return $Async.currentThread();
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function async_thread>"); }
    };
    
    // Class-related builtins
    
    public static final $O SUPER = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) {
                // Zero-argument super() - requires magic to get class and self
                // For now, require explicit arguments
                throw new $X("TypeError", "super() requires at least 1 argument (use super(ClassName, self))");
            }
            if (args.length == 1) {
                if (!(args[0] instanceof $Cls)) {
                    throw new $X("TypeError", "super() argument 1 must be a class");
                }
                return new $Super(($Cls) args[0]);
            }
            if (args.length == 2) {
                if (!(args[0] instanceof $Cls)) {
                    throw new $X("TypeError", "super() argument 1 must be a class");
                }
                return new $Super(($Cls) args[0], args[1]);
            }
            throw new $X("TypeError", "super() takes 1 or 2 arguments");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'super'>"); }
    };
    
    public static final $O STATICMETHOD = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "staticmethod() takes exactly 1 argument");
            return new $SM(args[0]);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'staticmethod'>"); }
    };
    
    public static final $O CLASSMETHOD = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "classmethod() takes exactly 1 argument");
            return new $CM(args[0]);
        }
        @Override
        public $S __repr__() { return $S.of("<class 'classmethod'>"); }
    };
    
    public static final $O PROPERTY = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length == 0) {
                // Return a property that can be used as decorator
                return new $Prop(null);
            }
            if (args.length == 1) {
                return new $Prop(args[0]);
            }
            if (args.length == 2) {
                return new $Prop(args[0], args[1]);
            }
            if (args.length == 3) {
                return new $Prop(args[0], args[1], args[2]);
            }
            if (args.length == 4) {
                String doc = args[3] instanceof $S ? (($S)args[3]).value : null;
                return new $Prop(args[0], args[1], args[2], doc);
            }
            throw new $X("TypeError", "property() takes at most 4 arguments");
        }
        @Override
        public $O __getattr__(String name) {
            // Allow property.setter etc. as decorators
            if (name.equals("setter") || name.equals("getter") || name.equals("deleter")) {
                return new $Prop(null).__getattr__(name);
            }
            throw new $X("AttributeError", "type object 'property' has no attribute '" + name + "'");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'property'>"); }
    };
    
    public static final $O MEMORYVIEW = new $O() {
        @Override
        public $O __call__($O... args) {
            if (args.length != 1) throw new $X("TypeError", "memoryview() takes exactly 1 argument");
            if (args[0] instanceof $BY) {
                return new $MV(($BY) args[0]);
            }
            throw new $X("TypeError", "memoryview: a bytes-like object is required");
        }
        @Override
        public $S __repr__() { return $S.of("<class 'memoryview'>"); }
    };
    
    public static final $O EXEC = new $O() {
        @Override
        public $O __call__($O... args) {
            // exec() requires runtime compilation which we don't support
            // Provide a stub that raises NotImplementedError
            throw new $X("NotImplementedError", 
                "exec() is not supported - Loretta compiles ahead-of-time");
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function exec>"); }
    };
    
    public static final $O EVAL = new $O() {
        @Override
        public $O __call__($O... args) {
            // eval() requires runtime compilation which we don't support
            // For simple cases, we could potentially evaluate literals
            if (args.length >= 1 && args[0] instanceof $S) {
                String code = (($S) args[0]).value.trim();
                
                // Try to evaluate simple literals
                try {
                    // Integer literal
                    if (code.matches("-?\\d+")) {
                        return $I.of(Long.parseLong(code));
                    }
                    // Float literal
                    if (code.matches("-?\\d+\\.\\d*([eE][+-]?\\d+)?")) {
                        return $F.of(Double.parseDouble(code));
                    }
                    // String literal
                    if ((code.startsWith("'") && code.endsWith("'")) ||
                        (code.startsWith("\"") && code.endsWith("\""))) {
                        return $S.of(code.substring(1, code.length() - 1));
                    }
                    // Boolean literals
                    if (code.equals("True")) return $B.TRUE;
                    if (code.equals("False")) return $B.FALSE;
                    if (code.equals("None")) return $N.INSTANCE;
                } catch (NumberFormatException e) {
                    // Fall through to error
                }
            }
            throw new $X("NotImplementedError", 
                "eval() only supports simple literals - Loretta compiles ahead-of-time");
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function eval>"); }
    };
    
    public static final $O COMPILE = new $O() {
        @Override
        public $O __call__($O... args) {
            // compile() requires runtime compilation which we don't support
            throw new $X("NotImplementedError", 
                "compile() is not supported - Loretta compiles ahead-of-time");
        }
        @Override
        public $S __repr__() { return $S.of("<built-in function compile>"); }
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
            throw new $X.StopIteration();
        }
        if (range.step < 0 && current <= range.stop) {
            throw new $X.StopIteration();
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
