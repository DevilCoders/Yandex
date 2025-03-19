#!/usr/bin/env python
# -*- coding: utf-8 -*-

Latin_trans = {
    u"À": "a", u"Á": "a", u"Â": "a", u"Ã": "a", u"Ä": "a", u"Å": "a", u"Æ": "ae",
    u"Ç": "c", u"È": "e", u"É": "e", u"Ê": "e", u"Ë": "e", u"Ì": "i", u"Í": "i",
    u"Î": "i", u"Ï": "i", u"Ð": "d", u"Ñ": "n", u"Ò": "o", u"Ó": "o", u"Ô": "o",
    u"Õ": "o", u"Ö": "o", u"×": "x", u"Ø": "o", u"Ù": "u", u"Ú": "u", u"Û": "u",
    u"Ü": "u", u"Ý": "y", u"Þ": "th", u"ß": "ss", u"à": "a", u"á": "a", u"â": "a",
    u"ã": "a", u"ä": "a", u"å": "a", u"æ": "ae", u"ç": "c", u"è": "e", u"é": "e",
    u"ê": "e", u"ë": "e", u"ì": "i", u"í": "i", u"î": "i", u"ï": "i", u"ð": "d",
    u"ñ": "n", u"ò": "o", u"ó": "o", u"ô": "o", u"õ": "o", u"ö": "o", u"ø": "o",
    u"ù": "u", u"ú": "u", u"û": "u", u"ü": "u", u"ý": "y", u"þ": "th", u"ÿ": "y",
    u"Ā": "a", u"ā": "a", u"Ă": "a", u"ă": "a", u"Ą": "a", u"ą": "a", u"Ć": "c",
    u"ć": "c", u"Ĉ": "c", u"ĉ": "c", u"Ċ": "c", u"ċ": "c", u"Č": "c", u"č": "c",
    u"Ď": "d", u"ď": "d", u"Đ": "d", u"đ": "d", u"Ē": "e", u"ē": "e", u"Ĕ": "e",
    u"ĕ": "e", u"Ė": "e", u"ė": "e", u"Ę": "e", u"ę": "e", u"Ě": "e", u"ě": "e",
    u"Ĝ": "g", u"ĝ": "g", u"Ğ": "g", u"ğ": "g", u"Ġ": "g", u"ġ": "g", u"Ģ": "g",
    u"ģ": "g", u"Ĥ": "h", u"ĥ": "h", u"Ħ": "h", u"ħ": "h", u"Ĩ": "i", u"ĩ": "i",
    u"Ī": "i", u"ī": "i", u"Ĭ": "i", u"ĭ": "i", u"Į": "i", u"į": "i", u"İ": "i",
    u"ı": "i", u"Ĳ": "ij", u"ĳ": "ij", u"Ĵ": "j", u"ĵ": "j", u"Ķ": "k", u"ķ": "k",
    u"ĸ": "k", u"Ĺ": "l", u"ĺ": "l", u"Ļ": "l", u"ļ": "l", u"Ľ": "l", u"ľ": "l",
    u"Ŀ": "l", u"ŀ": "l", u"Ł": "l", u"ł": "l", u"Ń": "n", u"ń": "n", u"Ņ": "n",
    u"ņ": "n", u"Ň": "n", u"ň": "n", u"ŉ": "'n", u"Ŋ": "ng", u"ŋ": "ng",
    u"Ō": "o", u"ō": "o", u"Ŏ": "o", u"ŏ": "o", u"Ő": "o", u"ő": "o", u"Œ": "oe",
    u"œ": "oe", u"Ŕ": "r", u"ŕ": "r", u"Ŗ": "r", u"ŗ": "r", u"Ř": "r", u"ř": "r",
    u"Ś": "s", u"ś": "s", u"Ŝ": "s", u"ŝ": "s", u"Ş": "s", u"ş": "s", u"Š": "s",
    u"š": "s", u"Ţ": "t", u"ţ": "t", u"Ť": "t", u"ť": "t", u"Ŧ": "t", u"ŧ": "t",
    u"Ũ": "u", u"ũ": "u", u"Ū": "u", u"ū": "u", u"Ŭ": "u", u"ŭ": "u", u"Ů": "u",
    u"ů": "u", u"Ű": "u", u"ű": "u", u"Ų": "u", u"ų": "u", u"Ŵ": "w", u"ŵ": "w",
    u"Ŷ": "y", u"ŷ": "y", u"Ÿ": "y", u"Ź": "z", u"ź": "z", u"Ż": "z", u"ż": "z",
    u"Ž": "z", u"ž": "z",
}

Cyrillic_trans = {
    u"Ґ": "g", u"Ё": "yo", u"Є": "e", u"Ї": "yi", u"І": "i",
    u"А": "a", u"Б": "b", u"В": "v", u"Г": "g",
    u"Д": "d", u"Е": "e", u"Ж": "zh", u"З": "z", u"И": "i",
    u"Й": "j", u"К": "k", u"Л": "l", u"М": "m", u"Н": "n",
    u"О": "o", u"П": "p", u"Р": "r", u"С": "s", u"Т": "t",
    u"У": "u", u"Ф": "f", u"Х": "kh", u"Ц": "c", u"Ч": "ch",
    u"Ш": "sh", u"Щ": "shh", u"Ъ": "'", u"Ы": "y", u"Ь": "",
    u"Э": "eh", u"Ю": "yu", u"Я": "ya", u"і": "i",
    u"ґ": "g", u"ё": "yo", u"є": "e",
    u"ї": "yi", u"а": "a", u"б": "b",
    u"в": "v", u"г": "g", u"д": "d", u"е": "e", u"ж": "zh",
    u"з": "z", u"и": "i", u"й": "j", u"к": "k", u"л": "l",
    u"м": "m", u"н": "n", u"о": "o", u"п": "p", u"р": "r",
    u"с": "s", u"т": "t", u"у": "u", u"ф": "f", u"х": "kh",
    u"ц": "c", u"ч": "ch", u"ш": "sh", u"щ": "shh", u"ъ": "'",
    u"ы": "y", u"ь": "", u"э": "eh", u"ю": "yu", u"я": "ya",
}

Greek_trans = {
    u'α': 'a',
    u'η': 'h',
    u'ν': 'n',
    u'τ': 't',
    u'β': 'b',
    u'θ': 'th',
    u'ξ': 'x',
    u'υ': 'y',
    u'γ': 'g',
    u'ι': 'i',
    u'ο': 'o',
    u'φ': 'f',
    u'δ': 'd',
    u'κ': 'k',
    u'π': 'p',
    u'χ': 'ch',
    u'ε': 'e',
    u'λ': 'l',
    u'ρ': 'r',
    u'ψ': 'ps',
    u'ζ': 'z',
    u'μ': 'm',
    u'σ': 's',
    u'ω': 'w',
    u'Θ': 'th',
    u'Ξ': 'x',
    u'Γ': 'g',
    u'Φ': 'f',
    u'Δ': 'd',
    u'Π': 'p',
    u'Λ': 'l',
    u'Ρ': 'r',
    u'Ψ': 'ps',
    u'Σ': 's',
    u'Ω': 'w'
}


def gen_cpp_syntax_array(lang_name, entries):
    res = 'static const TCharTable %s = {\n' % lang_name
    for e in entries:
        res += '    {%d, "%s"},\n' % (e[0], e[1])
    res += '};'
    return res


def print_lang_map(lang_name, lm):
    tss = list()
    for k,v in lm.iteritems():
        tss.append((ord(k), v))
    tss = sorted(tss)
    cpp_line = gen_cpp_syntax_array(lang_name, tss)
    print cpp_line


def main():
    print_lang_map("Latin", Latin_trans)
    print_lang_map("Cyrillic", Cyrillic_trans)
    print_lang_map("Greek", Greek_trans)


if __name__ == '__main__':
    main()
