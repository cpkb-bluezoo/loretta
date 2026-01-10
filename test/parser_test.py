# Test conditional expressions
a = 1 if x else 2
b = 1 if x else 2 if y else 3

# Test lambda expressions
f = lambda: 42
g = lambda x: x + 1
h = lambda x, y: x * y

# Test slices
a = lst[1]
b = lst[1:2]
c = lst[1:2:3]
d = lst[:]
e = lst[::2]
f = lst[1:]
g = lst[:2]

# Test augmented assignment
x += 1
y -= 2
z *= 3
w /= 4
v //= 5
u %= 6
t **= 2
s &= 0xff
r |= 0x100
q ^= 0xaa
p <<= 1
o >>= 2

# Test chained assignment
a = b = c = 10

