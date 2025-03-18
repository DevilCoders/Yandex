#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __init__ import _restorePackageStructure
_restorePackageStructure()

import downloader

import gzip
import urllib
import hashlib
import re
from urllib2 import urlopen, Request
from sys import stdin, stderr
from codecs import decode
from datetime import datetime
from os import remove
from StringIO import StringIO
from optparse import OptionParser
from snipmetricsview.snips_parser import SnippetsParser
from options import get_option_value
from common_utils.stringUtils import toStr
from snipmetricsview.utils import getRandomFileName


HEADERS = {
    "User-Agent": "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.13) Gecko/20080325 Ubuntu/7.10 (gutsy) Firefox/2.0.0.13",
    "Accept": "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5",
    "Accept-Language": "ru,en-us;q=0.7,en;q=0.3",
    "Accept-Encoding": "gzip,deflate",
    "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7",
    "Keep-Alive": "300",
    "Connection": "keep-alive"
}

_CHARSET_RE = re.compile("(?i)^.*?charset[ \t ]*=[ \t ]*([a-z0-9_-]*).*$")


def httpReadRaw(url, host=None):
    d = urlopen(Request(url=url, headers=HEADERS, origin_req_host=host))
    enc = None
    if d.headers.plisttext:
        enc = _CHARSET_RE.sub("\\g<1>", d.headers.plisttext)
    if d.headers.subtype != 'html' and d.headers.subtype != 'plain':
        return None, None
    if "content-encoding" in d.headers.dict and d.headers.dict['content-encoding'] == 'gzip':
        d = gzip.GzipFile(fileobj=StringIO(d.read()))
        d = d.read().strip()
    return (d, enc)


def getOptionParser():
    parser = OptionParser()
    parser.add_option("-u", "--url", dest="searchUrl", help="Url of search engine", metavar="URL", default="http://www.yandex.ru/")
    parser.add_option("-g", "--cgi", dest="cgiParams", help="CGI params of search engine", metavar="CGI", default="")
    parser.add_option("-c", "--cacheUrl", dest="docsCacheUrl", help="Url of the document cache. Use the %s - placeholder for document url", metavar="CACHEURL", default="")
    parser.add_option("-p", "--progressFilePath", dest="progressFilePath", help="Path to the file where to write progress", metavar="PROGRESSFILE", default="")
    parser.add_option("-n", "--dumpName", dest="dumpName", help="Dump name for log", default="noname_dump")
    return parser

if __name__ == "__main__":
    parser = getOptionParser()
    (options, args) = parser.parse_args()

    print >> stderr, "[Begin] download serp for dump: %s" % options.dumpName
    searchUrl = options.searchUrl.replace("http://", "").replace("www.", "").strip("/")
    docCacheUrl = options.docsCacheUrl
    progressFile = options.progressFilePath
    if docCacheUrl != "":
        docCacheUrl = "http://" + docCacheUrl.replace("http://", "")
    docCacheDir = get_option_value("documentCacheDir")
    queries = []
    for line in stdin:
        (query, region) = line.strip().split("\t")
        q = {"text": query, "region": int(region)}
        queries.append(q)
    if len(queries) == 0:
        exit()

    dl = downloader.Downloader(login="my34", results_count=30, pages_count=1,
                               host=searchUrl, params_string=options.cgiParams)

    for query in queries:
        dl.add_query(downloader.Query(query))

    tempFileName, fileObj = getRandomFileName()
    if dl.download(tempFileName):
        parser = SnippetsParser(tempFileName, SnippetsParser.ParserFileFormats["XmlDownloaderFormat"])
        res = parser.ParseSnippets()
        print >> stderr, "[%s] Parsed results count: %s" % (options.dumpName, len(res))
        for (query, url, title, snippet) in res:
            docCacheFileName = ""
            if len(docCacheUrl) > 10:
                if url.find("http://") < 0:
                    url = "http://" + url
                if url.find("/", 8) < 0:
                    url = url + "/"
                temp = docCacheUrl % urllib.quote(url)
                failed = False
                try:
                    (document, enc) = httpReadRaw(temp)
                except:
                    failed = True
                if not failed and document and len(document) > 0:
                    docCacheFileName = docCacheDir + str(hashlib.md5(url + str(datetime.now())).hexdigest()) + ".gz"
                    docOut = gzip.open(docCacheFileName, "w", 9)
                    docOut.write(document)
                    docOut.close()

            clean = lambda x: toStr(decode(x.replace("\n", ""), "utf8"), "utf8")
            print "\t".join([query, url, docCacheFileName, clean(title), clean(snippet)])
    else:
        print >> stderr, "Fail download serp for dump: %s" % options.dumpName
    print >> stderr, "Remove: " + tempFileName
    remove(tempFileName)
    print >> stderr, "[End] download serp for dump: %s" % options.dumpName
