# Test function arguments
def simple(a, b, c):
    pass

def with_defaults(a, b=1, c=2):
    pass

def with_star_args(a, *args):
    pass

def with_kwargs(a, **kwargs):
    pass

def full_args(a, b=1, *args, c, d=2, **kwargs):
    pass

def with_annotations(a: int, b: str = "hello") -> bool:
    pass

# Test decorators
@decorator
def decorated():
    pass

@decorator_with_args(1, 2)
def decorated2():
    pass

@module.decorator
@another_decorator
def multi_decorated():
    pass

@decorator
class DecoratedClass:
    pass

# Test starred expressions in calls
result = func(1, 2, *args)
result = func(a=1, b=2, **kwargs)
result = func(1, *args, key=value, **kwargs)

# Test yield expressions
def generator():
    yield
    yield 42
    yield from other_gen()

# Test await expressions
async def async_func():
    result = await something()
    return result

# Test walrus operator
if (n := len(items)) > 10:
    pass

while (line := input()):
    pass

# Test class with base classes
class Child(Parent):
    pass

class Multi(Base1, Base2):
    pass

