# Test lambda expressions

# Simple lambda
add_one = lambda x: x + 1
print(add_one(5))

# Lambda with multiple args
add = lambda x, y: x + y
print(add(3, 4))

# Lambda with no args
get_42 = lambda: 42
print(get_42())

# Lambda in a list
funcs = [lambda x: x * 2, lambda x: x * 3, lambda x: x + 10]
print(funcs[0](5))
print(funcs[1](5))
print(funcs[2](5))

# Lambda with closure
def make_adder(n):
    return lambda x: x + n

add5 = make_adder(5)
add10 = make_adder(10)
print(add5(3))
print(add10(3))

# Lambda as argument (map-like)
def test_lambda_loop():
    nums = [1, 2, 3, 4, 5]
    result = []
    for n in nums:
        result.append((lambda x: x * x)(n))
    print(result)

test_lambda_loop()

print("Done!")
