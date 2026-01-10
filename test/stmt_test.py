# Test try/except/finally
try:
    risky()
except:
    handle()

try:
    risky()
except ValueError:
    handle_value()
except TypeError as e:
    handle_type(e)

try:
    risky()
except ValueError:
    handle()
else:
    success()
finally:
    cleanup()

try:
    risky()
finally:
    cleanup()

# Test with statement
with open("file") as f:
    data = f.read()

with a as x, b as y:
    process(x, y)

with context:
    do_something()

# Test raise
raise
raise ValueError
raise ValueError("message")
raise ValueError from cause

# Test assert
assert True
assert x > 0
assert x > 0, "x must be positive"

# Test del
del x
del x, y, z
del obj.attr
del lst[0]

# Test global/nonlocal
def func():
    global x
    global a, b, c
    nonlocal y
    nonlocal p, q

