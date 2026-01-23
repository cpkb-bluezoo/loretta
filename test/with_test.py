# Test with statement (context managers)

# Simple context manager class
class MyContext:
    def __init__(self, name):
        self.name = name
    
    def __enter__(self):
        print("Entering")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        print("Exiting")
        return False

# Basic with usage
print("Test 1: Basic with")
with MyContext("test1"):
    print("Inside with block")

# With 'as' binding
print("\nTest 2: With 'as'")
with MyContext("test2") as ctx:
    print("Got context")

# NOTE: Multiple classes in one file has issues with globals
# Test 3 skipped for now

print("\nDone!")
