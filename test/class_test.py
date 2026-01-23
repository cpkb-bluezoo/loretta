# Test class definitions

# Simple class
class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y
    
    def move(self, dx, dy):
        self.x = self.x + dx
        self.y = self.y + dy
    
    def __str__(self):
        return "Point(" + str(self.x) + ", " + str(self.y) + ")"

# Create instance
p = Point(3, 4)
print(p)

# Access attributes
print("x:", p.x)
print("y:", p.y)

# Call method
p.move(1, 2)
print("After move:", p)

# Class with instance variable
class Counter:
    def __init__(self):
        self.count = 0
    
    def increment(self):
        self.count = self.count + 1
    
    def get_count(self):
        return self.count

c = Counter()
c.increment()
c.increment()
c.increment()
print("Count:", c.get_count())

# Method that returns value
class Calculator:
    def add(self, a, b):
        return a + b
    
    def mul(self, a, b):
        return a * b

calc = Calculator()
print("2 + 3 =", calc.add(2, 3))
print("4 * 5 =", calc.mul(4, 5))

# Inheritance
class Animal:
    def __init__(self, name):
        self.name = name
    
    def speak(self):
        return "..."

class Dog(Animal):
    def speak(self):
        return self.name + " says woof!"

class Cat(Animal):
    def speak(self):
        return self.name + " says meow!"

dog = Dog("Buddy")
cat = Cat("Whiskers")
print(dog.speak())
print(cat.speak())

print("Done!")
