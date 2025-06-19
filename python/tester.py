import shtRipper
import json
import faulthandler
faulthandler.enable()

ripper = shtRipper.ripper

import time

#filename = '/home/nz/Downloads/sht41384.SHT'
filename = '\\\\172.16.12.127\\Data\\sht41384.SHT'
filename = '\\\\172.16.12.127\\Pub\\!!!TS_RESULTS\\shots\\46127\\TS_46127.SHT'
filename = 'test.sht'

times = [0, 1, 2, 4]
y = [3, 2, 1, 0]
e = [1, 1, 2, 3]
to_pack = {
    'XYE': {
        'comment': 'концентрация, усреднённая по двум центральным точкам измерения с учётом веса',
        'unit': 'ne(m^-3)',
        'x': times,
        'y': y,
        'err': e
    },
    'XY': {
        'comment': 'площадь полоидального сечения',
        'unit': 'S(m^2)',
        'x': times,
        'y': y
    }
}

packed = shtRipper.ripper.write(path='', filename=filename, data=to_pack)

start_time = time.time()
for iteration in range(1):
    #pass
    #res = shtRipper.ripper.read(filename)
    res = ripper.read(filename)
    print(res.keys())
print("--- %.2f seconds ---" % (time.time() - start_time))

with open('dump.json', 'w') as file:
    json.dump(res, file, indent=2)

for i in range(50000000):  # wait for possible errors in dll
    d = 56784678 / 5423621543

print('OK.')
