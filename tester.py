import ctypes
import time

#require python >= 3.5


class In(ctypes.Structure):
    _fields_ = [
        ('size', ctypes.c_int),
        ('point', ctypes.POINTER(ctypes.c_char))
    ]


class Time(ctypes.Structure):
    _fields_ = [
        ('year', ctypes.c_ushort),
        ('month', ctypes.c_ushort),
        ('dayOfWeek', ctypes.c_ushort),
        ('day', ctypes.c_ushort),
        ('hour', ctypes.c_ushort),
        ('min', ctypes.c_ushort),
        ('sec', ctypes.c_ushort),
        ('msec', ctypes.c_ushort),
    ]


class Signal(ctypes.Structure):
    _fields_ = [
        ('type', ctypes.c_int),
        ('name', ctypes.c_char * 128),
        ('comment', ctypes.c_char * 128),
        ('unit', ctypes.c_char * 128),
        ('time', Time),
        ('count', ctypes.c_int)
    ]


encoding = 'cp1251'

print('shtRipper v2')

lib = ctypes.cdll.LoadLibrary('distribution/ripperForPython.dll')
#print('DLL loaded.\n')

lib.test.argtypes = [ctypes.c_int]
lib.test.restype = ctypes.c_int

lib.rip.argtypes = [ctypes.c_char_p]
lib.rip.restype = In

lib.freeOut.argtypes = None
lib.freeOut.restype = None

if lib.test(5) != 25:
    print('DLL failed simple test.')
    exit()

filename = 'd:/data/cfm/original/sht40808.SHT'
#filename = 'd:/data/cfm/original/eft38800.sht'

start_time = time.time()
with open(filename, 'rb') as file:
    data = file.read()


data_p = ctypes.string_at(data, len(data))
for i in range(10):
    resp = lib.rip(data_p)

    curr = 0
    header = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(Signal)).contents
    print('signal name: ', header.name.decode(encoding))
    print('comment: ', header.comment.decode(encoding))
    print('unit: ', header.unit.decode(encoding))
    t = header.time
    print('time: ', '%d.%d.%d %d:%d:%d.%d' % (t.year, t.month, t.day, t.hour, t.min, t.sec, t.msec))
    if header.type >> 16 == 0:
        print(header.count)
        curr += 408
        t_min = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double)).contents.value
        print('tMin = ', t_min)
        curr += 8
        t_max = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double)).contents.value
        print('tMax = ', t_max)
        curr += 8

        data = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
        curr += header.count * 8

        '''with open('out.txt', 'w') as file:
            for i in range(header.count):
                file.write('%.6f, %.3f\n' % (i * (t_max - t_min) / (header.count - 1) + t_min,
                                             data[i]))'''
        #print('iteration %d' % i)

print("--- %.2f seconds ---" % (time.time() - start_time))
lib.freeOut()


for i in range(1000000):
    d = 56784678 / 5423621543

print('OK.')
