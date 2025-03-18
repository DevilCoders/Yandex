#!/usr/bin/env python
# -*- coding: utf8 -*-

from BeautifulSoup import BeautifulSoup
from urllib2 import urlopen, Request, URLError
from urllib import urlencode

import random
import warnings
import sys
import re

def ToUnicode(obj, encoding=sys.getdefaultencoding(), errors="replace"):
    if isinstance(obj, unicode):
        return obj
    if hasattr(obj, "__unicode__"):
        return unicode(obj)
    return str(obj).decode(encoding, errors) 

class TEncodingSafe(object):
    def __str__(self):
        return unicode(self).encode(sys.getdefaultencoding(), "xmlcharrefreplace")
    def __unicode__(self):
        return object.__str__(self)
    
HEADERS = {
    "User-Agent" : "Mozilla/5.0 (X11; U; Linux i686; en-RU; rv:1.8.1.13) Gecko/20080325 Ubuntu/7.10 (gutsy) Firefox/2.0.0.13",
    "Accept" : "text/html",
    "Accept-Language" : "ru,en-us;q=0.7,en;q=0.3",
    "Accept-Charset" : "ISO-8859-1,utf-8;q=0.7,*;q=0.7",
}

_CHARSET_RE = re.compile("(?i)^.*?charset[ \t]*=[ \t]*([a-z0-9_-]*).*$")

def HttpReadRaw(url, host=None):
    d = urlopen(Request(url=url, headers=HEADERS, origin_req_host=host))
    enc = None

    if d.headers.plisttext:
        enc = _CHARSET_RE.sub("\\g<1>", d.headers.plisttext)

    if d.headers.subtype != 'html' and d.headers.subtype != 'plain':
        return None, None

    d = d.read().strip()

    return (d, enc)
    
def HttpRead(url, host=None, parse=True):
    doc, enc = HttpReadRaw(url, host)
    return BeautifulSoup(doc)
    
def Zen(numdoc=30):
    r0 = random.getrandbits(random.randint(1, 256))
    r1 = random.getrandbits(random.randint(1, 256))

    return HttpRead( "http://www.yandex.ru/yandsearch?random=%s&stype=www&randomtext=%s&numdoc=%s&i-m-a-hacker=1" 
        % ( r0, r1, numdoc))
        
__all__ = [BeautifulSoup.__name__, ToUnicode.__name__, TEncodingSafe.__name__, "HEADERS", 
    HttpReadRaw.__name__, HttpRead.__name__, Zen.__name__]

if __name__ == "__main__":
    print Zen()
