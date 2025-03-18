#!/usr/bin/env python2.7
# encoding: utf-8

#
#   утилита для генерации файлов для мультикритериальной оценки
#   на вход подаются строчки вида:
#            запрос\tурл\tрегион\tзаголовок1\tсниппет1\tзаголовок2\tсниппет2\tзелёный_урл
#

import json
import codecs
import sys
from template import *
from optparse import OptionParser

def replaceSpec(s):
    s = s.replace('&amp;', '&')
    s = s.replace('&quot;', '"')
    s = s.replace('&apos;', "'")
    s = s.replace('&gt;', ">")
    s = s.replace('&lt;', "<")
    return s

class Snippet():
    def __init__(self, url, title, snip, green):
        self.url = url
        self.title = replaceSpec(title)
        self.snip = replaceSpec(snip)
        self.green = green

    def toJSON(self):
        result = ''
        isFirst = True
        for key in self.__dict__.keys():
            if not isFirst:
                result += ','
            isFirst = False
            v = self.__dict__[key]
            result += '"%s":' % key
            result += json.dumps(v, ensure_ascii=False)
        return '{' + result + '}'

class Pair():
    def __init__(self, left, right, query, region, regId):
        self.left = left
        self.right = right
        self.query = query
        self.region = region
        self.regId = regId
        self.marks = [-1 for i in xrange(3)]

    def toJSON(self):
        result = ''
        isFirst = True
        for key in self.__dict__.keys():
            if not isFirst:
                result += ','
            isFirst = False
            v = self.__dict__[key]
            result += '"%s":' % key
            if isinstance(v, Snippet):
                result += v.toJSON()
            else:
                result += json.dumps(v, ensure_ascii=False)
        return '{' + result + '}'

def render(fileName, pairs):
    if len(pairs) == 0:
        return
    out = codecs.open(fileName, 'w', 'utf-8')
    fullnames = [u"Информативность", u"Содержательность", u"Читабельность"]
    names = [u"inf", u"cont", u"read"]

    tscripts = scripts.replace('$$pairs$$', ',\n'.join(pairs))
    tscripts = tscripts.replace('$$fileName$$', fileName.decode('utf-8'))
    tmarks = u""

    for i in xrange(3):
        tmark = mark.replace('$$number$$', str(i))
        tmark = tmark.replace('$$fullname$$', fullnames[i])
        tmark = tmark.replace('$$name$$', names[i])
        tmarks += tmark;

    tbody = body.replace('$$marks$$', tmarks)

    thtml = html.replace('$$styles$$', styles)
    thtml = thtml.replace('$$scripts$$', tscripts)
    thtml = thtml.replace('$$body$$', tbody)
    out.write(thtml)

def parsePair(line, num):
    tokens = line.split('\t')
    if len(tokens) < 8:
        sys.stderr.write('line %d contains less than 8 tokens\n' % num)
        return None

    query = tokens[0]
    url = tokens[1]
    regId = tokens[2]
    title1 = tokens[3]
    snip1 = tokens[4]
    title2 = tokens[5]
    snip2 = tokens[6]
    green = tokens[7]

    left = Snippet(url, title1, snip1, green)
    right = Snippet(url, title2, snip2, green)

    region = u"Анкара"
    if int(regId) == 213:
        region = u"Москва"

    return Pair(left, right, query, region, regId)

parser = OptionParser()
parser.add_option("-n", help="max jobs per file, default=100",
                  action="store", dest="jobsPerFile", default=100)
parser.add_option("-f", help="input file, if not specified - stdin, encoding must be utf-8",
                  action="store", dest="inp", default=None)

(opts, args) = parser.parse_args()
jobsPerFile = int(opts.jobsPerFile)

if opts.inp:
    inp = codecs.open(opts.inp, 'r', 'utf-8')
else:
    inp = codecs.getreader('utf-8')(sys.stdin)

pairs = []
fileNum = 1
lineNum = 1
for line in inp:
    pair = parsePair(line, lineNum)
    lineNum += 1
    if pair:
        pairs.append(pair.toJSON())
    if len(pairs) >= jobsPerFile:
        render(str(fileNum) + ".html", pairs)
        fileNum += 1
        pairs = []

render(str(fileNum) + ".html", pairs)
