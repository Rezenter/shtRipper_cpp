import shtRipper_entry
ripper = shtRipper_entry.Ripper(True)

import json

shots = [
    39338,
    42190,
    43198,
    44824
]

for shotn in shots:
    print('shotn ', shotn)
    filename = '\\\\172.16.12.127\\Data\\sht%05d.SHT' % shotn
    #filename = 'sht%05d.SHT' % shotn

    res = ripper.read(filename)
    #res = ripper.read(filename, ['Лазер', 'Emission electrode voltage', 'Emission electrode current'])
    if 'err' in res:
        print(res['err'])

    print('read OK\n')

d = 1
for i in range(50000000):  # wait for possible errors in dll
    d *= 56784678 * i / 5423621543

print('OK. ', d)
