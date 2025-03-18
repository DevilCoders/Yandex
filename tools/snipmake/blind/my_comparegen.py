#!/usr/bin/env python2.5

import sys
import snippet
import codecs
import tplcore
import cgentpl
import random

#sys.stdin = codecs.getreader("utf-8")(sys.stdin)

def run(snips, filename, log):
    print >>log, u"Start "
    xmlString = u""
    f = codecs.open(filename, "r", "utf-8")
    for line in f:
        if line.find(u"<queries>") != -1:
            continue
        if line.find(u"</queries>") != -1:
            continue
        xmlString += unicode(line)
        if line.find(u"</snippet>") != -1:
            try:
                snip = snippet.SnippetIO.readString(xmlString.encode(u"utf-8"))[0]
                snips[unicode(snip.query) + unicode(snip.url)] = snip
            except Exception , ex :
                print >>log, ex
            xmlString = u""
    f.close()
    print >>log, u"Done "
    print >>log, len(snips)

def cmp(snippets1, snippets2, log):
    print >>log, u"Start cmp"
    print >>log, len(snippets1), len(snippets2)
    count = 0
    output = codecs.open("html/" + sys.argv[3] + "_" + str(count / 70) + ".html", "w", "utf-8")
    output.write(tplcore.render(cgentpl.header, {"eq": True, "rnd": True, "total" : 70} ))
    for q1, snip1 in snippets1.iteritems():
        try:
            snip2 = snippets2[q1]
            if snip1.snippet != snip2.snippet:
                output.write(tplcore.render(cgentpl.queryRow, {"q": snip1.query}))
                #output.write(tplcore.render(cgentpl.queryRow, {"q": q1[:q1.find(" <<url")]}))
                n = random.random()
                #print n
                if n < 0.5 :
                    output.write(tplcore.render(cgentpl.snipRow, {"snipTitle1": snip2.title, "snipTitle2": snip1.title, "snipHeadline1": snip2.headline, "snipHeadline2": snip1.headline, "snipTxt1": snip2.snippet, "snipTxt2": snip1.snippet, "snipUrl1": snip2.url, "snipUrl2": snip1.url, "left": "s2", "right": "s1", "num": count % 70}))
                else:
                    output.write(tplcore.render(cgentpl.snipRow, {"snipTitle1": snip1.title, "snipTitle2": snip2.title, "snipHeadline1": snip1.headline, "snipHeadline2": snip2.headline ,"snipTxt1": snip1.snippet, "snipTxt2": snip2.snippet, "snipUrl1": snip1.url, "snipUrl2": snip2.url, "left": "s1", "right": "s2", "num": count % 70}))

                   #output.write(tplcore.render(cgentpl.snipRow, {"snipTitle1": snip1.title, "snipTitle2": snip2.title, "snipTxt1": snip1.snippet, "snipTxt2": snip2.snippet, "snipUrl1": snip1.url, "snipUrl2": snip2.url, "left": "s1", "right": "s2", "num": count % 70}))
                count += 1


               #data.write(q1)
                #print >>log, u"====\n",q1,u"\n", snip1, u"\n", snip2 
            #else:
                #print >>log, u"====\n", snip1, u"\n", snip2 
                #sys.stdout.write(u"===\n" + snip1 + u"\n\n" + snip2 +u"\n\n")
        except Exception, ex:
            #print >>log, u"====\n", snip1, u"\n", snip2 
            #print >>sys.stderr, snip1
            pass
            #print >>log, ex
        if count % 70 == 0 :
            output.write(tplcore.render(cgentpl.footer, {}))
            output.close()
            output = codecs.open("html/" + sys.argv[3] + "_" + str(count / 70) + ".html", "w", "utf-8")
            output.write(tplcore.render(cgentpl.header, {"eq": True, "rnd": True, "total" : 70} ))
 
    output.write(tplcore.render(cgentpl.footer, {}))
    output.close()
    print >>log, count

#log = codecs.open("log", "w", "utf-8")
log = codecs.getwriter("utf-8")(sys.stderr)
#data = codecs.open("cmp_1.4_vs_ML", "w", "utf-8")

snippets1 = dict()
snippets2 = dict()

run(snippets1, sys.argv[1], log)
run(snippets2, sys.argv[2], log)
cmp(snippets1, snippets2, log)

log.close()
