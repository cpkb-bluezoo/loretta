# Test assert statement

# Assert that passes
x = 5
assert x > 0
print("assert x > 0 passed")

assert x == 5
print("assert x == 5 passed")

# Assert with truthy values
assert [1, 2, 3]
print("assert [1, 2, 3] passed")

assert "hello"
print("assert 'hello' passed")

# Assert with message (should pass)
assert x > 0, "x should be positive"
print("assert with message passed")

print("All assert tests passed!")
