

# This script finds a minimal useful set of NotoSans fonts to cover the maximum number of characters


import os, re, sys, os
from PIL import PcfFontFile


# FreeBSD packages:
#   noto-2.0 (which implies: noto-tc noto-sc noto-kr noto-jp noto-hk noto-extra noto-emoji noto-basic)
#   font-misc-misc

# These two are missing from FreeBSD:
#   NotoSansMath-Regular.ttf
#   NotoMusic-Regular.ttf



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
    if s.find('Black') >= 0:
        r -= 1
    if s.find('Display') >= 0:
        r -= 2
    if s.find('Thin') >= 0:
        r -= 3
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
fnoto = os.popen('ls -1 NotoSans-Regular.ttf NotoSansSymbols-Regular.ttf NotoSansSymbols2-Regular.ttf NotoSansMath-Regular.ttf').read().split('\n')
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
    if i in ('NotoSansSyriacWestern-Black.ttf', 'NotoSansSyriacWestern-Thin.ttf', 'NotoSansSyriacWestern-Regular.ttf', 'NotoSansSyriac-Black.ttf', 'NotoSansSyriacEastern-Black.ttf', 'NotoSansSyriacEastern-Regular.ttf', 'NotoSansSyriacEastern-Thin.ttf', 'NotoSansSyriac-Thin.ttf'):
        continue
    if i in ('NotoSansTifinaghAdrar-Regular.ttf', 'NotoSansTifinaghAgrawImazighen-Regular.ttf', 'NotoSansTifinaghAhaggar-Regular.ttf', 'NotoSansTifinaghAir-Regular.ttf', 'NotoSansTifinaghAPT-Regular.ttf', 'NotoSansTifinaghAzawagh-Regular.ttf', 'NotoSansTifinaghGhat-Regular.ttf', 'NotoSansTifinaghHawad-Regular.ttf', 'NotoSansTifinaghRhissaIxa-Regular.ttf', 'NotoSansTifinaghSIL-Regular.ttf', 'NotoSansTifinaghTawellemmet-Regular.ttf'):
        continue
    if i in ('NotoSansSinhala-BlackCondensed.ttf', 'NotoSansSinhala-Black.ttf', 'NotoSansSinhala-Bold.ttf', 'NotoSansSinhala-CondensedBlack.ttf', 'NotoSansSinhala-CondensedBold.ttf', 'NotoSansSinhala-CondensedExtraBold.ttf', 'NotoSansSinhala-CondensedExtraLight.ttf', 'NotoSansSinhala-CondensedLight.ttf', 'NotoSansSinhala-CondensedMedium.ttf', 'NotoSansSinhala-CondensedSemiBold.ttf', 'NotoSansSinhala-CondensedThin.ttf', 'NotoSansSinhala-Condensed.ttf', 'NotoSansSinhala-ExtraBold.ttf', 'NotoSansSinhala-ExtraCondensedBlack.ttf', 'NotoSansSinhala-ExtraCondensedBold.ttf', 'NotoSansSinhala-ExtraCondensedExtraBold.ttf', 'NotoSansSinhala-ExtraCondensedExtraLight.ttf', 'NotoSansSinhala-ExtraCondensedLight.ttf', 'NotoSansSinhala-ExtraCondensedMedium.ttf', 'NotoSansSinhala-ExtraCondensedSemiBold.ttf', 'NotoSansSinhala-ExtraCondensedThin.ttf', 'NotoSansSinhala-ExtraCondensed.ttf', 'NotoSansSinhala-ExtraLight.ttf', 'NotoSansSinhala-Light.ttf', 'NotoSansSinhala-Medium.ttf', 'NotoSansSinhala-Regular.ttf', 'NotoSansSinhala-SemiBold.ttf', 'NotoSansSinhala-SemiCondensedBlack.ttf', 'NotoSansSinhala-SemiCondensedBold.ttf', 'NotoSansSinhala-SemiCondensedExtraBold.ttf', 'NotoSansSinhala-SemiCondensedExtraLight.ttf', 'NotoSansSinhala-SemiCondensedLight.ttf', 'NotoSansSinhala-SemiCondensedMedium.ttf', 'NotoSansSinhala-SemiCondensedSemiBold.ttf', 'NotoSansSinhala-SemiCondensedThin.ttf', 'NotoSansSinhala-SemiCondensed.ttf', 'NotoSansSinhala-ThinCondensed.ttf', 'NotoSansSinhala-Thin.ttf', 'NotoSansSinhalaUI-Black.ttf', 'NotoSansSinhalaUI-Bold.ttf', 'NotoSansSinhalaUI-CondensedBlack.ttf', 'NotoSansSinhalaUI-CondensedBold.ttf', 'NotoSansSinhalaUI-CondensedExtraBold.ttf', 'NotoSansSinhalaUI-CondensedExtraLight.ttf', 'NotoSansSinhalaUI-CondensedLight.ttf', 'NotoSansSinhalaUI-CondensedMedium.ttf', 'NotoSansSinhalaUI-CondensedSemiBold.ttf', 'NotoSansSinhalaUI-CondensedThin.ttf', 'NotoSansSinhalaUI-Condensed.ttf', 'NotoSansSinhalaUI-ExtraBold.ttf', 'NotoSansSinhalaUI-ExtraCondensedBlack.ttf', 'NotoSansSinhalaUI-ExtraCondensedBold.ttf', 'NotoSansSinhalaUI-ExtraCondensedExtraBold.ttf', 'NotoSansSinhalaUI-ExtraCondensedExtraLight.ttf', 'NotoSansSinhalaUI-ExtraCondensedLight.ttf', 'NotoSansSinhalaUI-ExtraCondensedMedium.ttf', 'NotoSansSinhalaUI-ExtraCondensedSemiBold.ttf', 'NotoSansSinhalaUI-ExtraCondensedThin.ttf', 'NotoSansSinhalaUI-ExtraCondensed.ttf', 'NotoSansSinhalaUI-ExtraLight.ttf', 'NotoSansSinhalaUI-Light.ttf', 'NotoSansSinhalaUI-Medium.ttf', 'NotoSansSinhalaUI-SemiBold.ttf', 'NotoSansSinhalaUI-SemiCondensedBlack.ttf', 'NotoSansSinhalaUI-SemiCondensedBold.ttf', 'NotoSansSinhalaUI-SemiCondensedExtraBold.ttf', 'NotoSansSinhalaUI-SemiCondensedExtraLight.ttf', 'NotoSansSinhalaUI-SemiCondensedLight.ttf', 'NotoSansSinhalaUI-SemiCondensedMedium.ttf', 'NotoSansSinhalaUI-SemiCondensedSemiBold.ttf', 'NotoSansSinhalaUI-SemiCondensedThin.ttf', 'NotoSansSinhalaUI-SemiCondensed.ttf', 'NotoSansSinhalaUI-Thin.ttf'):
        continue
    if i in ('NotoFangsongKSSRotated-Regular.ttf', 'NotoSerifKhitanSmallScript-Regular.ttf'):
        continue
    if i in ('NotoKufiArabic-Regular.ttf', 'NotoNaskhArabic-Bold.ttf', 'NotoNaskhArabic-Medium.ttf', 'NotoNaskhArabic-Regular.ttf', 'NotoNaskhArabic-SemiBold.ttf', 'NotoNaskhArabic-Bold.ttf', 'NotoNaskhArabic-Regular.ttf', 'NotoKufiArabic-Black.ttf', 'NotoKufiArabic-Bold.ttf', 'NotoKufiArabic-ExtraBold.ttf', 'NotoKufiArabic-ExtraLight.ttf', 'NotoKufiArabic-Light.ttf', 'NotoKufiArabic-Medium.ttf', 'NotoKufiArabic-SemiBold.ttf', 'NotoKufiArabic-Thin.ttf', 'NotoSansArabic-Black.ttf', 'NotoSansArabic-Bold.ttf', 'NotoSansArabic-CondensedBlack.ttf', 'NotoSansArabic-CondensedBold.ttf', 'NotoSansArabic-CondensedExtraBold.ttf', 'NotoSansArabic-CondensedExtraLight.ttf', 'NotoSansArabic-CondensedLight.ttf', 'NotoSansArabic-CondensedMedium.ttf', 'NotoSansArabic-CondensedSemiBold.ttf', 'NotoSansArabic-CondensedThin.ttf', 'NotoSansArabic-Condensed.ttf', 'NotoSansArabic-ExtraBold.ttf', 'NotoSansArabic-ExtraCondensedBlack.ttf', 'NotoSansArabic-ExtraCondensedBold.ttf', 'NotoSansArabic-ExtraCondensedExtraBold.ttf', 'NotoSansArabic-ExtraCondensedExtraLight.ttf', 'NotoSansArabic-ExtraCondensedLight.ttf', 'NotoSansArabic-ExtraCondensedMedium.ttf', 'NotoSansArabic-ExtraCondensedSemiBold.ttf', 'NotoSansArabic-ExtraCondensedThin.ttf', 'NotoSansArabic-ExtraCondensed.ttf', 'NotoSansArabic-ExtraLight.ttf', 'NotoSansArabic-Light.ttf', 'NotoSansArabic-Medium.ttf', 'NotoSansArabic-SemiBold.ttf', 'NotoSansArabic-SemiCondensedBlack.ttf', 'NotoSansArabic-SemiCondensedBold.ttf', 'NotoSansArabic-SemiCondensedExtraBold.ttf', 'NotoSansArabic-SemiCondensedExtraLight.ttf', 'NotoSansArabic-SemiCondensedLight.ttf', 'NotoSansArabic-SemiCondensedMedium.ttf', 'NotoSansArabic-SemiCondensedSemiBold.ttf', 'NotoSansArabic-SemiCondensedThin.ttf', 'NotoSansArabic-SemiCondensed.ttf', 'NotoSansArabic-Thin.ttf'):
        continue
    if i in ('NotoSansLao-Black.ttf', 'NotoSansLao-Bold.ttf', 'NotoSansLao-CondensedBlack.ttf', 'NotoSansLao-CondensedBold.ttf', 'NotoSansLao-CondensedExtraBold.ttf', 'NotoSansLao-CondensedExtraLight.ttf', 'NotoSansLao-CondensedLight.ttf', 'NotoSansLao-CondensedMedium.ttf', 'NotoSansLao-CondensedSemiBold.ttf', 'NotoSansLao-CondensedThin.ttf', 'NotoSansLao-Condensed.ttf', 'NotoSansLao-ExtraBold.ttf', 'NotoSansLao-ExtraCondensedBlack.ttf', 'NotoSansLao-ExtraCondensedBold.ttf', 'NotoSansLao-ExtraCondensedExtraBold.ttf', 'NotoSansLao-ExtraCondensedExtraLight.ttf', 'NotoSansLao-ExtraCondensedLight.ttf', 'NotoSansLao-ExtraCondensedMedium.ttf', 'NotoSansLao-ExtraCondensedSemiBold.ttf', 'NotoSansLao-ExtraCondensedThin.ttf', 'NotoSansLao-ExtraCondensed.ttf', 'NotoSansLao-ExtraLight.ttf', 'NotoSansLao-Light.ttf', 'NotoSansLaoLooped-Black.ttf', 'NotoSansLaoLooped-Bold.ttf', 'NotoSansLaoLooped-CondensedBlack.ttf', 'NotoSansLaoLooped-CondensedBold.ttf', 'NotoSansLaoLooped-CondensedExtraBold.ttf', 'NotoSansLaoLooped-CondensedExtraLight.ttf', 'NotoSansLaoLooped-CondensedLight.ttf', 'NotoSansLaoLooped-CondensedMedium.ttf', 'NotoSansLaoLooped-CondensedSemiBold.ttf', 'NotoSansLaoLooped-CondensedThin.ttf', 'NotoSansLaoLooped-Condensed.ttf', 'NotoSansLaoLooped-ExtraBold.ttf', 'NotoSansLaoLooped-ExtraCondensedBlack.ttf', 'NotoSansLaoLooped-ExtraCondensedBold.ttf', 'NotoSansLaoLooped-ExtraCondensedExtraBold.ttf', 'NotoSansLaoLooped-ExtraCondensedExtraLight.ttf', 'NotoSansLaoLooped-ExtraCondensedLight.ttf', 'NotoSansLaoLooped-ExtraCondensedMedium.ttf', 'NotoSansLaoLooped-ExtraCondensedSemiBold.ttf', 'NotoSansLaoLooped-ExtraCondensedThin.ttf', 'NotoSansLaoLooped-ExtraCondensed.ttf', 'NotoSansLaoLooped-ExtraLight.ttf', 'NotoSansLaoLooped-Light.ttf', 'NotoSansLaoLooped-Medium.ttf', 'NotoSansLaoLooped-Regular.ttf', 'NotoSansLaoLooped-SemiBold.ttf', 'NotoSansLaoLooped-SemiCondensedBlack.ttf', 'NotoSansLaoLooped-SemiCondensedBold.ttf', 'NotoSansLaoLooped-SemiCondensedExtraBold.ttf', 'NotoSansLaoLooped-SemiCondensedExtraLight.ttf', 'NotoSansLaoLooped-SemiCondensedLight.ttf', 'NotoSansLaoLooped-SemiCondensedMedium.ttf', 'NotoSansLaoLooped-SemiCondensedSemiBold.ttf', 'NotoSansLaoLooped-SemiCondensedThin.ttf', 'NotoSansLaoLooped-SemiCondensed.ttf', 'NotoSansLaoLooped-Thin.ttf', 'NotoSansLao-Medium.ttf', 'NotoSansLao-SemiBold.ttf', 'NotoSansLao-SemiCondensedBlack.ttf', 'NotoSansLao-SemiCondensedBold.ttf', 'NotoSansLao-SemiCondensedExtraBold.ttf', 'NotoSansLao-SemiCondensedExtraLight.ttf', 'NotoSansLao-SemiCondensedLight.ttf', 'NotoSansLao-SemiCondensedMedium.ttf', 'NotoSansLao-SemiCondensedSemiBold.ttf', 'NotoSansLao-SemiCondensedThin.ttf', 'NotoSansLao-SemiCondensed.ttf', 'NotoSansLao-Thin.ttf', 'NotoSerifLao-Black.ttf', 'NotoSerifLao-Bold.ttf', 'NotoSerifLao-CondensedBlack.ttf', 'NotoSerifLao-CondensedBold.ttf', 'NotoSerifLao-CondensedExtraBold.ttf', 'NotoSerifLao-CondensedExtraLight.ttf', 'NotoSerifLao-CondensedLight.ttf', 'NotoSerifLao-CondensedMedium.ttf', 'NotoSerifLao-CondensedSemiBold.ttf', 'NotoSerifLao-CondensedThin.ttf', 'NotoSerifLao-Condensed.ttf', 'NotoSerifLao-ExtraBold.ttf', 'NotoSerifLao-ExtraCondensedBlack.ttf', 'NotoSerifLao-ExtraCondensedBold.ttf', 'NotoSerifLao-ExtraCondensedExtraBold.ttf', 'NotoSerifLao-ExtraCondensedExtraLight.ttf', 'NotoSerifLao-ExtraCondensedLight.ttf', 'NotoSerifLao-ExtraCondensedMedium.ttf', 'NotoSerifLao-ExtraCondensedSemiBold.ttf', 'NotoSerifLao-ExtraCondensedThin.ttf', 'NotoSerifLao-ExtraCondensed.ttf', 'NotoSerifLao-ExtraLight.ttf', 'NotoSerifLao-Light.ttf', 'NotoSerifLao-Medium.ttf', 'NotoSerifLao-Regular.ttf', 'NotoSerifLao-SemiBold.ttf', 'NotoSerifLao-SemiCondensedBlack.ttf', 'NotoSerifLao-SemiCondensedBold.ttf', 'NotoSerifLao-SemiCondensedExtraBold.ttf', 'NotoSerifLao-SemiCondensedExtraLight.ttf', 'NotoSerifLao-SemiCondensedLight.ttf', 'NotoSerifLao-SemiCondensedMedium.ttf', 'NotoSerifLao-SemiCondensedSemiBold.ttf', 'NotoSerifLao-SemiCondensedThin.ttf', 'NotoSerifLao-SemiCondensed.ttf', 'NotoSerifLao-Thin.ttf'):
        continue
    if i in ('NotoSansThaiLooped-Black.ttf', 'NotoSansThaiLooped-Bold.ttf', 'NotoSansThaiLooped-CondensedBlack.ttf', 'NotoSansThaiLooped-CondensedBold.ttf', 'NotoSansThaiLooped-CondensedExtraBold.ttf', 'NotoSansThaiLooped-CondensedExtraLight.ttf', 'NotoSansThaiLooped-CondensedLight.ttf', 'NotoSansThaiLooped-CondensedMedium.ttf', 'NotoSansThaiLooped-CondensedSemiBold.ttf', 'NotoSansThaiLooped-CondensedThin.ttf', 'NotoSansThaiLooped-Condensed.ttf', 'NotoSansThaiLooped-ExtraBold.ttf', 'NotoSansThaiLooped-ExtraCondensedBlack.ttf', 'NotoSansThaiLooped-ExtraCondensedBold.ttf', 'NotoSansThaiLooped-ExtraCondensedExtraBold.ttf', 'NotoSansThaiLooped-ExtraCondensedExtraLight.ttf', 'NotoSansThaiLooped-ExtraCondensedLight.ttf', 'NotoSansThaiLooped-ExtraCondensedMedium.ttf', 'NotoSansThaiLooped-ExtraCondensedSemiBold.ttf', 'NotoSansThaiLooped-ExtraCondensedThin.ttf', 'NotoSansThaiLooped-ExtraCondensed.ttf', 'NotoSansThaiLooped-ExtraLight.ttf', 'NotoSansThaiLooped-Light.ttf', 'NotoSansThaiLooped-Medium.ttf', 'NotoSansThaiLooped-Regular.ttf', 'NotoSansThaiLooped-SemiBold.ttf', 'NotoSansThaiLooped-SemiCondensedBlack.ttf', 'NotoSansThaiLooped-SemiCondensedBold.ttf', 'NotoSansThaiLooped-SemiCondensedExtraBold.ttf', 'NotoSansThaiLooped-SemiCondensedExtraLight.ttf', 'NotoSansThaiLooped-SemiCondensedLight.ttf', 'NotoSansThaiLooped-SemiCondensedMedium.ttf', 'NotoSansThaiLooped-SemiCondensedSemiBold.ttf', 'NotoSansThaiLooped-SemiCondensedThin.ttf', 'NotoSansThaiLooped-SemiCondensed.ttf', 'NotoSansThaiLooped-Thin.ttf'):
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
    print('semething is broken(%d). exitting' % broken)
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
copy_script = open('copy.sh', 'w')
copy_script.write("L='")
for i in initapp_list:
    copy_script.write(' ' + i)
copy_script.write("'")

copy_script.write('\n\n')
copy_script.write('for i in $L ; do cp -L $i ../cooledit/notosans/ ; done')
copy_script.write('\n\n')

for i in initapp_list[:-1]:
    print("        \"%s,\" \\" % i)
print("        \"%s\"" % initapp_list[-1])


print('')
print('total bytes ======================================================')
print('')

print(total_bytes)

sys.stderr.write('\nSuccess\n')
        
sys.exit(0)

