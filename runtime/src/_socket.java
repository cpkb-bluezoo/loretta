import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.channels.*;
import java.util.Set;

/**
 * _socket - Low-level networking module.
 * 
 * Provides socket operations for TCP and UDP networking.
 */
public class _socket {
    
    // Address families
    public static final int AF_INET = 2;
    public static final int AF_INET6 = 10;
    public static final int AF_UNIX = 1;
    public static final int AF_UNSPEC = 0;
    
    // Socket types
    public static final int SOCK_STREAM = 1;  // TCP
    public static final int SOCK_DGRAM = 2;   // UDP
    public static final int SOCK_RAW = 3;
    
    // Protocol numbers
    public static final int IPPROTO_TCP = 6;
    public static final int IPPROTO_UDP = 17;
    public static final int IPPROTO_IP = 0;
    
    // Socket options
    public static final int SOL_SOCKET = 1;
    public static final int SO_REUSEADDR = 2;
    public static final int SO_KEEPALIVE = 9;
    public static final int SO_BROADCAST = 6;
    public static final int SO_RCVBUF = 8;
    public static final int SO_SNDBUF = 7;
    public static final int SO_RCVTIMEO = 20;
    public static final int SO_SNDTIMEO = 21;
    
    public static final int IPPROTO_IPV6 = 41;
    public static final int TCP_NODELAY = 1;
    
    // Shutdown modes
    public static final int SHUT_RD = 0;
    public static final int SHUT_WR = 1;
    public static final int SHUT_RDWR = 2;
    
    // Special addresses
    public static final $S INADDR_ANY = $S.of("0.0.0.0");
    public static final $S INADDR_BROADCAST = $S.of("255.255.255.255");
    public static final $S INADDR_LOOPBACK = $S.of("127.0.0.1");
    
    /**
     * socket - Network socket class.
     */
    public static class socket extends $O {
        private final int family;
        private final int type;
        private final int proto;
        
        private java.net.Socket tcpSocket;
        private java.net.ServerSocket serverSocket;
        private java.net.DatagramSocket udpSocket;
        private SocketChannel tcpChannel;
        private DatagramChannel udpChannel;
        private ServerSocketChannel serverChannel;
        
        private InputStream inputStream;
        private OutputStream outputStream;
        private boolean closed = false;
        private boolean blocking = true;
        private int timeout = 0;  // milliseconds, 0 = no timeout
        
        public socket($O family, $O type, $O proto) {
            this.family = family instanceof $I ? (int)(($I)family).value : AF_INET;
            this.type = type instanceof $I ? (int)(($I)type).value : SOCK_STREAM;
            this.proto = proto instanceof $I ? (int)(($I)proto).value : 0;
        }
        
        public socket() {
            this($I.of(AF_INET), $I.of(SOCK_STREAM), $I.of(0));
        }
        
        // Private constructor for accepted sockets
        private socket(java.net.Socket acceptedSocket, int family, int type, int proto) {
            this.family = family;
            this.type = type;
            this.proto = proto;
            this.tcpSocket = acceptedSocket;
            try {
                this.inputStream = acceptedSocket.getInputStream();
                this.outputStream = acceptedSocket.getOutputStream();
            } catch (IOException e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        /**
         * Connect to remote address.
         */
        public $N connect($O address) {
            checkClosed();
            $T addr = ($T)address;
            String host = (($S)addr.items[0]).value;
            int port = (int)(($I)addr.items[1]).value;
            
            try {
                if (type == SOCK_STREAM) {
                    if (blocking) {
                        tcpSocket = new java.net.Socket();
                        if (timeout > 0) {
                            tcpSocket.connect(new InetSocketAddress(host, port), timeout);
                        } else {
                            tcpSocket.connect(new InetSocketAddress(host, port));
                        }
                        inputStream = tcpSocket.getInputStream();
                        outputStream = tcpSocket.getOutputStream();
                    } else {
                        tcpChannel = SocketChannel.open();
                        tcpChannel.configureBlocking(false);
                        tcpChannel.connect(new InetSocketAddress(host, port));
                    }
                } else if (type == SOCK_DGRAM) {
                    if (udpSocket == null) {
                        udpSocket = new DatagramSocket();
                    }
                    udpSocket.connect(new InetSocketAddress(host, port));
                }
            } catch (IOException e) {
                throw new $X.OSError("Connection failed: " + e.getMessage());
            }
            return $N.INSTANCE;
        }
        
        /**
         * Bind to local address.
         */
        public $N bind($O address) {
            checkClosed();
            $T addr = ($T)address;
            String host = (($S)addr.items[0]).value;
            int port = (int)(($I)addr.items[1]).value;
            
            try {
                InetSocketAddress sockAddr = host.isEmpty() 
                    ? new InetSocketAddress(port)
                    : new InetSocketAddress(host, port);
                
                if (type == SOCK_STREAM) {
                    serverSocket = new ServerSocket();
                    serverSocket.setReuseAddress(true);
                    serverSocket.bind(sockAddr);
                } else if (type == SOCK_DGRAM) {
                    udpSocket = new DatagramSocket(sockAddr);
                }
            } catch (IOException e) {
                throw new $X.OSError("Bind failed: " + e.getMessage());
            }
            return $N.INSTANCE;
        }
        
        /**
         * Listen for connections.
         */
        public $N listen($O backlog) {
            checkClosed();
            int bl = backlog instanceof $I ? (int)(($I)backlog).value : 5;
            
            try {
                if (serverSocket == null) {
                    throw new $X.OSError("Socket not bound");
                }
                // ServerSocket backlog is set in bind() for Java
                // Just validate we have a server socket
            } catch (Exception e) {
                throw new $X.OSError("Listen failed: " + e.getMessage());
            }
            return $N.INSTANCE;
        }
        
        /**
         * Accept a connection.
         */
        public $T accept() {
            checkClosed();
            try {
                if (serverSocket == null) {
                    throw new $X.OSError("Socket not bound");
                }
                
                if (timeout > 0) {
                    serverSocket.setSoTimeout(timeout);
                }
                
                java.net.Socket client = serverSocket.accept();
                socket clientSock = new socket(client, family, type, proto);
                
                InetSocketAddress remoteAddr = (InetSocketAddress)client.getRemoteSocketAddress();
                $T addr = $T.of($S.of(remoteAddr.getAddress().getHostAddress()), $I.of(remoteAddr.getPort()));
                
                return $T.of(clientSock, addr);
            } catch (SocketTimeoutException e) {
                throw new $X("timeout", "timed out");
            } catch (IOException e) {
                throw new $X.OSError("Accept failed: " + e.getMessage());
            }
        }
        
        /**
         * Send data.
         */
        public $I send($O data) {
            return send(data, $I.of(0));
        }
        
        public $I send($O data, $O flags) {
            checkClosed();
            byte[] bytes = getBytes(data);
            
            try {
                if (type == SOCK_STREAM) {
                    if (outputStream != null) {
                        outputStream.write(bytes);
                        return $I.of(bytes.length);
                    } else if (tcpChannel != null) {
                        ByteBuffer buf = ByteBuffer.wrap(bytes);
                        return $I.of(tcpChannel.write(buf));
                    }
                    throw new $X.OSError("Socket not connected");
                } else if (type == SOCK_DGRAM) {
                    if (udpSocket != null && udpSocket.isConnected()) {
                        DatagramPacket packet = new DatagramPacket(bytes, bytes.length);
                        udpSocket.send(packet);
                        return $I.of(bytes.length);
                    }
                    throw new $X.OSError("Socket not connected");
                }
            } catch (IOException e) {
                throw new $X.OSError("Send failed: " + e.getMessage());
            }
            return $I.of(0);
        }
        
        /**
         * Send all data (blocks until complete).
         */
        public $N sendall($O data) {
            sendall(data, $I.of(0));
            return $N.INSTANCE;
        }
        
        public $N sendall($O data, $O flags) {
            checkClosed();
            byte[] bytes = getBytes(data);
            
            try {
                if (type == SOCK_STREAM && outputStream != null) {
                    outputStream.write(bytes);
                    outputStream.flush();
                } else {
                    send(data, flags);
                }
            } catch (IOException e) {
                throw new $X.OSError("Sendall failed: " + e.getMessage());
            }
            return $N.INSTANCE;
        }
        
        /**
         * Send data to specific address (UDP).
         */
        public $I sendto($O data, $O address) {
            return sendto(data, $I.of(0), address);
        }
        
        public $I sendto($O data, $O flags, $O address) {
            checkClosed();
            byte[] bytes = getBytes(data);
            $T addr = ($T)address;
            String host = (($S)addr.items[0]).value;
            int port = (int)(($I)addr.items[1]).value;
            
            try {
                if (type == SOCK_DGRAM) {
                    if (udpSocket == null) {
                        udpSocket = new DatagramSocket();
                    }
                    InetAddress inetAddr = InetAddress.getByName(host);
                    DatagramPacket packet = new DatagramPacket(bytes, bytes.length, inetAddr, port);
                    udpSocket.send(packet);
                    return $I.of(bytes.length);
                }
                throw new $X.OSError("sendto only supported for UDP sockets");
            } catch (IOException e) {
                throw new $X.OSError("Sendto failed: " + e.getMessage());
            }
        }
        
        /**
         * Receive data.
         */
        public $BY recv($O bufsize) {
            return recv(bufsize, $I.of(0));
        }
        
        public $BY recv($O bufsize, $O flags) {
            checkClosed();
            int size = (int)(($I)bufsize).value;
            byte[] buf = new byte[size];
            
            try {
                if (type == SOCK_STREAM) {
                    if (inputStream != null) {
                        if (timeout > 0 && tcpSocket != null) {
                            tcpSocket.setSoTimeout(timeout);
                        }
                        int read = inputStream.read(buf);
                        if (read < 0) return $BY.of(new byte[0]);
                        if (read < size) {
                            byte[] result = new byte[read];
                            System.arraycopy(buf, 0, result, 0, read);
                            return $BY.of(result);
                        }
                        return $BY.of(buf);
                    } else if (tcpChannel != null) {
                        ByteBuffer bb = ByteBuffer.allocate(size);
                        int read = tcpChannel.read(bb);
                        if (read < 0) return $BY.of(new byte[0]);
                        byte[] result = new byte[read];
                        bb.flip();
                        bb.get(result);
                        return $BY.of(result);
                    }
                    throw new $X.OSError("Socket not connected");
                } else if (type == SOCK_DGRAM) {
                    if (udpSocket != null) {
                        DatagramPacket packet = new DatagramPacket(buf, buf.length);
                        udpSocket.receive(packet);
                        byte[] result = new byte[packet.getLength()];
                        System.arraycopy(packet.getData(), 0, result, 0, packet.getLength());
                        return $BY.of(result);
                    }
                    throw new $X.OSError("Socket not bound");
                }
            } catch (SocketTimeoutException e) {
                throw new $X("timeout", "timed out");
            } catch (IOException e) {
                throw new $X.OSError("Recv failed: " + e.getMessage());
            }
            return $BY.of(new byte[0]);
        }
        
        /**
         * Receive data and sender address (UDP).
         */
        public $T recvfrom($O bufsize) {
            return recvfrom(bufsize, $I.of(0));
        }
        
        public $T recvfrom($O bufsize, $O flags) {
            checkClosed();
            int size = (int)(($I)bufsize).value;
            byte[] buf = new byte[size];
            
            try {
                if (type == SOCK_DGRAM) {
                    if (udpSocket == null) {
                        throw new $X.OSError("Socket not bound");
                    }
                    DatagramPacket packet = new DatagramPacket(buf, buf.length);
                    udpSocket.receive(packet);
                    
                    byte[] data = new byte[packet.getLength()];
                    System.arraycopy(packet.getData(), 0, data, 0, packet.getLength());
                    
                    InetSocketAddress addr = (InetSocketAddress)packet.getSocketAddress();
                    $T addrTuple = $T.of($S.of(addr.getAddress().getHostAddress()), $I.of(addr.getPort()));
                    
                    return $T.of($BY.of(data), addrTuple);
                }
                throw new $X.OSError("recvfrom only supported for UDP sockets");
            } catch (IOException e) {
                throw new $X.OSError("Recvfrom failed: " + e.getMessage());
            }
        }
        
        /**
         * Close the socket.
         */
        public $N close() {
            if (closed) return $N.INSTANCE;
            closed = true;
            
            try {
                if (tcpSocket != null) tcpSocket.close();
                if (serverSocket != null) serverSocket.close();
                if (udpSocket != null) udpSocket.close();
                if (tcpChannel != null) tcpChannel.close();
                if (udpChannel != null) udpChannel.close();
                if (serverChannel != null) serverChannel.close();
            } catch (IOException e) {
                // Ignore close errors
            }
            return $N.INSTANCE;
        }
        
        /**
         * Shutdown one or both halves of the connection.
         */
        public $N shutdown($O how) {
            checkClosed();
            int h = (int)(($I)how).value;
            
            try {
                if (tcpSocket != null) {
                    if (h == SHUT_RD || h == SHUT_RDWR) {
                        tcpSocket.shutdownInput();
                    }
                    if (h == SHUT_WR || h == SHUT_RDWR) {
                        tcpSocket.shutdownOutput();
                    }
                }
            } catch (IOException e) {
                throw new $X.OSError("Shutdown failed: " + e.getMessage());
            }
            return $N.INSTANCE;
        }
        
        /**
         * Set socket option.
         */
        public $N setsockopt($O level, $O optname, $O value) {
            checkClosed();
            int opt = (int)(($I)optname).value;
            
            try {
                if (tcpSocket != null) {
                    switch (opt) {
                        case SO_REUSEADDR:
                            tcpSocket.setReuseAddress(toBool(value));
                            break;
                        case SO_KEEPALIVE:
                            tcpSocket.setKeepAlive(toBool(value));
                            break;
                        case SO_RCVBUF:
                            tcpSocket.setReceiveBufferSize((int)(($I)value).value);
                            break;
                        case SO_SNDBUF:
                            tcpSocket.setSendBufferSize((int)(($I)value).value);
                            break;
                        case TCP_NODELAY:
                            tcpSocket.setTcpNoDelay(toBool(value));
                            break;
                    }
                }
                if (serverSocket != null) {
                    switch (opt) {
                        case SO_REUSEADDR:
                            serverSocket.setReuseAddress(toBool(value));
                            break;
                        case SO_RCVBUF:
                            serverSocket.setReceiveBufferSize((int)(($I)value).value);
                            break;
                    }
                }
                if (udpSocket != null) {
                    switch (opt) {
                        case SO_REUSEADDR:
                            udpSocket.setReuseAddress(toBool(value));
                            break;
                        case SO_BROADCAST:
                            udpSocket.setBroadcast(toBool(value));
                            break;
                        case SO_RCVBUF:
                            udpSocket.setReceiveBufferSize((int)(($I)value).value);
                            break;
                        case SO_SNDBUF:
                            udpSocket.setSendBufferSize((int)(($I)value).value);
                            break;
                    }
                }
            } catch (IOException e) {
                throw new $X.OSError("setsockopt failed: " + e.getMessage());
            }
            return $N.INSTANCE;
        }
        
        /**
         * Get socket option.
         */
        public $O getsockopt($O level, $O optname) {
            checkClosed();
            int opt = (int)(($I)optname).value;
            
            try {
                if (tcpSocket != null) {
                    switch (opt) {
                        case SO_REUSEADDR: return $B.of(tcpSocket.getReuseAddress());
                        case SO_KEEPALIVE: return $B.of(tcpSocket.getKeepAlive());
                        case SO_RCVBUF: return $I.of(tcpSocket.getReceiveBufferSize());
                        case SO_SNDBUF: return $I.of(tcpSocket.getSendBufferSize());
                        case TCP_NODELAY: return $B.of(tcpSocket.getTcpNoDelay());
                    }
                }
            } catch (IOException e) {
                throw new $X.OSError("getsockopt failed: " + e.getMessage());
            }
            return $I.of(0);
        }
        
        /**
         * Set blocking mode.
         */
        public $N setblocking($O flag) {
            blocking = flag.__bool__();
            try {
                if (tcpChannel != null) {
                    tcpChannel.configureBlocking(blocking);
                }
            } catch (IOException e) {
                throw new $X.OSError("setblocking failed: " + e.getMessage());
            }
            return $N.INSTANCE;
        }
        
        /**
         * Get blocking mode.
         */
        public $B getblocking() {
            return $B.of(blocking);
        }
        
        /**
         * Set timeout.
         */
        public $N settimeout($O timeout) {
            if (timeout instanceof $N) {
                this.timeout = 0;
                this.blocking = true;
            } else if (timeout instanceof $I) {
                this.timeout = (int)((($I)timeout).value * 1000);
                this.blocking = this.timeout > 0;
            } else if (timeout instanceof $F) {
                this.timeout = (int)((($F)timeout).value * 1000);
                this.blocking = this.timeout > 0;
            }
            return $N.INSTANCE;
        }
        
        /**
         * Get timeout.
         */
        public $O gettimeout() {
            return timeout == 0 ? $N.INSTANCE : $F.of(timeout / 1000.0);
        }
        
        /**
         * Get peer address.
         */
        public $T getpeername() {
            checkClosed();
            try {
                InetSocketAddress addr = null;
                if (tcpSocket != null) {
                    addr = (InetSocketAddress)tcpSocket.getRemoteSocketAddress();
                } else if (udpSocket != null && udpSocket.isConnected()) {
                    addr = (InetSocketAddress)udpSocket.getRemoteSocketAddress();
                }
                if (addr == null) {
                    throw new $X.OSError("Socket not connected");
                }
                return $T.of($S.of(addr.getAddress().getHostAddress()), $I.of(addr.getPort()));
            } catch ($X e) {
                throw e;
            } catch (Exception e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        /**
         * Get local address.
         */
        public $T getsockname() {
            checkClosed();
            try {
                InetSocketAddress addr = null;
                if (tcpSocket != null) {
                    addr = (InetSocketAddress)tcpSocket.getLocalSocketAddress();
                } else if (serverSocket != null) {
                    addr = (InetSocketAddress)serverSocket.getLocalSocketAddress();
                } else if (udpSocket != null) {
                    addr = (InetSocketAddress)udpSocket.getLocalSocketAddress();
                }
                if (addr == null) {
                    return $T.of($S.of("0.0.0.0"), $I.of(0));
                }
                return $T.of($S.of(addr.getAddress().getHostAddress()), $I.of(addr.getPort()));
            } catch (Exception e) {
                throw new $X.OSError(e.getMessage());
            }
        }
        
        /**
         * Get file descriptor.
         */
        public $I fileno() {
            // Java doesn't expose real file descriptors
            return $I.of(-1);
        }
        
        /**
         * Create file-like object from socket.
         */
        public $O makefile($O mode, $O buffering) {
            checkClosed();
            String m = mode instanceof $S ? (($S)mode).value : "r";
            
            if (type != SOCK_STREAM) {
                throw new $X.OSError("makefile only supported for TCP sockets");
            }
            
            // Return a simple file-like wrapper
            return new SocketFile(this, m);
        }
        
        // Context manager
        public socket __enter__() {
            return this;
        }
        
        public $O __exit__($O excType, $O excVal, $O excTb) {
            close();
            return $N.INSTANCE;
        }
        
        private void checkClosed() {
            if (closed) {
                throw new $X.OSError("Socket is closed");
            }
        }
        
        private byte[] getBytes($O data) {
            if (data instanceof $BY) {
                return (($BY)data).data;
            } else if (data instanceof $S) {
                return (($S)data).value.getBytes(java.nio.charset.StandardCharsets.UTF_8);
            }
            throw new $X.TypeError("a bytes-like object is required");
        }
        
        private boolean toBool($O value) {
            if (value instanceof $B) return (($B)value).__bool__();
            if (value instanceof $I) return (($I)value).value != 0;
            return value.__bool__();
        }
        
        @Override
        public $O __getattr__(String name) {
            final socket self = this;
            switch (name) {
                case "family": return $I.of(family);
                case "type": return $I.of(type);
                case "proto": return $I.of(proto);
                case "connect": return new $O() {
                    @Override public $O __call__($O... args) { return self.connect(args[0]); }
                };
                case "bind": return new $O() {
                    @Override public $O __call__($O... args) { return self.bind(args[0]); }
                };
                case "listen": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.listen(args.length > 0 ? args[0] : $I.of(5)); 
                    }
                };
                case "accept": return new $O() {
                    @Override public $O __call__($O... args) { return self.accept(); }
                };
                case "send": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 1 ? self.send(args[0], args[1]) : self.send(args[0]); 
                    }
                };
                case "sendall": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 1 ? self.sendall(args[0], args[1]) : self.sendall(args[0]); 
                    }
                };
                case "sendto": return new $O() {
                    @Override public $O __call__($O... args) { 
                        if (args.length == 2) return self.sendto(args[0], args[1]);
                        return self.sendto(args[0], args[1], args[2]); 
                    }
                };
                case "recv": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 1 ? self.recv(args[0], args[1]) : self.recv(args[0]); 
                    }
                };
                case "recvfrom": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return args.length > 1 ? self.recvfrom(args[0], args[1]) : self.recvfrom(args[0]); 
                    }
                };
                case "close": return new $O() {
                    @Override public $O __call__($O... args) { return self.close(); }
                };
                case "shutdown": return new $O() {
                    @Override public $O __call__($O... args) { return self.shutdown(args[0]); }
                };
                case "setsockopt": return new $O() {
                    @Override public $O __call__($O... args) { return self.setsockopt(args[0], args[1], args[2]); }
                };
                case "getsockopt": return new $O() {
                    @Override public $O __call__($O... args) { return self.getsockopt(args[0], args[1]); }
                };
                case "setblocking": return new $O() {
                    @Override public $O __call__($O... args) { return self.setblocking(args[0]); }
                };
                case "getblocking": return new $O() {
                    @Override public $O __call__($O... args) { return self.getblocking(); }
                };
                case "settimeout": return new $O() {
                    @Override public $O __call__($O... args) { return self.settimeout(args[0]); }
                };
                case "gettimeout": return new $O() {
                    @Override public $O __call__($O... args) { return self.gettimeout(); }
                };
                case "getpeername": return new $O() {
                    @Override public $O __call__($O... args) { return self.getpeername(); }
                };
                case "getsockname": return new $O() {
                    @Override public $O __call__($O... args) { return self.getsockname(); }
                };
                case "fileno": return new $O() {
                    @Override public $O __call__($O... args) { return self.fileno(); }
                };
                case "makefile": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.makefile(
                            args.length > 0 ? args[0] : $S.of("r"),
                            args.length > 1 ? args[1] : $N.INSTANCE
                        ); 
                    }
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
        
        @Override
        public $S __repr__() {
            return $S.of("<socket.socket fd=" + fileno().value + ", family=" + family + ", type=" + type + ", proto=" + proto + ">");
        }
    }
    
    /**
     * SocketFile - File-like wrapper for socket.
     */
    static class SocketFile extends $O {
        private final socket sock;
        private final String mode;
        private boolean closed = false;
        
        SocketFile(socket sock, String mode) {
            this.sock = sock;
            this.mode = mode;
        }
        
        public $BY read($O size) {
            if (!mode.contains("r")) {
                throw new $X.OSError("not readable");
            }
            return sock.recv(size);
        }
        
        public $I write($O data) {
            if (!mode.contains("w")) {
                throw new $X.OSError("not writable");
            }
            return sock.send(data);
        }
        
        public $N close() {
            closed = true;
            return $N.INSTANCE;
        }
        
        @Override
        public $O __getattr__(String name) {
            final SocketFile self = this;
            switch (name) {
                case "read": return new $O() {
                    @Override public $O __call__($O... args) { 
                        return self.read(args.length > 0 ? args[0] : $I.of(4096)); 
                    }
                };
                case "write": return new $O() {
                    @Override public $O __call__($O... args) { return self.write(args[0]); }
                };
                case "close": return new $O() {
                    @Override public $O __call__($O... args) { return self.close(); }
                };
                case "closed": return $B.of(closed);
                default: return super.__getattr__(name);
            }
        }
    }
    
    /**
     * gethostname - Get local hostname.
     */
    public static $S gethostname() {
        try {
            return $S.of(InetAddress.getLocalHost().getHostName());
        } catch (UnknownHostException e) {
            return $S.of("localhost");
        }
    }
    
    /**
     * gethostbyname - Get IP address for hostname.
     */
    public static $S gethostbyname($O hostname) {
        try {
            String host = (($S)hostname).value;
            return $S.of(InetAddress.getByName(host).getHostAddress());
        } catch (UnknownHostException e) {
            throw new $X("gaierror", "Name or service not known");
        }
    }
    
    /**
     * getaddrinfo - Get address info.
     */
    public static $L getaddrinfo($O host, $O port, $O family, $O type, $O proto, $O flags) {
        String h = host instanceof $S ? (($S)host).value : "";
        int p = port instanceof $I ? (int)(($I)port).value : 0;
        int fam = family instanceof $I ? (int)(($I)family).value : AF_UNSPEC;
        int typ = type instanceof $I ? (int)(($I)type).value : 0;
        
        $L result = new $L();
        try {
            InetAddress[] addrs = InetAddress.getAllByName(h.isEmpty() ? null : h);
            for (InetAddress addr : addrs) {
                int af = addr instanceof Inet6Address ? AF_INET6 : AF_INET;
                if (fam != AF_UNSPEC && fam != af) continue;
                
                int sockType = typ != 0 ? typ : SOCK_STREAM;
                $T addrTuple = $T.of($S.of(addr.getHostAddress()), $I.of(p));
                
                result.append($T.of(
                    $I.of(af),
                    $I.of(sockType),
                    $I.of(sockType == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP),
                    $S.of(addr.getCanonicalHostName()),
                    addrTuple
                ));
            }
        } catch (UnknownHostException e) {
            throw new $X("gaierror", "Name or service not known");
        }
        return result;
    }
    
    /**
     * inet_aton - Convert IPv4 address to bytes.
     */
    public static $BY inet_aton($O ip) {
        try {
            InetAddress addr = InetAddress.getByName((($S)ip).value);
            return $BY.of(addr.getAddress());
        } catch (UnknownHostException e) {
            throw new $X.OSError("illegal IP address string");
        }
    }
    
    /**
     * inet_ntoa - Convert bytes to IPv4 address string.
     */
    public static $S inet_ntoa($O packed) {
        try {
            byte[] bytes = (($BY)packed).data;
            InetAddress addr = InetAddress.getByAddress(bytes);
            return $S.of(addr.getHostAddress());
        } catch (UnknownHostException e) {
            throw new $X.OSError("packed IP wrong length");
        }
    }
    
    /**
     * create_connection - Convenience function to connect to host:port.
     */
    public static socket create_connection($O address, $O timeout) {
        $T addr = ($T)address;
        socket sock = new socket($I.of(AF_INET), $I.of(SOCK_STREAM), $I.of(0));
        if (!(timeout instanceof $N)) {
            sock.settimeout(timeout);
        }
        sock.connect(address);
        return sock;
    }
    
    /**
     * Get module attribute.
     */
    public static $O getAttr(String name) {
        switch (name) {
            case "AF_INET": return $I.of(AF_INET);
            case "AF_INET6": return $I.of(AF_INET6);
            case "AF_UNIX": return $I.of(AF_UNIX);
            case "AF_UNSPEC": return $I.of(AF_UNSPEC);
            case "SOCK_STREAM": return $I.of(SOCK_STREAM);
            case "SOCK_DGRAM": return $I.of(SOCK_DGRAM);
            case "SOCK_RAW": return $I.of(SOCK_RAW);
            case "IPPROTO_TCP": return $I.of(IPPROTO_TCP);
            case "IPPROTO_UDP": return $I.of(IPPROTO_UDP);
            case "IPPROTO_IP": return $I.of(IPPROTO_IP);
            case "SOL_SOCKET": return $I.of(SOL_SOCKET);
            case "SO_REUSEADDR": return $I.of(SO_REUSEADDR);
            case "SO_KEEPALIVE": return $I.of(SO_KEEPALIVE);
            case "SO_BROADCAST": return $I.of(SO_BROADCAST);
            case "SO_RCVBUF": return $I.of(SO_RCVBUF);
            case "SO_SNDBUF": return $I.of(SO_SNDBUF);
            case "TCP_NODELAY": return $I.of(TCP_NODELAY);
            case "SHUT_RD": return $I.of(SHUT_RD);
            case "SHUT_WR": return $I.of(SHUT_WR);
            case "SHUT_RDWR": return $I.of(SHUT_RDWR);
            case "INADDR_ANY": return INADDR_ANY;
            case "INADDR_BROADCAST": return INADDR_BROADCAST;
            case "INADDR_LOOPBACK": return INADDR_LOOPBACK;
            default: return null;
        }
    }
}
