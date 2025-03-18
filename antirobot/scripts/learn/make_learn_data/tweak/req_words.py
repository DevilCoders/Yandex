#!/usr/bin/env python

import re
import urllib

EMPTY_REQUEST_SUBST = '1f81bf20_a988_4587_9796_1008153faf87'

reEncX = re.compile(r'\\x([0-9a-f][0-9a-f])', re.IGNORECASE)

def TryUnquotePlus(s):
    if s.find('\\x') >= 0:
        s = reEncX.sub(r'%\1', s)
    s = urllib.unquote_plus(s)
    for cp in ('utf-8', 'cp1251',  'koi8_r'):
        try:
            return s.decode(cp)
        except:
            pass
    return ''

#reWord = re.compile(r'(\"[^\"]*?")|([\,\!\?\(\)\:]+)|([^\s\(\)\:\,\!\?\+]+)')
# word is alphanumeric or url
reWord = re.compile(r'(?:[a-zA-Z0-9]\.[a-zA-Z0-9]|\w)+', re.UNICODE)

def SplitReqWords(req):
    def Select(tuple):
        for i in tuple:
            if len(i) > 0:
                return i.encode('utf-8')
        return EMPTY_REQUEST_SUBST

    if req is None or req == '':
        return (EMPTY_REQUEST_SUBST,)
    else:
        unq = TryUnquotePlus(req)
        words = reWord.findall(unq)
        if len(words) == 0:
            return (EMPTY_REQUEST_SUBST,)
        #return [Select(x) for x in words]
        return tuple(x.encode('utf-8') for x in words)


if __name__ == "__main__":
    import sys

    for i in sys.stdin:
        i = i.strip()
        if not len(i):
            continue

        words = SplitReqWords(i.strip())
        print ' / '.join(words)
