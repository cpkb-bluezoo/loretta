# Test closures

# Simple closure - inner function captures x
def make_adder(x):
    def adder(y):
        return x + y
    return adder

add5 = make_adder(5)
add10 = make_adder(10)

print(add5(3))
print(add5(7))
print(add10(3))
print(add10(7))

# Multiple captured variables
def make_multiplier(a, b):
    def mult(x):
        return a * x + b
    return mult

f = make_multiplier(2, 3)
print(f(5))
print(f(10))

# Counter using closure
def make_counter(start):
    count = start
    def get_count():
        return count
    return get_count

counter = make_counter(100)
print(counter())

print("Done!")
