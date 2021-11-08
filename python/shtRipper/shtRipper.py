import ctypes
import os.path

# require python >= 3.5 for ctypes compiler match

# c++ toolchain: Visual Studio 2019 (pro). Version 16. Arc: x86_amd64. CMake: bundled


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


class Ripper:
    encoding: str = 'cp1251'

    def __init__(self):
        print('shtRipper v2')

        self.lib = ctypes.cdll.LoadLibrary('binary/ripperForPython.dll')

        self.lib.test.argtypes = [ctypes.c_int]
        self.lib.test.restype = ctypes.c_int

        self.lib.rip.argtypes = [ctypes.c_char_p]
        self.lib.rip.restype = In

        self.lib.freeOut.argtypes = None
        self.lib.freeOut.restype = None

        if self.lib.test(5) != 25:
            print('DLL failed simple test.')
            exit(-1)

    def __del__(self):
        print('ripper normal exit')
        self.lib.freeOut()

    def read(self, filename: str) -> dict:  # add "defaultX" option.
        if not os.path.isfile(filename):
            err: str = 'requested file "%s" does not exist.' % filename
            print(err)
            return {
                'ok': False,
                'err': err
            }
        with open(filename, 'rb') as file:
            data = file.read()
        data_p = ctypes.string_at(data, len(data))

        #try-catch
        resp = self.lib.rip(data_p)

        res: dict = {}

        curr: int = 0
        print('extracted %d signals.' % resp.size)
        for signal_count in range(resp.size):
            header = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(Signal)).contents
            curr += 408
            #print('signal name: ', header.name.decode(encoding))
            #print('comment: ', header.comment.decode(encoding))
            #print('unit: ', header.unit.decode(encoding))
            t = header.time
            #print('time: ', '%d.%d.%d %d:%d:%d.%d' % (t.year, t.month, t.day, t.hour, t.min, t.sec, t.msec))
            #print(header.count)
            signal = {
                'comment': header.comment.decode(self.encoding),
                'unit': header.unit.decode(self.encoding),
                'time': '%d.%d.%d %d:%d:%d.%d' % (t.year, t.month, t.day, t.hour, t.min, t.sec, t.msec)
            }
            if header.type >> 16 == 0:
                t_min = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double)).contents.value
                curr += 8
                t_max = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double)).contents.value
                curr += 8
                data = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                t_mult = (t_max - t_min) / (header.count - 1)
                signal['x'] = [i * t_mult + t_min for i in range(header.count)]  # move computation to c++
                signal['y'] = data[:]

            elif header.type >> 16 == 1:
                print('!!! this file type is not supported yet. Please, give it to Nikita.')
                curr += header.count * 8 * 2
            elif header.type >> 16 == 2:
                print('!!! this file type is not supported yet. Please, give it to Nikita.')
                curr += header.count * 8 * 3
            res[header.name.decode(self.encoding)] = signal
        return res