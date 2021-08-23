
import os, re, sys

f = os.popen('ls -1 *.otf *.ttf').read().split('\n')

import re

langs = {}

special = [ 
'NotoColorEmoji.ttf',
'NotoEmoji-Regular.ttf',
'NotoKufiArabic-Regular.ttf',
'NotoNaskhArabicUI-Regular.ttf',
'NotoNastaliqUrdu-Regular.ttf',
]


for l in f:
    if not l:
        continue
    p = re.match('(Noto)(SansMono|SerifDisplay|SansDisplay|Serif|Sans|Display)([A-Za-z0-9]*)(|[-]([A-Za-z0-9]*))[.](.*)', l)
    if not p:
        if l in special:
            pass
        else:
            print('not handled: ' + l)
        continue

    dummy = p.group(1)
    style = p.group(2)
    lang = p.group(3)
    font = p.group(5)
    format = p.group(6)

    ui = ''
    if lang.endswith('UI'):
        ui = 'UI'
        lang = lang[:-2]

    if lang in ('SerifDisplay', 'SansDisplay', 'Serif', 'Sans', 'Display', 'Regular', 'SansRegular'):
        print('huh?: ' + l)
        continue

    if lang in ('', 'Arabic'):
        continue

    if not langs.has_key(lang):
        langs[lang] = {}
    langs[lang][(style, font, format, ui)] = True

total_bytes = 0
all_files = []

for lang in langs.keys():
    found = False
    for ui in ('UI', ''):
        for format in ('ttf', 'otf'):
            for style in ('Mono', 'SansMono', 'Sans', 'Serif'):
                for font in ('Mono', 'Regular', 'Medium'):
                    if langs[lang].has_key((style, font, format, ui)):
                        found = (style, font, format, ui)
                        break
                if found:
                    break
            if found:
                break
        if found:
            break
    if not found:
        print('huh2?:' + lang + ' - ' + str(langs[lang]))
    fname = 'Noto' + style + lang + ui + '-' + font + '.' + format
    all_files.append(fname)

for i in special:
    all_files.append(i)

for fname in all_files:
    st = None
    try:
        st = os.stat(fname)
    except:
        print('huh3?:' + fname)
    if st:
        total_bytes = total_bytes + st.st_size
    print(fname)

print(total_bytes)


