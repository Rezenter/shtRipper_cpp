# shtRipper

На винде в зависимостях MSVS140

Код для извлечения данных из .sht файлов.

Перед обновлением пакета остановить все запущенные питоновские интерпретаторы!
Надёжнее удалять предыдущую версию, чтобы гарантированно обновились бинарные файлы.


Пример кода для чтения всего sht файла:

    import shtRipper
    
    filename = '\\\\172.16.12.127\\Data\\sht40808.SHT'
    res = shtRipper.ripper.read(filename)
    print(res.keys())

---   

Пример кода для чтения только выбранных сигналов:

    import shtRipper
    
    filename = '\\\\172.16.12.127\\Data\\sht40808.SHT'
    res = shtRipper.ripper.read(filename, ['Лазер', 'Emission electrode voltage', 'Emission electrode current'])
    print(res.keys())

---


Пример кода для объединения нескольких sht файлов в один без распаковки/запаковки:

    import shtRipper
    
    path: str = ''
    filename: str = 'out.sht'
    filenames: list[str] = ['in1.sht', 'in2.sht']
    ripper.merge('', filename, filenames)
---


Пример кода для упаковки данных в формат .sht:

    import shtRipper
    import math  # used only for example sin() signal

    to_pack = {
       'signal with error': {
           'comment': 'Точки хранят ещё и погрешность, требует много памяти.',
           'unit': 'T_e(eV)',
           'x': x,
           'y': T_c,
           'err': T_err
       },
       'X Y signal': {
           'comment': 'Точки хранятся парой значений, требует много памяти.',
           'unit': 'T_e(eV)',
           'x': x,
           'y': n_c
       },
       'Y signal with fixed X-grid': {
           'comment': 'Этот тип хранения данных предназначен для данных АЦП',
           'unit': 'U(V)',
           'tMin': 0.0,  # mininun time
           'tMax': 100.0,  # maximum time
           'offset': 0.0,  # ADC zero level offset
           'yRes': 0.0001,  # ADC resolution: 0.0001 Volt per adc bit
           'y': [math.sin(i / 10) for i in range(100)]
       }
    }
    
    packed = shtRipper.ripper.write(path='D:/tmp/', filename='example.SHT', data=to_pack)
    if len(packed) != 0:
        print('packed error = "%s"' % packed)

---   

Пример кода для упаковки данных АЦП spectraltech в формат sht:

    import shtRipper

    shotn = 46128
    Tukhmeneva_path = '\\\\172.16.12.127\\Pub\\!!!SHT Tuxmeneva\\'
    sht_path = '\\\\172.16.12.127\\Data\\'
    
    names = [
    {
        'yRes': 2.5,
        'frequencyHz': 50e3,
        'ch': [
            {
                'name': 'continuum',
                'comment': 'PassiveSpectroscopy, ADC 1, ch 1',
                'unit': 'S(V)'
            },
            {
                'name': 'Линия C III (465 nm)',
                'comment': 'PassiveSpectroscopy, ADC 1, ch 2',
                'unit': 'S(V)'
            },
            {
                'name': 'Линия He I (587 nm)',
                'comment': 'PassiveSpectroscopy, ADC 1, ch 3'
            },{
                'name': 'Линия N II (568 nm)',
                'comment': 'PassiveSpectroscopy, ADC 1, ch 4',
                'yRes': -2.5
            },{
                'name': 'Линия O II (441 nm)',
                'comment': 'PassiveSpectroscopy, ADC 1, ch 5',
                'yRes': -2.5
            },{
                'name': 'Линия B II (343 nm)',
                'comment': 'PassiveSpectroscopy, ADC 1, ch 6',
                'yRes': -2.5
            },{
                'name': 'Линия Fe I (427 nm)',
                'comment': 'PassiveSpectroscopy, ADC 1, ch 7',
                'yRes': -2.5
            },{
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 8',
            },{
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 9',
            },
            {
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 10',
            },
            {
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 11',
            },
            {
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 12',
            },{
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 13',
            },{
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 14',
            },{
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 15',
            },{
                'skip': True,
                'comment': 'PassiveSpectroscopy, ADC 1, ch 16',
            }
        ]
    },
    {
        'yRes': 2.5,
        'frequencyHz': 50e3,
        'ch': [
            {
                'comment': 'PassiveSpectroscopy, ADC 2, ch 1'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 2'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 3'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 4'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 5'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 6'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 7'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 8'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 9'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 10'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 11'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 12'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 13'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 14'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 15'
            },{
                'comment': 'PassiveSpectroscopy, ADC 2, ch 16'
            }
        ]
    },
    {
        'yRes': 2.5,
        'frequencyHz': 50e3,
        'ch': [
            {
                'comment': 'PassiveSpectroscopy, ADC 3, ch 1'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 3, ch 2'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 3, ch 3'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 4'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 5'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 6'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 7'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 8'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 9'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 10'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 3, ch 11'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 3, ch 12'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 13'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 14'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 15'
            },{
                'comment': 'PassiveSpectroscopy, ADC 3, ch 16'
            }
        ]
    },
        {
        'yRes': 2.5,
        'frequencyHz': 50e3,
        'ch': [
            {
                'comment': 'PassiveSpectroscopy, ADC 4, ch 1'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 4, ch 2'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 4, ch 3'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 4'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 5'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 6'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 7'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 8'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 9'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 4, ch 10'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 4, ch 11'
            },
            {
                'comment': 'PassiveSpectroscopy, ADC 4, ch 12'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 13'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 14'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 15'
            },{
                'comment': 'PassiveSpectroscopy, ADC 4, ch 16'
            }
        ]
    }
    ]
    
    merge: list[str] = []
    #merge.append('%ssht%d.SHT' % (sht_path, shotn))
    
    for i in range(4):
        path = '%s%d\\ADC%d\\' % (Tukhmeneva_path, shotn, i+1)
        packed = shtRipper.ripper.write_ADC(path=path, filename='adc_data.slow', data=names[i])
        merge.append('%s\\adc_data.slow.sht' % path)
    
    print(merge)
    
    shtRipper.ripper.merge('', 'all_%d.sht' % shotn, merge)
    
    #res = shtRipper.ripper.read('all_%d.sht' % shotn)
    #print(res.keys())
    
    
    print('OK.')

---