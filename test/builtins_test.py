# Test built-in functions

# Math
print(abs(-5))          # 5
print(abs(3.14))        # 3.14
print(min(3, 1, 4))     # 1
print(max(3, 1, 4))     # 4
print(pow(2, 10))       # 1024
print(round(3.7))       # 4
print(round(3.14159, 2))  # 3.14

# Sequences
nums = [5, 2, 8, 1, 9]
print(sum(nums))        # 25
print(sorted(nums))     # [1, 2, 5, 8, 9]
print(len(nums))        # 5

# Logic
print(all([True, True, True]))   # True
print(all([True, False, True]))  # False
print(any([False, False, True])) # True
print(any([False, False]))       # False

# String/char
print(ord("A"))         # 65
print(chr(65))          # A
print(hex(255))         # 0xff
print(oct(64))          # 0o100
print(bin(10))          # 0b1010

# Type constructors
print(tuple([1, 2, 3])) # (1, 2, 3)
print(list((4, 5, 6)))  # [4, 5, 6]

# Object
print(id(nums) > 0)     # True

