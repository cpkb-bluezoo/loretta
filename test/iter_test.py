# Test iteration built-ins

def test_enumerate():
    # enumerate - access as tuples
    names = ["alice", "bob", "carol"]
    e = enumerate(names)
    for pair in e:
        print(pair)

def test_zip():
    # zip
    nums = [1, 2, 3]
    letters = ["a", "b", "c"]
    z = zip(nums, letters)
    for pair in z:
        print(pair)

def test_reversed():
    # reversed
    r = reversed([1, 2, 3])
    for x in r:
        print(x)

def test_map():
    # map with str
    m = map(str, [1, 2, 3])
    mapped = list(m)
    print(mapped)

test_enumerate()
print("---")
test_zip()
print("---")
test_reversed()
print("---")
test_map()
