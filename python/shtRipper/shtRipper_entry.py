import ctypes
from pathlib import Path
import sys
import platform
from datetime import datetime
import struct

# require python >= 3.5 for ctypes compiler match

# c++ toolchain: Visual Studio 2019 (pro). Version 16. Arc: x86_amd64. CMake: bundled
# c++ toolchain: Visual Studio 2019 (pro). Version 16. Arc: x86. CMake: bundled

# to-do:
# 1) try-catch read()
# 2) clean memory before read

class _Array(ctypes.Structure):
    _fields_ = [
        ('size', ctypes.c_int),
        ('point', ctypes.POINTER(ctypes.c_char))
    ]


class _Time(ctypes.Structure):
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


class _Signal(ctypes.Structure):
    _fields_ = [
        ('type', ctypes.c_int),
        ('name', ctypes.c_char * 128),
        ('comment', ctypes.c_char * 128),
        ('unit', ctypes.c_char * 128),
        ('time', _Time),
        ('count', ctypes.c_int)
    ]


class _Header(ctypes.Structure):
    _fields_ = [
        ('type', ctypes.c_int),
        ('name', ctypes.c_char * 128),
        ('comment', ctypes.c_char * 128),
        ('unit', ctypes.c_char * 128),
        ('time', _Time),
        ('count', ctypes.c_int),
        ('tMin', ctypes.c_double),
        ('tMax', ctypes.c_double),
        ('yMin', ctypes.c_double),
        ('delta', ctypes.c_double),
        ('void', ctypes.c_char_p)
    ]


encoding: str = 'cp1251'


class Ripper:
    class _Unpacked:
        def __init__(self, data: dict):
            self.count = 0
            self.error = ''

            py_headers = []
            py_data = []
            data_size = 0
            for signal_name in data.keys():
                signal = data[signal_name]
                if type(signal) is not dict:
                    self.error = 'Error: data contains bad signal "%s"' % signal
                    return
                header = _Header()
                header.name = signal_name[:128].encode(encoding)
                if 'comment' in signal:
                    header.comment = signal['comment'][:128].encode(encoding)
                else:
                    header.comment = ''.encode(encoding)
                if 'unit' in signal:
                    header.unit = signal['unit'][:128].encode(encoding)
                else:
                    header.unit = ''.encode(encoding)
                header.time = _Time()
                '''if 'timestamp' in signal:
                    header.unit = signal['unit'].encode(encoding)
                else:
                    header.unit = ''.encode(encoding)
                    #dt_object = datetime.fromtimestamp(time.time())
                    '''
                if 'x' in signal and 'y' in signal:
                    if 'err' in signal:
                        header.type = 2 << 16
                        if len(signal['x']) != len(signal['y']) != len(signal['err']):
                            self.error = 'Error: X, Y, and Err arrays have different length: %d vs %d vs %d'  % \
                                         (len(signal['x']), len(signal['y']), len(signal['err']))
                            return
                        header.count = len(signal['x'])

                        serialised_data = (ctypes.c_char * (len(signal['x']) * ctypes.sizeof(ctypes.c_double) * 3))()
                        for i in range(len(signal['x'])):
                            ctypes.memmove(ctypes.byref(serialised_data, i * 3 * ctypes.sizeof(ctypes.c_double)),
                                           ctypes.pointer(ctypes.c_double(signal['x'][i])),
                                           ctypes.sizeof(ctypes.c_double))
                            ctypes.memmove(ctypes.byref(serialised_data, (i * 3 + 1) * ctypes.sizeof(ctypes.c_double)),
                                           ctypes.pointer(ctypes.c_double(signal['y'][i])),
                                           ctypes.sizeof(ctypes.c_double))
                            ctypes.memmove(ctypes.byref(serialised_data, (i * 3 + 2) * ctypes.sizeof(ctypes.c_double)),
                                           ctypes.pointer(ctypes.c_double(signal['err'][i])),
                                           ctypes.sizeof(ctypes.c_double))
                        py_data.append({
                            'size': (len(signal['x']) * ctypes.sizeof(ctypes.c_double) * 3),
                            'data': serialised_data
                        })
                        data_size += (len(signal['x']) * ctypes.sizeof(ctypes.c_double) * 3)
                    else:
                        header.type = 1 << 16
                        if len(signal['x']) != len(signal['y']):
                            self.error = 'Error: X and Y arrays have different length: %d vs %d' % \
                                         (len(signal['x']), len(signal['y']))
                            return
                        header.count = len(signal['x'])

                        serialised_data = (ctypes.c_char * (len(signal['x']) * ctypes.sizeof(ctypes.c_double) * 2))()
                        for i in range(len(signal['x'])):
                            ctypes.memmove(ctypes.byref(serialised_data, i * 2 * ctypes.sizeof(ctypes.c_double)),
                                           ctypes.pointer(ctypes.c_double(signal['x'][i])),
                                           ctypes.sizeof(ctypes.c_double))
                            ctypes.memmove(ctypes.byref(serialised_data, (i * 2 + 1) * ctypes.sizeof(ctypes.c_double)),
                                           ctypes.pointer(ctypes.c_double(signal['y'][i])),
                                           ctypes.sizeof(ctypes.c_double))
                        py_data.append({
                            'size': (len(signal['x']) * ctypes.sizeof(ctypes.c_double) * 2),
                            'data': serialised_data
                        })
                        data_size += (len(signal['x']) * ctypes.sizeof(ctypes.c_double) * 2)
                elif 'tMin' in signal and 'tMax' in signal and 'y' in signal and 'offset' in signal and 'yRes' in signal:
                    header.count = len(signal['y'])
                    if signal['tMin'] >= signal['tMax']:
                        self.error = 'Error: tMin must be less than tMax: %f < %f' % \
                                     (signal['tMin'], signal['tMax'])
                        return
                    header.tMin = signal['tMin']
                    header.tMax = signal['tMax']

                    header.yMin = signal['offset']
                    header.delta = signal['yRes']
                    header.type = 0 << 16

                    serialised_data = (ctypes.c_char * header.count * ctypes.sizeof(ctypes.c_long))()
                    for i in range(header.count):
                        ctypes.memmove(ctypes.byref(serialised_data, i * ctypes.sizeof(ctypes.c_long)),
                                       ctypes.pointer(ctypes.c_long(int((signal['y'][i] - header.yMin) / header.delta))),
                                       ctypes.sizeof(ctypes.c_long))
                    py_data.append({
                        'size': (header.count * ctypes.sizeof(ctypes.c_long)),
                        'data': serialised_data
                    })
                    data_size += header.count * ctypes.sizeof(ctypes.c_long)
                else:
                    self.error = 'Error: bad signal format.'
                    return

                py_headers.append(header)
                self.count += 1

            _headers = (ctypes.c_char * (ctypes.sizeof(_Header) * self.count))()
            for header_ind in range(len(py_headers)):
                ctypes.memmove(ctypes.byref(_headers, ctypes.sizeof(_Header) * header_ind),
                               ctypes.pointer(py_headers[header_ind]), ctypes.sizeof(_Header))
            self.headers = ctypes.cast(ctypes.pointer(_headers), ctypes.c_char_p)

            _data = (ctypes.c_char * data_size)()
            offset: int = 0
            for entry in py_data:
                ctypes.memmove(ctypes.byref(_data, offset), ctypes.pointer(entry['data']), entry['size'])
                offset += entry['size']

            self.data = ctypes.cast(ctypes.pointer(_data), ctypes.c_char_p)

    class _UnpackedADC:
        def __init__(self, data: dict):
            self.error = ''
            py_headers = []

            if 'ch' not in data or len(data['ch']) != 16:
                self.error = 'Error: data description must cover 16 channels'
                return
            if 'yRes' not in data:
                self.error = 'Error: bad signal format:  yRes is required.'
                return
            if 'frequencyHz' not in data:
                self.error = 'Error: bad signal format:  frequency is required.'
                return
            if data['frequencyHz'] <= 0:
                self.error = 'Error: bad signal format:  frequency must be positive.'
                return
            for ch_ind in range(16):
                ch = data['ch'][ch_ind]
                header = _Header()
                header.type = 0 << 16
                header.time = _Time()
                if 'chMap' in data and data['chMap']:
                    header.count = 1
                else:
                    header.count = 0
                header.yMin = 0
                if 'skip' in ch and ch['skip']:
                    header.tMin = 999
                    header.tMax = -999

                    header.name = 'NONE'[:128].encode(encoding)
                    header.comment = 'NONE'[:128].encode(encoding)
                    header.unit = 'NONE'[:128].encode(encoding)
                    header.delta = 999
                else:
                    if 'name' in ch:
                        header.name = ch['name'][:128].encode(encoding)
                    else:
                        if 'comment' in ch:
                            header.name = ch['comment'][:128].encode(encoding)
                        else:
                            header.name = ('channel %d' % (ch_ind + 1))[:128].encode(encoding)
                    if 'comment' in ch:
                        header.comment = ch['comment'][:128].encode(encoding)
                    else:
                        header.comment = ('channel %d' % (ch_ind + 1))[:128].encode(encoding)
                    if 'unit' in ch:
                        header.unit = ch['unit'][:128].encode(encoding)
                    else:
                        header.unit = 'V(V)'.encode(encoding)
                    if 'tMin' in data:
                        header.tMin = data['tMin']
                    else:
                        header.tMin = 0
                    if 'yRes' in ch:
                        header.delta = ch['yRes'] / 32768
                    else:
                        header.delta = data['yRes'] / 32768
                    header.tMax = data['frequencyHz']
                py_headers.append(header)

            _headers = (ctypes.c_char * (ctypes.sizeof(_Header) * 16))()
            for header_ind in range(len(py_headers)):
                ctypes.memmove(ctypes.byref(_headers, ctypes.sizeof(_Header) * header_ind),
                               ctypes.pointer(py_headers[header_ind]), ctypes.sizeof(_Header))
            self.headers = ctypes.cast(ctypes.pointer(_headers), ctypes.c_char_p)


    def __init__(self):
        print('shtRipper v1.6.1')
        #debug = True
        debug = False
        if platform.system() == 'Windows':
            if debug:
                print('RUNNING DEBUG DLL!')
                self.lib = ctypes.cdll.LoadLibrary('D:/code/shtRipper_cpp/python/shtRipper/binary/ripperForPython.dll')
            else:
                self.lib = ctypes.cdll.LoadLibrary('%s\\binary\\ripperForPython_%d.dll' %
                                               (Path(__file__).parent, 64 if sys.maxsize > 0x100000000 else 32))

        elif platform.system() == 'Linux':
            self.lib = ctypes.cdll.LoadLibrary('%s/binary/libripperForPython.so' % Path(__file__).parent)
            #self.lib = ctypes.cdll.LoadLibrary('/home/nz/CLionProjects/shtRipper_cpp/python/shtRipper/binary/libripperForPython.so')
        else:
            print("Unsupported OS. Trying to load MacOS binary")
            self.lib = ctypes.cdll.LoadLibrary('%s/binary/libripperForPythonFor___.so' % Path(__file__).parent)
            #fuck_off

        self.lib.test.argtypes = [ctypes.c_int]
        self.lib.test.restype = ctypes.c_int

        self.lib.rip.argtypes = [ctypes.c_char_p, ctypes.c_uint, ctypes.c_char_p]
        self.lib.rip.restype = _Array

        self.lib.cram.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p]
        self.lib.cram.restype = _Array

        self.lib.cramADC.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p]
        self.lib.cramADC.restype = _Array

        self.lib.freeOut.argtypes = None
        self.lib.freeOut.restype = None

        if self.lib.test(5) != 25:
            print('DLL failed simple test.')
            exit(-1)

    def __del__(self):
        print('ripper normal exit')
        self.lib.freeOut()

    def read(self, filename: str, signals: list = None) -> dict:  # add "defaultX" option.
        path = Path(filename)
        #print('Reading file: ', str(path.absolute()))

        if not path.is_file():
            err: str = 'requested file "%s" does not exist.' % filename
            print(err)
            return {
                'ok': False,
                'err': err
            }

        pathStr = str(path.absolute()) + '\0'
        path_p = ctypes.c_char_p(pathStr.encode())

        if signals is None:
            s_count = ctypes.c_uint(0)
            s_point = ctypes.c_char_p()
        else:
            s_count = ctypes.c_uint(len(signals))
            request = ''
            for s in signals:
                request += (s[:127] + '\0').ljust(128, '\0')
            s_point = ctypes.create_string_buffer(request.encode(encoding), 128 * len(signals))


        #try-catch
        resp = self.lib.rip(path_p,
                            s_count,
                            s_point)

        res: dict = {}

        curr: int = 0
        #print('extracted %d signals.' % resp.size)
        for signal_count in range(resp.size):
            header = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(_Signal)).contents
            curr += 408
            #print('signal name: ', header.name.decode(encoding))
            #print('comment: ', header.comment.decode(encoding))
            #print('unit: ', header.unit.decode(encoding))
            t = header.time
            #print('time: ', '%d.%d.%d %d:%d:%d.%d' % (t.year, t.month, t.day, t.hour, t.min, t.sec, t.msec))
            #print(header.count)
            signal = {
                'comment': header.comment.decode(encoding),
                'unit': header.unit.decode(encoding),
                'time': '%d.%d.%d %d:%d:%d.%d' % (t.year, t.month, t.day, t.hour, t.min, t.sec, t.msec)
            }
            if header.type >> 16 == 0:
                #t_min = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double)).contents.value
                #curr += 8
                #t_max = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double)).contents.value
                #curr += 8
                data_x = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                data_y = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                #t_mult = (t_max - t_min) / (header.count - 1)
                signal['x'] = data_x[:]
                signal['y'] = data_y[:]

            elif header.type >> 16 == 1:
                data_x = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                data_y = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                signal['x'] = data_x[:]
                signal['y'] = data_y[:]
            elif header.type >> 16 == 2:
                data_x = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                data_y = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                data_z = ctypes.cast(ctypes.byref(resp.point.contents, curr), ctypes.POINTER(ctypes.c_double * header.count)).contents
                curr += header.count * 8
                signal['x'] = data_x[:]
                signal['y'] = data_y[:]
                signal['z'] = data_z[:]
            try:
                res[header.name.decode(encoding)] = signal
            except UnicodeDecodeError:
                tmp = header.name.decode(encoding, 'replace')
                print('sht file is broken. bad signal name: ', tmp)
                res[tmp] = signal

        self.lib.freeOut()
        return res

    def write(self, path: str, filename: str, data: dict) -> str:
        filepath = Path(path)
        if not filepath.is_dir():
            err: str = 'requested path "%s" does not exist.' % path
            print(err)
            return err

        prepared_data = self._Unpacked(data)
        if prepared_data.error != '':
            print(prepared_data.error)
            return prepared_data.error

        resp = self.lib.cram(ctypes.c_int(prepared_data.count), prepared_data.headers, prepared_data.data)
        if resp.size < 0:
            return 'dll error %d' % resp.size

        with open('%s/%s' % (filepath, filename), 'wb') as file:
            buff = ctypes.cast(resp.point, ctypes.POINTER(ctypes.c_char * resp.size))
            file.write(bytearray(buff.contents))
        return ''

    def pack(self, data: dict) -> bytearray:
        prepared_data = self._Unpacked(data)
        if prepared_data.error != '':
            print(prepared_data.error)
            return bytearray()

        resp = self.lib.cram(ctypes.c_int(prepared_data.count), prepared_data.headers, prepared_data.data)
        if resp.size < 0:
            return bytearray()

        buff = ctypes.cast(resp.point, ctypes.POINTER(ctypes.c_char * resp.size))
        return bytearray(buff.contents)

    def write_ADC(self, path: str, filename: str, data: dict) -> str:
        #print('debug write ADC: ', path, filename, data)
        filepath = Path(path)
        if not filepath.is_dir():
            err: str = 'requested path "%s" does not exist.' % filename
            print(err)
            return err

        prepared_data = self._UnpackedADC(data)
        if prepared_data.error != '':
            print(prepared_data.error)
            return prepared_data.error

        pathStr = str(filepath.absolute()) + '\\' + filename + '\0'
        path_p = ctypes.c_char_p(pathStr.encode())
        #or loaded raw data

        resp = self.lib.cramADC(ctypes.c_int(0), prepared_data.headers, path_p)
        if resp.size <= 0:
            return 'dll error %d' % resp.size

        with open('%s/%s.sht' % (filepath, filename), 'wb') as file:
            buff = ctypes.cast(resp.point, ctypes.POINTER(ctypes.c_char * resp.size))
            file.write(bytearray(buff.contents))
        return ''

    def merge(self, path: str, filename: str, shts: list[str]) -> str:
        filepath = Path(path).absolute()
        if not filepath.is_dir():
            err: str = 'requested path "%s" does not exist.' % path
            print(err)
            return err

        data = []
        total_count = 0
        for sht in shts:
            p = Path(sht)
            #print('Reading file: ', str(path.absolute()))
            if not p.is_file():
                err: str = 'requested file "%s" does not exist.' % p
                print(err)
                return {
                    'ok': False,
                    'err': err
                }
            
            with open(sht, 'rb') as file:
                file.seek(12, 0) 
                count = struct.unpack('i', file.read(4))[0]
                total_count += count
                data += file.read()
                #print(len(data))
            #print('Found %d signals.' % count)
        #print('Merged %d signals' % total_count)

        with open('%s\\%s' % (filepath, filename), 'wb') as file:
            s = "ANALIZER1.2"
            file.write(s.encode('ascii'))
            file.write(bytes([0]))
            file.write(struct.pack('i', total_count))
            file.write(bytearray(data))

        return ''
