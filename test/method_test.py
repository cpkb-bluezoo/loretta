# Test method calls

# String methods
s = "hello world"
print(s.upper())
print(s.lower())
print(s.strip())
print(s.split())

# With arguments
print(s.replace("world", "python"))
print(s.startswith("hello"))
print(s.find("world"))

# List methods
nums = [3, 1, 4, 1, 5]
print(nums)
nums.append(9)
print(nums)
print(nums.pop())
print(nums)
nums.reverse()
print(nums)

# Storing method reference
upper_method = "test".upper
print(upper_method())

# Dict methods
d = {"a": 1, "b": 2}
print(d.keys())
print(d.values())
print(d.get("a"))
print(d.get("z", 99))

print("Done!")
