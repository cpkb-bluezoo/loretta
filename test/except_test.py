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

# Exception types are available as builtins
print("\nTest 4: Exception classes available")
print("TypeError:", TypeError)
print("ValueError:", ValueError)
print("KeyError:", KeyError)
print("IndexError:", IndexError)

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

# Raise exception class instances
print("\nTest 6: Raise TypeError('message')")
try:
    raise TypeError("this is a type error")
except:
    print("Caught TypeError")

print("\nTest 7: Raise ValueError('message')")
try:
    raise ValueError("invalid value")
except:
    print("Caught ValueError")

# Raise bare exception class (no message)
print("\nTest 8: Raise bare exception class")
try:
    raise RuntimeError
except:
    print("Caught RuntimeError")

# Test typed except clause
print("\nTest 9: Typed except (catch specific type)")
try:
    raise TypeError("test type error")
except TypeError:
    print("Caught TypeError specifically")

# Test typed except with binding
print("\nTest 10: Typed except with binding")
try:
    raise ValueError("the value was bad")
except ValueError as e:
    print("Caught ValueError:", e)

# Test that wrong type doesn't catch
print("\nTest 11: Type mismatch should re-raise")
try:
    try:
        raise TypeError("inner type error")
    except ValueError:
        print("ERROR: Should not catch TypeError as ValueError")
except TypeError:
    print("Correctly re-raised TypeError")

# Test exception hierarchy - ZeroDivisionError is an ArithmeticError
print("\nTest 12: Exception hierarchy (ZeroDivisionError as ArithmeticError)")
try:
    x = 1 / 0
except ArithmeticError:
    print("Caught ZeroDivisionError as ArithmeticError")

# Test raise from (exception chaining)
print("\nTest 13: Raise from (exception chaining)")
try:
    try:
        x = 1 / 0
    except ZeroDivisionError as e:
        raise ValueError("failed operation") from e
except ValueError:
    print("Caught chained ValueError")

# Test try/finally - finally runs on normal path
print("\nTest 14: try/finally (normal path)")
finally_ran = False
try:
    z = 1 + 1
finally:
    finally_ran = True
    print("Finally ran (normal):", finally_ran)

print("\nDone!")
