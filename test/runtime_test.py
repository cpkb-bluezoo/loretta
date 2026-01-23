# Test basic features with runtime

# Arithmetic
print(10 + 5)
print(10 - 5)
print(10 * 5)
print(10 / 4)
print(10 // 3)
print(10 % 3)

# Comparisons
print(5 > 3)
print(5 < 3)
print(5 == 5)

# Control flow
x = 10
if x > 5:
    print("x is big")
else:
    print("x is small")

# Loops
i = 0
while i < 3:
    print(i)
    i = i + 1

# For loop with range
for j in range(3):
    print(j)

# Strings
s = "Hello"
print(s + " World")
print(len(s))

# Lists
nums = [1, 2, 3]
print(nums)
print(len(nums))
print(nums[0])

print("All tests passed!")
