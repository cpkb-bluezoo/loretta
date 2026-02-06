# Minimal test for _io.readinto
import io

# BytesIO readinto (write then seek so buffer has data; b"x" is emitted as str by compiler)
buf = io.BytesIO()
buf.write(b"hello")
buf.seek(0)
b = bytearray(10)
n = buf.readinto(b)
print("n:", n)
print("bytes:", bytes(b[:n]))

# File readinto
f = open("/tmp/loretta_readinto_test.txt", "wb")
f.write(b"file readinto")
f.close()
f = open("/tmp/loretta_readinto_test.txt", "rb")
b = bytearray(64)
n = f.readinto(b)
f.close()
print("file n:", n)
print("file bytes:", bytes(b[:n]))
print("readinto_test OK")
