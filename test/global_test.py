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

# Augmented assignment on global
total = 0

def add_to_total(n):
    global total
    total += n

add_to_total(10)
add_to_total(5)
print("total after += (10 and 5):", total)

print("Done!")
