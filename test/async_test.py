# Test async/await (eager evaluation - no true concurrency)

async def get_value():
    return 42

async def compute(x):
    return x * 2

async def main():
    result = await get_value()
    print("Got:", result)
    a = await compute(5)
    b = await compute(10)
    print("Computed:", a, b)
    return a + b

# Call async function
result = main()
print("Total:", result)

