# Test generators (yield)

def gen():
    yield 1
    yield 2
    yield 3

# Basic generator iteration
print("Basic generator:")
for x in gen():
    print(x)

# Generator with loop
def squares(n):
    i = 0
    while i < n:
        yield i * i
        i = i + 1

print("Squares of 0-4:")
for s in squares(5):
    print(s)

