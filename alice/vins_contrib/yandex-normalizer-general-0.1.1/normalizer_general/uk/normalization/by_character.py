#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import g, replace, anyof, word, remove, insert, pp, ss, cost

replace_dict = {
    # https://uk.wikipedia.org/wiki/%D0%A3%D0%BA%D1%80%D0%B0%D1%97%D0%BD%D1%81%D1%8C%D0%BA%D0%B0_%D0%B0%D0%B1%D0%B5%D1%82%D0%BA%D0%B0
    "а": "а",
    "б": "бе",
    "в": "ве",
    "г": "ге",
    "ґ": "ґе",
    "д": "де",
    "е": "е",
    "є": "є",
    "ж": "же",
    "з": "зе",
    "и": "и",
    "і": "і",
    "ї": "ї",
    "й": "йот",
    "к": "ка",
    "л": "ел",
    "м": "ем",
    "н": "ен",
    "о": "о",
    "п": "пе",
    "р": "ер",
    "с": "ес",
    "т": "те",
    "у": "у",
    "ф": "еф",
    "х": "ха",
    "ц": "це",
    "ч": "че",
    "ш": "ша",
    "щ": "ща",
    "ь": "м'який знак",
    "ю": "ю",
    "я": "я",
    "'": "апостроф",

    # https://uk.wikipedia.org/wiki/%D0%9F%D1%83%D0%BD%D0%BA%D1%82%D1%83%D0%B0%D1%86%D1%96%D1%8F
    # https://uk.wikipedia.org/wiki/%D0%A2%D0%B0%D0%B1%D0%BB%D0%B8%D1%86%D1%8F_%D0%BC%D0%B0%D1%82%D0%B5%D0%BC%D0%B0%D1%82%D0%B8%D1%87%D0%BD%D0%B8%D1%85_%D1%81%D0%B8%D0%BC%D0%B2%D0%BE%D0%BB%D1%96%D0%B2
    ".": "крапка",
    ",": "кома",
    ":": "двокрапка",
    "(": "відкриваюча дужка",
    ")": "закриваюча дужка",
    "\"": "лапка",  # should check
    ";": "крапка з комою",
    "!": "знак оклику",
    "?": "знак питання",
    "-": "дефіс",
    "--": "тире",

    "/": "коса риска",
    "\\": "обернена коса риска",
    "±": "плюс-мінус",
    "€": "євро",
    "§": "символ параграфу",
    "©": "копірайт",  # "знак охорони авторського права"
    "®": "символ правової охорони товарного знаку",
    "$": "долар",
    "%": "відсотків",  # "знак відсотка"
    "‰": "проміле",
    "№": "номер",
    "<": "менше",
    ">": "більше",
    "^": "циркумфлекс",
    "|": "вертикальна риска",
    "~": "тильда",
    "_": "підкреслювання",
    "@": "равлик",
    "=": "дорівнює",
    "#": "октоторп",
    "*": "зірочка",

    "0": "нуль",
    "1": "один",
    "2": "два",
    "3": "три",
    "4": "чотири",
    "5": "п'ят",
    "6": "шіст",
    "7": "сім",
    "8": "вісім",
    "9": "дев'ят",
}

replacer = anyof([insert(" ") + replace(xx, replace_dict[xx]) for xx in replace_dict])

convert_by_character = word(
    remove("#[character|") +
    ss(replacer | " ") +
    remove("]"),
    permit_inner_space=True,
    need_outer_space=False
)

cvt = ss(convert_by_character | cost(word(pp(g.any_symb)), 1)) + ss(" ")
