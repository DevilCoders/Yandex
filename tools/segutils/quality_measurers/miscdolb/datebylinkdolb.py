#!/usr/bin/env python
# -*- coding: utf8 -*-

import sys
import urllib2
import urllib
import datetime
import xml.sax
import re

from xml.sax.handler import ContentHandler

from dolbsimple import Init, Dolb, PrintLog

class TFoundEvent(Exception):
    pass
class TNotFoundEvent(Exception):
    pass

class TYaHandler (ContentHandler):
    def __init__(self):
        ContentHandler.__init__(self)

    def startElement(self, name, attrs):
        if name == "error" and attrs.get("code", "") == "15":
            raise TNotFoundEvent()
        if name == "results":
            raise TFoundEvent()

def Ask(line):
    try:
        line = line.strip()
        t = line.split()
        if len(t) < 2:
            return
        r = urllib2.urlopen("http://xmlsearch1.yandex.ru/xmlsearch?text=1&qtree=%s" % t[1])
        xml.sax.parse(r, TYaHandler())
    except:
        if isinstance(sys.exc_value, TFoundEvent):
            PrintLog(line + "\t1")
        elif isinstance(sys.exc_value, TNotFoundEvent):
            PrintLog(line + "\t0")
        else:
            PrintLog(sys.exc_info())
            raise

if __name__ == '__main__':
    Init()
    Dolb(int(sys.argv[1]), int(sys.argv[2]), Ask, sys.stdin)
