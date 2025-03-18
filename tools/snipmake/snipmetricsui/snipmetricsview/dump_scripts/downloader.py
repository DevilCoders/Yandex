#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __init__ import _restorePackageStructure
_restorePackageStructure()

import gzip
import urllib, urllib2
import hashlib
import re
from sys import argv, stdin, stdout, stderr
from time import sleep
from codecs import encode, decode
from datetime import datetime
from gzip import GzipFile
from os import remove
from StringIO import StringIO
from optparse import OptionParser
from pysimplesoap.client import SoapClient, DownloaderError
from pysimplesoap.simplexml import SimpleXMLElement
from snipmetricsview.snips_parser import SnippetsParser
from options import get_option_value
from common_utils.stringUtils import toStr
from snipmetricsview.utils import getRandomFileName
from common_utils.html import removeHtml

def getOptionParser():
    parser = OptionParser()
    parser.add_option("-u", "--url", dest = "searchUrl", help = "Url of search engine", metavar = "URL", default = "http://www.yandex.ru/")
    parser.add_option("-g", "--cgi", dest = "cgiParams", help = "CGI params of search engine", metavar = "CGI", default = "")
    parser.add_option("-c", "--cacheUrl", dest = "docsCacheUrl", help = "Url of the document cache. Use the %s - placeholder for document url", metavar="CACHEURL", default="")
    parser.add_option("-p", "--progressFilePath", dest = "progressFilePath", help = "Path to the file where to write progress", metavar="PROGRESSFILE", default="")
    parser.add_option("-n", "--dumpName", dest = "dumpName", help = "Dump name for log", default = "noname_dump")
    return parser

HEADERS = {
    "User-Agent" : "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.13) Gecko/20080325 Ubuntu/7.10 (gutsy) Firefox/2.0.0.13",
    "Accept" : "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5",
    "Accept-Language" : "ru,en-us;q=0.7,en;q=0.3",
    "Accept-Encoding" : "gzip,deflate",
    "Accept-Charset" : "ISO-8859-1,utf-8;q=0.7,*;q=0.7",
    "Keep-Alive" : "300",
    "Connection" : "keep-alive"
}

_CHARSET_RE = re.compile("(?i)^.*?charset[ \t]*=[ \t]*([a-z0-9_-]*).*$")

def httpReadRaw(url, host = None):
    from urllib2 import urlopen, Request, URLError
    from gzip import GzipFile

    d = urlopen(Request(url = url, headers = HEADERS, origin_req_host = host))
    enc = None
    if d.headers.plisttext:
        enc = _CHARSET_RE.sub("\\g<1>", d.headers.plisttext)
    if d.headers.subtype != 'html' and d.headers.subtype != 'plain':
        return None, None
    if "content-encoding" in d.headers.dict and d.headers.dict['content-encoding'] == 'gzip':
        d = GzipFile(fileobj = StringIO(d.read()))
    d = d.read().strip()
    return (d, enc)

def createAndStartCommandXML(queries, host, setype, cgi, description):
    xml = u"""<ns1:createAndStartDownload xmlns:ns1="http://ang-downloader.yandex.ru">
    <ns1:login>my34</ns1:login>
    <ns1:queries xmlns:ns2="http://ang-downloader.yandex.ru/downloader">
    %(queries)s
    </ns1:queries>
    <ns1:host>%(host)s</ns1:host>
    <ns1:type>%(setype)s</ns1:type>
    <ns1:cgi-params><![CDATA[%(cgi)s]]></ns1:cgi-params>
    <ns1:description>%(description)s</ns1:description>
    </ns1:createAndStartDownload>"""

    queryxml = u"""<ns2:query>
    <ns2:region-id>%(region-id)s</ns2:region-id>
    <ns2:text><![CDATA[%(text)s]]></ns2:text>
    </ns2:query>"""

    queriesxml = []
    for query in queries:
        queriesxml.append(queryxml % query)
    queriesxml = u"\n".join(queriesxml)

    xml = xml % {"queries" : queriesxml, "host" : host, "setype" : setype, "cgi" : cgi, "description" : description}
    return xml

def isFinishedCommandXML(creationDate, downloadId):
    xml = u"""<ns1:isFinished xmlns:ns1="http://ang-downloader.yandex.ru">
         <ns1:login>my34</ns1:login>
         <ns1:download-ticket>
         <creation-date xmlns="http://ang-downloader.yandex.ru/downloader">%s</creation-date>
         <download-id xmlns="http://ang-downloader.yandex.ru/downloader">%d</download-id>
         </ns1:download-ticket>
         </ns1:isFinished>
    """ % (creationDate, downloadId)
    return xml

def getCurrentStateCommandXML(creationDate, downloadId):
    xml = u"""<ns1:getCurrentState xmlns:ns1="http://ang-downloader.yandex.ru">
         <ns1:login>my34</ns1:login>
         <ns1:download-ticket>
         <creation-date xmlns="http://ang-downloader.yandex.ru/downloader">%s</creation-date>
         <download-id xmlns="http://ang-downloader.yandex.ru/downloader">%d</download-id>
         </ns1:download-ticket>
         </ns1:getCurrentState>
    """ % (creationDate, downloadId)
    return xml

def getAllSerpsCommandXML(creationDate, downloadId):
    xml = u"""
    <ns1:getAllSerps xmlns:ns1="http://ang-downloader.yandex.ru">
    <ns1:login>my34</ns1:login>
    <ns1:download-ticket>
    <creation-date xmlns="http://ang-downloader.yandex.ru/downloader">%s</creation-date>
    <download-id xmlns="http://ang-downloader.yandex.ru/downloader">%d</download-id>
    </ns1:download-ticket>
    </ns1:getAllSerps>
    """ % (creationDate, downloadId)
    return xml

if __name__ == "__main__":
    parser = getOptionParser()
    (options, args) = parser.parse_args()

    print >> stderr, "[Begin] download serp for dump: %s" % options.dumpName
    searchUrl = options.searchUrl.replace("http://", "").replace("www.","").strip("/")
    cgiParams = options.cgiParams
    docCacheUrl = options.docsCacheUrl
    progressFile = options.progressFilePath
    if docCacheUrl != "":
        docCacheUrl = "http://" + docCacheUrl.replace("http://","")
    docCacheDir = get_option_value("documentCacheDir")
    queries = []
    for line in stdin:
        (query, region) = line.strip().split("\t")
        q = {}
        q["region-id"] = decode(region, "utf8")
        q["text"] = decode(query, "utf8")
        queries.append(q)
    if len(queries) == 0:
        exit()
        
    client = SoapClient(wsdl = "http://serp-downloader.yandex-team.ru/services/soap-downloader?wsdl",
            location = "http://serp-downloader.yandex-team.ru/services/soap-downloader",
            action = "http://serp-downloader.yandex-team.ru/services/soap-downloader",
            ns = "ns1",
            trace = False, exceptions = True)
    try:
        response = client.call('createAndStartDownloadWithDescription', SimpleXMLElement(encode(createAndStartCommandXML(queries, searchUrl, "", cgiParams, options.dumpName.decode("utf8")), "utf8")))
    except DownloaderError, e:
        print >> stderr, e.getXml()
        raise
    downloadTicket = (str(response(tag = "creation-date")), int(response(tag = "download-id")))
    #downloadTicket = ("2011-05-16T14:21:59+03:00", 40813)
    while True:
        response = client.call("getCurrentState", SimpleXMLElement(getCurrentStateCommandXML(downloadTicket[0], downloadTicket[1])))
        progress = float(response(tag = "ns1:grab-state")["ns2:progress"])
        # Print current progress to the file
        if progressFile != "":
            out = open(progressFile, "w")
            print >> out, progress
            out.close()
        response = client.call("isFinished", SimpleXMLElement(isFinishedCommandXML(downloadTicket[0], downloadTicket[1])))
        res = response(tag = "ns1:is-finished")
        if str(res) == "true":
            break
        sleep(10)

    response = client.call("getAllSerpsGZip", SimpleXMLElement(getAllSerpsCommandXML(downloadTicket[0], downloadTicket[1])))
    gzipSerp = StringIO(client.getResponsePart(1))
    serp = GzipFile(fileobj = gzipSerp)

    tempFileName, fileObj = getRandomFileName()
    out = open(tempFileName, "w")
    out.writelines(serp)
    out.close()

    parser = SnippetsParser(tempFileName, SnippetsParser.ParserFileFormats["XmlDownloaderFormat"])
    res = parser.ParseSnippets()
    print >> stderr, "[%s] Parsed results count: %s" % (options.dumpName, len(res))
    for r in res:
        docCacheFileName = ""
        url = r[1]
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
                 docCacheFileName = docCacheDir + str(hashlib.md5(url + str(datetime.now())).hexdigest()) +".gz"
                 docOut  = gzip.open(docCacheFileName,"w", 9)
                 docOut.write(document)
                 docOut.close()
        print "\t".join([r[0], r[1], "", toStr(removeHtml(decode(r[4].replace("\n",""), "utf8")), "utf8"), toStr(removeHtml(decode(r[5].replace("\n",""), "utf8")), "utf8")])
    print >> stderr, "Remove: " + tempFileName
    remove(tempFileName)
    print >> stderr, "[End] download serp for dump: %s" % options.dumpName
    
