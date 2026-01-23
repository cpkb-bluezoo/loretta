# Test all function features

def simple():
    return 42

def with_args(a, b):
    return a + b

def with_defaults(x, y=10, z=20):
    return x + y + z

def with_varargs(first, *rest):
    print(first)
    return rest

def with_kwargs(name, **options):
    print(name)
    return options

def mixed_all(a, b=5, *args, key=None, **kwargs):
    return a

# Test calls
r1 = simple()
r2 = with_args(1, 2)
r3 = with_defaults(100)
r4 = with_defaults(100, 200)
r5 = with_defaults(100, 200, 300)
r6 = with_varargs(1, 2, 3, 4)
r7 = mixed_all(10)

print(r1)
print(r2)
print(r3)
