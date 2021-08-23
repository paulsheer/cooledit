
f = open('/home/paul/UnicodeData.txt').readlines()

def LEVANT(c):
    return \
((c) >= 0x00590 and ((c) <= 0x008FF or \
                      ((c) >= 0x0FB1D and ((c) <= 0x0FDFF or \
                      ((c) >= 0x0FE70 and ((c) <= 0x0FEFF or \
                      ((c) >= 0x10800 and ((c) <= 0x10CFF or \
                      ((c) >= 0x1E800 and ((c) <= 0x1EEFF))))))))))

for ll in f:
    ll = ll.strip()
    if not ll:
        continue
    l = ll.split(';')
    if l[4] == 'R':
        i = l[0]
        i = eval('0x' + i)
        if not LEVANT(i):
            print('R ===> ' + ll)
    if l[4] == 'L':
        i = l[0]
        i = eval('0x' + i)
        if LEVANT(i):
            print('R ===> ' + ll)


