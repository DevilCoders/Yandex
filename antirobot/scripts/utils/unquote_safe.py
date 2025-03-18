# -*- coding: utf-8 -*-

import urllib
import types


def DoSafeUnquote(s, unquoteFunc):
    """First unquote s with unquoteFunc.
    Converts the result to unicode type assuming source string is encoded as UTF-8 or Windows-1251.
    In case of decode errors, just convert source string into unicode string that looks like origin string.
    """

    EscapeToUnicode = lambda x: x.encode('string_escape').decode('utf-8')

    try:
        unquoted = unquoteFunc(s)
        if isinstance(unquoted, types.UnicodeType):
            return unquoted

        try:
            return unquoted.decode('utf-8')
        except:
            pass

        try:
            return unquoted.decode('cp1251')
        except:
            pass

        return EscapeToUnicode(unquoted)
    except:
        return s if isinstance(s, types.UnicodeType) else EscapeToUnicode(s)


def SafeUnquote(anyString):
    """Unquote anyString using urllib.unquote and convert the result into unicode."""

    return DoSafeUnquote(anyString, urllib.unquote)


def SafeUnquotePlus(anyString):
    """Unquote anyString using urllib.unquote_plus and convert the result into unicode."""

    return DoSafeUnquote(anyString, urllib.unquote_plus)
