# -*- coding: utf-8 -*-


"""Generate C++ code for alphabets' constants.

Note: in some operations the python-lowercase isused instead of
arcadia-lowercase.

"""

import sys

sys.dont_write_bytecode = True

import argparse
import sys
import os
import itertools

from abc_code import (TRawCode, WCharStaticArrayCode, TTrClassCode, TAlphaClassCode,
                      TDiacriticsMapClassCode, TLanguageCode, TRegisterCode,
                      compose, reverse_mapping, merge,
                      generate_code, get_symbols_range)

# common for all languages

APOSTROPHES = [u"'", u"\u2019"]
HYPHENS = [u'-']
STRESS = [u'\u0301']
STRESS_BULGARIAN = [u'\u0300']
CIRCUMFLEX = [u'\u0302']

APOSTROPHE_CONVERT_MAP = {
    u'\u2019': u'\u0027',  # ’ -> '
}

CYRILLIC_RUSSIAN_VOWELS = [
    # uppercase
    u'\u0410', u'\u0415', u'\u0418', u'\u041E', u'\u0423', u'\u042B',
    u'\u042D', u'\u042E', u'\u042F', u'\u0401',
    # lowercase
    u'\u0430', u'\u0435', u'\u0438', u'\u043E', u'\u0443', u'\u044B',
    u'\u044D', u'\u044E', u'\u044F', u'\u0451',
]
CYRILLIC_RUSSIAN_CONSONANT = [
    # uppercase
    u'\u0411', u'\u0412', u'\u0413', u'\u0414', u'\u0416', u'\u0417',
    u'\u0419', u'\u041A', u'\u041B', u'\u041C', u'\u041D', u'\u041F',
    u'\u0420', u'\u0421', u'\u0422', u'\u0424', u'\u0425', u'\u0426',
    u'\u0427', u'\u0428', u'\u0429', u'\u042A', u'\u042C',
    # lowercase
    u'\u0431', u'\u0432', u'\u0433', u'\u0434', u'\u0436', u'\u0437',
    u'\u0439', u'\u043A', u'\u043B', u'\u043C', u'\u043D', u'\u043F',
    u'\u0440', u'\u0441', u'\u0442', u'\u0444', u'\u0445', u'\u0446',
    u'\u0447', u'\u0448', u'\u0449', u'\u044A', u'\u044C',
]
CYRILLIC_RENYXA = [
    u'A', u'B', u'C', u'E', u'H',       u'I', u'K',      u'M', u'O', u'P',
    u'a',       u'c', u'e',       u'g', u'i', u'k', u'n',      u'o', u'p',
          u'T', u'X', u'Y',
    u'u',       u'x', u'y',
]

LATINS_VOWELS = [u'A', u'E', u'I', u'O', u'U', u'a', u'e', u'i', u'o', u'u']
LATINS_CONSONANT = [
    u'B', u'C', u'D', u'F', u'G', u'H', u'J', u'K', u'L', u'M',
    u'N', u'P', u'Q', u'R', u'S', u'T', u'V', u'W', u'X', u'Y', u'Z',
    u'b', u'c', u'd', u'f', u'g', u'h', u'j', u'k', u'l', u'm',
    u'n', u'p', u'q', u'r', u's', u't', u'v', u'w', u'x', u'y', u'z',
]

MIN_UNICODE_SYMBOL_CODE = 0x0021
MAX_UNICODE_SYMBOL_CODE = 0xffff

UNICODE_LOWERCASE_MAP = dict(
    (unichr(code), unichr(code).lower()) for code in
    xrange(MIN_UNICODE_SYMBOL_CODE, MAX_UNICODE_SYMBOL_CODE)
)

DEFAULT_PRE_CONVERTER_MAP = UNICODE_LOWERCASE_MAP

EASTERN_SLAVIC_RENYXA = CYRILLIC_RENYXA + [u'\u00CB', u'\u00EB']
EASTERN_SLAVIC_RENYXA_MAP = {
    u'\u0041': u'\u0410',  # A -> А
    u'\u0042': u'\u0412',  # B -> В
    u'\u0043': u'\u0421',  # C -> С
    u'\u0045': u'\u0415',  # E -> Е
    u'\u0048': u'\u041D',  # H -> Н
    u'\u0049': u'\u0406',  # I -> І
    u'\u004B': u'\u041A',  # K -> К
    u'\u004D': u'\u041C',  # M -> М
    u'\u004F': u'\u041E',  # O -> О
    u'\u0050': u'\u0420',  # P -> Р
    u'\u0054': u'\u0422',  # T -> Т
    u'\u0058': u'\u0425',  # X -> Х
    u'\u0059': u'\u0423',  # Y -> У
    u'\u0061': u'\u0430',  # a -> а
    u'\u0062': u'\u0432',  # b -> в
    u'\u0063': u'\u0441',  # c -> с
    u'\u0065': u'\u0435',  # e -> е
    u'\u0067': u'\u0434',  # g -> д
    u'\u0068': u'\u043D',  # h -> н
    u'\u0069': u'\u0456',  # i -> і
    u'\u006B': u'\u043A',  # k -> к
    u'\u006D': u'\u043C',  # m -> м
    u'\u006E': u'\u043F',  # n -> п
    u'\u006F': u'\u043E',  # o -> о
    u'\u0070': u'\u0440',  # p -> р
    u'\u0074': u'\u0442',  # t -> т
    u'\u0075': u'\u0438',  # u -> и
    u'\u0078': u'\u0445',  # x -> х
    u'\u0079': u'\u0443',  # y -> у
    u'\u00CB': u'\u0401',  # Ë -> Ё
    u'\u00EB': u'\u0451',  # ë -> ё
    u'\u0462': u'\u0415',  # Ѣ -> Е
    u'\u0472': u'\u0424',  # Ѳ -> Ф
    u'\u0474': u'\u0418',  # Ѵ -> И
    u'\u0463': u'\u0435',  # ѣ -> е
    u'\u0473': u'\u0444',  # ѳ -> ф
    u'\u0475': u'\u0438',  # ѵ -> и
}
EASTERN_SLAVIC_PRE_CONVERT_MAP = compose(UNICODE_LOWERCASE_MAP,
                                         EASTERN_SLAVIC_RENYXA_MAP)
GENERAL_CYRILIC_CONVERTER_MAP = {
    u'\u2019': u'\u0027',  # ’ -> '
    u'\u0401': u'\u0415',  # Ё -> Е
    u'\u0451': u'\u0435',  # ё -> е
    u'\u0490': u'\u0413',  # Ґ -> Г
    u'\u0491': u'\u0433',  # ґ -> г
}

EASTERN_SLAVIC_CONVERTERS = {
    'preconverter': 'TEasternSlavicPreConverter',
    'derenyxer': 'TEasternSlavicDerenyxer',
}


# CJK (common for Chinese, Japanese, Korean)
CJK_UNIFIED_BEGIN = 0x4E00
CJK_UNIFIED_END = 0x9FFF + 1
CJK_UNFIED = get_symbols_range(CJK_UNIFIED_BEGIN, CJK_UNIFIED_END)

CJK_UNIFIED_EXT_A_BEGIN = 0x3400
CJK_UNIFIED_EXT_A_END = 0x4DBF + 1
CJK_UNIFIED_EXT_A = get_symbols_range(CJK_UNIFIED_EXT_A_BEGIN, CJK_UNIFIED_EXT_A_END)

CJK_COMPATIBILITY_BEGIN = 0xF900
CJK_COMPATIBILITY_END = 0xFAFF + 1
CJK_COMPATIBILITY = get_symbols_range(CJK_COMPATIBILITY_BEGIN, CJK_COMPATIBILITY_END)

CJK_DESCRIPTION_BEGIN = 0x2FF0
CJK_DESCRIPTION_END = 0x2FFB + 1
CJK_DESCRIPTION = get_symbols_range(CJK_DESCRIPTION_BEGIN, CJK_DESCRIPTION_END)

BOPOMOFO_BEGIN = 0x3100
BOPOMOFO_END = 0x312F + 1
BOPOMOFO = get_symbols_range(BOPOMOFO_BEGIN, BOPOMOFO_END)

HIRAGANA_KATAKANA_BEGIN = 0x3040
HIRAGANA_KATAKANA_END = 0x30FF + 1
HIRAGANA_KATAKANA = get_symbols_range(HIRAGANA_KATAKANA_BEGIN, HIRAGANA_KATAKANA_END)

HALFWIDTH_KATAKANA_BEGIN = 0xFF66
HALFWIDTH_KATAKANA_END = 0xFF9F + 1
HALFWIDTH_KATAKANA = get_symbols_range(HALFWIDTH_KATAKANA_BEGIN, HALFWIDTH_KATAKANA_END)

HANGUL_BEGIN = 0x1100
HANGUL_END = 0x11FF + 1
HANGUL = get_symbols_range(HANGUL_BEGIN, HANGUL_END)

HANGUL_EXT_A_BEGIN = 0xA960
HANGUL_EXT_A_END = 0xA97F + 1
HANGUL_EXT_A = get_symbols_range(HANGUL_EXT_A_BEGIN, HANGUL_EXT_A_END)

HANGUL_EXT_B_BEGIN = 0xD7B0
HANGUL_EXT_B_END = 0xD7FF + 1
HANGUL_EXT_B = get_symbols_range(HANGUL_EXT_B_BEGIN, HANGUL_EXT_B_END)

HANGUL_COMPATIBILITY_BEGIN = 0x3130
HANGUL_COMPATIBILITY_END = 0x318F + 1
HANGUL_COMPATIBILITY = get_symbols_range(HANGUL_COMPATIBILITY_BEGIN, HANGUL_COMPATIBILITY_END)

HANGUL_SYLLABLES_BEGIN = 0xAC00
HANGUL_SYLLABLES_END = 0xD7A3 + 1
HANGUL_SYLLABLES = get_symbols_range(HANGUL_SYLLABLES_BEGIN, HANGUL_SYLLABLES_END)

HALFWIDTH_HANGUL_BEGIN = 0xFFA0
HALFWIDTH_HANGUL_END = 0xFFDC + 1
HALFWIDTH_HANGUL = get_symbols_range(HALFWIDTH_HANGUL_BEGIN, HALFWIDTH_HANGUL_END)

CJK_APOST_AND_HYPH = APOSTROPHES + HYPHENS + [
    u'\uFF07', u'\uFF0D',  # fullwidth apostrophe and hyphen
]

CJK_CODE = [
    WCharStaticArrayCode('CJK_APOST_AND_HYPH', CJK_APOST_AND_HYPH)
]

# all languages in alphabetical order

# Arabic

ARA_HARAKAT = [
    u'\u0670',  # ARABIC LETTER SUPERSCRIPT ALEF
    u'\u064E',  # ARABIC FATHA
    u'\u064F',  # ARABIC DAMMA
    u'\u0650',  # ARABIC KASRA
    u'\u064B',  # ARABIC FATHATAN
    u'\u064C',  # ARABIC DAMMATAN
    u'\u064D',  # ARABIC KASRATAN
    u'\u0651',  # ARABIC SHADDA
    u'\u0652',  # ARABIC SUKUN
]

ARA_REQUIRED_ALPHA = [
    u'\u0621',  # ARABIC LETTER HAMZA
    u'\u0622',  # ARABIC LETTER ALEF WITH MADDA ABOVE
    u'\u0623',  # ARABIC LETTER ALEF WITH HAMZA ABOVE
    u'\u0623',  # ARABIC LETTER ALEF WITH HAMZA ABOVE
    u'\u0625',  # ARABIC LETTER ALEF WITH HAMZA BELOW
    u'\u0625',  # ARABIC LETTER ALEF WITH HAMZA BELOW
    u'\u0627',  # ARABIC LETTER ALEF
    u'\u0628',  # ARABIC LETTER BEH
    u'\u0629',  # ARABIC LETTER TEH MARBUTA
    u'\u062A',  # ARABIC LETTER TEH
    u'\u062B',  # ARABIC LETTER THEH
    u'\u062C',  # ARABIC LETTER JEEM
    u'\u062D',  # ARABIC LETTER HAH
    u'\u062E',  # ARABIC LETTER KHAH
    u'\u062F',  # ARABIC LETTER DAL
    u'\u0630',  # ARABIC LETTER THAL
    u'\u0631',  # ARABIC LETTER REH
    u'\u0632',  # ARABIC LETTER ZAIN
    u'\u0633',  # ARABIC LETTER SEEN
    u'\u0634',  # ARABIC LETTER SHEEN
    u'\u0635',  # ARABIC LETTER SAD
    u'\u0636',  # ARABIC LETTER DAD
    u'\u0637',  # ARABIC LETTER TAH
    u'\u0638',  # ARABIC LETTER ZAH
    u'\u0639',  # ARABIC LETTER AIN
    u'\u063A',  # ARABIC LETTER GHAIN
    u'\u0641',  # ARABIC LETTER FEH
    u'\u0642',  # ARABIC LETTER QAF
    u'\u0643',  # ARABIC LETTER KAF
    u'\u0644',  # ARABIC LETTER LAM
    u'\u0645',  # ARABIC LETTER MEEM
    u'\u0646',  # ARABIC LETTER NOON
    u'\u0647',  # ARABIC LETTER HEH
    u'\u0624',  # ARABIC LETTER WAW WITH HAMZA ABOVE
    u'\u0624',  # ARABIC LETTER WAW WITH HAMZA ABOVE
    u'\u0648',  # ARABIC LETTER WAW
    u'\u0649',  # ARABIC LETTER ALEF MAKSURA
    u'\u064A',  # ARABIC LETTER YEH
    u'\u0626',  # ARABIC LETTER YEH WITH HAMZA ABOVE
    u'\u0671',  # ARABIC LETTER ALEF WASLA
] + ARA_HARAKAT

ARA_NORMAL_ALPHA = []

ARABIC_CODE = [
    TLanguageCode('ara', 'arabic', required=ARA_REQUIRED_ALPHA, normal=ARA_NORMAL_ALPHA, accidental='ARA_HARAKAT', converter='TEmptyConverter'),
]

# Armenian

ARM_REQUIRED_ALPHA = [
    u'\u0531', u'\u0535', u'\u0537', u'\u0538', u'\u053B', u'\u0548',
    u'\u0555', u'\u0561', u'\u0565', u'\u0567', u'\u0568', u'\u056B',
    u'\u0578', u'\u0585',
]
ARM_NORMAL_ALPHA = [
    u'\u0531', u'\u0532', u'\u0533', u'\u0534', u'\u0535', u'\u0536',
    u'\u0537', u'\u0538', u'\u0539', u'\u053A', u'\u053B', u'\u053C',
    u'\u053D', u'\u053E', u'\u053F', u'\u0540', u'\u0541', u'\u0542',
    u'\u0543', u'\u0544', u'\u0545', u'\u0546', u'\u0547', u'\u0548',
    u'\u0549', u'\u054A', u'\u054B', u'\u054C', u'\u054D', u'\u054E',
    u'\u054F', u'\u0550', u'\u0551', u'\u0552', u'\u0553', u'\u0554',
    u'\u0555', u'\u0556', u'\u0561', u'\u0562', u'\u0563', u'\u0564',
    u'\u0565', u'\u0566', u'\u0567', u'\u0568', u'\u0569', u'\u056A',
    u'\u056B', u'\u056C', u'\u056D', u'\u056E', u'\u056F', u'\u0570',
    u'\u0571', u'\u0572', u'\u0573', u'\u0574', u'\u0575', u'\u0576',
    u'\u0577', u'\u0578', u'\u0579', u'\u057A', u'\u057B', u'\u057C',
    u'\u057D', u'\u057E', u'\u057F', u'\u0580', u'\u0581', u'\u0582',
    u'\u0583', u'\u0584', u'\u0585', u'\u0586', u'\u0587',
]
ARM_SIGNS = APOSTROPHES + HYPHENS + [
    u'\u0559',  # armenian modifier letter left half ring
    u'\u055A',  # armenian apostrophe
    # u'\u055B',  # armenian emphasis mark
    # u'\u055C',  # armenian exclamation mark
    # u'\u055D',  # armenian comma
    # u'\u055E',  # armenian question mark
    # u'\u055F',  # armenian abbreviation mark
]
ARM_INNER_PUNCT = [
    u'\u055B',  # ARMENIAN EMPHASIS MARK
    u'\u055C',  # ARMENIAN EXCLAMATION MARK
    u'\u055E',  # ARMENIAN QUESTION MARK
]

ARMENIAN_CODE = [
    TLanguageCode('arm', 'armenian', required=ARM_REQUIRED_ALPHA, normal=ARM_NORMAL_ALPHA, signs=ARM_SIGNS, accidental=ARM_INNER_PUNCT),
]

# Bashkir

BAK_REQUIRED_ALPHA = CYRILLIC_RUSSIAN_VOWELS + [
    u'\u0401', u'\u0451', u'\u04e8', u'\u04e9', u'\u04d8', u'\u04d9',
]
BAK_NORMAL_ALPHA = CYRILLIC_RUSSIAN_CONSONANT + [
    u'\u0492', u'\u0493', u'\u0498', u'\u0499', u'\u04a0', u'\u04a1',
    u'\u04a2', u'\u04a3', u'\u04aa', u'\u04ab', u'\u04ae', u'\u04af',
    u'\u04ba', u'\u04bb', u'\u042a', u'\u044a',
]

BASHKIR_CODE = [
    TLanguageCode('bak', 'bashkir', required=BAK_REQUIRED_ALPHA, normal=BAK_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Belarusian

BEL_REQUIRED_ALPHA = [
    # uppercase
    u'\u0410', u'\u0415', u'\u041E', u'\u0423', u'\u042B', u'\u042D',
    u'\u042E', u'\u042F', u'\u0401', u'\u0406',
    # lowercase
    u'\u0430', u'\u0435', u'\u043E', u'\u0443', u'\u044B', u'\u044D',
    u'\u044E', u'\u044F', u'\u0451', u'\u0456',
]
BEL_NORMAL_ALPHA = [
    # uppercase
    u'\u0411', u'\u0412', u'\u0413', u'\u0414', u'\u0416', u'\u0417',
    u'\u0419', u'\u041A', u'\u041B', u'\u041C', u'\u041D', u'\u041F',
    u'\u0420', u'\u0421', u'\u0422', u'\u0424', u'\u0425', u'\u0426',
    u'\u0427', u'\u0428', u'\u042C', u'\u040E', u'\u0490',
    # lowercase
    u'\u0431', u'\u0432', u'\u0433', u'\u0434', u'\u0436', u'\u0437',
    u'\u0439', u'\u043A', u'\u043B', u'\u043C', u'\u043D', u'\u043F',
    u'\u0440', u'\u0441', u'\u0442', u'\u0444', u'\u0445', u'\u0446',
    u'\u0447', u'\u0448', u'\u044C', u'\u045E', u'\u0491',
]
BELARUSIAN_CONVERTER_MAP = {
    u'\u2019': u'\u0027',  # ’ -> '
    u'\u0490': u'\u0413',  # Ґ -> Г
    u'\u0491': u'\u0433',  # ґ -> г
}

BELARUSIAN_CODE = [
    TTrClassCode('TBelarusianConverter', BELARUSIAN_CONVERTER_MAP, STRESS),
    TLanguageCode('bel', 'belarusian', required=BEL_REQUIRED_ALPHA, normal=BEL_NORMAL_ALPHA, alien='EASTERN_SLAVIC_RENYXA', accidental='STRESS', signs='APOST_AND_HYPH', converter='TBelarusianConverter', **EASTERN_SLAVIC_CONVERTERS),
]

# Bulgarian

BUL_REQUIRED_ALPHA = [
    # uppercase
    u'\u0410', u'\u0415', u'\u0418', u'\u041E', u'\u0423', u'\u042A',
    u'\u042E', u'\u042F',
    # lowercase
    u'\u0430', u'\u0435', u'\u0438', u'\u043E', u'\u0443', u'\u044A',
    u'\u044E', u'\u044F',
]
BUL_NORMAL_ALPHA = [
    # uppercase
    u'\u0411', u'\u0412', u'\u0413', u'\u0414', u'\u0416', u'\u0417',
    u'\u0419', u'\u041A', u'\u041B', u'\u041C', u'\u041D', u'\u041F',
    u'\u0420', u'\u0421', u'\u0422', u'\u0424', u'\u0425', u'\u0426',
    u'\u0427', u'\u0428', u'\u0429', u'\u042C',
    # lowercase
    u'\u0431', u'\u0432', u'\u0433', u'\u0434', u'\u0436', u'\u0437',
    u'\u0439', u'\u043A', u'\u043B', u'\u043C', u'\u043D', u'\u043F',
    u'\u0440', u'\u0441', u'\u0442', u'\u0444', u'\u0445', u'\u0446',
    u'\u0447', u'\u0448', u'\u0449', u'\u044C',
]
BUL_ALIEN_ALPHA = CYRILLIC_RENYXA

BULGARIAN_CODE = [
    TLanguageCode('bul', 'bulgarian', required=BUL_REQUIRED_ALPHA, normal=BUL_NORMAL_ALPHA, alien=BUL_ALIEN_ALPHA, accidental='STRESS_BULGARIAN', signs='APOST_AND_HYPH', converter='TGeneralCyrilicConverter', **EASTERN_SLAVIC_CONVERTERS),
]

# Chinese

CHI_REQUIRED_ALPHA = CJK_UNFIED + CJK_UNIFIED_EXT_A + CJK_COMPATIBILITY + CJK_DESCRIPTION + BOPOMOFO

CHINESE_CODE = [
    TLanguageCode('chi', 'chinese', required=CHI_REQUIRED_ALPHA, signs='CJK_APOST_AND_HYPH'),
]

# Church Slavonic

CHU_REQUIRED_ALPHA = CYRILLIC_RUSSIAN_VOWELS + [
    u'\u0404', u'\u0454',  # Old cyrillic yest
    u'\u0406', u'\u0456',  # Old cyrillic i
    u'\u0460', u'\u0461',  # Cyrillic omega
    u'\u0462', u'\u0463',  # Cyrillic yat
    u'\u0474', u'\u0475',  # Cyrillic izhitsa
    u'\u0478', u'\u0479',  # Cyrillic uk
    u'\u047A', u'\u047B',  # Cyrillic round omega
    u'\u047C', u'\u047D',  # Cyrillic omega with titlo
]
CHU_NORMAL_ALPHA = CYRILLIC_RUSSIAN_CONSONANT + [
    u'\u0405', u'\u0455',  # Cyrillic dze
    u'\u046E', u'\u046F',  # Cyrillic ksi
    u'\u0470', u'\u0471',  # Cyrillic psi
    u'\u0472', u'\u0473',  # Cyrillic fita
    u'\u047E', u'\u047F',  # Cyrillic ot
]
CHU_STRESS = [
    u'\u0300',  # Combining grave accent
    u'\u0301',  # Combining acute accent
    u'\u0302',  # Combining circumflex accent
    u'\u030F',  # Combining double grave accent
    u'\u0311',  # Combining inverted breve
    u'\u0483',  # Cyrillic titlo
]

CHURCH_SLAVONIC_CODE = [
    TLanguageCode('chu', 'ChurchSlavonic', required=CHU_REQUIRED_ALPHA, normal=CHU_NORMAL_ALPHA, accidental=CHU_STRESS, signs='APOST_AND_HYPH', converter='TGeneralCyrilicConverter', **EASTERN_SLAVIC_CONVERTERS),
]

# Czech

CZE_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C1', u'\u00E1',  # A with acute
    u'\u00C9', u'\u00E9',  # E with acute
    u'\u011A', u'\u011B',  # E with hacek
    u'\u00CD', u'\u00ED',  # I with acute
    u'\u00D3', u'\u00F3',  # O with acute
    u'\u00DA', u'\u00FA',  # U with acute
    u'\u016E', u'\u016F',  # U with circle
    u'\u00DD', u'\u00FD',  # Y with acute
    u'\u0052', u'\u0072',  # R
    u'\u0158', u'\u0159',  # R with hacek
]
CZE_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u010C', u'\u010D',  # C with hacek
    u'\u010E', u'\u010F',  # D with hacek
    u'\u0147', u'\u0148',  # N with hacek
    u'\u0160', u'\u0161',  # S with hacek
    u'\u0164', u'\u0165',  # T with hacek
    u'\u017D', u'\u017E',  # Z with hacek
]

CZECH_CODE = [
    TLanguageCode('cze', 'czech', required=CZE_REQUIRED_ALPHA, normal=CZE_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Danish

DAN_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00c6', u'\u00d8', u'\u00c5', u'\u00e6', u'\u00f8', u'\u00e5',
]

DAN_NORMAL_ALPHA = LATINS_CONSONANT

DANISH_CODE = [
    TLanguageCode('dan', 'danish', required=DAN_REQUIRED_ALPHA, normal=DAN_NORMAL_ALPHA, accidental='STRESS', signs='APOST_AND_HYPH'),
]

# Dutch

DUT_REQUIRED_ALPHA = LATINS_VOWELS
DUT_NORMAL_ALPHA = LATINS_CONSONANT

DUTCH_CODE = [
    TLanguageCode('dut', 'dutch', required=DUT_REQUIRED_ALPHA, normal=DUT_NORMAL_ALPHA, accidental='STRESS', signs='APOST_AND_HYPH'),
]

# English

ENG_REQUIRED_ALPHA = LATINS_VOWELS
ENG_NORMAL_ALPHA = LATINS_CONSONANT

ENGLISH_CODE = [
    TLanguageCode('eng', 'english', required=ENG_REQUIRED_ALPHA, normal=ENG_NORMAL_ALPHA, accidental='STRESS', signs='APOST_AND_HYPH', mask='TLangMask(LANG_ENG, LANG_BASIC_ENG)'),
]

# Estonian

EST_REQUIRED_ALPHA = [
    u'\u0041', u'\u0061', u'\u0045', u'\u0065', u'\u0049', u'\u0069',
    u'\u004f', u'\u006f', u'\u0055', u'\u0075', u'\u00d5', u'\u00f5',
    u'\u00c4', u'\u00e4', u'\u00d6', u'\u00f6', u'\u00dc', u'\u00fc',
    u'\u0059', u'\u0079',
]

EST_NORMAL_ALPHA = [
    u'\u0042', u'\u0062', u'\u0043', u'\u0063', u'\u0044', u'\u0064',
    u'\u0046', u'\u0066', u'\u0047', u'\u0067', u'\u0048', u'\u0068',
    u'\u004a', u'\u006a', u'\u004b', u'\u006b', u'\u004c', u'\u006c',
    u'\u004d', u'\u006d', u'\u004e', u'\u006e', u'\u0050', u'\u0070',
    u'\u0051', u'\u0071', u'\u0052', u'\u0072', u'\u0053', u'\u0073',
    u'\u0160', u'\u0161', u'\u005a', u'\u007a', u'\u017d', u'\u017e',
    u'\u0054', u'\u0074', u'\u0056', u'\u0076', u'\u0057', u'\u0077',
    u'\u0058', u'\u0078',
]

ESTONIAN_CODE = [
    TLanguageCode('est', 'estonian', required=EST_REQUIRED_ALPHA, normal=EST_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Finnish

FIN_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C5', u'\u00E5',
    u'\u00C4', u'\u00E4',
    u'\u00D6', u'\u00F6',
]
FIN_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u0160', u'\u0161',
    u'\u017D', u'\u017E',
]

FINNISH_CODE = [
    TLanguageCode('fin', 'finnish', required=FIN_REQUIRED_ALPHA, normal=FIN_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# French

FRE_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C0', u'\u00E0',  # A with grave
    u'\u00C2', u'\u00E2',  # A with circumflex
    u'\u00C8', u'\u00E8',  # E with grave
    u'\u00C9', u'\u00E9',  # E with acute
    u'\u00CA', u'\u00EA',  # E with circumflex
    u'\u00CB', u'\u00EB',  # E with diaeresis
    u'\u00CE', u'\u00EE',  # I with circumflex
    u'\u00CF', u'\u00EF',  # I with diaeresis
    u'\u00D4', u'\u00F4',  # O with circumflex
    u'\u00D9', u'\u00F9',  # U with grave
    u'\u00DB', u'\u00FB',  # U with circumflex
    u'\u00DC', u'\u00FC',  # U with diaeresis
    u'\u0178', u'\u00FF',  # Y with diaeresis
    u'\u00C6', u'\u00E6',  # ae ligature
    u'\u0152', u'\u0153',  # oe ligature
]
FRE_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u00C7', u'\u00E7',  # C with cedilla
]

FRENCH_CODE = [
    TLanguageCode('fre', 'french', required=FRE_REQUIRED_ALPHA, normal=FRE_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Georgian

GEO_REQUIRED_ALPHA = [
    u'\u10D0', u'\u10D4', u'\u10F1', u'\u10D8', u'\u10F2', u'\u10DD',
    u'\u10E3', u'\u10EF',
]
GEO_NORMAL_ALPHA = [
    u'\u10D1', u'\u10D2', u'\u10D3', u'\u10D5', u'\u10D6', u'\u10D7',
    u'\u10D8', u'\u10D9', u'\u10DA', u'\u10DB', u'\u10Dc', u'\u10DE',
    u'\u10DF', u'\u10E0', u'\u10E1', u'\u10E2', u'\u10F3', u'\u10E4',
    u'\u10E5', u'\u10E6', u'\u10E7', u'\u10E8', u'\u10E9', u'\u10EA',
    u'\u10EB', u'\u10EC', u'\u10ED', u'\u10EE', u'\u10F0', u'\u10F5',
    u'\u10F6',
]

GEORGIAN_CODE = [
    TLanguageCode('geo', 'georgian', required=GEO_REQUIRED_ALPHA, normal=GEO_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# German

GER_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C4', u'\u00E4',  # A umlaut
    u'\u00D6', u'\u00F6',  # O umlaut
    u'\u00DC', u'\u00FC',  # U umlaut
]
GER_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u00DF',  # Ligatures - Scharfes S
]
GERMAN_ADV_CONVERTER_MAP = {
    u'\u00E4': u'ae',  # ä
    u'\u00F6': u'oe',  # ö
    u'\u00FC': u'ue',  # ü
    u'\u00DF': u'ss',  # ß
}
GERMAN_CONVERTER_MAP = merge(APOSTROPHE_CONVERT_MAP, GERMAN_ADV_CONVERTER_MAP)

GERMAN_CODE = [
    TTrClassCode('TGermanConverter', GERMAN_CONVERTER_MAP),
    TLanguageCode('ger', 'german', required=GER_REQUIRED_ALPHA, normal=GER_NORMAL_ALPHA, signs='APOST_AND_HYPH', converter='TGermanConverter'),
]

# Gaelic

GLE_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00c1', u'\u00c9', u'\u00cd', u'\u00d3', u'\u00da', u'\u00e1',
    u'\u00e9', u'\u00ed', u'\u00f3', u'\u00fa',
]

GLE_NORMAL_ALPHA = LATINS_CONSONANT

GAELIC_CODE = [
    TLanguageCode('gle', 'gaelic', required=GLE_REQUIRED_ALPHA, normal=GLE_NORMAL_ALPHA, signs='APOST_AND_HYPH')
]

# Greek

GRE_REQUIRED_ALPHA = [
    u'\u0386', u'\u0388', u'\u038a', u'\u038c', u'\u038e', u'\u0390',
    u'\u03aa', u'\u03ab', u'\u03b0', u'\u03ca', u'\u03cb', u'\u0391',
    u'\u03b1', u'\u0395', u'\u03b5', u'\u0397', u'\u03b7', u'\u0399',
    u'\u03b9', u'\u039f', u'\u03bf', u'\u03a5', u'\u03c5', u'\u03a9',
    u'\u03c9', u'\u03ac', u'\u03ce', u'\u03cd', u'\u03ad', u'\u03af',
    u'\u03cc', u'\u03ae',
]
GRE_NORMAL_ALPHA = [
    u'\u0389', u'\u038f', u'\u0392', u'\u03b2', u'\u0393', u'\u03b3',
    u'\u0394', u'\u03b4', u'\u0396', u'\u03b6', u'\u0398', u'\u03b8',
    u'\u039a', u'\u03ba', u'\u039b', u'\u03bb', u'\u039c', u'\u03bc',
    u'\u039d', u'\u03bd', u'\u039e', u'\u03be', u'\u03a0', u'\u03c0',
    u'\u03a1', u'\u03c1', u'\u03a3', u'\u03c3', u'\u03c2', u'\u03a4',
    u'\u03c4', u'\u03a6', u'\u03c6', u'\u03a7', u'\u03c7', u'\u03a8',
    u'\u03c8',
]

GREEK_CODE = [
    TLanguageCode('gre', 'greek', required=GRE_REQUIRED_ALPHA, normal=GRE_NORMAL_ALPHA, signs='APOST_AND_HYPH', converter='TGermanConverter'),
]

# Hebrew

HEB_REQUIRED_ALPHA = [
    u'\u05db', u'\u05de', u'\u05e0', u'\u05e4', u'\u05e6', u'\u05d0',
    u'\u05d1', u'\u05d2', u'\u05d3', u'\u05d4', u'\u05d5', u'\u05d6',
    u'\u05d7', u'\u05d8', u'\u05d9', u'\u05da', u'\u05dc', u'\u05dd',
    u'\u05df', u'\u05e1', u'\u05e2', u'\u05e3', u'\u05e5', u'\u05e7',
    u'\u05e8', u'\u05e9', u'\u05ea'
]

HEB_NORMAL_ALPHA = []

HEBREW_CODE = [
    TLanguageCode('heb', 'hebrew', required=HEB_REQUIRED_ALPHA, normal=HEB_NORMAL_ALPHA, signs='APOST_AND_HYPH')
]

# Hungarian

HUN_REQUIRED_ALPHA = [
    u'\u0041', u'\u00c1', u'\u0045', u'\u00c9', u'\u0049', u'\u00cd',
    u'\u004f', u'\u00d3', u'\u00d6', u'\u0150', u'\u0055', u'\u00da',
    u'\u00dc', u'\u0170', u'\u0059', u'\u0061', u'\u00e1', u'\u0065',
    u'\u00e9', u'\u0069', u'\u00ed', u'\u006f', u'\u00f3', u'\u00f6',
    u'\u0151', u'\u0075', u'\u00fa', u'\u00fc', u'\u0171', u'\u0079',
]

HUN_NORMAL_ALPHA = [
    u'\u0042', u'\u0043', u'\u0044', u'\u0046', u'\u0047', u'\u0048',
    u'\u004a', u'\u004b', u'\u004c', u'\u004d', u'\u004e', u'\u0050',
    u'\u0051', u'\u0052', u'\u0053', u'\u0054', u'\u0056', u'\u0057',
    u'\u0058', u'\u005a', u'\u0062', u'\u0063', u'\u0064', u'\u0066',
    u'\u0067', u'\u0068', u'\u006a', u'\u006b', u'\u006c', u'\u006d',
    u'\u006e', u'\u0070', u'\u0071', u'\u0072', u'\u0073', u'\u0074',
    u'\u0076', u'\u0077', u'\u0078', u'\u007a',
]

HUNGARIAN_CODE = [
    TLanguageCode('hun', 'hungarian', required=HUN_REQUIRED_ALPHA, normal=HUN_NORMAL_ALPHA, signs='APOST_AND_HYPH')
]

# Indonesian

IND_REQUIRED_ALPHA = LATINS_VOWELS
IND_NORMAL_ALPHA = LATINS_CONSONANT

INDONESIAN_CODE = [
    TLanguageCode('ind', 'indonesian', required=IND_REQUIRED_ALPHA, normal=IND_NORMAL_ALPHA, accidental='CIRCUMFLEX', signs='APOST_AND_HYPH'),
]

# Italian

ITA_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C0', u'\u00E0',  # A with grave
    u'\u00C1', u'\u00E1',  # A with acute
    u'\u00C8', u'\u00E8',  # E with grave
    u'\u00C9', u'\u00E9',  # E with acute
    u'\u00CC', u'\u00EC',  # I with grave
    u'\u00CD', u'\u00ED',  # I with acute
    u'\u00D2', u'\u00F2',  # O with grave
    u'\u00D3', u'\u00F3',  # O with acute
    u'\u00D9', u'\u00F9',  # U with grave
    u'\u00DA', u'\u00FA',  # U with acute
]
ITA_NORMAL_ALPHA = LATINS_CONSONANT

ITALIAN_CODE = [
    TLanguageCode('ita', 'italian', required=ITA_REQUIRED_ALPHA, normal=ITA_NORMAL_ALPHA, accidental='CIRCUMFLEX', signs='APOST_AND_HYPH'),
]

# Japanese

JPN_REQUIRED_ALPHA = CJK_UNFIED + CJK_UNIFIED_EXT_A + CJK_COMPATIBILITY + CJK_DESCRIPTION + HIRAGANA_KATAKANA + HALFWIDTH_KATAKANA

JAPANESE_CODE = [
    TLanguageCode('jpn', 'japanese', required=JPN_REQUIRED_ALPHA, signs='CJK_APOST_AND_HYPH'),
]

# Kazakh

KAZ_REQUIRED_ALPHA = CYRILLIC_RUSSIAN_VOWELS
KAZ_NORMAL_ALPHA = CYRILLIC_RUSSIAN_CONSONANT + [
    u'\u0492', u'\u0493',  # cyrillic letter ghe with stroke
    u'\u04B0', u'\u04B1',  # cyrillic letter straight u with stroke
    u'\u04E8', u'\u04E9',  # cyrillic letter barred o
    u'\u04A2', u'\u04A3',  # cyrillic letter en with descender
    u'\u049A', u'\u049B',  # cyrillic letter ka with descender
    u'\u04BA', u'\u04BB',  # cyrillic letter shha
    u'\u04AE', u'\u04AF',  # cyrillic letter straight u
    u'\u0406', u'\u0456',  # cyrillic letter byelorussian-ukrainian i
    u'\u04D8', u'\u04D9',  # cyrillic capital letter schwa
]
KAZ_DIACRITICS_MAP = [
    (u'\u0430', u'\u04D9'),  # cyrillic letter schwa
    (u'\u0433', u'\u0493'),  # cyrillic letter ghe with stroke
    (u'\u0438', u'\u0456'),  # cyrillic letter byelorussian-ukrainian i
    (u'\u043A', u'\u049B'),  # cyrillic letter ka with descender
    (u'\u043D', u'\u04A3'),  # cyrillic letter en with descender
    (u'\u043E', u'\u04E9'),  # cyrillic letter barred o
    (u'\u0443', u'\u04AF', u'\u04B1'),  # cyrillic letter straight u (+with stroke)
    (u'\u0445', u'\u04BB'),  # cyrillic letter shha
]

KAZ_DERENYXA_MAP = reverse_mapping({k: v for k, v in EASTERN_SLAVIC_RENYXA_MAP.items() if u'A' <= k <= u'z'})
KAZAKH_CODE = [
    TDiacriticsMapClassCode('TKazakhDiacriticsMap', KAZ_DIACRITICS_MAP),
    TTrClassCode('TKazRenyxer', KAZ_DERENYXA_MAP),
    TTrClassCode('TKazPreconverterSecondary', compose(UNICODE_LOWERCASE_MAP, KAZ_DERENYXA_MAP)),
    TLanguageCode(
        'kaz', 'kazakh', required=KAZ_REQUIRED_ALPHA, normal=KAZ_NORMAL_ALPHA, alien='EASTERN_SLAVIC_RENYXA', accidental='STRESS', signs='APOST_AND_HYPH', converter='TGeneralCyrilicConverter', diamap='TKazakhDiacriticsMap',
        secondary = {
            # 'constructer': 'eng',
            'required': 'ENG_REQUIRED_ALPHA',
            'normal': 'ENG_NORMAL_ALPHA',
            'accidental': 'STRESS',
            'signs': 'APOST_AND_HYPH',
            'preconverter': 'TKazPreconverterSecondary',
            'derenyxer': 'TKazRenyxer',
            'alien': list(KAZ_DERENYXA_MAP.keys()),
        },
        **EASTERN_SLAVIC_CONVERTERS
    ),
]

# Korean

KOR_REQUIRED_ALPHA = CJK_UNFIED + CJK_UNIFIED_EXT_A + CJK_COMPATIBILITY + CJK_DESCRIPTION + HANGUL + HANGUL_EXT_A + HANGUL_EXT_B + HANGUL_COMPATIBILITY + HANGUL_SYLLABLES + HALFWIDTH_HANGUL

KOREAN_CODE = [
    TLanguageCode('kor', 'korean', required=KOR_REQUIRED_ALPHA, signs='CJK_APOST_AND_HYPH'),
]

# Latvian

LAV_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u0100', u'\u0101', u'\u0112', u'\u0113', u'\u012a', u'\u012b',
    u'\u016a', u'\u016b',
]

LAV_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u010c', u'\u010d', u'\u0112', u'\u0113', u'\u0122', u'\u0123',
    u'\u0136', u'\u0137', u'\u013b', u'\u013c', u'\u0145', u'\u0146',
    u'\u0160', u'\u0161', u'\u017d', u'\u017e',
]

LATVIAN_CODE = [
    TLanguageCode('lav', 'latvian', required=LAV_REQUIRED_ALPHA, normal=LAV_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Lithuanian

LIT_REQUIRED_ALPHA = [
    u'\u0041', u'\u0061', u'\u0104', u'\u0105', u'\u0045', u'\u0065',
    u'\u0118', u'\u0119', u'\u0116', u'\u0117', u'\u0049', u'\u0069',
    u'\u012e', u'\u012f', u'\u0059', u'\u0079', u'\u004a', u'\u006a',
    u'\u004f', u'\u006f', u'\u0055', u'\u0075', u'\u0172', u'\u0173',
    u'\u016a', u'\u016b',
]

LIT_NORMAL_ALPHA = [
    u'\u0042', u'\u0062', u'\u0043', u'\u0063', u'\u010c', u'\u010d',
    u'\u0044', u'\u0064', u'\u0046', u'\u0066', u'\u0047', u'\u0067',
    u'\u0048', u'\u0068', u'\u004b', u'\u006b', u'\u004c', u'\u006c',
    u'\u004d', u'\u006d', u'\u004e', u'\u006e', u'\u0050', u'\u0070',
    u'\u0052', u'\u0072', u'\u0053', u'\u0073', u'\u0160', u'\u0161',
    u'\u0054', u'\u0074', u'\u0056', u'\u0076', u'\u005a', u'\u007a',
    u'\u017d', u'\u017e',
]

LITHUANIAN_CODE = [
    TLanguageCode('lit', 'lithuanian', required=LIT_REQUIRED_ALPHA, normal=LIT_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Norwegian

NOR_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00c6', u'\u00d8', u'\u00c5', u'\u00e6', u'\u00f8', u'\u00e5',
]

NOR_NORMAL_ALPHA = LATINS_CONSONANT

NORWEGIAN_CODE = [
    TLanguageCode('nor', 'norwegian', required=NOR_REQUIRED_ALPHA, normal=NOR_NORMAL_ALPHA, accidental='STRESS', signs='APOST_AND_HYPH'),
]

# Polish

POL_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u0104', u'\u0105',  # A with ogonek
    u'\u0118', u'\u0119',  # E with ogonek
    u'\u00D3', u'\u00F3',  # O with acute
]
POL_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u0106', u'\u0107',  # C with acute
    u'\u0141', u'\u0142',  # L with stroke
    u'\u0143', u'\u0144',  # N with acute
    u'\u015A', u'\u015B',  # S with acute
    u'\u0179', u'\u017A',  # Z with acute
    u'\u017B', u'\u017C',  # Z with dot above
]

POLISH_CODE = [
    TLanguageCode('pol', 'polish', required=POL_REQUIRED_ALPHA, normal=POL_NORMAL_ALPHA, accidental='STRESS', signs='APOST_AND_HYPH'),
]

# Portuguese

POR_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C0', u'\u00E0',  # A with grave
    u'\u00C1', u'\u00E1',  # A with acute
    u'\u00C2', u'\u00E2',  # A with circumflex
    u'\u00C3', u'\u00E3',  # A with tilde
    u'\u00C9', u'\u00E9',  # E with acute
    u'\u00CA', u'\u00EA',  # E with circumflex
    u'\u00CD', u'\u00ED',  # I with acute
    u'\u00D3', u'\u00F3',  # O with acute
    u'\u00D4', u'\u00F4',  # O with circumflex
    u'\u00D5', u'\u00F5',  # O with tilde
    u'\u00DA', u'\u00FA',  # U with acute
]
POR_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u00C7', u'\u00E7',  # C with cedille
]
PORTUGUESE_CONVERTER_MAP = {
    u'\u2019': u"'",  # ’
}

PORTUGUESE_CODE = [
    TTrClassCode('TPortugueseConverter', PORTUGUESE_CONVERTER_MAP),
    TLanguageCode('por', 'portuguese', required=POR_REQUIRED_ALPHA, normal=POR_NORMAL_ALPHA, signs='APOST_AND_HYPH', converter='TPortugueseConverter'),
]

# Romanian

RUM_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u0102', u'\u0103',  # A with breve
    u'\u00C2', u'\u00E2',  # A with circumflex
    u'\u00CE', u'\u00EE',  # I with circumflex
]
RUM_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u0218', u'\u0219',  # S with comma
    u'\u015E', u'\u015F',  # S with cedille - compatibility
    u'\u021A', u'\u021B',  # T with comma
    u'\u0162', u'\u0163',  # T with cedille - compatibility
]
ROMANIAN_RENYXA_MAP = {
    u'\u0218': u'\u015E',  # Ș -> Ş
    u'\u0219': u'\u015F',  # ș -> ş
    u'\u021A': u'\u0162',  # Ț -> Ţ
    u'\u021B': u'\u0163',  # ț -> ţ
}
ROMANIAN_PRE_CONVERT_MAP = compose(UNICODE_LOWERCASE_MAP, ROMANIAN_RENYXA_MAP)

ROMANIAN_CODE = [
    TTrClassCode('TRomanianDerenyxer', ROMANIAN_RENYXA_MAP, STRESS),
    TTrClassCode('TRomanianPreConverter', ROMANIAN_PRE_CONVERT_MAP),
    TLanguageCode('rum', 'romanian', required=RUM_REQUIRED_ALPHA, normal=RUM_NORMAL_ALPHA, accidental='STRESS', signs='APOST_AND_HYPH', preconverter='TRomanianPreConverter', derenyxer='TRomanianDerenyxer'),
]

# Russian

RUS_REQUIRED_ALPHA = CYRILLIC_RUSSIAN_VOWELS
RUS_NORMAL_ALPHA = CYRILLIC_RUSSIAN_CONSONANT + [
    # specific old russian characters
    u'\u0406', u'\u0456', u'\u0462', u'\u0463', u'\u0472', u'\u0473',
    u'\u0474', u'\u0475',
]
RUS_BASIC_ALPHA = CYRILLIC_RUSSIAN_CONSONANT

RUSSIAN_CODE = [
    WCharStaticArrayCode('RUS_REQUIRED_ALPHA', RUS_REQUIRED_ALPHA),
    WCharStaticArrayCode('RUS_NORMAL_ALPHA', RUS_NORMAL_ALPHA),
    WCharStaticArrayCode('RUS_BASIC_ALPHA', RUS_BASIC_ALPHA),
    TLanguageCode('rus', 'russian', required='RUS_REQUIRED_ALPHA', normal='RUS_NORMAL_ALPHA', alien='EASTERN_SLAVIC_RENYXA', accidental='STRESS', signs='APOST_AND_HYPH', constructer='TRussianWordNormalizer', secondary={'constructer': 'eng'}),
    TLanguageCode('basic_rus', 'BasicRussian', required='RUS_REQUIRED_ALPHA', normal='RUS_BASIC_ALPHA', alien='EASTERN_SLAVIC_RENYXA', accidental='STRESS', signs='APOST_AND_HYPH', converter='TGeneralCyrilicConverter', **EASTERN_SLAVIC_CONVERTERS),
    TRawCode(open(os.path.join(os.path.dirname(__file__), 'ru_normalizer.cpp.tpl'), 'r').read())
]

# Spanish

SPA_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C1', u'\u00E1',  # A with acute
    u'\u00C9', u'\u00E9',  # E with acute
    u'\u00CD', u'\u00ED',  # I with acute
    u'\u00D3', u'\u00F3',  # O with acute
    u'\u00DA', u'\u00FA',  # U with acute
    u'\u00DC', u'\u00FC',  # U with diaeresis
]
SPA_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u00D1', u'\u00F1',  # N with tilde
]

SPANISH_CODE = [
    TLanguageCode('spa', 'spanish', required=SPA_REQUIRED_ALPHA, normal=SPA_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Tatar

TAT_REQUIRED_ALPHA = CYRILLIC_RUSSIAN_VOWELS
TAT_NORMAL_ALPHA = CYRILLIC_RUSSIAN_CONSONANT + [
    u'\u04E8', u'\u04E9',  # cyrillic letter barred o
    u'\u04A2', u'\u04A3',  # cyrillic letter en with descender
    u'\u04BA', u'\u04BB',  # cyrillic letter shha
    u'\u04AE', u'\u04AF',  # cyrillic letter straight u
    u'\u0496', u'\u0497',  # cyrillic letter zhe with descender
    u'\u04D8', u'\u04D9',  # cyrillic capital letter schwa
]

TATAR_CODE = [
    TLanguageCode('tat', 'tatar', required=TAT_REQUIRED_ALPHA, normal=TAT_NORMAL_ALPHA, alien='EASTERN_SLAVIC_RENYXA', accidental='STRESS', signs='APOST_AND_HYPH', converter='TGeneralCyrilicConverter', **EASTERN_SLAVIC_CONVERTERS),
]

# Turkish

TUR_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00C2', u'\u00E2',  # A with circumflex
    u'\u0130',            # capital I with dot
               u'\u0131',  # dotless small I
    u'\u00CE', u'\u00EE',  # I with circumflex
    u'\u00D6', u'\u00F6',  # O with diaeresis
    u'\u00DB', u'\u00FB',  # U with circumflex
    u'\u00DC', u'\u00FC',  # U with diaeresis
]
TUR_NORMAL_ALPHA = LATINS_CONSONANT + [
    u'\u00C7', u'\u00E7',  # C with cedille
    u'\u011E', u'\u011F',  # G with breve
    u'\u015E', u'\u015F',  # S with cedille
]
TURKISH_PRE_LOWER_MAP = {
    u'\u0130': u'\u0069',    # İ -> i
    u'\u0049': u'\u0131',    # I -> ı
}
TURKISH_PRE_UPPER_MAP = reverse_mapping(TURKISH_PRE_LOWER_MAP)
TUR_PRE_CONVERT_MAP = compose(TURKISH_PRE_LOWER_MAP, UNICODE_LOWERCASE_MAP)
TURKISH_CONVERTER_MAP = {
    u'\u00C2': u'A',  # Â
    u'\u00E2': u'a',  # â
    u'\u00CE': u'I',  # Î
    u'\u00EE': u'i',  # î
    u'\u00DB': u'U',  # Û
    u'\u00FB': u'u',  # û
}
TURKISH_CONVERTER_REMOVE = [u'\u2019', u"'"]
TUR_DIACRITICS_MAP = [
    (u'c', u'\u00E7'),  # c with cedille
    (u'g', u'\u011F'),  # g with breve
    (u'i', u'\u0131'),  # dotless small I
    (u'o', u'\u00F6'),  # o with diaeresis
    (u's', u'\u015F'),  # s with cedille
    (u'u', u'\u00FC'),  # u with diaeresis
]


TURKISH_CODE = [
    TTrClassCode('TTurkishPreLower', TURKISH_PRE_LOWER_MAP),
    TTrClassCode('TTurkishPreUpper', TURKISH_PRE_UPPER_MAP),
    TTrClassCode('TTurkishPreConverter', TUR_PRE_CONVERT_MAP),
    TTrClassCode('TTurkishConverter', TURKISH_CONVERTER_MAP,
                 TURKISH_CONVERTER_REMOVE),
    TDiacriticsMapClassCode('TTurkishDiacriticsMap', TUR_DIACRITICS_MAP),
    TLanguageCode('tur', 'turkish', required=TUR_REQUIRED_ALPHA, normal=TUR_NORMAL_ALPHA, signs='APOST_AND_HYPH', preconverter='TTurkishPreConverter', converter='TTurkishConverter', prelower='TTurkishPreLower', preupper='TTurkishPreUpper', pretitle='TTurkishPreUpper', diamap='TTurkishDiacriticsMap', secondary={'constructer': 'eng'})
]

# Ukrainian

UKR_REQUIRED_ALPHA = [
    # uppercase
    u'\u0410', u'\u0415', u'\u0418', u'\u041E', u'\u0423', u'\u042E',
    u'\u042F', u'\u0404', u'\u0406', u'\u0407',
    # lowercase
    u'\u0430', u'\u0435', u'\u0438', u'\u043E', u'\u0443', u'\u044E',
    u'\u044F', u'\u0454', u'\u0456', u'\u0457',
]
UKR_NORMAL_ALPHA = [
    # uppercase
    u'\u0411', u'\u0412', u'\u0413', u'\u0414', u'\u0416', u'\u0417',
    u'\u0419', u'\u041A', u'\u041B', u'\u041C', u'\u041D', u'\u041F',
    u'\u0420', u'\u0421', u'\u0422', u'\u0424', u'\u0425', u'\u0426',
    u'\u0427', u'\u0428', u'\u0429', u'\u042C', u'\u0490',
    # lowercase
    u'\u0431', u'\u0432', u'\u0433', u'\u0434', u'\u0436', u'\u0437',
    u'\u0439', u'\u043A', u'\u043B', u'\u043C', u'\u043D', u'\u043F',
    u'\u0440', u'\u0441', u'\u0442', u'\u0444', u'\u0445', u'\u0446',
    u'\u0447', u'\u0448', u'\u0449', u'\u044C', u'\u0491',
]
UKR_ALIEN_ALPHA = CYRILLIC_RENYXA + [u'\u0401', u'\u0451']

UKRAINIAN_CODE = [
    TLanguageCode('ukr', 'ukrainian', required=UKR_REQUIRED_ALPHA, normal=UKR_NORMAL_ALPHA, alien=UKR_ALIEN_ALPHA, accidental='STRESS', signs='APOST_AND_HYPH', converter='TGeneralCyrilicConverter', secondary={'constructer': 'eng'}, **EASTERN_SLAVIC_CONVERTERS),
]

# auto-generated from docrec

# Azerbaijani

AZE_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00d6',  # Ö
    u'\u00dc',  # Ü
    u'\u00f6',  # ö
    u'\u00fc',  # ü
    u'\u0130',  # İ
]

AZE_NORMAL_ALPHA = [
    u'\u0042',  # B
    u'\u0043',  # C
    u'\u0044',  # D
    u'\u0046',  # F
    u'\u0047',  # G
    u'\u0048',  # H
    u'\u004a',  # J
    u'\u004b',  # K
    u'\u004c',  # L
    u'\u004d',  # M
    u'\u004e',  # N
    u'\u0050',  # P
    u'\u0051',  # Q
    u'\u0052',  # R
    u'\u0053',  # S
    u'\u0054',  # T
    u'\u0056',  # V
    u'\u0058',  # X
    u'\u0059',  # Y
    u'\u005a',  # Z
    u'\u0062',  # b
    u'\u0063',  # c
    u'\u0064',  # d
    u'\u0066',  # f
    u'\u0067',  # g
    u'\u0068',  # h
    u'\u006a',  # j
    u'\u006b',  # k
    u'\u006c',  # l
    u'\u006d',  # m
    u'\u006e',  # n
    u'\u0070',  # p
    u'\u0071',  # q
    u'\u0072',  # r
    u'\u0073',  # s
    u'\u0074',  # t
    u'\u0076',  # v
    u'\u0078',  # x
    u'\u0079',  # y
    u'\u007a',  # z
    u'\u00c7',  # Ç
    u'\u00e7',  # ç
    u'\u011e',  # Ğ
    u'\u011f',  # ğ
    u'\u0131',  # ı
    u'\u015e',  # Ş
    u'\u015f',  # ş
    u'\u018f',  # Ə
    u'\u0259',  # ə
]

AZERBAIJANI_CODE = [
    TLanguageCode('aze', 'azerbaijani', required=AZE_REQUIRED_ALPHA, normal=AZE_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Kirghiz

KIR_REQUIRED_ALPHA = CYRILLIC_RUSSIAN_VOWELS + [
    u'\u0401',  # Ё
    u'\u0419',  # Й
    u'\u0439',  # й
    u'\u0451',  # ё
]

KIR_NORMAL_ALPHA = CYRILLIC_RUSSIAN_CONSONANT + [
    u'\u04a2',  # Ң
    u'\u04a3',  # ң
    u'\u04ae',  # Ү
    u'\u04af',  # ү
    u'\u04e8',  # Ө
    u'\u04e9',  # ө
]

KIRGHIZ_CODE = [
    TLanguageCode('kir', 'kirghiz', required=KIR_REQUIRED_ALPHA, normal=KIR_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Mongolian

MON_REQUIRED_ALPHA = CYRILLIC_RUSSIAN_VOWELS + [
    u'\u0401',  # Ё
    u'\u0419',  # Й
    u'\u0439',  # й
    u'\u0451',  # ё
]

MON_NORMAL_ALPHA = CYRILLIC_RUSSIAN_CONSONANT + [
    u'\u04ae',  # Ү
    u'\u04af',  # ү
    u'\u04e8',  # Ө
    u'\u04e9',  # ө
]

MONGOLIAN_CODE = [
    TLanguageCode('mon', 'mongolian', required=MON_REQUIRED_ALPHA, normal=MON_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Swedish

SWE_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00c4',  # Ä
    u'\u00c5',  # Å
    u'\u00d6',  # Ö
    u'\u00e4',  # ä
    u'\u00e5',  # å
    u'\u00f6',  # ö
]

SWE_NORMAL_ALPHA = LATINS_CONSONANT + [
]

SWEDISH_CODE = [
    TLanguageCode('swe', 'swedish', required=SWE_REQUIRED_ALPHA, normal=SWE_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Tajik

TGK_REQUIRED_ALPHA = [
    u'\u0401',  # Ё
    u'\u0410',  # А
    u'\u0415',  # Е
    u'\u0418',  # И
    u'\u0419',  # Й
    u'\u041e',  # О
    u'\u0423',  # У
    u'\u042d',  # Э
    u'\u042e',  # Ю
    u'\u042f',  # Я
    u'\u0430',  # а
    u'\u0435',  # е
    u'\u0438',  # и
    u'\u0439',  # й
    u'\u043e',  # о
    u'\u0443',  # у
    u'\u044d',  # э
    u'\u044e',  # ю
    u'\u044f',  # я
    u'\u0451',  # ё
    u'\u04e2',  # Ӣ
    u'\u04e3',  # ӣ
    u'\u04ee',  # Ӯ
    u'\u04ef',  # ӯ
]

TGK_NORMAL_ALPHA = [
    u'\u0411',  # Б
    u'\u0412',  # В
    u'\u0413',  # Г
    u'\u0414',  # Д
    u'\u0416',  # Ж
    u'\u0417',  # З
    u'\u041a',  # К
    u'\u041b',  # Л
    u'\u041c',  # М
    u'\u041d',  # Н
    u'\u041f',  # П
    u'\u0420',  # Р
    u'\u0421',  # С
    u'\u0422',  # Т
    u'\u0424',  # Ф
    u'\u0425',  # Х
    u'\u0427',  # Ч
    u'\u0428',  # Ш
    u'\u042a',  # Ъ
    u'\u0431',  # б
    u'\u0432',  # в
    u'\u0433',  # г
    u'\u0434',  # д
    u'\u0436',  # ж
    u'\u0437',  # з
    u'\u043a',  # к
    u'\u043b',  # л
    u'\u043c',  # м
    u'\u043d',  # н
    u'\u043f',  # п
    u'\u0440',  # р
    u'\u0441',  # с
    u'\u0442',  # т
    u'\u0444',  # ф
    u'\u0445',  # х
    u'\u0447',  # ч
    u'\u0448',  # ш
    u'\u044a',  # ъ
    u'\u0492',  # Ғ
    u'\u0493',  # ғ
    u'\u049a',  # Қ
    u'\u049b',  # қ
    u'\u04b2',  # Ҳ
    u'\u04b3',  # ҳ
    u'\u04b6',  # Ҷ
    u'\u04b7',  # ҷ
]

TAJIK_CODE = [
    TLanguageCode('tgk', 'tajik', required=TGK_REQUIRED_ALPHA, normal=TGK_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Tagalog

TGL_REQUIRED_ALPHA = LATINS_VOWELS + [
]

TGL_NORMAL_ALPHA = [
    u'\u0042',  # B
    u'\u0044',  # D
    u'\u0047',  # G
    u'\u0048',  # H
    u'\u004b',  # K
    u'\u004c',  # L
    u'\u004d',  # M
    u'\u004e',  # N
    u'\u0050',  # P
    u'\u0052',  # R
    u'\u0053',  # S
    u'\u0054',  # T
    u'\u0057',  # W
    u'\u0059',  # Y
    u'\u0062',  # b
    u'\u0064',  # d
    u'\u0067',  # g
    u'\u0068',  # h
    u'\u006b',  # k
    u'\u006c',  # l
    u'\u006d',  # m
    u'\u006e',  # n
    u'\u0070',  # p
    u'\u0072',  # r
    u'\u0073',  # s
    u'\u0074',  # t
    u'\u0077',  # w
    u'\u0079',  # y
]

TAGALOG_CODE = [
    TLanguageCode('tgl', 'tagalog', required=TGL_REQUIRED_ALPHA, normal=TGL_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Turkmen

TUK_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u00c4',  # Ä
    u'\u00d6',  # Ö
    u'\u00dc',  # Ü
    u'\u00e4',  # ä
    u'\u00f6',  # ö
    u'\u00fc',  # ü
]

TUK_NORMAL_ALPHA = [
    u'\u0042',  # B
    u'\u0044',  # D
    u'\u0046',  # F
    u'\u0047',  # G
    u'\u0048',  # H
    u'\u004a',  # J
    u'\u004b',  # K
    u'\u004c',  # L
    u'\u004d',  # M
    u'\u004e',  # N
    u'\u0050',  # P
    u'\u0052',  # R
    u'\u0053',  # S
    u'\u0054',  # T
    u'\u0057',  # W
    u'\u0059',  # Y
    u'\u005a',  # Z
    u'\u0062',  # b
    u'\u0064',  # d
    u'\u0066',  # f
    u'\u0067',  # g
    u'\u0068',  # h
    u'\u006a',  # j
    u'\u006b',  # k
    u'\u006c',  # l
    u'\u006d',  # m
    u'\u006e',  # n
    u'\u0070',  # p
    u'\u0072',  # r
    u'\u0073',  # s
    u'\u0074',  # t
    u'\u0077',  # w
    u'\u0079',  # y
    u'\u007a',  # z
    u'\u00c7',  # Ç
    u'\u00dd',  # Ý
    u'\u00e7',  # ç
    u'\u00fd',  # ý
    u'\u0147',  # Ň
    u'\u0148',  # ň
    u'\u015e',  # Ş
    u'\u015f',  # ş
    u'\u017d',  # Ž
    u'\u017e',  # ž
]

TURKMEN_CODE = [
    TLanguageCode('tuk', 'turkmen', required=TUK_REQUIRED_ALPHA, normal=TUK_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]

# Uzbek

UZB_REQUIRED_ALPHA = LATINS_VOWELS + [
    u'\u0401',  # Ё
    u'\u040e',  # Ў
    u'\u0410',  # А
    u'\u0415',  # Е
    u'\u0418',  # И
    u'\u0419',  # Й
    u'\u041e',  # О
    u'\u0423',  # У
    u'\u042d',  # Э
    u'\u042e',  # Ю
    u'\u042f',  # Я
    u'\u0430',  # а
    u'\u0435',  # е
    u'\u0438',  # и
    u'\u0439',  # й
    u'\u043e',  # о
    u'\u0443',  # у
    u'\u044d',  # э
    u'\u044e',  # ю
    u'\u044f',  # я
    u'\u0451',  # ё
    u'\u045e',  # ў
]

UZB_NORMAL_ALPHA = [
    u'\u0042',  # B
    u'\u0043',  # C
    u'\u0044',  # D
    u'\u0046',  # F
    u'\u0047',  # G
    u'\u0048',  # H
    u'\u004a',  # J
    u'\u004b',  # K
    u'\u004c',  # L
    u'\u004d',  # M
    u'\u004e',  # N
    u'\u0050',  # P
    u'\u0051',  # Q
    u'\u0052',  # R
    u'\u0053',  # S
    u'\u0054',  # T
    u'\u0056',  # V
    u'\u0058',  # X
    u'\u0059',  # Y
    u'\u005a',  # Z
    u'\u0062',  # b
    u'\u0063',  # c
    u'\u0064',  # d
    u'\u0066',  # f
    u'\u0067',  # g
    u'\u0068',  # h
    u'\u006a',  # j
    u'\u006b',  # k
    u'\u006c',  # l
    u'\u006d',  # m
    u'\u006e',  # n
    u'\u0070',  # p
    u'\u0071',  # q
    u'\u0072',  # r
    u'\u0073',  # s
    u'\u0074',  # t
    u'\u0076',  # v
    u'\u0078',  # x
    u'\u0079',  # y
    u'\u007a',  # z
    u'\u0411',  # Б
    u'\u0412',  # В
    u'\u0413',  # Г
    u'\u0414',  # Д
    u'\u0416',  # Ж
    u'\u0417',  # З
    u'\u041a',  # К
    u'\u041b',  # Л
    u'\u041c',  # М
    u'\u041d',  # Н
    u'\u041f',  # П
    u'\u0420',  # Р
    u'\u0421',  # С
    u'\u0422',  # Т
    u'\u0424',  # Ф
    u'\u0425',  # Х
    u'\u0426',  # Ц
    u'\u0427',  # Ч
    u'\u0428',  # Ш
    u'\u042a',  # Ъ
    u'\u042c',  # Ь
    u'\u0431',  # б
    u'\u0432',  # в
    u'\u0433',  # г
    u'\u0434',  # д
    u'\u0436',  # ж
    u'\u0437',  # з
    u'\u043a',  # к
    u'\u043b',  # л
    u'\u043c',  # м
    u'\u043d',  # н
    u'\u043f',  # п
    u'\u0440',  # р
    u'\u0441',  # с
    u'\u0442',  # т
    u'\u0444',  # ф
    u'\u0445',  # х
    u'\u0446',  # ц
    u'\u0447',  # ч
    u'\u0448',  # ш
    u'\u044a',  # ъ
    u'\u044c',  # ь
    u'\u0492',  # Ғ
    u'\u0493',  # ғ
    u'\u049a',  # Қ
    u'\u049b',  # қ
    u'\u04b2',  # Ҳ
    u'\u04b3',  # ҳ
]

UZBEK_CODE = [
    TLanguageCode('uzb', 'uzbek', required=UZB_REQUIRED_ALPHA, normal=UZB_NORMAL_ALPHA, signs='APOST_AND_HYPH'),
]


# collecting all in one file

GENERAL_CODE = [
    WCharStaticArrayCode('APOST_AND_HYPH', APOSTROPHES + HYPHENS),
    WCharStaticArrayCode('STRESS', STRESS),
    WCharStaticArrayCode('STRESS_BULGARIAN', STRESS_BULGARIAN),
    WCharStaticArrayCode('CIRCUMFLEX', CIRCUMFLEX),
    WCharStaticArrayCode('ARA_HARAKAT', ARA_HARAKAT),

    TTrClassCode('TApostropheConverter', APOSTROPHE_CONVERT_MAP),
    TTrClassCode('TDefaultPreConverter', DEFAULT_PRE_CONVERTER_MAP, add_declaration=False),

    WCharStaticArrayCode('EASTERN_SLAVIC_RENYXA', EASTERN_SLAVIC_RENYXA),
    TTrClassCode('TEasternSlavicDerenyxer', EASTERN_SLAVIC_RENYXA_MAP, STRESS),
    TTrClassCode('TEasternSlavicPreConverter', EASTERN_SLAVIC_PRE_CONVERT_MAP),
    TTrClassCode('TGeneralCyrilicConverter', GENERAL_CYRILIC_CONVERTER_MAP,
                 STRESS),
]


CODE_BUILDERS = list(itertools.chain(
    GENERAL_CODE,
    CJK_CODE,
    ARABIC_CODE,
    ARMENIAN_CODE,
    BASHKIR_CODE,
    BELARUSIAN_CODE,
    BULGARIAN_CODE,
    CHINESE_CODE,
    CHURCH_SLAVONIC_CODE,
    CZECH_CODE,
    DANISH_CODE,
    DUTCH_CODE,
    ENGLISH_CODE,
    ESTONIAN_CODE,
    FINNISH_CODE,
    FRENCH_CODE,
    GEORGIAN_CODE,
    GERMAN_CODE,
    GAELIC_CODE,
    GREEK_CODE,
    HEBREW_CODE,
    HUNGARIAN_CODE,
    INDONESIAN_CODE,
    ITALIAN_CODE,
    JAPANESE_CODE,
    KAZAKH_CODE,
    KOREAN_CODE,
    LATVIAN_CODE,
    LITHUANIAN_CODE,
    NORWEGIAN_CODE,
    POLISH_CODE,
    PORTUGUESE_CODE,
    ROMANIAN_CODE,
    RUSSIAN_CODE,
    SPANISH_CODE,
    TATAR_CODE,
    TURKISH_CODE,
    UKRAINIAN_CODE,

    AZERBAIJANI_CODE,
    KIRGHIZ_CODE,
    MONGOLIAN_CODE,
    SWEDISH_CODE,
    TAJIK_CODE,
    TAGALOG_CODE,
    TURKMEN_CODE,
    UZBEK_CODE,

    [TRegisterCode()],
))


def generate(output_file):
    generate_code(CODE_BUILDERS, output_file)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output_filename', help='path where to store result.', type=lambda name: open(name, 'w'), default=sys.stdout, nargs='?')
    args = parser.parse_args()

    generate(args.output_filename)


if __name__ == '__main__':
    main()
