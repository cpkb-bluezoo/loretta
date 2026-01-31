# Test I/O module functionality
# Tests _io (file I/O) and socket modules

print("=== I/O Module Tests ===")

# Test 1: Basic file writing and reading
print("\n--- Test 1: File write/read ---")
f = open("/tmp/loretta_test.txt", "w")
f.write("Hello, World!\n")
f.write("Second line\n")
f.close()
print("File written")

f = open("/tmp/loretta_test.txt", "r")
content = f.read()
f.close()
print("File content:", content)

# Test 2: File readline
print("\n--- Test 2: File readline ---")
f = open("/tmp/loretta_test.txt", "r")
line1 = f.readline()
line2 = f.readline()
f.close()
print("Line 1:", line1)
print("Line 2:", line2)

# Test 3: File iteration
print("\n--- Test 3: File iteration ---")
f = open("/tmp/loretta_test.txt", "r")
count = 0
for line in f:
    count = count + 1
    print("Iter line:", line)
f.close()
print("Lines counted:", count)

# Test 4: Context manager (with statement)
print("\n--- Test 4: Context manager ---")
with open("/tmp/loretta_test.txt", "r") as f:
    data = f.read()
    print("With statement read:", data)

# Test 5: io module BytesIO
print("\n--- Test 5: BytesIO ---")
import io
buf = io.BytesIO()
buf.write(b"Hello bytes!")
buf.seek(0)
data = buf.read()
print("BytesIO data:", data)
buf.close()

# Test 6: io module StringIO
print("\n--- Test 6: StringIO ---")
sio = io.StringIO()
sio.write("Hello StringIO!\n")
sio.write("More text")
val = sio.getvalue()
print("StringIO value:", val)
sio.close()

# Test 7: Socket module constants
print("\n--- Test 7: Socket module ---")
import socket
print("socket.AF_INET:", socket.AF_INET)
print("socket.SOCK_STREAM:", socket.SOCK_STREAM)
print("socket.SOCK_DGRAM:", socket.SOCK_DGRAM)

# Test 8: gethostname
print("\n--- Test 8: gethostname ---")
hostname = socket.gethostname()
print("Hostname:", hostname)

# Test 9: gethostbyname
print("\n--- Test 9: gethostbyname ---")
ip = socket.gethostbyname("localhost")
print("localhost IP:", ip)

# Test 10: Create socket (don't connect)
print("\n--- Test 10: Socket creation ---")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("Created socket:", s)
s.close()
print("Socket closed")

# Cleanup - just try to remove, ignore errors
import posix
posix.remove("/tmp/loretta_test.txt")
print("\nCleanup complete")

print("\n=== All I/O tests passed! ===")
