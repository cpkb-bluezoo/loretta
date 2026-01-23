# Test comparison chains

# Simple chains
print(1 < 2 < 3)
print(1 < 3 < 2)
print(3 < 2 < 1)

# All equal comparisons  
print(1 <= 2 <= 3)
print(1 == 1 == 1)
print(1 < 2 == 2 < 3)

# With variables
a = 5
b = 10
c = 15
print(a < b < c)
print(c > b > a)
print(a < c < b)

# Longer chains
print(1 < 2 < 3 < 4 < 5)
print(1 < 2 < 3 < 2 < 5)

# Mixed operators
print(1 < 2 <= 2 < 3)
print(1 <= 1 < 2 <= 2)

print("Done!")
