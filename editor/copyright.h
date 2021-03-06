

static char *latin1_help[][5] = {
{"     ", "Ctrl+   ", "2nd   ", "Hex ", "                                               "},
{"     ", "        ", "      ", "    ", "                                               "},
{"     ", " Space  ", " None ", " A0 ", " NO-BREAK SPACE                                "},
{"  ¡  ", "   ^    ", "  !   ", " A1 ", " INVERTED EXCLAMATION MARK                     "},
{"  ¢  ", "   c    ", "  /   ", " A2 ", " CENT SIGN                                     "},
{"  £  ", "   l    ", "  -   ", " A3 ", " POUND SIGN                                    "},
{"  ¤  ", "   x    ", "  o   ", " A4 ", " CURRENCY SIGN                                 "},
{"  ¥  ", "   y    ", "  =   ", " A5 ", " YEN SIGN                                      "},
{"  ¦  ", "   |    ", "  |   ", " A6 ", " BROKEN BAR                                    "},
{"  §  ", "   O    ", "  S   ", " A7 ", " SECTION SIGN                                  "},
{"  §  ", "   o    ", "  S   ", " A7 ", " SECTION SIGN                                  "},
{"  ¨  ", "   '    ", "  '   ", " A8 ", " DIAERESIS                                     "},
{"  ©  ", "   O    ", "  c   ", " A9 ", " COPYRIGHT SIGN                                "},
{"  ©  ", "   o    ", "  c   ", " A9 ", " COPYRIGHT SIGN                                "},
{"  ª  ", "   a    ", "  _   ", " AA ", " FEMININE ORDINAL INDICATOR                    "},
{"  «  ", "   <    ", "  <   ", " AB ", " LEFT-POINTING DOUBLE ANGLE QUOTATION MARK     "},
{"  ¬  ", "   !    ", "  !   ", " AC ", " NOT SIGN                                      "},
{"  ­  ", "   -    ", "  -   ", " AD ", " SOFT HYPHEN                                   "},
{"  ®  ", "   O    ", "  r   ", " AE ", " REGISTERED SIGN                               "},
{"  ®  ", "   o    ", "  r   ", " AE ", " REGISTERED SIGN                               "},
{"  ¯  ", "   ^    ", "  -   ", " AF ", " MACRON                                        "},
{"  °  ", "   ^    ", "  0   ", " B0 ", " DEGREE SIGN                                   "},
{"  ±  ", "   +    ", "  -   ", " B1 ", " PLUS-MINUS SIGN                               "},
{"  ²  ", "   ^    ", "  2   ", " B2 ", " SUPERSCRIPT TWO                               "},
{"  ³  ", "   ^    ", "  3   ", " B3 ", " SUPERSCRIPT THREE                             "},
{"  ´  ", "   ^    ", "  '   ", " B4 ", " ACUTE ACCENT                                  "},
{"  µ  ", "   u    ", "  |   ", " B5 ", " MICRO SIGN                                    "},
{"  ¶  ", "   Q    ", "  |   ", " B6 ", " PILCROW SIGN                                  "},
{"  ¶  ", "   q    ", "  |   ", " B6 ", " PILCROW SIGN                                  "},
{"  ·  ", "   ^    ", "  .   ", " B7 ", " MIDDLE DOT                                    "},
{"  ¸  ", "   _    ", "  ,   ", " B8 ", " CEDILLA                                       "},
{"  ¹  ", "   ^    ", "  1   ", " B9 ", " SUPERSCRIPT ONE                               "},
{"  º  ", "   o    ", "  _   ", " BA ", " MASCULINE ORDINAL INDICATOR                   "},
{"  »  ", "   >    ", "  >   ", " BB ", " RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK    "},
{"  ¼  ", "   1    ", "  4   ", " BC ", " VULGAR FRACTION ONE QUARTER                   "},
{"  ½  ", "   1    ", "  2   ", " BD ", " VULGAR FRACTION ONE HALF                      "},
{"  ¾  ", "   3    ", "  4   ", " BE ", " VULGAR FRACTION THREE QUARTERS                "},
{"  ¿  ", "   ^    ", "  ?   ", " BF ", " INVERTED QUESTION MARK                        "},
{"  À  ", "   A    ", "  `   ", " C0 ", " LATIN CAPITAL LETTER A WITH GRAVE ACCENT      "},
{"  Á  ", "   A    ", "  '   ", " C1 ", " LATIN CAPITAL LETTER A WITH ACUTE ACCENT      "},
{"  Â  ", "   A    ", "  ^   ", " C2 ", " LATIN CAPITAL LETTER A WITH CIRCUMFLEX ACCENT "},
{"  Ã  ", "   A    ", "  ~   ", " C3 ", " LATIN CAPITAL LETTER A WITH TILDE             "},
{"  Ä  ", "   A    ", "  \"  ", " C4 ", " LATIN CAPITAL LETTER A WITH DIAERESIS         "},
{"  Å  ", "   A    ", "  o   ", " C5 ", " LATIN CAPITAL LETTER A WITH RING ABOVE        "},
{"  Æ  ", "   A    ", "  E   ", " C6 ", " LATIN CAPITAL LIGATURE AE                     "},
{"  Ç  ", "   C    ", "  ,   ", " C7 ", " LATIN CAPITAL LETTER C WITH CEDILLA           "},
{"  È  ", "   E    ", "  `   ", " C8 ", " LATIN CAPITAL LETTER E WITH GRAVE ACCENT      "},
{"  É  ", "   E    ", "  '   ", " C9 ", " LATIN CAPITAL LETTER E WITH ACUTE ACCENT      "},
{"  Ê  ", "   E    ", "  ^   ", " CA ", " LATIN CAPITAL LETTER E WITH CIRCUMFLEX ACCENT "},
{"  Ë  ", "   E    ", "  \"  ", " CB ", " LATIN CAPITAL LETTER E WITH DIAERESIS         "},
{"  Ì  ", "   I    ", "  `   ", " CC ", " LATIN CAPITAL LETTER I WITH GRAVE ACCENT      "},
{"  Í  ", "   I    ", "  '   ", " CD ", " LATIN CAPITAL LETTER I WITH ACUTE ACCENT      "},
{"  Î  ", "   I    ", "  ^   ", " CE ", " LATIN CAPITAL LETTER I WITH CIRCUMFLEX ACCENT "},
{"  Ï  ", "   I    ", "  \"  ", " CF ", " LATIN CAPITAL LETTER I WITH DIAERESIS         "},
{"  Ð  ", "   D    ", "  -   ", " D0 ", " LATIN CAPITAL LETTER ETH                      "},
{"  Ñ  ", "   N    ", "  ~   ", " D1 ", " LATIN CAPITAL LETTER N WITH TILDE             "},
{"  Ò  ", "   O    ", "  `   ", " D2 ", " LATIN CAPITAL LETTER O WITH GRAVE ACCENT      "},
{"  Ó  ", "   O    ", "  '   ", " D3 ", " LATIN CAPITAL LETTER O WITH ACUTE ACCENT      "},
{"  Ô  ", "   O    ", "  ^   ", " D4 ", " LATIN CAPITAL LETTER O WITH CIRCUMFLEX ACCENT "},
{"  Õ  ", "   O    ", "  ~   ", " D5 ", " LATIN CAPITAL LETTER O WITH TILDE             "},
{"  Ö  ", "   O    ", "  \"  ", " D6 ", " LATIN CAPITAL LETTER O WITH DIAERESIS         "},
{"  ×  ", "   X    ", " None ", " D7 ", " MULTIPLICATION SIGN                           "},
{"  Ø  ", "   O    ", "  /   ", " D8 ", " LATIN CAPITAL LETTER O WITH STROKE            "},
{"  Ù  ", "   U    ", "  `   ", " D9 ", " LATIN CAPITAL LETTER U WITH GRAVE ACCENT      "},
{"  Ú  ", "   U    ", "  '   ", " DA ", " LATIN CAPITAL LETTER U WITH ACUTE ACCENT      "},
{"  Û  ", "   U    ", "  ^   ", " DB ", " LATIN CAPITAL LETTER U WITH CIRCUMFLEX ACCENT "},
{"  Ü  ", "   U    ", "  \"  ", " DC ", " LATIN CAPITAL LETTER U WITH DIAERESIS         "},
{"  Ý  ", "   Y    ", "  '   ", " DD ", " LATIN CAPITAL LETTER Y WITH ACUTE ACCENT      "},
{"  Þ  ", "   P    ", "  |   ", " DE ", " LATIN CAPITAL LETTER THORN                    "},
{"  ß  ", "   s    ", " None ", " DF ", " LATIN SMALL LETTER SHARP S                    "},
{"  à  ", "   a    ", "  `   ", " E0 ", " LATIN SMALL LETTER A WITH GRAVE ACCENT        "},
{"  á  ", "   a    ", "  '   ", " E1 ", " LATIN SMALL LETTER A WITH ACUTE ACCENT        "},
{"  â  ", "   a    ", "  ^   ", " E2 ", " LATIN SMALL LETTER A WITH CIRCUMFLEX ACCENT   "},
{"  ã  ", "   a    ", "  ~   ", " E3 ", " LATIN SMALL LETTER A WITH TILDE               "},
{"  ä  ", "   a    ", "  \"  ", " E4 ", " LATIN SMALL LETTER A WITH DIAERESIS           "},
{"  å  ", "   a    ", "  o   ", " E5 ", " LATIN SMALL LETTER A WITH RING ABOVE          "},
{"  æ  ", "   a    ", "  e   ", " E6 ", " LATIN SMALL LIGATURE AE                       "},
{"  ç  ", "   c    ", "  ,   ", " E7 ", " LATIN SMALL LETTER C WITH CEDILLA             "},
{"  è  ", "   e    ", "  `   ", " E8 ", " LATIN SMALL LETTER E WITH GRAVE ACCENT        "},
{"  é  ", "   e    ", "  '   ", " E9 ", " LATIN SMALL LETTER E WITH ACUTE ACCENT        "},
{"  ê  ", "   e    ", "  ^   ", " EA ", " LATIN SMALL LETTER E WITH CIRCUMFLEX ACCENT   "},
{"  ë  ", "   e    ", "  \"  ", " EB ", " LATIN SMALL LETTER E WITH DIAERESIS           "},
{"  ì  ", "   i    ", "  `   ", " EC ", " LATIN SMALL LETTER I WITH GRAVE ACCENT        "},
{"  í  ", "   i    ", "  '   ", " ED ", " LATIN SMALL LETTER I WITH ACUTE ACCENT        "},
{"  î  ", "   i    ", "  ^   ", " EE ", " LATIN SMALL LETTER I WITH CIRCUMFLEX ACCENT   "},
{"  ï  ", "   i    ", "  \"  ", " EF ", " LATIN SMALL LETTER I WITH DIAERESIS           "},
{"  ð  ", "   d    ", "  -   ", " F0 ", " LATIN SMALL LETTER ETH                        "},
{"  ñ  ", "   n    ", "  ~   ", " F1 ", " LATIN SMALL LETTER N WITH TILDE               "},
{"  ò  ", "   o    ", "  `   ", " F2 ", " LATIN SMALL LETTER O WITH GRAVE ACCENT        "},
{"  ó  ", "   o    ", "  '   ", " F3 ", " LATIN SMALL LETTER O WITH ACUTE ACCENT        "},
{"  ô  ", "   o    ", "  ^   ", " F4 ", " LATIN SMALL LETTER O WITH CIRCUMFLEX ACCENT   "},
{"  õ  ", "   o    ", "  ~   ", " F5 ", " LATIN SMALL LETTER O WITH TILDE               "},
{"  ö  ", "   o    ", "  \"  ", " F6 ", " LATIN SMALL LETTER O WITH DIAERESIS           "},
{"  ÷  ", "   :    ", "  -   ", " F7 ", " DIVISION SIGN                                 "},
{"  ø  ", "   o    ", "  /   ", " F8 ", " LATIN SMALL LETTER O WITH OBLIQUE BAR         "},
{"  ù  ", "   u    ", "  `   ", " F9 ", " LATIN SMALL LETTER U WITH GRAVE ACCENT        "},
{"  ú  ", "   u    ", "  '   ", " FA ", " LATIN SMALL LETTER U WITH ACUTE ACCENT        "},
{"  û  ", "   u    ", "  ^   ", " FB ", " LATIN SMALL LETTER U WITH CIRCUMFLEX ACCENT   "},
{"  ü  ", "   u    ", "  \"  ", " FC ", " LATIN SMALL LETTER U WITH DIAERESIS           "},
{"  ý  ", "   y    ", "  '   ", " FD ", " LATIN SMALL LETTER Y WITH ACUTE ACCENT        "},
{"  þ  ", "   p    ", "  |   ", " FE ", " LATIN SMALL LETTER THORN                      "},
{"  ÿ  ", "   y    ", "  \"  ", " FF ", " LATIN SMALL LETTER Y WITH DIAERESIS           "},
};


