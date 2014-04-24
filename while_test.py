

j = 0

while j < 10000:
    i = 0
    x = 1
    y = 2
    while i < 1000:
        x = x + y * i
        i = i + 1
    j = j + 1

print(x)
