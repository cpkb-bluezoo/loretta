import java.io.*;

/**
 * $File - File object for open().
 */
public class $File extends $O {
    
    private final String filename;
    private final String mode;
    private BufferedReader reader;
    private BufferedWriter writer;
    private boolean closed = false;
    
    public $File(String filename, String mode) throws IOException {
        this.filename = filename;
        this.mode = mode;
        
        if (mode.contains("r")) {
            reader = new BufferedReader(new FileReader(filename));
        }
        if (mode.contains("w")) {
            writer = new BufferedWriter(new FileWriter(filename));
        }
        if (mode.contains("a")) {
            writer = new BufferedWriter(new FileWriter(filename, true));
        }
    }
    
    @Override
    public $S __repr__() {
        return $S.of("<_io.TextIOWrapper name='" + filename + "' mode='" + mode + "'>");
    }
    
    @Override
    public $O __iter__() {
        return this;
    }
    
    @Override
    public $O __next__() {
        if (reader == null) throw new $X("UnsupportedOperation", "not readable");
        try {
            String line = reader.readLine();
            if (line == null) throw new $X.StopIteration();
            return $S.of(line + "\n");
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    public $S read() {
        return read($I.of(-1));
    }
    
    public $S read($O size) {
        if (reader == null) throw new $X("UnsupportedOperation", "not readable");
        try {
            int n = size instanceof $I ? (int)(($I)size).value : -1;
            if (n < 0) {
                StringBuilder sb = new StringBuilder();
                char[] buf = new char[8192];
                int len;
                while ((len = reader.read(buf)) != -1) {
                    sb.append(buf, 0, len);
                }
                return $S.of(sb.toString());
            }
            char[] buf = new char[n];
            int len = reader.read(buf);
            return $S.of(len > 0 ? new String(buf, 0, len) : "");
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    public $S readline() {
        if (reader == null) throw new $X("UnsupportedOperation", "not readable");
        try {
            String line = reader.readLine();
            return $S.of(line != null ? line + "\n" : "");
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    public $L readlines() {
        if (reader == null) throw new $X("UnsupportedOperation", "not readable");
        $L lines = new $L();
        try {
            String line;
            while ((line = reader.readLine()) != null) {
                lines.items.add($S.of(line + "\n"));
            }
            return lines;
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    public $N write($O text) {
        if (writer == null) throw new $X("UnsupportedOperation", "not writable");
        try {
            writer.write((($S)text).value);
            return $N.INSTANCE;
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    public $N writelines($O lines) {
        if (writer == null) throw new $X("UnsupportedOperation", "not writable");
        try {
            $O iter = lines.__iter__();
            while (true) {
                try {
                    $O line = iter.__next__();
                    writer.write((($S)line).value);
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            return $N.INSTANCE;
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    public $N flush() {
        try {
            if (writer != null) writer.flush();
            return $N.INSTANCE;
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    public $N close() {
        if (closed) return $N.INSTANCE;
        closed = true;
        try {
            if (reader != null) reader.close();
            if (writer != null) writer.close();
            return $N.INSTANCE;
        } catch (IOException e) {
            throw new $X("IOError", e.getMessage());
        }
    }
    
    @Override
    public $O __getattr__(String name) {
        switch (name) {
            case "name": return $S.of(filename);
            case "mode": return $S.of(mode);
            case "closed": return $B.of(closed);
            default: return super.__getattr__(name);
        }
    }
    
    // Context manager support
    public $File __enter__() {
        return this;
    }
    
    public $N __exit__($O excType, $O excVal, $O excTb) {
        close();
        return $N.INSTANCE;
    }
}
