import struct

chMap = [0, 2, 4, 6, 10, 8, 14, 12, 1, 3, 5, 7, 11, 9, 15, 13]

with open('192.168.10.50.slow', 'rb') as file:
    data_raw = file.read()
    point_count = int(len(data_raw) / (16 * 2))  # ch count = 16, sizeof(short) = 2
    print(point_count, len(data_raw))
    board = [[] for ch in range(16)]
    for ch_ind in range(16):
        for i in range(1, point_count):  # skip first as it is sometimes corrupted
            board[ch_ind].append(
                struct.unpack_from('<h', buffer=data_raw, offset=16 * i * 2 + 2 * chMap[ch_ind])[0])

    Vref = 1
    freq = 5e5*0.99
    with open('dump.csv', 'w') as out:
        for i in range(point_count):
            out.write('%f, %d, %f, %f\n' % (i/freq, board[1][i-1], board[2][i-1]*Vref*(5/32768), board[3][i-1]*Vref*(5/32768)))
