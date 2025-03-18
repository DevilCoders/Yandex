#!/usr/bin/env python
# -*- coding: utf8 -*-

from __future__ import with_statement

if __name__ == "__main__":
    from __init__ import _restorePackageStructure
    _restorePackageStructure()

from stringUtils import markSentences, SBR_RE, SPACE_RE, hasText
import re
from htmlentitydefs import name2codepoint
from codecs import encode

#===========
#private:
#===========

_BR = u"(?si)(?:<\\s*br\\s*/?>[\\s\u00A0]*)"
_BLOCKS = "|".join(('title', 'p', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6',
            'div', 'layer', 'multicol', 'listing', 'pre', 'xmp', 'plaintext', 'dir',
            'menu', 'dl', 'dd', 'dt', 'ol', 'ul', 'li', 'table', 'tr', 'td', 'th', 'caption',
            'address', 'blockquote', 'center', 'form', 'fieldset'))

#==========
#public:
#==========

BLOCK_RE = re.compile(r"(?si)</?\s*(?:" + _BLOCKS + ")[^>]*>")
BR_BR_RE = re.compile(_BR + u"{2,}")
BR_RE = re.compile(_BR + "+")
COMM_RE = re.compile(r"(?s)<!--[^>]*-->")
JUNK_RE = re.compile(r"(?si)<\s*(script|style)[^>]*>.*?</\s*\1[^>]*>")
HTML_RE = re.compile(r"(?s)<[^>]*>")
ABR_RE = re.compile(r"(?s)(?:#ABR#\s*)+")

def removeHtml(ds, paragraphs = True):
    ds = COMM_RE.sub("", ds)
    ds = JUNK_RE.sub("", ds)

    if paragraphs:
        ds = BLOCK_RE.sub("<br><br>", ds)
        ds = BR_BR_RE.sub("#ABR#", ds)

    ds = HTML_RE.sub("", ds)
    ds = removeEntities(ds)

    return ds

def removeEntities(ds, fixSpaces = True):
    if not isinstance(ds, unicode):
        raise Exception("Not unicode")

    i0 = 0
    i = _findAmp(i0, ds)
    res = []

    while(i != -1):
        res.append(ds[i0:i])
        ent = _readEntity(i, ds)
        if ent[1] != None:
            res.append(' ' if fixSpaces and ent[1].isspace() else ent[1])
        else:
            res.append(ent[0])
        i0 = i + len(ent[0])
        i = _findAmp(i0, ds)

    res.append(ds[i0:])

    return u"".join(res)

def makeSentences(ts):
    ts = ABR_RE.split(ts)
    res = []
    for t in ts:
        t = markSentences(t)
        t = SBR_RE.split(t)
        t = [x.strip() for x in t if hasText(x)]
        if t:
            res.append(t)
    return res


__all__ = ("BLOCK_RE", "BR_BR_RE", "BR_RE", "COMM_RE", "JUNK_RE", "HTML_RE", "ABR_RE"
    , removeHtml.__name__, removeEntities.__name__, makeSentences.__name__)

#===========
#private:
#===========

def _checkSC(i, n, ds):
    return i < n and ds[i] == ';'

def _findAmp(i, ds):
    n = len(ds)

    if i > n - 4:
        return -1

    while i <= n - 4 and ds[i] != '&':
        i += 1

    return -1 if i > n - 4 else i


def _readHexEntity(i0, ds):
    i = i0 + 3
    n = len(ds)

    while i < n and i < i0 + 32 and (ds[i].isdigit() or 'a' <= ds[i] <= 'f' or 'A' <= ds[i] <= 'F'):
        i += 1

    cp = None

    try:
        cp = unichr(int(ds[i0 + 3:i], 16))
    except ValueError, e:
        pass

    if _checkSC(i, n, ds):
        i += 1

    return (ds[i0:i], cp)

def _readDecEntity(i0, ds):
    i = i0 + 2
    n = len(ds)

    while i < n and i < i0 + 32 and ds[i].isdigit():
        i += 1

    cp = None

    try:
       cp = unichr(int(ds[i0 + 2:i]))
    except ValueError, e:
        pass

    if _checkSC(i, n, ds):
        i += 1

    return (ds[i0:i], cp)

def _readNamedEntity(i0, ds):
    i = i0 + 1
    n = len(ds)

    while i < n and i < i0 + 10 and (ds[i].isalpha() or (i < n - 1 and ds[i] == '-' and not ds[i+1] == '-')):
        i+= 1

    cp = name2codepoint.get(ds[i0+1:i])

    if _checkSC(i, n, ds):
        i += 1

    return (ds[i0:i], unichr(cp) if cp != None else None)

def _readEntity(i, ds):
    if ds[i+1] == '#':
        if ds[i+2] == 'x':
            return _readHexEntity(i, ds)
        return _readDecEntity(i, ds)
    return _readNamedEntity(i, ds)

#=========
#tests:
#=========

if __name__ == "__main__":
    s = u"&#x00061; &amp; A&00000160this&#000000000000009 is no good&tetesstsfsfsf Б&#233;лки знают секретное средство от рака &lt;strong>"

    print encode(s, "utf8")
    print encode(removeEntities(s), "utf8")
