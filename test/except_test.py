# Test try/except/raise

# Basic try/except
print("Test 1: Basic try/except")
try:
    x = 10 / 0
except:
    print("Caught division error")

# Try with no exception
print("\nTest 2: Try with no exception")
try:
    y = 5 + 3
    print("y =", y)
except:
    print("This should not print")

# Raise and catch
print("\nTest 3: Raise and catch")
try:
    raise "Something went wrong"
except:
    print("Caught raised exception")

# Skip exception binding for now - needs type checking
print("\nTest 4: Skipped (needs exception type handling)")

# Nested try
print("\nTest 5: Nested try")
try:
    try:
        raise "Inner error"
    except:
        print("Caught inner")
        raise "Outer error"
except:
    print("Caught outer")

print("\nDone!")
