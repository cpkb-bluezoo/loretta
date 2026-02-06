# Test raw strings (r'...'): backslashes are not escape sequences

# In raw string, \n is two chars (backslash, n), not newline
r_hello = r"hello\nworld"
normal = "hello\nworld"
print("raw len:", len(r_hello))
print("normal len:", len(normal))
assert len(r_hello) == 12
assert len(normal) == 11

# repr shows the backslash
print("raw repr:", repr(r_hello))

# Backslash only escapes the closing quote in raw strings (so \" = backslash + quote)
r_quote = r"say \"hi\""
print("r_quote len:", len(r_quote))
print("r_quote:", r_quote)

# Common use: Windows path
path = r"C:\Users\name\file.txt"
print("path:", path)
assert "\\" in path

print("raw_string_test OK")
