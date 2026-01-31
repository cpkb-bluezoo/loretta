# Test imports

print("Test 1: Basic import")
import mymodule
print("Module:", mymodule)
print("VERSION:", mymodule.VERSION)

print("\nTest 2: from import")
from mymodule import greet, add
greet("Python")
print("5 + 6 =", add(5, 6))

print("\nTest 3: import as")
import mymodule as mm
print("Aliased module:", mm)
mm.greet("Alias")

print("\nTest 4: from import as")
from mymodule import add as addition
print("7 + 8 =", addition(7, 8))

print("\nTest 5: import sys module")
import sys
print("sys.version:", sys.version)
print("sys.platform:", sys.platform)
print("sys.path length:", len(sys.path))

print("\nTest 6: from module import *")
from mymodule import *
# Should have VERSION, greet, add in globals now
print("VERSION after star import:", VERSION)
greet("Star Import")
print("3 + 4 =", add(3, 4))

print("\nDone!")
