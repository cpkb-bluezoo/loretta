# Test decorators

# Simple decorator that returns the function unchanged
def identity(func):
    return func

@identity
def f():
    return 42

# Test that the decorator was applied
print("Decorated function result:", f())

# Test staticmethod decorator
class Counter:
    @staticmethod
    def get_value():
        return 100

print("Staticmethod result:", Counter.get_value())

# Test classmethod decorator  
class Greeter:
    msg = "Hello"
    
    @classmethod
    def greet(cls):
        return cls.msg

print("Classmethod result:", Greeter.greet())

