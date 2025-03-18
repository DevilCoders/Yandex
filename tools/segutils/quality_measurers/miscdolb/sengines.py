#!/usr/bin/env python
# -*- coding: utf8 -*-

from BeautifulSoup import BeautifulSoup, Tag
from sys import getdefaultencoding
from utils import *
from urllib import urlencode, unquote

import re

#===========
#public:
#===========
GOOGLE = "google"
YAHOO = "yahoo"
YANDEX = "yandex"
ZEN = "zen"

    
class TSerpElement(TEncodingSafe):
    url = "",
    title = "",
    body = "",
    seType = None,

    def __init__(self, url="", title="", body="", type=None, query=""):
        self.url = unicode(url)
        self.title = ToUnicode(title, errors="ignore")
        self.body = ToUnicode(body, errors="ignore")
        self.seType = type
        self.query = ToUnicode(query, errors="ignore")

    def __unicode__(self):
        return u"type: %s\nquery: %s\nurl: %s\ntitle: %s\nbody: %s" % (
            self.seType, self.query, self.url, self.title, self.body
        )


def GoogleSerp(query, numdoc=60):
    return  GoogleParser(
        HttpRead("http://www.google.ru/search?" + GoogleUrlencode(query, numdoc), "www.google.ru")
        , GOOGLE, query
    )


YANDEX_PURIFICATION_PARAMS = "&no_mangle_yaca_snippets=da&no_mangle_slovari_snippets=da&no_mangle_remove_links=da"


def YandexSerp(query, numdoc=60, yandsite="www.yandex.ru/yandsearch/"):
    return YandexParser(
        HttpRead("http://" + yandsite + "?" + YandexUrlencode(query, numdoc), "www.yandex.ru")
        , YANDEX, query
    )


def ZenSerp(query="", numdoc=60):
    return YandexParser( Zen(numdoc) , ZEN, None)


def YahooSerp(query, numdoc=60):
    return YahooParser(
        HttpRead("http://search.yahoo.ru/search?" + YahooUrlencode(query, numdoc), "www.yahoo.ru")
        , YAHOO, query
    )


def GoogleUrlencode(query, numdoc=60):
    return urlencode({"num":numdoc, "q" : query})


def YandexUrlencode(query, numdoc=60):
    return urlencode({"numdoc":numdoc, "text" : query})


def YahooUrlencode(query, numdoc=60):
    return urlencode({"n":numdoc, "p" : query})


def GoogleParser(html, type, query):
    res = []

    if not html:
        return res
    
    serp, enc = html.find("div", id="res"), "utf8"

    if not serp:
        return res
        
    unwanted = set(["br", "span", "cite", "nobr", "div", "script", "table"])

    for div in serp.findAll("li", "g"):
        a = div.find("h3", "r")
        a = a.find("a") if a else None
        b = None

        if a:
            href = a["href"]
            title = a.renderContents()
            b = div.find("div", "s")

        if b:
            hasGoo = True
            while hasGoo:
                hasGoo = False

                for goo in b.contents:
                    if isinstance(goo, Tag) and str(goo.name) in unwanted:
                        goo.extract()
                        hasGoo = True

            body = b.renderContents() if b else _NO_TEXT_MARKER

            res.append(TSerpElement(href, title, body, type, query))

    return res


def YandexParser(html, type, query):
    res = []

    if not html:
        return res

    serp, enc = html.find("body"), "utf8"

    if not serp:
        return res

    if not query:
        query = serp.find("input", "zen")
        if query:
            query = query["value"]
        if not query:
            return []

    serp = serp.find("ol", "results")

    if not serp:
        return res

    for li in serp.findAll("li", recursive=False):
        a = li.find("a")
        if a:
            bb = li.findAll("div", "text", recursive=False)
            b = "<br>".join([x.renderContents() for x in bb])
            res.append(TSerpElement( a["href"], a.renderContents(), b, type, query ))

    return res


def YahooParser(html, type, query):
    res = []

    if not html:
        return res
    
    serp, enc = html.find("div", id="web"), "utf8"

    if not serp:
        return res
        
    for li in serp.findAll("li"):
        a = li.find("a")
        if a:
            b = li.find("div", "abstr")
            u = a["href"]
            u = u[u.find("EXP="):]
            u = unquote(u[u.find("http"):])
            res.append(
                TSerpElement( u, a.renderContents(), b.renderContents() if b else _NO_TEXT_MARKER, type, query ))
    return res


SERPS = {
    GOOGLE : GoogleSerp,
    YAHOO : YahooSerp,
    YANDEX : YandexSerp,
    ZEN : ZenSerp
}


__all__ = ("GOOGLE", "YAHOO", "YANDEX", "SERPS", TSerpElement.__name__, ZenSerp.__name__
    , GoogleSerp.__name__, GoogleUrlencode.__name__, GoogleParser.__name__
    , YandexSerp.__name__, YandexUrlencode.__name__, YandexParser.__name__
    , YahooSerp.__name__, YahooUrlencode.__name__ , YahooParser.__name__ )

#============
#private:
#============

_NO_TEXT_MARKER = u""

if __name__ == "__main__":
    z = ZenSerp(numdoc=1)[0]

    query = z.query

    print query

    print
    print z

    gserp = GoogleSerp(query, 1)
    yserp = YahooSerp(query, 1)

    if gserp:
        print
        print gserp[0]
        
    if yserp:
        print
        print yserp[0]
    

