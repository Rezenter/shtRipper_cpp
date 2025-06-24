import shtRipper
import json
import faulthandler

faulthandler.enable()


ripper = shtRipper.ripper

import time

start_time = time.time()

filename = '192.168.10.52.slow'
filename = 'adc_data.slow'

to_pack = {
    'yRes': 5,
    'chMap': True,
    'frequencyHz': 500e3 * 0.987652,
    'ch': [
        {
            'name': 'name 1',
            'unit': 'S(V)'
        },
        {
            'skip': True
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
            'skip': True
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

packed = shtRipper.ripper.write_ADC(path='', filename=filename, data=to_pack)

print("--- %.2f seconds ---" % (time.time() - start_time))


for i in range(50000000):  # wait for possible errors in dll
    d = 56784678 / 5423621543

print('OK.')