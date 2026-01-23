# A simple test module
# This will be compiled to mymodule.class

# Module-level variable
VERSION = "1.0"

# Module-level function
def greet(name):
    print("Hello,", name)

def add(a, b):
    return a + b

# When the module is loaded, this runs
print("mymodule loaded!")
