

# This script finds a minimal useful set of NotoSans fonts to cover the maximum number of characters


import os, re, sys, os
from PIL import PcfFontFile

priority_order = """
NotoSans-Regular.ttf
NotoSansSymbols-Regular.ttf
NotoSansSymbols2-Regular.ttf
NotoSansMath-Regular.ttf
NotoMusic-Regular.ttf
NotoColorEmoji.ttf
"""
priority_order = priority_order.split()

manual_font_placement = """
NotoSansHK-Regular.otf
NotoSansJP-Regular.otf
NotoSansKR-Regular.otf
NotoSansTC-Regular.otf
NotoSansSC-Regular.otf
"""

manual_font_placement = manual_font_placement.split()

#  ls -1 *.ttf *.otf | sed -e 's/^.*[-]\([A-Z][A-Za-z]*\)[.]\(otf\|ttf\)$/\1/g' | sort | uniq -c | sort -rn | awk '{print $2}' | grep -v '[.]'

weight = """
Regular
Bold
Medium
SemiBold
Thin
Black
Light
ExtraLight
ExtraBold
ExtraCondensed
Condensed
SemiCondensed
ExtraCondensedThin
ExtraCondensedBold
ExtraCondensedBlack
CondensedThin
CondensedBold
CondensedBlack
VF
SemiCondensedThin
SemiCondensedSemiBold
SemiCondensedMedium
SemiCondensedLight
SemiCondensedExtraLight
SemiCondensedExtraBold
SemiCondensedBold
SemiCondensedBlack
ExtraCondensedSemiBold
ExtraCondensedMedium
ExtraCondensedLight
ExtraCondensedExtraLight
ExtraCondensedExtraBold
CondensedSemiBold
CondensedMedium
CondensedLight
CondensedExtraLight
CondensedExtraBold
ThinItalic
SemiCondensedThinItalic
SemiCondensedSemiBoldItalic
SemiCondensedMediumItalic
SemiCondensedLightItalic
SemiCondensedItalic
SemiCondensedExtraLightItalic
SemiCondensedExtraBoldItalic
SemiCondensedBoldItalic
SemiCondensedBlackItalic
SemiBoldItalic
MediumItalic
LightItalic
Italic
ExtraLightItalic
ExtraCondensedThinItalic
ExtraCondensedSemiBoldItalic
ExtraCondensedMediumItalic
ExtraCondensedLightItalic
ExtraCondensedItalic
ExtraCondensedExtraLightItalic
ExtraCondensedExtraBoldItalic
ExtraCondensedBoldItalic
ExtraCondensedBlackItalic
ExtraBoldItalic
CondensedThinItalic
CondensedSemiBoldItalic
CondensedMediumItalic
CondensedLightItalic
CondensedItalic
CondensedExtraLightItalic
CondensedExtraBoldItalic
CondensedBoldItalic
CondensedBlackItalic
BoldItalic
BlackItalic
Semibold
DisplayThinItalic
DisplayThin
DisplaySemiCondensedThinItalic
DisplaySemiCondensedThin
DisplaySemiCondensedSemiBoldItalic
DisplaySemiCondensedSemiBold
DisplaySemiCondensedMediumItalic
DisplaySemiCondensedMedium
DisplaySemiCondensedLightItalic
DisplaySemiCondensedLight
DisplaySemiCondensedItalic
DisplaySemiCondensedExtraLightItalic
DisplaySemiCondensedExtraLight
DisplaySemiCondensedExtraBoldItalic
DisplaySemiCondensedExtraBold
DisplaySemiCondensedBoldItalic
DisplaySemiCondensedBold
DisplaySemiCondensedBlackItalic
DisplaySemiCondensedBlack
DisplaySemiCondensed
DisplaySemiBoldItalic
DisplaySemiBold
DisplayRegular
DisplayMediumItalic
DisplayMedium
DisplayLightItalic
DisplayLight
DisplayItalic
DisplayExtraLightItalic
DisplayExtraLight
DisplayExtraCondensedThinItalic
DisplayExtraCondensedThin
DisplayExtraCondensedSemiBoldItalic
DisplayExtraCondensedSemiBold
DisplayExtraCondensedMediumItalic
DisplayExtraCondensedMedium
DisplayExtraCondensedLightItalic
DisplayExtraCondensedLight
DisplayExtraCondensedItalic
DisplayExtraCondensedExtraLightItalic
DisplayExtraCondensedExtraLight
DisplayExtraCondensedExtraBoldItalic
DisplayExtraCondensedExtraBold
DisplayExtraCondensedBoldItalic
DisplayExtraCondensedBold
DisplayExtraCondensedBlackItalic
DisplayExtraCondensedBlack
DisplayExtraCondensed
DisplayExtraBoldItalic
DisplayExtraBold
DisplayCondensedThinItalic
DisplayCondensedThin
DisplayCondensedSemiBoldItalic
DisplayCondensedSemiBold
DisplayCondensedMediumItalic
DisplayCondensedMedium
DisplayCondensedLightItalic
DisplayCondensedLight
DisplayCondensedItalic
DisplayCondensedExtraLightItalic
DisplayCondensedExtraLight
DisplayCondensedExtraBoldItalic
DisplayCondensedExtraBold
DisplayCondensedBoldItalic
DisplayCondensedBold
DisplayCondensedBlackItalic
DisplayCondensedBlack
DisplayCondensed
DisplayBoldItalic
DisplayBold
DisplayBlackItalic
DisplayBlack
Extralight
Extrabold
"""

weight = weight.split()

def len_sort(a):
    a = []
    for i in weight:
        a.append((len(i), i))
    a.sort(reverse = True)
    r = []
    for i in a:
        r.append(i[1])
    return r

weight_lsort = len_sort(weight)

from fontTools import ttLib

def progress():
    sys.stderr.write('.')
    sys.stderr.flush()

def hash64bit(a):
    r = 0
    a = list(a)[:]
    a.sort()
    for c in a:
        r += (((c + 9) * (c + 2) * 401) >> 1)
        r = (r ^ (r << 48)) % 18446744073709551557
    return r


def font_charset(fname):
    def pcf_charmap(a):
        encoding = {}
        fp, format, i16, i32 = a._getformat(PcfFontFile.PCF_BDF_ENCODINGS)
        firstCol, lastCol = i16(fp.read(2)), i16(fp.read(2))
        firstRow, lastRow = i16(fp.read(2)), i16(fp.read(2))
        i16(fp.read(2))  # default
        nencoding = (lastCol - firstCol + 1) * (lastRow - firstRow + 1)
        for i in range(nencoding):
            encodingOffset = i16(fp.read(2))
            if encodingOffset != 0xFFFF:
                encoding[i + firstCol] = encodingOffset
        return encoding

    if fname.endswith('.pcf.gz'):
        os.system("gzip -cd '%s' > font.tmp" % fname)
        a = pcf_charmap(PcfFontFile.PcfFontFile(open('font.tmp', 'rb')))
        a = list(a.keys())
        a.sort()
        return set(a)
    elif fname.endswith('.pcf'):
        f = PcfFontFile.PcfFontFile(open(fname, 'rb'))
        a = list(a.keys())
        a.sort()
        return set(a)
    else:        
        tt = ttLib.TTFont(fname)
        a = list(tt.getBestCmap().keys())
        a.sort()
        return set(a)



def fontname_replace(s):
    for k1 in weight_lsort:
        for k2 in ('otf', 'ttf'):
            rstr = '-%s.%s' % (k1, k2)
            s = s.replace('-Italic-VF', '')
            s = s.replace('Slanted' + rstr, '')
            s = s.replace('Unjoined' + rstr, '')
            s = s.replace('UI' + rstr, '')
            s = s.replace('Display' + rstr, '')
            s = s.replace(rstr, '')
            s = s.replace('NotoSerif', 'NotoSans')
    for k2 in ('otf', 'ttf'):
        s = s.replace('.' + k2, '')
    return s


def fontname_priority(s):
    r = 2000
    w = 1000
    found = 0
    for k1 in weight:
        for k2 in ('ttf', 'otf'):
            w = w - 1
            rstr = '-%s.%s' % (k1, k2)
            if s.endswith(rstr):
                r = r + w
                found = 1
                break
        if found:
            break
    if s.find('NotoSans') >= 0:
        r += 500
    if s.find('-Italic-VF') >= 0:
        r -= 10
    if s.find('Slanted') >= 0:
        r -= 5
    if s.find('Unjoined') >= 0:
        r -= 1
    if s.find('UI') >= 0:
        r -= 1
    if s.find('Display') >= 0:
        r -= 2
    return r


# print fontname_priority('NotoSansLao-ExtraCondensedExtraLight.ttf')
# print('---')
# print fontname_priority('NotoSansLao-Regular.ttf')
# 
# sys.exit(0)

# print fontname_priority('NotoSans-Regular.otf')
# print fontname_priority('NotoSans-Regular.ttf')
# print fontname_priority('NotoSans-Bold.ttf')
# print fontname_priority('NotoSerif-Regular.ttf')
# print fontname_priority('NotoSerif-Bold.ttf')
# 
# sys.exit(0)

notosans = []
notosans_fname = {}
fnoto = os.popen('ls -1 8x13B.pcf.gz NotoSans-Regular.ttf NotoSansSymbols-Regular.ttf NotoSansSymbols2-Regular.ttf NotoSansMath-Regular.ttf').read().split('\n')
for i in fnoto:
    progress()
    i = i.strip()
    if not i:
        continue
    notosans_fname[i] = True
    c = font_charset(i)
    notosans.append(("%016x" % hash64bit(c), set(c)))
# Private use area defined as U+E000-U+F8FF
private_use_area = set(range(0xE000, 0xF900))


f = os.popen('ls -1 *.ttf *.otf').read().split('\n')

m = {}

for i in f:
    progress()
    i = i.strip()
    if not i:
        continue

    # special cases:
    if i in ('NotoKufiArabic[wght].ttf'):
        continue
    if i in ('NotoSansDisplay-Italic-VF.ttf', 'NotoSerifDisplay-Italic-VF.ttf'):
        continue
    if i.startswith('NotoSansSyriacWestern') or i.startswith('NotoSansSyriacEastern') or i.startswith('NotoSansSyriacEstrangela'):
        continue
    if i in ('NotoSansTamilUI-VF-torecover.ttf', 'NotoSansTamil-android-VF.ttf', 'NotoSansTamilUI-android-VF.ttf'):
        continue
    if i.startswith('NotoSansTifinagh') and not i.startswith('NotoSansTifinagh-Regular'):
        continue
    if i.startswith('NotoSansHebrewNew') or i.startswith('NotoSansHebrewDroid'):
        continue
    charset = font_charset(i)
    if not i in notosans_fname:
        for k in notosans:
            charset = charset.difference(k[1])
    private_use = False
    if charset.intersection(private_use_area):
        private_use = True
    charset = charset.difference(private_use_area)
    if not len(charset):
        if private_use:
            print('Dropping, contains only private-use chars ==>  ', i)
        else:
            print('Dropping, already covered by NotoSans basic fonts ==>  ', i)
        continue
    h = "%016x" % hash64bit(charset)
    if not h in m:
        m[h] = {'charset': charset, 'files' : []}
    m[h]['files'].append(i)


print('')
print('fonts not from google font main.zip archive ======================================================')
print('')

print(manual_font_placement)

print('')
print('dump hashes ======================================================')
print('')

for i in m:
    print(i, m[i]['files'])

print('')
print('check that algorithm for filename similarity is valid ======================================================')
print('')

broken = 0
for i in m:
    p = {}
    for j in m[i]['files']:
        p[fontname_replace(j)] = True
    p = list(p.keys())
    if len(p) != 1:
        print(p, '    ======>   ', m[i]['files'])
        broken = broken + 1

if broken:
    sys.exit(0)

print('')
print('put best filename first ======================================================')
print('')

for i in m:
    p = []
    for j in m[i]['files']:
        p.append((fontname_priority(j), j))
    p.sort(reverse = True)
    q = []
    for j in p:
        q.append(j[1])
    m[i]['files'] = q

for i in m:
    print(i, m[i]['files'])

print('')
print('delete fonts which are a subset of another font ======================================================')
print('')

allchars = []
m2 = {}

for i in m:
    pair = m[i]
    m2[i] = pair

for i in list(m2.keys()):
    progress()
    try:
        pair = m2[i]
    except:
        pass
    for j in list(m2.keys()):
        pair2 = m2[j]
        if pair['files'][0] == pair2['files'][0]:
            continue
        if pair2['charset'].issubset(pair['charset']):
            difference = pair['charset'].difference(pair2['charset'])
            print('  %s(%d)\n  is a subset of\n  %s(%d),\n  delta %d\n---------------\n' % (str(pair2['files']), len(pair2['charset']), str(pair['files']), len(pair['charset']), len(difference)))
            del m2[j]

print('')
print('show overlapping fonts ======================================================')
print('')

if True:
    for i in m2:  # was m for all fonts
        progress()
        pair = m2[i]   # was m for all fonts
        for j in list(m2.keys()):
            pair2 = m2[j]
            if pair['files'][0] == pair2['files'][0]:
                continue
            inters = len(pair2['charset'].intersection(pair['charset']))
            if inters:
                v1 = len(pair2['charset'].difference(pair['charset']))
                v2 = len(pair['charset'].difference(pair2['charset']))
                print('%d(%d,%d): %s(%d) has an overlap with %s(%d)' % (inters, v1, v2, str(pair2['files']), len(pair2['charset']), str(pair['files']), len(pair['charset'])))

print('')
print('final dump ======================================================')
print('')

total_bytes = 0
max_char = -1

final = []
names_only = []
for j in m2:
    ch = list(m2[j]['charset'])
    ch.sort()
    a_file = m2[j]['files'][0]
    total_bytes = total_bytes + os.stat(a_file).st_size
    if len(ch) < 10000:
        pch = ''
        for k in ch:
            pch = pch + '%x ' % k
    else:
        pch = 'too long'
    final.append([len(ch), "%d-%d" % (ch[0], ch[-1]), a_file, pch])
    names_only.append(a_file)

    if max_char < ch[-1]:
        max_char = ch[-1]

final.sort()
for j in final:
    print(j[0], j[1], j[2], j[3])

print('')
print('last char is 0x%x (decimal %d)' % (max_char, max_char))

print('')
print('filename list ======================================================')
print('')

names_only.sort()
for j in names_only:
    sys.stdout.write(j + ' ')

print('')
print('cooledit initialization ======================================================')
print('')

initapp_list = []
for i in priority_order:
    if os.stat(i).st_size < 1:
        print('empty ', i)
        sys.exit(1)
    initapp_list.append(i)
for i in names_only:
    if os.stat(i).st_size < 1:
        print('empty ', i)
        sys.exit(1)
    if i in priority_order:
        continue
    if i in manual_font_placement:
        continue
    initapp_list.append(i)
for i in manual_font_placement:
    initapp_list.append(i)

print('')
sys.stdout.write("L='")
for i in initapp_list:
    sys.stdout.write(' ' + i)
sys.stdout.write("'")
print('for i in $L ; do cp -L $i ../cooledit/notosans/ ; done')
print('')

for i in initapp_list[:-1]:
    print("        \"%s,\" \\" % i)
print("        \"%s\"" % initapp_list[-1])


print('')
print('total bytes ======================================================')
print('')

print(total_bytes)

sys.stderr.write('\n')
        
sys.exit(0)

