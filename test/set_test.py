# Test set literals

# Basic set literal
s = {1, 2, 3}
print(s)

# Set with duplicates (should be deduplicated)
s2 = {1, 2, 2, 3, 3, 3}
print(s2)
print(len(s2))

# Set operations
a = {1, 2, 3, 4}
b = {3, 4, 5, 6}
print(a | b)
print(a & b)
print(a - b)

# Membership test
print(2 in a)
print(10 in a)

print("Done!")
