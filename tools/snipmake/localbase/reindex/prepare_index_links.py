#!/usr/bin/env python

import os
import sys

PREFIX = "index"
SUFFIXES = ["arc", "dir", "key", "inv", "tag", "tdr"]

def CreateLink(dbDir, dirPath, linkname):
    filename = os.path.join(dbDir, linkname)
    linkname = os.path.join(dirPath, linkname)
    if os.path.islink(linkname):
        os.unlink(linkname)
    os.symlink(filename, linkname)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print "usage: python prepare_index_links.py indexDir reindexDir"
        sys.exit(1)

    dbDir = sys.argv[1]
    reindexDir = sys.argv[2]
    if not os.path.isdir(reindexDir):
        os.mkdir(reindexDir)
    for suffix in SUFFIXES:
        CreateLink(dbDir, reindexDir, PREFIX + suffix)
    urlDatDirPath = os.path.join(reindexDir, "walrus/000")
    if os.path.isdir(urlDatDirPath):
        CreateLink(dbDir, urlDatDirPath, "url.dat")
