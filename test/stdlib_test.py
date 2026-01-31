# Test stdlib C-extension replacements
# Tests the _posix, _collections, and _sre modules

print("=== Stdlib C-Extension Tests ===")

# Test 1: posix module
print("\n--- Test 1: posix module ---")
import posix

print("posix.sep:", posix.sep)
print("posix.pathsep:", posix.pathsep)
print("posix.curdir:", posix.curdir)
print("posix.pardir:", posix.pardir)

cwd = posix.getcwd()
print("posix.getcwd():", cwd)

files = posix.listdir(".")
print("posix.listdir('.') count:", len(files))

print("posix.F_OK:", posix.F_OK)
print("posix.R_OK:", posix.R_OK)

print("posix tests: PASS")

# Test 2: _collections module
print("\n--- Test 2: _collections module ---")
import _collections

# Test deque
d = _collections.deque()
d.append(1)
d.append(2)
d.append(3)
print("deque after append:", d)

d.appendleft(0)
print("deque after appendleft:", d)

popped = d.pop()
print("deque.pop():", popped)

popleft = d.popleft()
print("deque.popleft():", popleft)

d2 = _collections.deque([1, 2, 3, 4, 5])
print("deque([1,2,3,4,5]):", d2)

print("d2[0]:", d2[0])
print("d2[-1]:", d2[-1])

d2.rotate(2)
print("after rotate(2):", d2)

# Test defaultdict
dd = _collections.defaultdict(list)
dd["a"].append(1)
dd["a"].append(2)
print("dd['a']:", dd["a"])

# Test Counter
c = _collections.Counter("abracadabra")
print("Counter('abracadabra'):", c)
print("c['a']:", c["a"])

print("_collections tests: PASS")

# Test 3: re module (via _sre)
print("\n--- Test 3: re module ---")
import re

# Test search
m = re.search(r"\d+", "abc123def")
print("re.search found:", m.group())

# Test findall
results = re.findall(r"\d+", "a1b2c3d4")
print("re.findall:", results)

# Test sub
result = re.sub(r"\d+", "X", "a1b2c3")
print("re.sub:", result)

# Test split
parts = re.split(r"\s+", "hello   world  test")
print("re.split:", parts)

# Test compile
pattern = re.compile(r"\w+")
matches = pattern.findall("hello world")
print("compiled findall:", matches)

print("re tests: PASS")

print("\n=== All stdlib tests passed! ===")
