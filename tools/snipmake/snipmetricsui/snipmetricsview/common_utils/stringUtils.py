#!/usr/bin/env python
# -*- coding: utf8 -*-

from __future__ import with_statement

if __name__ == "__main__":
    from __init__ import _restorePackageStructure
    _restorePackageStructure()

import codecs
import re
import sys
from unicodedata import normalize

#===========
#public:
#===========

def isRusWord(strng):
    return bool(RUS_WORD_RE.search(strng))

def hasText(strng):
    return bool(HAS_TEXT_RE.search(strng))

def hasLetter(strng):
    return bool(HAS_LETTER_RE.search(strng))

def hasDigit(strng):
    return bool(HAS_DIGIT_RE.search(strng))

def splitClean(string):
    """Функция, разрезающая строку по пробельным символам. Упрощённая.

    @return: список токенов в нижнем регистре без знаков препинания и буквы ё.
    """
    return [ x[1] for x in splitRich(string) if x[1]]

def splitRich(string):
    """Функция, разрезающая строку по пробельным символам.

    @return: список пар вида (u"токен", u"он же в нижнем регистре, без знаков препинания, без ё")
    """
    return [ (
            x.strip(),
            cleanWord(x)
    ) for x in unicode(string).strip().split() ]

def cleanWord(word):
    return word.lower().replace(u"ё", u"е").strip().strip(u"\u00A0.?!:-,;…«»()\"'/<>`~=+*&%#^|\\[]{}’”‘“")

def markSentences(strng):
    return _SENT_RE.sub(SENTENCE_MARK, strng)

def toUnicode(obj, encoding = sys.getdefaultencoding(), errors = "replace"):
    if isinstance(obj, unicode):
        return obj
    if hasattr(obj, "__unicode__"):
        return unicode(obj)
    return str(obj).decode(encoding, errors) 

def toStr(obj, encoding = sys.getdefaultencoding(), errors = "replace"):
    if isinstance(obj, unicode):
        return obj.encode(encoding, errors)
    if hasattr(obj, "__unicode__"):
        return unicode(obj).encode(encoding, errors)
    return str(obj)

def normalizeText(text, encoding = sys.getdefaultencoding(), errors = "replace"):
    text = toUnicode(text, encoding, errors)
    text = normalize("NFKC", text) #todo: dashes, joiners, spaces
    text = JOINERS_RE.sub("", text)
    text = DASHES_RE.sub("-", text)
    text = SPACE_RE.sub(" ", text)
    
    return text.strip()

#Маркер конца предложения
SENTENCE_MARK = r"\g<1>\g<2>#SBR#\g<3>"

SBR_RE = re.compile(r"(?s)(?:#SBR#\s*)+")
SPACE_RE = re.compile(u"(?su)[\\s\u00A0]+")
JOINERS_RE = re.compile(u"(?su)[\ufeff\u200b-\u200f\u2027\u0000\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u0008\u000e-\u001f]+")
DASHES_RE = re.compile(u"(?su)[\u2010-\u2014]")

OUT_IG = codecs.getwriter(sys.getdefaultencoding())(sys.stdout, "ignore")
ERR_IG = codecs.getwriter(sys.getdefaultencoding())(sys.stderr, "ignore")
OUT_REP = codecs.getwriter(sys.getdefaultencoding())(sys.stdout, "replace")
ERR_REP = codecs.getwriter(sys.getdefaultencoding())(sys.stderr, "replace")
OUT_ENT = codecs.getwriter(sys.getdefaultencoding())(sys.stdout, "xmlcharrefreplace")
ERR_ENT = codecs.getwriter(sys.getdefaultencoding())(sys.stderr, "xmlcharrefreplace")
RUS_WORD_RE = re.compile(u"(?su)^[а-яёА-ЯЁ]+$")
HAS_LETTER_RE = re.compile(u"(?su)[а-яёA-ЯЁa-zA-Z]")
HAS_DIGIT_RE = re.compile("(?su)\\d")
HAS_TEXT_RE = re.compile(u"(?su)[а-яёA-ЯЁa-zA-Z]{2,}")

__all__ = (isRusWord.__name__, hasText.__name__, hasLetter.__name__, hasDigit.__name__
    , splitClean.__name__, splitRich.__name__
    , cleanWord.__name__, markSentences.__name__, 
    toUnicode.__name__, toStr.__name__, normalizeText.__name__
    , "SENTENCE_MARK", "SBR_RE", "SPACE_RE"
    , "OUT_IG", "ERR_IG", "OUT_REP", "ERR_REP", "OUT_ENT", "ERR_ENT"
    , "RUS_WORD_RE", "HAS_TEXT_RE" 
)

#=========
#init:
#=========

_SENT_RE = re.compile(u"(?su)([^\\s\u00A0][a-zA-Zа-яёА-ЯЁ0-9\"’'”»)$%#*+|\\]])[\\s\u00A0]*([.!?…])+[\\s\u00A0]*([\\s\u00A0][A-ZА-Я0-9\"‘'“«(+#*$|[]|$)")

#=========
#test:
#=========

if __name__ == '__main__':
    print >> OUT_ENT, u"Тестируем поиск слов"
    s = u"Слово"
    print >> OUT_ENT, s, hasText(s), isRusWord(s)
    s = u"Word"
    print >> OUT_ENT, s, hasText(s), isRusWord(s)
    s = u"Текст?"
    print >> OUT_ENT, s, hasText(s), isRusWord(s)
    s = u"a123"
    print >> OUT_ENT, s, hasText(s), isRusWord(s)
    print >> OUT_ENT, u"Тестируем разбиение на предложения."
    print >> OUT_ENT, markSentences(u"""
      Пешки сашки машки пост.
А ещё тут про компост. М.Ам.Мам. Пенька.
Хорошо сидеть под солнцем!
Петров В.А. пошёл сад.""")
    s = u'Тестируем раз-биение\u00A0и - очистку [|слов|].\n"Ёжики"'
    print >> OUT_ENT, "#".join([x[0] for x in splitRich(s)])
    print >> OUT_ENT, "#".join([x[1] for x in splitRich(s)])
    print >> OUT_ENT, toUnicode(u"тест toUnicode".encode(sys.getdefaultencoding(), "ignore") + "\0\ff")