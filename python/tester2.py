filename = 'test.sht'


file1 = open('merged.sht', 'rb')
merged = file1.read()
file2 = open(filename, 'rb')
original = file2.read()

print('OK')
