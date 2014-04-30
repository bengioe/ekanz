

j = 2
k = 82
while j < 1000:
    i = 0
    x = 1
    y = 2
    while i < 20000:
        x = x + 5 + i * y / k
        y = 0 * (2 * 16 - i + 19) / 4 + 2
        i = i + 1
    j = j + 1
print(x - 100)
