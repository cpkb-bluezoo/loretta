import java.io.File;
import java.io.IOException;
import java.nio.file.*;
import java.nio.file.attribute.*;
import java.util.Map;

/**
 * _posix - Low-level OS operations for Unix-like systems.
 * 
 * This module provides the C-extension replacement for Python's posix/nt module,
 * which underlies the os module. Uses Java's File and NIO APIs.
 */
public class _posix {
    
    /** Module name */
    public static final $S name = $S.of("posix");
    
    /** Environment variables */
    public static final $D environ = loadEnviron();
    
    /** Path separator */
    public static final $S sep = $S.of(File.separator);
    
    /** Alternative path separator (Windows) - null on Unix */
    public static final $O altsep = File.separatorChar == '\\' ? $S.of("/") : $N.INSTANCE;
    
    /** Path list separator */
    public static final $S pathsep = $S.of(File.pathSeparator);
    
    /** Line separator */
    public static final $S linesep = $S.of(System.lineSeparator());
    
    /** Current directory string */
    public static final $S curdir = $S.of(".");
    
    /** Parent directory string */
    public static final $S pardir = $S.of("..");
    
    /** Extension separator */
    public static final $S extsep = $S.of(".");
    
    /** Null device */
    public static final $S devnull = $S.of(System.getProperty("os.name").toLowerCase().contains("win") 
        ? "NUL" : "/dev/null");
    
    /**
     * Load environment variables into a Python dict.
     */
    private static $D loadEnviron() {
        $D env = new $D();
        for (Map.Entry<String, String> entry : System.getenv().entrySet()) {
            env.__setitem__($S.of(entry.getKey()), $S.of(entry.getValue()));
        }
        return env;
    }
    
    /**
     * Get environment variable.
     */
    public static $O getenv($O key) {
        return getenv(key, $N.INSTANCE);
    }
    
    public static $O getenv($O key, $O defaultVal) {
        String k = (($S)key).value;
        String val = System.getenv(k);
        if (val == null) {
            return defaultVal;
        }
        return $S.of(val);
    }
    
    /**
     * Get current working directory.
     */
    public static $S getcwd() {
        return $S.of(System.getProperty("user.dir"));
    }
    
    /**
     * Alias for getcwd (bytes version, but we return string).
     */
    public static $S getcwdb() {
        return getcwd();
    }
    
    /**
     * Change current working directory.
     */
    public static void chdir($O path) {
        String p = pathStr(path);
        File dir = new File(p);
        if (!dir.isDirectory()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + p + "'");
        }
        System.setProperty("user.dir", dir.getAbsolutePath());
    }
    
    /**
     * List directory contents.
     */
    public static $L listdir($O path) {
        String p = pathStr(path);
        File dir = new File(p);
        if (!dir.exists()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + p + "'");
        }
        if (!dir.isDirectory()) {
            throw new $X.NotADirectoryError("Not a directory: '" + p + "'");
        }
        
        String[] files = dir.list();
        $L result = new $L();
        if (files != null) {
            for (String f : files) {
                result.append($S.of(f));
            }
        }
        return result;
    }
    
    /**
     * List directory with no argument (current directory).
     */
    public static $L listdir() {
        return listdir($S.of("."));
    }
    
    /**
     * Create a directory.
     */
    public static void mkdir($O path) {
        mkdir(path, $I.of(0777));
    }
    
    public static void mkdir($O path, $O mode) {
        String p = pathStr(path);
        File dir = new File(p);
        if (dir.exists()) {
            throw new $X.FileExistsError("File exists: '" + p + "'");
        }
        if (!dir.mkdir()) {
            throw new $X.OSError("Cannot create directory: '" + p + "'");
        }
    }
    
    /**
     * Create directories recursively.
     */
    public static void makedirs($O path) {
        makedirs(path, $I.of(0777), $B.FALSE);
    }
    
    public static void makedirs($O path, $O mode) {
        makedirs(path, mode, $B.FALSE);
    }
    
    public static void makedirs($O path, $O mode, $O exist_ok) {
        String p = pathStr(path);
        File dir = new File(p);
        if (dir.exists()) {
            if (!exist_ok.__bool__()) {
                throw new $X.FileExistsError("File exists: '" + p + "'");
            }
            return;
        }
        if (!dir.mkdirs()) {
            throw new $X.OSError("Cannot create directories: '" + p + "'");
        }
    }
    
    /**
     * Remove a file.
     */
    public static void remove($O path) {
        String p = pathStr(path);
        File file = new File(p);
        if (!file.exists()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + p + "'");
        }
        if (file.isDirectory()) {
            throw new $X.IsADirectoryError("Is a directory: '" + p + "'");
        }
        if (!file.delete()) {
            throw new $X.OSError("Cannot remove file: '" + p + "'");
        }
    }
    
    /**
     * Alias for remove.
     */
    public static void unlink($O path) {
        remove(path);
    }
    
    /**
     * Remove an empty directory.
     */
    public static void rmdir($O path) {
        String p = pathStr(path);
        File dir = new File(p);
        if (!dir.exists()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + p + "'");
        }
        if (!dir.isDirectory()) {
            throw new $X.NotADirectoryError("Not a directory: '" + p + "'");
        }
        if (!dir.delete()) {
            throw new $X.OSError("Cannot remove directory: '" + p + "'");
        }
    }
    
    /**
     * Rename a file or directory.
     */
    public static void rename($O src, $O dst) {
        String s = pathStr(src);
        String d = pathStr(dst);
        File srcFile = new File(s);
        File dstFile = new File(d);
        if (!srcFile.exists()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + s + "'");
        }
        if (!srcFile.renameTo(dstFile)) {
            throw new $X.OSError("Cannot rename '" + s + "' to '" + d + "'");
        }
    }
    
    /**
     * Rename with replace semantics.
     */
    public static void replace($O src, $O dst) {
        String s = pathStr(src);
        String d = pathStr(dst);
        try {
            Files.move(Paths.get(s), Paths.get(d), StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException e) {
            throw new $X.OSError("Cannot replace '" + s + "' with '" + d + "': " + e.getMessage());
        }
    }
    
    /**
     * Get file status (stat).
     * Returns a stat_result tuple-like object.
     */
    public static $O stat($O path) {
        String p = pathStr(path);
        File file = new File(p);
        if (!file.exists()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + p + "'");
        }
        return new stat_result(file);
    }
    
    /**
     * Get file status, don't follow symlinks.
     */
    public static $O lstat($O path) {
        // Java doesn't easily distinguish - just call stat
        return stat(path);
    }
    
    /**
     * Check file access.
     */
    public static $B access($O path, $O mode) {
        String p = pathStr(path);
        File file = new File(p);
        int m = (int)(($I)mode).value;
        
        if ((m & F_OK) != 0 && !file.exists()) return $B.FALSE;
        if ((m & R_OK) != 0 && !file.canRead()) return $B.FALSE;
        if ((m & W_OK) != 0 && !file.canWrite()) return $B.FALSE;
        if ((m & X_OK) != 0 && !file.canExecute()) return $B.FALSE;
        
        return $B.TRUE;
    }
    
    // Access mode constants
    public static final int F_OK = 0;
    public static final int R_OK = 4;
    public static final int W_OK = 2;
    public static final int X_OK = 1;
    
    /**
     * Check if path exists.
     */
    public static $B path_exists($O path) {
        return $B.of(new File(pathStr(path)).exists());
    }
    
    /**
     * Check if path is a file.
     */
    public static $B path_isfile($O path) {
        return $B.of(new File(pathStr(path)).isFile());
    }
    
    /**
     * Check if path is a directory.
     */
    public static $B path_isdir($O path) {
        return $B.of(new File(pathStr(path)).isDirectory());
    }
    
    /**
     * Check if path is absolute.
     */
    public static $B path_isabs($O path) {
        return $B.of(new File(pathStr(path)).isAbsolute());
    }
    
    /**
     * Get absolute path.
     */
    public static $S path_abspath($O path) {
        try {
            return $S.of(new File(pathStr(path)).getCanonicalPath());
        } catch (IOException e) {
            return $S.of(new File(pathStr(path)).getAbsolutePath());
        }
    }
    
    /**
     * Get real path (resolve symlinks).
     */
    public static $S path_realpath($O path) {
        try {
            return $S.of(Paths.get(pathStr(path)).toRealPath().toString());
        } catch (IOException e) {
            return path_abspath(path);
        }
    }
    
    /**
     * Get basename of path.
     */
    public static $S path_basename($O path) {
        String p = pathStr(path);
        int idx = Math.max(p.lastIndexOf('/'), p.lastIndexOf('\\'));
        return $S.of(idx < 0 ? p : p.substring(idx + 1));
    }
    
    /**
     * Get dirname of path.
     */
    public static $S path_dirname($O path) {
        String p = pathStr(path);
        int idx = Math.max(p.lastIndexOf('/'), p.lastIndexOf('\\'));
        return $S.of(idx < 0 ? "" : p.substring(0, idx));
    }
    
    /**
     * Join path components.
     */
    public static $S path_join($O... parts) {
        if (parts.length == 0) return $S.of("");
        
        StringBuilder result = new StringBuilder(pathStr(parts[0]));
        for (int i = 1; i < parts.length; i++) {
            String part = pathStr(parts[i]);
            if (part.isEmpty()) continue;
            
            // Absolute path resets
            if (new File(part).isAbsolute()) {
                result = new StringBuilder(part);
                continue;
            }
            
            // Add separator if needed
            if (result.length() > 0) {
                char last = result.charAt(result.length() - 1);
                if (last != '/' && last != '\\') {
                    result.append(File.separator);
                }
            }
            result.append(part);
        }
        return $S.of(result.toString());
    }
    
    /**
     * Split path into (head, tail).
     */
    public static $T path_split($O path) {
        String p = pathStr(path);
        int idx = Math.max(p.lastIndexOf('/'), p.lastIndexOf('\\'));
        if (idx < 0) {
            return $T.of($S.of(""), $S.of(p));
        }
        return $T.of($S.of(p.substring(0, idx)), $S.of(p.substring(idx + 1)));
    }
    
    /**
     * Split extension.
     */
    public static $T path_splitext($O path) {
        String p = pathStr(path);
        int dot = p.lastIndexOf('.');
        int sep = Math.max(p.lastIndexOf('/'), p.lastIndexOf('\\'));
        if (dot <= sep) {
            return $T.of($S.of(p), $S.of(""));
        }
        return $T.of($S.of(p.substring(0, dot)), $S.of(p.substring(dot)));
    }
    
    /**
     * Normalize path.
     */
    public static $S path_normpath($O path) {
        try {
            return $S.of(Paths.get(pathStr(path)).normalize().toString());
        } catch (Exception e) {
            return (($S)path);
        }
    }
    
    /**
     * Expand user home directory.
     */
    public static $S path_expanduser($O path) {
        String p = pathStr(path);
        if (p.startsWith("~")) {
            String home = System.getProperty("user.home");
            if (p.length() == 1) {
                return $S.of(home);
            } else if (p.charAt(1) == '/' || p.charAt(1) == '\\') {
                return $S.of(home + p.substring(1));
            }
        }
        return $S.of(p);
    }
    
    /**
     * Get file size.
     */
    public static $I path_getsize($O path) {
        String p = pathStr(path);
        File file = new File(p);
        if (!file.exists()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + p + "'");
        }
        return $I.of(file.length());
    }
    
    /**
     * Get modification time.
     */
    public static $F path_getmtime($O path) {
        String p = pathStr(path);
        File file = new File(p);
        if (!file.exists()) {
            throw new $X.FileNotFoundError("No such file or directory: '" + p + "'");
        }
        return $F.of(file.lastModified() / 1000.0);
    }
    
    /**
     * Walk directory tree.
     */
    public static $L walk($O top) {
        return walk(top, $B.TRUE, $N.INSTANCE, $B.FALSE);
    }
    
    public static $L walk($O top, $O topdown, $O onerror, $O followlinks) {
        $L result = new $L();
        walkDir(new File(pathStr(top)), result, topdown.__bool__());
        return result;
    }
    
    private static void walkDir(File dir, $L result, boolean topdown) {
        if (!dir.isDirectory()) return;
        
        $L dirs = new $L();
        $L files = new $L();
        
        File[] contents = dir.listFiles();
        if (contents != null) {
            for (File f : contents) {
                if (f.isDirectory()) {
                    dirs.append($S.of(f.getName()));
                } else {
                    files.append($S.of(f.getName()));
                }
            }
        }
        
        $T entry = $T.of($S.of(dir.getPath()), dirs, files);
        
        if (topdown) {
            result.append(entry);
            for ($O d : dirs.items) {
                walkDir(new File(dir, (($S)d).value), result, topdown);
            }
        } else {
            for ($O d : dirs.items) {
                walkDir(new File(dir, (($S)d).value), result, topdown);
            }
            result.append(entry);
        }
    }
    
    /**
     * Helper to convert path object to string.
     */
    private static String pathStr($O path) {
        if (path instanceof $S) {
            return (($S)path).value;
        }
        return path.__str__().value;
    }
    
    /**
     * stat_result - Result of stat() call.
     */
    public static class stat_result extends $O {
        public final $I st_mode;
        public final $I st_ino;
        public final $I st_dev;
        public final $I st_nlink;
        public final $I st_uid;
        public final $I st_gid;
        public final $I st_size;
        public final $F st_atime;
        public final $F st_mtime;
        public final $F st_ctime;
        
        public stat_result(File file) {
            // Mode: approximate from Java
            int mode = 0;
            if (file.isDirectory()) mode |= 0040000;  // S_IFDIR
            else if (file.isFile()) mode |= 0100000;  // S_IFREG
            if (file.canRead()) mode |= 0444;
            if (file.canWrite()) mode |= 0222;
            if (file.canExecute()) mode |= 0111;
            
            this.st_mode = $I.of(mode);
            this.st_ino = $I.of(0);  // Not available in Java
            this.st_dev = $I.of(0);
            this.st_nlink = $I.of(1);
            this.st_uid = $I.of(0);
            this.st_gid = $I.of(0);
            this.st_size = $I.of(file.length());
            
            long mtime = file.lastModified();
            this.st_atime = $F.of(mtime / 1000.0);
            this.st_mtime = $F.of(mtime / 1000.0);
            this.st_ctime = $F.of(mtime / 1000.0);
        }
        
        @Override
        public $O __getattr__(String name) {
            switch (name) {
                case "st_mode": return st_mode;
                case "st_ino": return st_ino;
                case "st_dev": return st_dev;
                case "st_nlink": return st_nlink;
                case "st_uid": return st_uid;
                case "st_gid": return st_gid;
                case "st_size": return st_size;
                case "st_atime": return st_atime;
                case "st_mtime": return st_mtime;
                case "st_ctime": return st_ctime;
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("os.stat_result(st_mode=" + st_mode.value + 
                ", st_size=" + st_size.value + ", st_mtime=" + st_mtime.value + ")");
        }
    }
    
    /**
     * Get module attribute.
     */
    public static $O getAttr(String name) {
        switch (name) {
            case "name": return _posix.name;
            case "sep": return sep;
            case "altsep": return altsep != null ? altsep : $N.INSTANCE;
            case "pathsep": return pathsep;
            case "linesep": return linesep;
            case "curdir": return curdir;
            case "pardir": return pardir;
            case "extsep": return extsep;
            case "devnull": return devnull;
            case "environ": return environ;
            case "F_OK": return $I.of(F_OK);
            case "R_OK": return $I.of(R_OK);
            case "W_OK": return $I.of(W_OK);
            case "X_OK": return $I.of(X_OK);
            default: return null;
        }
    }
}
