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
for i in range(1):
    resp = lib.rip(data_p)

    data = ctypes.cast(resp.point, ctypes.POINTER(ctypes.c_char * resp.size)).contents
    b_data = bytes(data)

    header = ctypes.cast(resp.point, ctypes.POINTER(Signal)).contents
    print('signal name: ', header.name.decode(encoding))
    print('comment: ', header.comment.decode(encoding))
    print('unit: ', header.unit.decode(encoding))
    t = header.time
    print('time: ', '%d.%d.%d %d:%d:%d.%d' % (t.year, t.month, t.day, t.hour, t.min, t.sec, t.msec))

    if header.type == 1:
        print(header.count)

    #print('iteration %d' % i)

print("--- %.2f seconds ---" % (time.time() - start_time))
lib.freeOut()


for i in range(1000000):
    d = 56784678 / 5423621543

print('OK.')
