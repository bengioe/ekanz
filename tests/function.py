

def f(z,k,p):
    return 4 + z


i = 0
x = f(i)
while i < 100000:
    i = i+1
    x = f(i)
#    x = f(i)
#    i = i + 1

print(x)
print("Test")
