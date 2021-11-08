# shtRipper

Код для извлечения данных из .sht файлов. На текущий момент файлы версии ниже 2 не поддерживаются, что будет исправлено
 при необходимости.


    import shtRipper
    
    filename = 'd:/data/cfm/original/sht40808.SHT'
    res = shtRipper.ripper.read(filename)
    print(res.keys())

---   