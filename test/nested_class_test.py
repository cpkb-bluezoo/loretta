# Test nested class (class defined inside another class body).
# NOTE: Currently skipped in run_tests.sh (local variable table overflow in some builds).

class Outer:
    value = 42

    class Inner:
        def get_val(self):
            return 100

    def use_inner(self):
        inner = Outer.Inner()
        return inner.get_val()


outer = Outer()
print("Outer.value:", outer.value)
print("outer.use_inner():", outer.use_inner())
print("nested_class_test OK")
