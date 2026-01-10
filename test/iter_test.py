# Test iteration built-ins

# enumerate - access as tuples
names = ["alice", "bob", "carol"]
e = enumerate(names)
for pair in e:
    print(pair)

print("---")

# zip
nums = [1, 2, 3]
letters = ["a", "b", "c"]
z = zip(nums, letters)
for pair in z:
    print(pair)

print("---")

# reversed
r = reversed([1, 2, 3])
for x in r:
    print(x)

print("---")

# map with str
m = map(str, [1, 2, 3])
mapped = list(m)
print(mapped)
