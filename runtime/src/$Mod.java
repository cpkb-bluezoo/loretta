/**
 * PyModule - Python module object.
 * 
 * Represents a Python module with its name and attributes dict.
 * Integrates with Java class loading to load compiled Python modules.
 */
public final class $Mod extends $O {
    
    /** Module name (e.g., "os", "mypackage.mymodule") */
    public final String name;
    
    /** Module attributes (functions, classes, variables) */
    public final $D attrs;
    
    /** Java class backing this module (if loaded from .class file) */
    private Class<?> javaClass;
    
    /** Whether __init__ has been run */
    private boolean initialized;
    
    /** Source file path (for __file__ attribute) */
    private String filePath;
    
    /** Package name (for __package__ attribute) */
    private String packageName;
    
    /**
     * Create a new module.
     */
    public $Mod(String name) {
        this.name = name;
        this.attrs = new $D();
        this.javaClass = null;
        this.initialized = false;
        this.filePath = null;
        this.packageName = "";
        setupModuleAttrs();
    }
    
    /**
     * Create a module backed by a Java class.
     */
    public $Mod(String name, Class<?> javaClass) {
        this.name = name;
        this.attrs = new $D();
        this.javaClass = javaClass;
        this.initialized = false;
        this.packageName = "";
        
        // Derive package name from module name
        int lastDot = name.lastIndexOf('.');
        if (lastDot > 0) {
            this.packageName = name.substring(0, lastDot);
        }
        
        // Try to find the source file
        if (javaClass != null) {
            // Use the class location as a proxy for the file
            try {
                java.security.CodeSource cs = javaClass.getProtectionDomain().getCodeSource();
                if (cs != null && cs.getLocation() != null) {
                    this.filePath = cs.getLocation().getPath() + "/" + 
                                    name.replace('.', '/') + ".class";
                }
            } catch (Exception e) {
                this.filePath = "<compiled>";
            }
        }
        setupModuleAttrs();
    }
    
    /**
     * Set up standard module attributes.
     */
    private void setupModuleAttrs() {
        attrs.__setitem__($S.of("__name__"), $S.of(name));
        attrs.__setitem__($S.of("__package__"), packageName.isEmpty() ? $N.INSTANCE : $S.of(packageName));
        attrs.__setitem__($S.of("__file__"), filePath != null ? $S.of(filePath) : $N.INSTANCE);
        attrs.__setitem__($S.of("__doc__"), $N.INSTANCE);
        attrs.__setitem__($S.of("__loader__"), $N.INSTANCE);
        attrs.__setitem__($S.of("__spec__"), $N.INSTANCE);
    }
    
    /**
     * Set the file path for this module.
     */
    public void setFilePath(String path) {
        this.filePath = path;
        attrs.__setitem__($S.of("__file__"), path != null ? $S.of(path) : $N.INSTANCE);
    }
    
    /**
     * Set the package name for this module.
     */
    public void setPackage(String pkg) {
        this.packageName = pkg != null ? pkg : "";
        attrs.__setitem__($S.of("__package__"), packageName.isEmpty() ? $N.INSTANCE : $S.of(packageName));
    }
    
    /**
     * Get an attribute from the module.
     */
    public $O getAttr(String attrName) {
        $O value = attrs.get($S.of(attrName));
        if (value == null) {
            throw new $X("AttributeError: module '" + name + "' has no attribute '" + attrName + "'");
        }
        return value;
    }
    
    /**
     * Set an attribute on the module.
     */
    public void setAttr(String attrName, $O value) {
        attrs.__setitem__($S.of(attrName), value);
    }
    
    /**
     * Check if module has an attribute.
     */
    public boolean hasAttr(String attrName) {
        return attrs.get($S.of(attrName)) != null;
    }
    
    /**
     * Initialize the module by running its Java class's static initializer
     * or calling its __init__ method.
     */
    public void initialize() {
        if (initialized) {
            return;
        }
        initialized = true;
        
        if (javaClass != null) {
            try {
                // Try to call the main method which runs module-level code
                java.lang.reflect.Method main = javaClass.getMethod("main", String[].class);
                main.invoke(null, (Object) new String[0]);
            } catch (NoSuchMethodException e) {
                // No main method, try __init__
                try {
                    java.lang.reflect.Method init = javaClass.getMethod("__init__", $Mod.class);
                    init.invoke(null, this);
                } catch (NoSuchMethodException e2) {
                    // Just loading the class may be sufficient (static initializers run)
                } catch (Exception e2) {
                    throw new $X("ImportError: failed to initialize module '" + name + "': " + e2.getMessage());
                }
            } catch (Exception e) {
                throw new $X("ImportError: failed to initialize module '" + name + "': " + e.getMessage());
            }
        }
    }
    
    /**
     * Get all public attribute names (for 'from module import *').
     */
    public $L getPublicNames() {
        $L names = new $L();
        for ($O key : attrs.keys().items) {
            String keyStr = key.toString();
            // Skip private names (starting with _)
            if (!keyStr.startsWith("_")) {
                names.append(key);
            }
        }
        return names;
    }
    
    @Override
    public $O __getattr__(String name) {
        return getAttr(name);
    }
    
    @Override
    public $S __repr__() {
        if (javaClass != null) {
            return $S.of("<module '" + name + "' from '" + javaClass.getName() + "'>");
        }
        return $S.of("<module '" + name + "'>");
    }
    
    @Override
    public $S __str__() {
        return __repr__();
    }
    
    @Override
    public boolean __bool__() {
        return true;
    }
    
    /**
     * Factory method.
     */
    public static $Mod of(String name) {
        return new $Mod(name);
    }
    
    /**
     * Factory method with Java class.
     */
    public static $Mod of(String name, Class<?> javaClass) {
        return new $Mod(name, javaClass);
    }
}
