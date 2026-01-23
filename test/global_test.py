# Test global/nonlocal

# Global variable
counter = 0

def increment():
    global counter
    counter = counter + 1

def get_count():
    global counter
    return counter

print("Initial:", get_count())
increment()
print("After 1st increment:", get_count())
increment()
increment()
print("After 3 increments:", get_count())

# Another global example
message = "Hello"

def change_message():
    global message
    message = "World"

print("Before:", message)
change_message()
print("After:", message)

print("Done!")
