import java.io.File;

/**
 * $Sys - sys module implementation.
 * 
 * Provides Python's sys module functionality including:
 * - sys.path: Module search paths
 * - sys.modules: Loaded module registry
 * - sys.argv: Command line arguments
 * - sys.version: Python version info
 * - sys.platform: Platform identifier
 */
public final class $Sys {
    
    /** Module search paths */
    public static final $L path = new $L();
    
    /** Loaded modules dictionary (wraps $G.modules) */
    public static final $SysModules modules = new $SysModules();
    
    /** Command line arguments */
    public static $L argv = new $L();
    
    /** Python version (we emulate 3.12) */
    public static final $S version = $S.of("3.12.0 (Loretta JVM)");
    
    /** Version info tuple */
    public static final $T version_info = $T.of(
        $I.of(3), $I.of(12), $I.of(0), $S.of("final"), $I.of(0)
    );
    
    /** Platform identifier */
    public static final $S platform = $S.of("java");
    
    /** Standard file descriptors */
    public static $O stdin = $N.INSTANCE;  // TODO: implement properly
    public static $O stdout = $N.INSTANCE; // TODO: implement properly
    public static $O stderr = $N.INSTANCE; // TODO: implement properly
    
    /** Maximum recursion depth */
    public static $I maxsize = $I.of(Integer.MAX_VALUE);
    
    /** Meta path for import hooks */
    public static final $L meta_path = new $L();
    
    /** Path hooks for import */
    public static final $L path_hooks = new $L();
    
    /** Path importer cache */
    public static final $D path_importer_cache = new $D();
    
    /** Executable path */
    public static $S executable = $S.of(System.getProperty("java.home") + "/bin/java");
    
    /** Prefix (Python installation prefix) */
    public static $S prefix = $S.of(System.getProperty("java.home"));
    
    static {
        // Initialize sys.path with current directory and classpath
        path.append($S.of("."));
        
        String classpath = System.getProperty("java.class.path", "");
        for (String p : classpath.split(File.pathSeparator)) {
            if (!p.isEmpty()) {
                path.append($S.of(p));
            }
        }
        
        // Initialize argv with empty list (will be set by main)
        argv.append($S.of("")); // argv[0] is script name
    }
    
    /**
     * Set command line arguments.
     */
    public static void setArgv(String[] args) {
        argv = new $L();
        if (args != null) {
            for (String arg : args) {
                argv.append($S.of(arg));
            }
        }
        if (argv.items.isEmpty()) {
            argv.append($S.of(""));
        }
    }
    
    /**
     * Add a path to sys.path.
     */
    public static void addPath(String p) {
        $S pathStr = $S.of(p);
        // Check if already in path
        for ($O item : path.items) {
            if (item instanceof $S && ((($S)item).value.equals(p))) {
                return;
            }
        }
        path.append(pathStr);
    }
    
    /**
     * Exit the program.
     */
    public static void exit($O code) {
        int exitCode = 0;
        if (code instanceof $I) {
            exitCode = (int)(($I)code).value;
        } else if (code instanceof $B) {
            exitCode = (($B)code).__bool__() ? 0 : 1;
        } else if (code != null && code != $N.INSTANCE) {
            System.err.println(code.__str__().value);
            exitCode = 1;
        }
        System.exit(exitCode);
    }
    
    /**
     * Get an attribute from the sys module.
     */
    public static $O getAttr(String name) {
        switch (name) {
            case "path": return path;
            case "modules": return modules;
            case "argv": return argv;
            case "version": return version;
            case "version_info": return version_info;
            case "platform": return platform;
            case "stdin": return stdin;
            case "stdout": return stdout;
            case "stderr": return stderr;
            case "maxsize": return maxsize;
            case "meta_path": return meta_path;
            case "path_hooks": return path_hooks;
            case "path_importer_cache": return path_importer_cache;
            case "executable": return executable;
            case "prefix": return prefix;
            default:
                throw new $X("AttributeError", "module 'sys' has no attribute '" + name + "'");
        }
    }
    
    /**
     * Wrapper for sys.modules that exposes $G.modules as a dict-like object.
     */
    public static class $SysModules extends $O {
        @Override
        public $O __getitem__($O key) {
            if (key instanceof $S) {
                String name = (($S)key).value;
                if ($G.hasModule(name)) {
                    return $G.getModule(name);
                }
            }
            throw new $X("KeyError", key.__repr__().value);
        }
        
        @Override
        public void __setitem__($O key, $O value) {
            if (key instanceof $S && value instanceof $Mod) {
                $G.registerModule((($S)key).value, ($Mod)value);
            }
        }
        
        @Override
        public $B __contains__($O key) {
            if (key instanceof $S) {
                return $B.of($G.hasModule((($S)key).value));
            }
            return $B.FALSE;
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<sys.modules>");
        }
        
        public $O get($O key) {
            return get(key, $N.INSTANCE);
        }
        
        public $O get($O key, $O defaultValue) {
            if (key instanceof $S) {
                String name = (($S)key).value;
                if ($G.hasModule(name)) {
                    return $G.getModule(name);
                }
            }
            return defaultValue;
        }
        
        public $L keys() {
            // This would need $G to expose module names
            return new $L();
        }
    }
}
