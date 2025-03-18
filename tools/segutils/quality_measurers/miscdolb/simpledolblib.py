#!/usr/bin/env python
# -*- coding: utf8 -*-

import os
import tarfile
import gzip
import urllib2
import random
from utils import *
from sengines import *
from simplemaplib import *

class Tar:
    def __init__(self, fname, mapping = None):
        self.__tar = tarfile.TarFile(fname, "w")
        self.__dir = os.path.dirname(os.path.abspath(fname))
        self.__counter = mp.RawValue('i', 0)
        self.__mapping = open(mapping, "w") if mapping else None
        self.__mutex = mp.RLock()
    def MakeFname(self):
        with self.__mutex:
            old = self.__counter.value
            self.__counter.value += 1
            return str(old) + '.gz'
    def Write(self, line, doc):
        #fname is unique between processes, neither race conditions nor locks are possible
        fname = self.MakeFname()    

        gfile = gzip.GzipFile(fname, "w")
        gfile.write(doc if self.__mapping else '\n'.join([line, doc]))
        gfile.close()

        with self.__mutex:
            if(self.__mapping):
                self.__mapping.write("%s\t%s\n" % (fname, line))
                self.__mapping.flush()
            self.__tar.add(fname)
            self.__tar.fileobj.flush()

        os.unlink(fname)

    def Close(self):
        with self.__mutex:
            self.__tar.close()
            self.__tar.fileobj.close()
            if(self.__mapping):
                self.__mapping.close()
        
def ProcessEntry(line, tarobj):
    url = ""
    line = line.strip()

    if (not line):
        return
    
    tokens = line.split('\t', 2)
    url, other = tokens[0], (tokens[1] if len(tokens) > 1 else "") 
    PrintLog('pid\t%d\tDown\t%s' % (mp.current_process().pid, line))
    url = url if url.startswith("http://") else "http://" + url
    
    try:
        doc = urllib2.urlopen(urllib2.Request(url, headers = HEADERS), timeout = 15)
        if doc.headers.subtype != 'html' and doc.headers.subtype != 'plain':
            return
        doc = doc.read().strip()
    except:
        PrintLog('pid\t%d\tFail\t%s\t%s' % (mp.current_process().pid, line, sys.exc_value))
        return

    PrintLog('pid\t%d\tSave\t%s' % (mp.current_process().pid, line))
    tarobj.Write(line, doc)
    PrintLog('pid\t%d\tOK..\t%s' % (mp.current_process().pid, line))

def RandomQuery():
    try:
        r = Zen(1)
        i = r.find("input", "zen")
        if not i:
            return None
        return unicode(i["value"]) or None
    except:
        PrintLog(sys.exc_value)
        return None


def RandomUrl():
    try:
        r = Zen(30)
        l = r.find("ol", "results")
        lis = [x.find("a") for x in l.findAll("li", recursive = False)] if l else None
        lis = [x for x in lis if x]
        if not lis:
            return None
        ri = random.randint(0, len(lis) - 1)
        return lis[ri]["href"] or None
    except:
        PrintLog(sys.exc_value)
        return None

def PrintRandomUrl(r):
    url = RandomUrl()
    if url:
        Print(url)

                
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print "nprocs nurls"
    Map(int(sys.argv[1]), PrintRandomUrl, xrange(0, int(sys.argv[2])))
