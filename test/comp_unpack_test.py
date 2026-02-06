# Test comprehension with tuple unpacking in for target.
# NOTE: Currently skipped in run_tests.sh due to stackmap bug in this code path.

# List comprehension: [a+b for a, b in pairs]
pairs = [(1, 2), (3, 4), (5, 6)]
sums = [a + b for a, b in pairs]
print("sums:", sums)
assert sums == [3, 7, 11]

# With filter
big = [a * b for a, b in pairs if a + b > 5]
print("big (a*b where a+b>5):", big)

# Dict comprehension with tuple unpacking (key, value from pairs)
d = {str(a): b for a, b in pairs}
print("dict:", d)

print("comp_unpack_test OK")
