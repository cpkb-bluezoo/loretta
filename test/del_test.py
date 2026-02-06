# Test del statement

# del from dict
d = {"a": 1, "b": 2, "c": 3}
print("Before:", d)
del d["b"]
print("After del d['b']:", d)

# del from list
lst = [1, 2, 3, 4, 5]
print("Before:", lst)
del lst[2]
print("After del lst[2]:", lst)

# del local variable
x = 42
print("x =", x)
del x
# Note: accessing x after del would raise NameError

# del global variable
global_var = 100
print("global_var =", global_var)
del global_var
print("del global OK (global_var removed from globals)")

# del instance attribute
class Obj:
    def __init__(self):
        self.a = 1
        self.b = 2

o = Obj()
print("o.a =", o.a)
print("o.b =", o.b)
del o.a
print("After del o.a, o.b =", o.b)

print("Done!")
