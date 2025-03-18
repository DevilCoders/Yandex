#!/usr/bin/python
# -*- coding: utf-8 -*-

from subprocess import Popen, PIPE
from log import *
from urlparse import urlparse
from urllib2 import urlopen
from PIL import Image
from StringIO import StringIO
from sys import stderr

def run(args):
    log=open(config.log, 'a')
    Popen(args, shell=True, stderr=log).wait()
    log.close()

def patchPoint(filename, metrics):
    tmp = filename + ".tmp"
    run("head -n -1 " + filename + " > " + tmp)
    point = open(tmp, "a")
    for k, v in metrics.iteritems():
        print >> point, "<point system=\"%s\" value=\"%s\" size=\"%s\"/>" % (k, v[0], v[1])
    point.write("</day>")
    run("mv %s %s" % (tmp, filename))

FAV_ICONS_HOST = 'https://favicon.yandex.net/favicon'
FAV_ICONS_SIZE = 16
FAV_ICONS_REQLEN = 1000

class FavIconRequest:
    text = FAV_ICONS_HOST
    hosts = {} # host -> [count, order index]

    def __init__(self):
        self.text = FAV_ICONS_HOST
        self.hosts = {} # can affect all instances if not initialized

    def push(self, host):
        if host in self.hosts:
            self.hosts[host][0] += 1
            return True
        req = '/'.join([self.text, host])
        if len(req) > FAV_ICONS_REQLEN:
            return False
        self.hosts[host] = [1, len(self.hosts)]
        self.text = req
        return True

    def drop(self):
        self.text = FAV_ICONS_HOST
        self.hosts.clear()

    def empty(self):
        return len(self.hosts) == 0

def getFavIcons(request):
    try:
        data = urlopen(request.text).read()
    except:
        return {}

    image = Image.open(StringIO(data))
    if image.size[1] != len(request.hosts) * FAV_ICONS_SIZE:
        log("Incorrect favicons answer: %s\nImage height %d, need - %d" %
            (request.text, image.size[1], len(request.hosts) * FAV_ICONS_SIZE))
        return {}

    pixel = image.load()
    def isFavIcon(y_offset):
        for y in range(15):
            for x in range(15):
                p = pixel[x, y_offset + y]
                if p[0] != 255 or p[1] != 255 or p[2] != 255:
                    return True
        return False

    res = {} # host -> [how many times host appears, host has non empty favicon]
    for k, v in request.hosts.iteritems():
        res[k] = (v[0], isFavIcon(v[1] * FAV_ICONS_SIZE))
    return res


def calcFavIcons(serpFilename):
    args = "zgrep -A 1 '<serp-component type=\"SEARCH_RESULT\"' %s | grep '<page-url>' | sed -r 's/<\/?page-url>//g' | sed -r 's/ +//g'" % (serpFilename)

    log=open(config.log, 'a')
    pipe = Popen(args, shell=True, stderr=log, stdout=PIPE)
    line = ' '
    cache = {}      # host -> has non blank favicon flag
    answer = [0, 0] # hasFavIconCount, nonEmptyCount
    request = FavIconRequest()

    def applyResults(res):
        for k, v in res.iteritems():
            if v[1]:
                answer[0] += v[0]
            answer[1] += v[0]
            cache[k] = v[1]

    while line:
        line = pipe.stdout.readline()
        url = line[:-1]
        if url:
            host = urlparse(url).hostname
            if host in cache:
                answer[0] += cache[host]
                answer[1] += 1
            else:
                if request.push(host) == False:
                    applyResults(getFavIcons(request))
                    request.drop()
                    request.push(host)
    if not request.empty():
        applyResults(getFavIcons(request))

    log.close()
    rate = float(answer[0]) / answer[1] if answer[1] else 0
    return {"favIconsRate": (rate, answer[1])}
