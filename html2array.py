import gzip
#Import("env")

with open('data/update.html', 'rb') as f:
    data = f.read()
    dataz = list(gzip.compress(data))
    #dataz2 = gzip.compress(data)
    #with open('html.gz', 'wb') as f3:
    #    f3.write(dataz2)
    with open('include/html.h', 'w') as f2:
        f2.write(f"const uint8_t UPDATE_HTML[{len(dataz)}] PROGMEM = " )
        f2.write(str(dataz).replace('[', '{').replace(']', '}') + ";")