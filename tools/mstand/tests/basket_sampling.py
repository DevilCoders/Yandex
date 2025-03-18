import random

for __ in range(100):
    flags = [1 if random.random() < 0.5 else 0 for _ in range(10000)]

    x, y = sum(flags[:5000]), sum(flags[5000:])
    print x, y, abs(x - y)
