#coding=utf-8

import sys
import codecs

sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

#print >>sys.stdout, u"Warning: скрипт можно использовать только для морд!!!"
#print >>sys.stdout, u"Warning: в yaca_tr есть не морда (youtube.com/music - другие исключения надо прописывать ручками) "

TAB = u"\t"
DEL = u"\x07;"

lang = sys.argv[2]

res = codecs.open("yaca_" + lang + ".snippets", mode = 'w', encoding = 'utf-8')

rowN = 0
url = u""
title = u""
text = u""
https = False

def out(res, url, title, text, https):
    data = u"yaca_" + lang + u"=title=" + title + DEL + u"snippet=" + text
    schema = u"https://" if https else u""
    #if u"/" in url:
    print >>res, url + TAB + data
    if https:
        print >>res, schema + url + TAB + data
    if u"/" not in url:
        print >>res, url + u"/" + TAB + data
        if https:
            print >>res, schema + url + u"/" + TAB + data
    #print >>sys.stderr, url + (u"/" if not url.endswith("/music") else u"")
    #print >>sys.stderr, title
    #print >>sys.stderr, text
    #print >>sys.stderr, u"www." + url + (u"/" if not url.endswith("/music") else u"")
    #print >>sys.stderr, title
    #print >>sys.stderr, text

for line in codecs.open(sys.argv[1], 'r', encoding = 'utf-8'):
    line = line.strip()
    state = rowN % 4
    if state == 0:
        url = unicode(line)
        https = url.startswith(u"https://")
        if (https):
            url = url[len(u"https://"):]
        if url.startswith(u"www."):
            url = url[4:]
        if url.endswith(u"/"):
            url = url[:len(url)]
    if state == 1:
        title = unicode(line)
    if state == 2:
        text = unicode(line)
    if state == 3:
        out(res, url, title, text, https)
        url = u""
        title = u""
        text = u""
        https = False
    rowN = rowN + 1

out(res, url, title, text, https)
res.close()

