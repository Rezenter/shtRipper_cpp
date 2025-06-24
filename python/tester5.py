import shtRipper
import faulthandler

faulthandler.enable()


ripper = shtRipper.ripper

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

ripper.merge('', 'all_%d.sht' % shotn, merge)

res = ripper.read('all_%d.sht' % shotn)
print(res.keys())

for i in range(50000000):  # wait for possible errors in dll
    d = 56784678 / 5423621543

print('OK.')