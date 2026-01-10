# Test list comprehension
a = [x for x in items]
b = [x * 2 for x in range(10)]
c = [x for x in items if x > 0]
d = [x for x in items if x > 0 if x < 100]

# Test nested comprehension
e = [x + y for x in range(3) for y in range(3)]

# Test set comprehension
f = {x for x in items}
g = {x * 2 for x in range(10) if x > 5}

# Test dict comprehension
h = {k: v for k, v in pairs}
i = {x: x * x for x in range(10)}
j = {k: v for k, v in items if v > 0}

# Test generator expression
k = (x for x in items)
l = (x * 2 for x in range(10) if x > 5)

# Test tuple and set literals
m = (1, 2, 3)
n = {1, 2, 3}
o = {1: "a", 2: "b"}

