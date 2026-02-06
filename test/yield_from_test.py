# Test yield from (PEP 380)

def sub():
    yield 1
    yield 2

def main():
    yield from sub()
    yield 3

print("yield from:")
for x in main():
    print(x)

# yield from with list
def from_list():
    yield from [10, 20, 30]

print("yield from list:")
for x in from_list():
    print(x)

print("Done!")
