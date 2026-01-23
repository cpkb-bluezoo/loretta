# Test inheritance specifically

class Base:
    def greet(self):
        return "Hello from Base"

class Child(Base):
    def greet(self):
        return "Hello from Child"

b = Base()
c = Child()
print("Base:", b.greet())
print("Child:", c.greet())

print("Done!")
