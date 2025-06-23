import shtRipper
import faulthandler

faulthandler.enable()


ripper = shtRipper.ripper

shotn = 46040
sht_path = '\\\\172.16.12.127\\Data\\'
slow_path = '\\\\192.168.10.41\\d\\data\\db\\plasma\\slow\\raw\\'
TS_path = ''
cfm_path = '\\\\172.16.12.127\\Pub\\!!!CURRENT_COIL_METHOD\\V3_zad7_mcc\\mcc0d_'
Tukhmeneva_path = '\\\\172.16.12.127\\Pub\\!!!SHT Tuxmeneva\\'
diamagnetic_path = '\\\\172.16.12.127\\Pub\\!diamagnetic_data\\sht\\'

filenames = ['192.168.10.50.slow',
            '192.168.10.51.slow',
            '192.168.10.52.slow',
            '192.168.10.53.slow']

to_pack = {
    'yRes': 5,
    'ch': [
        {
            'name': 'laser Sync',
            'unit': 'S(V)'
        },
        {
            'name': 'name 2',
            'unit': 'S(V)'
        },
        {
            'name': 'name 3'
        },{
            'name': 'name 4'
        },{
            'name': 'name 5'
        },{
            'name': 'name 6'
        },{
            'name': 'name 7'
        },{
            'name': 'name 8'
        },{
            'name': 'name 9'
        },
        {
            'name': 'name 10'
        },
        {
            'comment': 'comm 1'
        },
        {
            'comment': 'comm 1'
        },{
            'comment': 'comm 1'
        },{
            'comment': 'comm 1'
        },{
            'comment': 'comm 1'
        },{
            'comment': 'comm 1'
        }
    ]
}

merge: list[str] = []
merge.append('%ssht%d.SHT' % (sht_path, shotn))

for file in filenames:
    path = '%ssht%d\\' % (slow_path, shotn)
    packed = shtRipper.ripper.write_ADC(path=path, filename=file, data=to_pack)
    merge.append('%s\\%s.sht' % (path, file))

merge.append('%s%d.SHT' % (cfm_path, shotn))

merge.append('%sSPC%d_1.SHT' % (Tukhmeneva_path, shotn))
merge.append('%s%d_2.SHT' % (Tukhmeneva_path, shotn))
merge.append('%s%d_3.SHT' % (Tukhmeneva_path, shotn))

merge.append('%s%d.SHT' % (diamagnetic_path, shotn))


print(merge)

ripper.merge('', 'all_%d.sht' % shotn, merge)

for i in range(5000000):  # wait for possible errors in dll
    d = 56784678 / 5423621543

print('OK.')