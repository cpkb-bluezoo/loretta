# Test generator expressions

# Simple generator expression
nums = [1, 2, 3, 4, 5]
gen = (x * 2 for x in nums)

# Iterate through generator
print("Doubled:")
for val in gen:
    print(val)

# Generator with filter
gen2 = (x for x in nums if x > 2)
print("Greater than 2:")
for val in gen2:
    print(val)

# Generator with expression and filter
gen3 = (x * x for x in nums if x % 2 == 0)
print("Squares of evens:")
for val in gen3:
    print(val)

# Using generator with sum (via list for now)
total = 0
for x in (n * n for n in nums):
    total = total + x
print("Sum of squares:", total)

# Nested generator (falls back to eager evaluation)
matrix = [[1, 2], [3, 4], [5, 6]]
flat = (x for row in matrix for x in row)
print("Flattened:")
for val in flat:
    print(val)

print("Done!")
