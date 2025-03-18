#!/usr/bin/env python
import sys
import os.path
import hashlib
import heapq
import random
from pyhtml_snapshot import FetchedDoc, AsyncFetchResult, ZoraFetcher, DirectFetcher, WebContentParser, CanRetry, GetEncodingName
from cache import Cache

def Log(s):
    print >>sys.stderr, s
    sys.stderr.flush()

MAX_DEPTH = 3
PRIORITY_DOC_MAX = 0
PRIORITY_DOC_RANGE = 10000
PRIORITY_RESOURCE_MAX = 1000000
PRIORITY_RESOURCE_RANGE = 10000

def EnqueueLinkedUrls(parser, urldata, content, queue, hints, visitedUrls, hops):
    if hops > MAX_DEPTH:
        return

    def urlMapper(url):
        return url.split('\t')[0]

    encodingHint, mimeHint = -1, ''
    if urldata.url in hints:
        encodingHint, mimeHint = hints[urldata.url]
    elif urldata.finalUrl in hints:
        encodingHint, mimeHint = hints[urldata.finalUrl]
    mimeType = urldata.mimeType or mimeHint or ''
    encoding = urldata.encoding
    if encoding < 0:
        encoding = encodingHint
    if mimeType and not mimeType.startswith('text/'):
        return

    parseResult = parser.RewriteContent(content, urldata.finalUrl, mimeType, encoding, urlMapper)
    if parseResult.Error:
        Log('Failed to parse %s: %s' % (urldata.finalUrl, parseResult.ErrorMessage))
        return
    encoding = parseResult.Encoding
    newUrlCount = 0
    for link in parseResult.Links:
        url = link.Url
        if url in visitedUrls:
            continue
        visitedUrls.add(url)
        heapq.heappush(queue, (PRIORITY_RESOURCE_MAX + random.randrange(PRIORITY_RESOURCE_RANGE), url, hops))
        hints[link.Url] = (encoding, link.ContentTypeHint)
        newUrlCount += 1
    if newUrlCount:
        Log('Enqueued %d new URLs, queue size is %d' % (newUrlCount, len(queue)))

TYPE_MAIN_DOC = 0
TYPE_IMG = 1
TYPE_CSS = 2

class TUrlRec(object):
    url = None
    expectedType = None
    depth = 0
    errcode = None
    fileName = None

    def __init__(self, url, expectedType, depth):
        self.url = url
        self.expectedType = expectedType
        self.depth = depth

    def isJunk(self):
        return 'yandex.ru/cycounter' in self.url or 'mc.yandex.ru' in self.url

    def isExpectedMime(self, mime):
        if self.expectedType is None or not mime:
            return True
        elif self.expectedType == TYPE_MAIN_DOC:
            return mime == 'text/html'
        elif self.expectedType == TYPE_CSS:
            return mime == 'text/css'
        elif self.expectedType == TYPE_IMG:
            return mime.startswith('image/')
        else:
            return False

def CheckUrl(url, cache, parser, traversal):
    visited = set()
    inflight = []
    visited.add(url)
    inflight.append(TUrlRec(url, TYPE_MAIN_DOC, 0))
    statTotal = 0
    statGood = 0
    statNotCached = 0
    statFatal = 0
    statNonFatal = 0
    statParserError = 0
    statUnexpectedContent = 0

    while inflight:
        u = inflight.pop()
        if u.isJunk():
            continue
        needParsing = False
        if u.expectedType in (TYPE_CSS, TYPE_MAIN_DOC):
            c = cache.metadata.get(u.url, None)
            statTotal += 1
            if c and c.contentHash:
                u.fileName = c.contentHash
            if not c:
                u.errcode = 'Not cached'
                statNotCached += 1
            elif c.failed and CanRetry(c.httpCode):
                u.errcode = 'Nonfatal error %d: %s' % (c.httpCode, c.errorMessage)
                statNonFatal += 1
            elif c.failed and not CanRetry(c.httpCode):
                u.errcode = 'Fatal error %d: %s' % (c.httpCode, c.errorMessage)
                statFatal += 1
            elif not u.isExpectedMime(c.mimeType):
                u.errcode = 'Unexpected MIME type: ' + str(c.mimeType)
                statUnexpectedContent += 1
            else:
                needParsing = True
        if not needParsing and not u.errcode:
            continue
        traversal.append(u)
        content = cache.ReadContent(c.contentHash)
        if len(content) < 16:
            u.errcode = 'Content is empty'
            statUnexpectedContent += 1
            continue
        
        collected_urls = []
        
        def urlMapper(link):
            fields = link.split('\t')
            link = fields[0]
            mimeHint = ''
            if len(fields) > 1:
                mimeHint = fields[1]
            collected_urls.append((link, mimeHint))
            return link

        parseResult = parser.RewriteContent(content, c.finalUrl, c.mimeType, c.encoding, urlMapper)
        
        if parseResult.Error:
            u.errcode = 'Parser rejected the file'
            statParserError += 1
            continue
        if len(parseResult.RewrittenContent) < len(content) * 0.2:
            u.errcode = 'Damaged by parser (%d -> %d bytes)' % (len(content), len(parseResult.RewrittenContent))
            debugf = open('dmg/' + c.contentHash, 'wb')
            debugf.write(parseResult.RewrittenContent)
            debugf.close()
            statParserError += 1
            continue
        
        statGood += 1

        for nextUrl, mimeHint in collected_urls:
            if nextUrl in visited:
                continue
            expectType = None
            if mimeHint == 'text/css':
                expectType = TYPE_CSS
            visited.add(nextUrl)
            inflight.append(TUrlRec(nextUrl, expectType, u.depth + 1))
    fields = (statTotal, statGood, statNotCached, statFatal, statNonFatal, statParserError, statUnexpectedContent)
    return fields

if __name__ == '__main__':
    def rtfm():
        print >>sys.stderr, 'Usage: %s [cache directory]' % sys.argv[0]
        sys.exit(1)

    if len(sys.argv) > 2:
        rtfm()

    cachedir = '.cache'

    if len(sys.argv) == 2:
        cachedir = sys.argv[1]

    parser = WebContentParser('.')
    cache = Cache(cachedir)
    cache.InitCache()
    cache.LoadCache()
    goodf = open("good_urls.txt", "wb")
    for url in sys.stdin:
        url = url.strip()
        if not url:
            continue
        traversal = []
        fields = CheckUrl(url, cache, parser, traversal)
        if fields[0] == fields[1] and fields[0] >= 1:
            print >>goodf, url
        print "%s\t%s" % (url, '\t'.join([str(field) for field in fields]))
        for u in traversal:
            msg = ('\t' * u.depth) + u.url
            if u.errcode:
                msg += ': ' + u.errcode
            if u.fileName:
                msg += ' (' + u.fileName + ')'
            print msg
    goodf.close()
