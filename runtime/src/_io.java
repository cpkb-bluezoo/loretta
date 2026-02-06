import java.io.*;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

/**
 * _io - Low-level I/O module.
 * 
 * Provides the I/O class hierarchy used by Python's io module:
 * - IOBase: abstract base for all I/O
 * - RawIOBase: raw (unbuffered) I/O
 * - BufferedIOBase: buffered I/O
 * - TextIOBase: text I/O with encoding
 */
public class _io {
    
    // Default buffer size
    public static final int DEFAULT_BUFFER_SIZE = 8192;
    
    // Seek constants
    public static final int SEEK_SET = 0;
    public static final int SEEK_CUR = 1;
    public static final int SEEK_END = 2;
    
    /**
     * IOBase - Abstract base class for all I/O.
     */
    public static abstract class IOBase extends $O {
        protected boolean closed = false;
        
        public void close() {
            if (!closed) {
                try {
                    doClose();
                } catch (IOException e) {
                    throw new $X.OSError(e.getMessage());
                } finally {
                    closed = true;
                }
            }
        }
        
        protected abstract void doClose() throws IOException;
        
        public $B readable() { return $B.FALSE; }
        public $B writable() { return $B.FALSE; }
        public $B seekable() { return $B.FALSE; }
        
        public void flush() {
            // Don't check closed - may be called during close
        }
        
        public $B isatty() { return $B.FALSE; }
        
        public $I fileno() {
            throw new $X.OSError("fileno() not supported");
        }
        
        protected void checkClosed() {
            if (closed) {
                throw new $X.ValueError("I/O operation on closed file");
            }
        }
        
        protected void checkReadable() {
            checkClosed();
            if (!readable().__bool__()) {
                throw new $X.OSError("not readable");
            }
        }
        
        protected void checkWritable() {
            checkClosed();
            if (!writable().__bool__()) {
                throw new $X.OSError("not writable");
            }
        }
        
        protected void checkSeekable() {
            checkClosed();
            if (!seekable().__bool__()) {
                throw new $X.OSError("not seekable");
            }
        }
        
        // Context manager support
        public IOBase __enter__() {
            return this;
        }
        
        public $O __exit__($O excType, $O excVal, $O excTb) {
            close();
            return $N.INSTANCE;
        }
        
        @Override
        public $O __getattr__(String name) {
            final IOBase self = this;
            switch (name) {
                case "closed": return $B.of(closed);
                case "close": return new $O() {
                    @Override public $O __call__($O... args) { self.close(); return $N.INSTANCE; }
                };
                case "flush": return new $O() {
                    @Override public $O __call__($O... args) { self.flush(); return $N.INSTANCE; }
                };
                case "readable": return new $O() {
                    @Override public $O __call__($O... args) { return self.readable(); }
                };
                case "writable": return new $O() {
                    @Override public $O __call__($O... args) { return self.writable(); }
                };
                case "seekable": return new $O() {
                    @Override public $O __call__($O... args) { return self.seekable(); }
                };
                case "isatty": return new $O() {
                    @Override public $O __call__($O... args) { return self.isatty(); }
                };
                case "fileno": return new $O() {
                    @Override public $O __call__($O... args) { return self.fileno(); }
                };
                case "__enter__": return new $O() {
                    @Override public $O __call__($O... args) { return self.__enter__(); }
                };
                case "__exit__": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.__exit__(
                            args.length > 0 ? args[0] : $N.INSTANCE,
                            args.length > 1 ? args[1] : $N.INSTANCE,
                            args.length > 2 ? args[2] : $N.INSTANCE
                        );
                    }
                };
                default: return super.__getattr__(name);
            }
        }
    }
    
    /**
     * RawIOBase - Base class for raw (unbuffered) I/O.
     */
    public static abstract class RawIOBase extends IOBase {
        
        public abstract $O read($O size);
        public abstract $O readall();
        public abstract $I write($O data);
        
        /**
         * Read bytes into a writable buffer (bytearray). Returns number of bytes read,
         * or 0 at EOF. Override in subclasses to read directly into buffer when possible.
         */
        public $O readinto($O buffer) {
            checkReadable();
            if (!(buffer instanceof $BY)) {
                throw new $X.TypeError("readinto() argument must be a bytes-like object, not "
                    + (buffer != null ? buffer.getClass().getSimpleName() : "NoneType"));
            }
            byte[] dest = (($BY) buffer).data;
            if (dest.length == 0) return $I.of(0);
            $O chunk = read($I.of(dest.length));
            if (!(chunk instanceof $BY)) return $I.of(0);
            byte[] src = (($BY) chunk).data;
            int n = src.length;
            if (n > 0) System.arraycopy(src, 0, dest, 0, n);
            return $I.of(n);
        }
        
        @Override
        public $O __getattr__(String name) {
            final RawIOBase self = this;
            switch (name) {
                case "read": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.read(args.length > 0 ? args[0] : $I.of(-1)); 
                    }
                };
                case "readall": return new $O() {
                    @Override public $O __call__($O... args) { return self.readall(); }
                };
                case "write": return new $O() {
                    @Override public $O __call__($O... args) { return self.write(args[0]); }
                };
                case "readinto": return new $O() {
                    @Override public $O __call__($O... args) { return self.readinto(args[0]); }
                };
                default: return super.__getattr__(name);
            }
        }
    }
    
    /**
     * BufferedIOBase - Base class for buffered I/O.
     */
    public static abstract class BufferedIOBase extends IOBase {
        
        protected RawIOBase raw;
        
        public RawIOBase detach() {
            RawIOBase r = raw;
            raw = null;
            return r;
        }
        
        public abstract $O read($O size);
        public abstract $O read1($O size);
        public abstract $I write($O data);
        
        /** Read bytes into a writable buffer; uses read() and copies. */
        public $O readinto($O buffer) {
            checkClosed();
            if (!readable().__bool__()) throw new $X.OSError("not readable");
            if (!(buffer instanceof $BY)) {
                throw new $X.TypeError("readinto() argument must be a bytes-like object, not "
                    + (buffer != null ? buffer.getClass().getSimpleName() : "NoneType"));
            }
            byte[] dest = (($BY) buffer).data;
            if (dest.length == 0) return $I.of(0);
            $O chunk = read($I.of(dest.length));
            if (!(chunk instanceof $BY)) return $I.of(0);
            byte[] src = (($BY) chunk).data;
            int n = src.length;
            if (n > 0) System.arraycopy(src, 0, dest, 0, n);
            return $I.of(n);
        }
        
        @Override
        public $O __getattr__(String name) {
            final BufferedIOBase self = this;
            switch (name) {
                case "raw": return raw;
                case "detach": return new $O() {
                    @Override public $O __call__($O... args) { return self.detach(); }
                };
                case "read": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.read(args.length > 0 ? args[0] : $I.of(-1)); 
                    }
                };
                case "read1": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.read1(args.length > 0 ? args[0] : $I.of(-1)); 
                    }
                };
                case "readinto": return new $O() {
                    @Override public $O __call__($O... args) { return self.readinto(args[0]); }
                };
                case "write": return new $O() {
                    @Override public $O __call__($O... args) { return self.write(args[0]); }
                };
                default: return super.__getattr__(name);
            }
        }
    }
    
    /**
     * TextIOBase - Base class for text I/O.
     */
    public static abstract class TextIOBase extends IOBase {
        
        protected String encoding = "utf-8";
        protected String errors = "strict";
        protected String newline = null;
        
        public abstract $S read($O size);
        public abstract $I write($O text);
        public abstract $S readline($O limit);
        
        public $L readlines($O hint) {
            $L lines = new $L();
            int bytesRead = 0;
            int hintBytes = hint instanceof $I ? (int)(($I)hint).value : -1;
            
            while (true) {
                $S line = readline($I.of(-1));
                if (line.value.isEmpty()) break;
                lines.append(line);
                bytesRead += line.value.length();
                if (hintBytes > 0 && bytesRead >= hintBytes) break;
            }
            return lines;
        }
        
        public $N writelines($O lines) {
            $O iter = lines.__iter__();
            while (true) {
                try {
                    $O line = iter.__next__();
                    write(line);
                } catch ($X e) {
                    if (e.isStopIteration()) break;
                    throw e;
                }
            }
            return $N.INSTANCE;
        }
        
        @Override
        public $O __iter__() {
            return this;
        }
        
        @Override
        public $O __next__() {
            $S line = readline($I.of(-1));
            if (line.value.isEmpty()) {
                throw new $X.StopIteration();
            }
            return line;
        }
        
        @Override
        public $O __getattr__(String name) {
            final TextIOBase self = this;
            switch (name) {
                case "encoding": return $S.of(encoding);
                case "errors": return $S.of(errors);
                case "newlines": return newline != null ? $S.of(newline) : $N.INSTANCE;
                case "read": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.read(args.length > 0 ? args[0] : $I.of(-1)); 
                    }
                };
                case "readline": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.readline(args.length > 0 ? args[0] : $I.of(-1)); 
                    }
                };
                case "readlines": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.readlines(args.length > 0 ? args[0] : $I.of(-1)); 
                    }
                };
                case "write": return new $O() {
                    @Override public $O __call__($O... args) { return self.write(args[0]); }
                };
                case "writelines": return new $O() {
                    @Override public $O __call__($O... args) { return self.writelines(args[0]); }
                };
                default: return super.__getattr__(name);
            }
        }
    }
    
    /**
     * FileIO - Raw file I/O (unbuffered).
     */
    public static class FileIO extends RawIOBase {
        private RandomAccessFile file;
        private FileChannel channel;
        private final String name;
        private final String mode;
        private final boolean reading;
        private final boolean writing;
        private final boolean appending;
        private final boolean creating;
        private int fd = -1;
        
        public FileIO($O name, $O mode, $O closefd, $O opener) {
            this.name = (($S)name).value;
            this.mode = mode instanceof $S ? (($S)mode).value : "r";
            
            this.reading = this.mode.contains("r");
            this.writing = this.mode.contains("w") || this.mode.contains("a") || this.mode.contains("+");
            this.appending = this.mode.contains("a");
            this.creating = this.mode.contains("x") || this.mode.contains("w");
            
            try {
                File f = new File(this.name);
                
                if (this.mode.contains("x") && f.exists()) {
                    throw new $X.FileExistsError("File exists: '" + this.name + "'");
                }
                
                if (this.reading && !this.writing && !f.exists()) {
                    throw new $X.FileNotFoundError("No such file or directory: '" + this.name + "'");
                }
                
                String rafMode = this.writing ? "rw" : "r";
                this.file = new RandomAccessFile(f, rafMode);
                this.channel = file.getChannel();
                
                if (this.mode.contains("w")) {
                    // Truncate
                    file.setLength(0);
                }
                
                if (this.appending) {
                    file.seek(file.length());
                }
                
            } catch ($X e) {
                throw e;
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        @Override
        public $B readable() { return $B.of(reading); }
        
        @Override
        public $B writable() { return $B.of(writing); }
        
        @Override
        public $B seekable() { return $B.TRUE; }
        
        @Override
        protected void doClose() throws IOException {
            if (file != null) {
                channel.close();
                file.close();
            }
        }
        
        @Override
        public $O read($O size) {
            checkReadable();
            try {
                int n = size instanceof $I ? (int)(($I)size).value : -1;
                if (n < 0) {
                    return readall();
                }
                byte[] buf = new byte[n];
                int len = file.read(buf);
                if (len < 0) return $BY.of(new byte[0]);
                if (len < n) {
                    byte[] result = new byte[len];
                    System.arraycopy(buf, 0, result, 0, len);
                    return $BY.of(result);
                }
                return $BY.of(buf);
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        @Override
        public $O readall() {
            checkReadable();
            try {
                long remaining = file.length() - file.getFilePointer();
                byte[] buf = new byte[(int)remaining];
                file.readFully(buf);
                return $BY.of(buf);
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        @Override
        public $O readinto($O buffer) {
            checkReadable();
            if (!(buffer instanceof $BY)) {
                throw new $X.TypeError("readinto() argument must be a bytes-like object, not "
                    + (buffer != null ? buffer.getClass().getSimpleName() : "NoneType"));
            }
            byte[] dest = (($BY) buffer).data;
            if (dest.length == 0) return $I.of(0);
            try {
                int n = file.read(dest);
                return $I.of(n < 0 ? 0 : n);
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        @Override
        public $I write($O data) {
            checkWritable();
            try {
                byte[] bytes;
                if (data instanceof $BY) {
                    bytes = (($BY)data).data;
                } else if (data instanceof $S) {
                    bytes = (($S)data).value.getBytes(StandardCharsets.UTF_8);
                } else {
                    throw new $X.TypeError("a bytes-like object is required");
                }
                file.write(bytes);
                return $I.of(bytes.length);
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        public $I seek($O pos, $O whence) {
            checkSeekable();
            try {
                long offset = (($I)pos).value;
                int w = whence instanceof $I ? (int)(($I)whence).value : SEEK_SET;
                
                long newPos;
                switch (w) {
                    case SEEK_SET: newPos = offset; break;
                    case SEEK_CUR: newPos = file.getFilePointer() + offset; break;
                    case SEEK_END: newPos = file.length() + offset; break;
                    default: throw new $X.ValueError("invalid whence value");
                }
                file.seek(newPos);
                return $I.of(newPos);
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        public $I tell() {
            checkClosed();
            try {
                return $I.of(file.getFilePointer());
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        public $N truncate($O size) {
            checkWritable();
            try {
                long s = size instanceof $I ? (($I)size).value : file.getFilePointer();
                file.setLength(s);
                return $N.INSTANCE;
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        @Override
        public $O __getattr__(String name) {
            final FileIO self = this;
            switch (name) {
                case "name": return $S.of(this.name);
                case "mode": return $S.of(this.mode);
                case "seek": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.seek(args[0], args.length > 1 ? args[1] : $I.of(SEEK_SET)); 
                    }
                };
                case "tell": return new $O() {
                    @Override public $O __call__($O... args) { return self.tell(); }
                };
                case "truncate": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.truncate(args.length > 0 ? args[0] : $N.INSTANCE); 
                    }
                };
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<_io.FileIO name='" + name + "' mode='" + mode + "'>");
        }
    }
    
    /**
     * BufferedReader - Buffered reading.
     */
    public static class BufferedReader extends BufferedIOBase {
        private byte[] buffer;
        private int bufferPos = 0;
        private int bufferEnd = 0;
        
        public BufferedReader(RawIOBase raw, int bufferSize) {
            this.raw = raw;
            this.buffer = new byte[bufferSize > 0 ? bufferSize : DEFAULT_BUFFER_SIZE];
        }
        
        public BufferedReader(RawIOBase raw) {
            this(raw, DEFAULT_BUFFER_SIZE);
        }
        
        @Override
        public $B readable() { return $B.TRUE; }
        
        @Override
        public $B seekable() { return raw.seekable(); }
        
        @Override
        protected void doClose() throws IOException {
            raw.close();
        }
        
        private void fillBuffer() {
            $O data = raw.read($I.of(buffer.length));
            if (data instanceof $BY) {
                byte[] bytes = (($BY)data).data;
                System.arraycopy(bytes, 0, buffer, 0, bytes.length);
                bufferPos = 0;
                bufferEnd = bytes.length;
            } else {
                bufferEnd = 0;
            }
        }
        
        @Override
        public $O read($O size) {
            checkReadable();
            int n = size instanceof $I ? (int)(($I)size).value : -1;
            
            if (n < 0) {
                // Read all
                ByteArrayOutputStream out = new ByteArrayOutputStream();
                // First, drain buffer
                if (bufferPos < bufferEnd) {
                    out.write(buffer, bufferPos, bufferEnd - bufferPos);
                    bufferPos = bufferEnd;
                }
                // Then read rest directly
                $O rest = raw.readall();
                if (rest instanceof $BY) {
                    try {
                        out.write((($BY)rest).data);
                    } catch (IOException e) {
                        throw new $X.OSError(e.getMessage());
                    }
                }
                return $BY.of(out.toByteArray());
            }
            
            byte[] result = new byte[n];
            int read = 0;
            
            // Read from buffer first
            while (read < n && bufferPos < bufferEnd) {
                result[read++] = buffer[bufferPos++];
            }
            
            // If we need more, fill buffer and continue
            while (read < n) {
                fillBuffer();
                if (bufferEnd == 0) break;  // EOF
                while (read < n && bufferPos < bufferEnd) {
                    result[read++] = buffer[bufferPos++];
                }
            }
            
            if (read < n) {
                byte[] truncated = new byte[read];
                System.arraycopy(result, 0, truncated, 0, read);
                return $BY.of(truncated);
            }
            return $BY.of(result);
        }
        
        @Override
        public $O read1($O size) {
            checkReadable();
            int n = size instanceof $I ? (int)(($I)size).value : -1;
            if (n < 0) n = buffer.length;
            
            if (bufferPos >= bufferEnd) {
                fillBuffer();
                if (bufferEnd == 0) return $BY.of(new byte[0]);
            }
            
            int available = Math.min(n, bufferEnd - bufferPos);
            byte[] result = new byte[available];
            System.arraycopy(buffer, bufferPos, result, 0, available);
            bufferPos += available;
            return $BY.of(result);
        }
        
        @Override
        public $I write($O data) {
            throw new $X.OSError("not writable");
        }
        
        public $O peek($O size) {
            checkReadable();
            int n = size instanceof $I ? (int)(($I)size).value : 1;
            
            if (bufferPos >= bufferEnd) {
                fillBuffer();
            }
            
            int available = Math.min(n, bufferEnd - bufferPos);
            byte[] result = new byte[available];
            System.arraycopy(buffer, bufferPos, result, 0, available);
            return $BY.of(result);
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<_io.BufferedReader>");
        }
    }
    
    /**
     * BufferedWriter - Buffered writing.
     */
    public static class BufferedWriter extends BufferedIOBase {
        private byte[] buffer;
        private int bufferPos = 0;
        
        public BufferedWriter(RawIOBase raw, int bufferSize) {
            this.raw = raw;
            this.buffer = new byte[bufferSize > 0 ? bufferSize : DEFAULT_BUFFER_SIZE];
        }
        
        public BufferedWriter(RawIOBase raw) {
            this(raw, DEFAULT_BUFFER_SIZE);
        }
        
        @Override
        public $B writable() { return $B.TRUE; }
        
        @Override
        public $B seekable() { return raw.seekable(); }
        
        @Override
        protected void doClose() throws IOException {
            flush();
            raw.close();
        }
        
        @Override
        public void flush() {
            // Don't check closed - may be called during close
            if (bufferPos > 0) {
                byte[] data = new byte[bufferPos];
                System.arraycopy(buffer, 0, data, 0, bufferPos);
                raw.write($BY.of(data));
                bufferPos = 0;
            }
        }
        
        @Override
        public $O read($O size) {
            throw new $X.OSError("not readable");
        }
        
        @Override
        public $O read1($O size) {
            throw new $X.OSError("not readable");
        }
        
        @Override
        public $I write($O data) {
            checkWritable();
            byte[] bytes;
            if (data instanceof $BY) {
                bytes = (($BY)data).data;
            } else if (data instanceof $S) {
                bytes = (($S)data).value.getBytes(StandardCharsets.UTF_8);
            } else {
                throw new $X.TypeError("a bytes-like object is required");
            }
            
            int written = 0;
            while (written < bytes.length) {
                int space = buffer.length - bufferPos;
                int toWrite = Math.min(space, bytes.length - written);
                System.arraycopy(bytes, written, buffer, bufferPos, toWrite);
                bufferPos += toWrite;
                written += toWrite;
                
                if (bufferPos >= buffer.length) {
                    flush();
                }
            }
            
            return $I.of(bytes.length);
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<_io.BufferedWriter>");
        }
    }
    
    /**
     * TextIOWrapper - Text I/O with encoding.
     */
    public static class TextIOWrapper extends TextIOBase {
        private BufferedIOBase buffer;
        private Charset charset;
        private final String name;
        private final String mode;
        private boolean lineBuffering;
        private StringBuilder pendingLine = new StringBuilder();
        
        public TextIOWrapper(BufferedIOBase buffer, $O encoding, $O errors, $O newline, $O line_buffering) {
            this.buffer = buffer;
            this.encoding = encoding instanceof $S ? (($S)encoding).value : "utf-8";
            this.errors = errors instanceof $S ? (($S)errors).value : "strict";
            this.newline = newline instanceof $S ? (($S)newline).value : null;
            this.lineBuffering = line_buffering instanceof $B && (($B)line_buffering).__bool__();
            
            try {
                this.charset = Charset.forName(this.encoding);
            } catch (Exception e) {
                throw new $X.LookupError("unknown encoding: " + this.encoding);
            }
            
            // Get name and mode from underlying buffer if available
            if (buffer.raw instanceof FileIO) {
                FileIO fio = (FileIO)buffer.raw;
                this.name = fio.name;
                this.mode = fio.mode;
            } else {
                this.name = "<stream>";
                this.mode = "?";
            }
        }
        
        @Override
        public $B readable() { return buffer.readable(); }
        
        @Override
        public $B writable() { return buffer.writable(); }
        
        @Override
        public $B seekable() { return buffer.seekable(); }
        
        @Override
        protected void doClose() throws IOException {
            buffer.close();
        }
        
        @Override
        public void flush() {
            checkClosed();
            buffer.flush();
        }
        
        @Override
        public $S read($O size) {
            checkReadable();
            int n = size instanceof $I ? (int)(($I)size).value : -1;
            
            $O data = buffer.read(n < 0 ? $I.of(-1) : $I.of(n * 4));  // Estimate bytes needed
            if (!(data instanceof $BY)) return $S.of("");
            
            byte[] bytes = (($BY)data).data;
            String s = new String(bytes, charset);
            
            if (n >= 0 && s.length() > n) {
                // We read too much - would need to seek back in real impl
                return $S.of(s.substring(0, n));
            }
            return $S.of(s);
        }
        
        @Override
        public $S readline($O limit) {
            checkReadable();
            int maxLen = limit instanceof $I ? (int)(($I)limit).value : -1;
            
            StringBuilder line = new StringBuilder();
            
            while (true) {
                $O chunk = buffer.read1($I.of(1));
                if (!(chunk instanceof $BY) || (($BY)chunk).data.length == 0) {
                    break;  // EOF
                }
                
                String s = new String((($BY)chunk).data, charset);
                line.append(s);
                
                if (s.contains("\n")) {
                    break;
                }
                
                if (maxLen > 0 && line.length() >= maxLen) {
                    break;
                }
            }
            
            return $S.of(line.toString());
        }
        
        @Override
        public $I write($O text) {
            checkWritable();
            String s = text instanceof $S ? (($S)text).value : text.__str__().value;
            byte[] bytes = s.getBytes(charset);
            buffer.write($BY.of(bytes));
            
            if (lineBuffering && s.contains("\n")) {
                flush();
            }
            
            return $I.of(s.length());
        }
        
        public $I seek($O pos, $O whence) {
            checkSeekable();
            // Text seek is complex - simplify to byte seek
            if (buffer.raw instanceof FileIO) {
                return ((FileIO)buffer.raw).seek(pos, whence);
            }
            throw new $X.OSError("seek not supported");
        }
        
        public $I tell() {
            checkClosed();
            if (buffer.raw instanceof FileIO) {
                return ((FileIO)buffer.raw).tell();
            }
            throw new $X.OSError("tell not supported");
        }
        
        @Override
        public $O __getattr__(String name) {
            final TextIOWrapper self = this;
            switch (name) {
                case "name": return $S.of(this.name);
                case "mode": return $S.of(this.mode);
                case "buffer": return this.buffer;
                case "line_buffering": return $B.of(lineBuffering);
                case "seek": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.seek(args[0], args.length > 1 ? args[1] : $I.of(SEEK_SET)); 
                    }
                };
                case "tell": return new $O() {
                    @Override public $O __call__($O... args) { return self.tell(); }
                };
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<_io.TextIOWrapper name='" + name + "' mode='" + mode + "' encoding='" + encoding + "'>");
        }
    }
    
    /**
     * BytesIO - In-memory bytes I/O.
     */
    public static class BytesIO extends BufferedIOBase {
        private byte[] buffer;
        private int position = 0;
        private int length = 0;
        
        public BytesIO() {
            this.buffer = new byte[DEFAULT_BUFFER_SIZE];
        }
        
        public BytesIO($O initial) {
            if (initial instanceof $BY) {
                byte[] data = (($BY)initial).data;
                this.buffer = new byte[Math.max(data.length, DEFAULT_BUFFER_SIZE)];
                System.arraycopy(data, 0, this.buffer, 0, data.length);
                this.length = data.length;
            } else {
                this.buffer = new byte[DEFAULT_BUFFER_SIZE];
            }
        }
        
        @Override
        public $B readable() { return $B.TRUE; }
        
        @Override
        public $B writable() { return $B.TRUE; }
        
        @Override
        public $B seekable() { return $B.TRUE; }
        
        @Override
        protected void doClose() {}
        
        @Override
        public $O read($O size) {
            checkClosed();
            int n = size instanceof $I ? (int)(($I)size).value : -1;
            if (n < 0) n = length - position;
            n = Math.min(n, length - position);
            
            byte[] result = new byte[n];
            System.arraycopy(buffer, position, result, 0, n);
            position += n;
            return $BY.of(result);
        }
        
        @Override
        public $O read1($O size) {
            return read(size);
        }
        
        @Override
        public $O readinto($O buffer) {
            checkClosed();
            if (!(buffer instanceof $BY)) {
                throw new $X.TypeError("readinto() argument must be a bytes-like object, not "
                    + (buffer != null ? buffer.getClass().getSimpleName() : "NoneType"));
            }
            byte[] dest = (($BY) buffer).data;
            if (dest.length == 0) return $I.of(0);
            int available = length - position;
            int n = Math.min(dest.length, available);
            if (n > 0) {
                System.arraycopy(this.buffer, position, dest, 0, n);
                position += n;
            }
            return $I.of(n);
        }
        
        @Override
        public $I write($O data) {
            checkClosed();
            byte[] bytes;
            if (data instanceof $BY) {
                bytes = (($BY)data).data;
            } else if (data instanceof $S) {
                bytes = (($S)data).value.getBytes(StandardCharsets.UTF_8);
            } else {
                throw new $X.TypeError("a bytes-like object is required");
            }
            
            ensureCapacity(position + bytes.length);
            System.arraycopy(bytes, 0, buffer, position, bytes.length);
            position += bytes.length;
            if (position > length) length = position;
            
            return $I.of(bytes.length);
        }
        
        private void ensureCapacity(int needed) {
            if (needed > buffer.length) {
                int newSize = Math.max(buffer.length * 2, needed);
                byte[] newBuf = new byte[newSize];
                System.arraycopy(buffer, 0, newBuf, 0, length);
                buffer = newBuf;
            }
        }
        
        public $BY getvalue() {
            byte[] result = new byte[length];
            System.arraycopy(buffer, 0, result, 0, length);
            return $BY.of(result);
        }
        
        public $I seek($O pos, $O whence) {
            checkClosed();
            long offset = (($I)pos).value;
            int w = whence instanceof $I ? (int)(($I)whence).value : SEEK_SET;
            
            int newPos;
            switch (w) {
                case SEEK_SET: newPos = (int)offset; break;
                case SEEK_CUR: newPos = position + (int)offset; break;
                case SEEK_END: newPos = length + (int)offset; break;
                default: throw new $X.ValueError("invalid whence value");
            }
            
            if (newPos < 0) newPos = 0;
            position = newPos;
            return $I.of(position);
        }
        
        public $I tell() {
            checkClosed();
            return $I.of(position);
        }
        
        public $N truncate($O size) {
            checkClosed();
            int newLen = size instanceof $I ? (int)(($I)size).value : position;
            if (newLen < length) {
                length = newLen;
                if (position > length) position = length;
            }
            return $N.INSTANCE;
        }
        
        @Override
        public $O __getattr__(String name) {
            final BytesIO self = this;
            switch (name) {
                case "getvalue": return new $O() {
                    @Override public $O __call__($O... args) { return self.getvalue(); }
                };
                case "seek": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.seek(args[0], args.length > 1 ? args[1] : $I.of(SEEK_SET)); 
                    }
                };
                case "tell": return new $O() {
                    @Override public $O __call__($O... args) { return self.tell(); }
                };
                case "truncate": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.truncate(args.length > 0 ? args[0] : $N.INSTANCE); 
                    }
                };
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<_io.BytesIO>");
        }
    }
    
    /**
     * StringIO - In-memory text I/O.
     */
    public static class StringIO extends TextIOBase {
        private StringBuilder buffer;
        private int position = 0;
        
        public StringIO() {
            this.buffer = new StringBuilder();
        }
        
        public StringIO($O initial) {
            if (initial instanceof $S) {
                this.buffer = new StringBuilder((($S)initial).value);
            } else {
                this.buffer = new StringBuilder();
            }
        }
        
        @Override
        public $B readable() { return $B.TRUE; }
        
        @Override
        public $B writable() { return $B.TRUE; }
        
        @Override
        public $B seekable() { return $B.TRUE; }
        
        @Override
        protected void doClose() {}
        
        @Override
        public $S read($O size) {
            checkClosed();
            int n = size instanceof $I ? (int)(($I)size).value : -1;
            if (n < 0) n = buffer.length() - position;
            n = Math.min(n, buffer.length() - position);
            
            String result = buffer.substring(position, position + n);
            position += n;
            return $S.of(result);
        }
        
        @Override
        public $S readline($O limit) {
            checkClosed();
            int maxLen = limit instanceof $I ? (int)(($I)limit).value : -1;
            
            int end = position;
            while (end < buffer.length()) {
                if (buffer.charAt(end) == '\n') {
                    end++;
                    break;
                }
                end++;
                if (maxLen > 0 && end - position >= maxLen) break;
            }
            
            String result = buffer.substring(position, end);
            position = end;
            return $S.of(result);
        }
        
        @Override
        public $I write($O text) {
            checkClosed();
            String s = text instanceof $S ? (($S)text).value : text.__str__().value;
            
            // Overwrite or extend
            if (position < buffer.length()) {
                int end = Math.min(position + s.length(), buffer.length());
                buffer.replace(position, end, s.substring(0, end - position));
                if (s.length() > end - position) {
                    buffer.append(s.substring(end - position));
                }
            } else {
                // Extend with padding if needed
                while (buffer.length() < position) {
                    buffer.append('\0');
                }
                buffer.append(s);
            }
            position += s.length();
            
            return $I.of(s.length());
        }
        
        public $S getvalue() {
            return $S.of(buffer.toString());
        }
        
        public $I seek($O pos, $O whence) {
            checkClosed();
            long offset = (($I)pos).value;
            int w = whence instanceof $I ? (int)(($I)whence).value : SEEK_SET;
            
            int newPos;
            switch (w) {
                case SEEK_SET: newPos = (int)offset; break;
                case SEEK_CUR: newPos = position + (int)offset; break;
                case SEEK_END: newPos = buffer.length() + (int)offset; break;
                default: throw new $X.ValueError("invalid whence value");
            }
            
            if (newPos < 0) newPos = 0;
            position = newPos;
            return $I.of(position);
        }
        
        public $I tell() {
            checkClosed();
            return $I.of(position);
        }
        
        public $N truncate($O size) {
            checkClosed();
            int newLen = size instanceof $I ? (int)(($I)size).value : position;
            if (newLen < buffer.length()) {
                buffer.setLength(newLen);
                if (position > newLen) position = newLen;
            }
            return $N.INSTANCE;
        }
        
        @Override
        public $O __getattr__(String name) {
            final StringIO self = this;
            switch (name) {
                case "getvalue": return new $O() {
                    @Override public $O __call__($O... args) { return self.getvalue(); }
                };
                case "seek": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.seek(args[0], args.length > 1 ? args[1] : $I.of(SEEK_SET)); 
                    }
                };
                case "tell": return new $O() {
                    @Override public $O __call__($O... args) { return self.tell(); }
                };
                case "truncate": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.truncate(args.length > 0 ? args[0] : $N.INSTANCE); 
                    }
                };
                default: return super.__getattr__(name);
            }
        }
        
        @Override
        public $S __repr__() {
            return $S.of("<_io.StringIO>");
        }
    }
    
    /**
     * Factory: open() function.
     */
    public static $O open($O file, $O mode, $O buffering, $O encoding, $O errors, $O newline, $O closefd, $O opener) {
        String m = mode instanceof $S ? (($S)mode).value : "r";
        int buf = buffering instanceof $I ? (int)(($I)buffering).value : -1;
        
        boolean binary = m.contains("b");
        boolean reading = m.contains("r");
        boolean writing = m.contains("w") || m.contains("x") || m.contains("a");
        boolean updating = m.contains("+");
        
        // Create raw FileIO
        FileIO raw = new FileIO(file, $S.of(m.replace("t", "").replace("b", "") + (binary ? "" : "")), closefd, opener);
        
        if (buf == 0) {
            // Unbuffered - only valid for binary
            if (!binary) {
                throw new $X.ValueError("can't have unbuffered text I/O");
            }
            return raw;
        }
        
        // Create buffered layer
        BufferedIOBase buffered;
        if (updating) {
            // Would need BufferedRandom for read+write
            buffered = reading ? new BufferedReader(raw, buf > 0 ? buf : DEFAULT_BUFFER_SIZE)
                               : new BufferedWriter(raw, buf > 0 ? buf : DEFAULT_BUFFER_SIZE);
        } else if (writing) {
            buffered = new BufferedWriter(raw, buf > 0 ? buf : DEFAULT_BUFFER_SIZE);
        } else {
            buffered = new BufferedReader(raw, buf > 0 ? buf : DEFAULT_BUFFER_SIZE);
        }
        
        if (binary) {
            return buffered;
        }
        
        // Create text layer
        return new TextIOWrapper(buffered, encoding, errors, newline, $B.of(buf == 1));
    }
    
    /**
     * Get module attribute.
     */
    public static $O getAttr(String name) {
        switch (name) {
            case "DEFAULT_BUFFER_SIZE": return $I.of(DEFAULT_BUFFER_SIZE);
            case "SEEK_SET": return $I.of(SEEK_SET);
            case "SEEK_CUR": return $I.of(SEEK_CUR);
            case "SEEK_END": return $I.of(SEEK_END);
            default: return null;
        }
    }
}
