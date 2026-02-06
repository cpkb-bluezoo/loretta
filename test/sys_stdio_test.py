# Test sys.stdin, sys.stdout, sys.stderr (wrap Java System.in/out/err)

import sys

# Check they are file-like objects (have write/read, repr)
print("sys.stdin:", repr(sys.stdin))
print("sys.stdout:", repr(sys.stdout))
print("sys.stderr:", repr(sys.stderr))

# stdout.write and flush
sys.stdout.write("stdout write OK\n")
sys.stdout.flush()

# stderr.write and flush
sys.stderr.write("stderr write OK\n")
sys.stderr.flush()

# stdin should have read/readline (we don't interactively read in this test)
# Just verify we can get the attribute
_ = getattr(sys.stdin, "readline")
print("sys.stdin.readline: OK")

print("sys_stdio_test OK")
