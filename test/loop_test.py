# Test while loop StackMapTable
x = 0
while x < 5:
    print(x)
    x = x + 1
print("done")

# Test for-loop tuple unpacking
print("for (a,b) in pairs:")
pairs = [(1, 2), (3, 4), (5, 6)]
for a, b in pairs:
    print(a, b, "-> sum =", a + b)
print("tuple unpack loop done")
