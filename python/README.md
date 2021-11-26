# shtRipper

Код для извлечения данных из .sht файлов. На текущий момент файлы версии ниже 2 не поддерживаются, что будет исправлено
 при необходимости.


    import shtRipper
    
    filename = 'd:/data/cfm/original/sht40808.SHT'
    res = shtRipper.ripper.read(filename)
    print(res.keys())

---   


Для упаковки данных в формат .sht:

    import shtRipper

    to_pack = {
        'central Te': {
            'comment': 'Температура в центре',
            'unit': 'T_e(eV)',
            'x': [0.0, 1.2, 1.9],
            'y': [0, 1, 3.16]
            'err': [0.1, 0.1, 0.5]
        },
        'signal 2': {
            'comment': 'Температура в центре',
            'unit': 'n_e(m^-3)',
            'x': [0, 1],
            'y': [0, 16]
        }
    }
    
    packed = shtRipper.ripper.write(path='D:/tmp/', filename='example.SHT', data=to_pack)
    if len(packed) != 0:
        print('packed error = "%s"' % packed)

---   