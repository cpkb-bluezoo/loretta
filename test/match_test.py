# Test match/case statements (Python 3.10+)

# Simple literal patterns
match x:
    case 1:
        print("one")
    case 2:
        print("two")
    case _:
        print("other")

# Capture patterns
match point:
    case (0, 0):
        print("origin")
    case (x, 0):
        print("on x-axis")
    case (0, y):
        print("on y-axis")
    case (x, y):
        print("general point")

# Or patterns
match status:
    case 200 | 201 | 204:
        print("success")
    case 400 | 404:
        print("client error")
    case 500:
        print("server error")

# Guard patterns
match value:
    case x if x > 0:
        print("positive")
    case x if x < 0:
        print("negative")
    case 0:
        print("zero")

# Sequence patterns with star
match items:
    case []:
        print("empty")
    case [x]:
        print("single")
    case [first, *rest]:
        print("multiple")

# Class patterns
match obj:
    case Point(x, y):
        print("point")
    case Circle(radius):
        print("circle")

# As patterns
match expr:
    case (x, y) as point:
        print("got point")

