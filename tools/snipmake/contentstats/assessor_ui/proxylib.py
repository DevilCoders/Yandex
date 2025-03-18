#!/usr/bin/env python
import sys
import cgi
import urllib
from threading import Lock

from pyhtml_snapshot import FetchedDoc, AsyncFetchResult, ZoraFetcher, WebContentParser, CanRetry, GetEncodingName
from cache import Cache, UrlData

#TODO: infinite link sequences are possible; TODO: limit hop distance from the root url

def Log(s):
    print >>sys.stderr, s
    sys.stderr.flush()

class OutputDoc(object):
    content = None
    encoding = None
    mimeType = None
    httpCode = 200

    def MakeContentType(self):
        result = self.mimeType or 'text/plain'
        if self.encoding:
            result += ';charset=' + self.encoding
        return result

    def MakeError(self, message, httpCode = 500):
        if isinstance(message, UrlData):
            urlData = message
            self.content = 'Fetch failed with HTTP code %d: %s' % (urlData.httpCode, urlData.errorMessage);
        else:
            self.content = str(message)
        self.httpCode = httpCode
        self.mimeType = 'text/plain'
        self.encoding = 'utf-8'

class Proxy(object):
    cache = None
    fetcher = None
    parser = None
    counter = 0
    mutex = None
    rewriteMutex = None

    def __init__(self, fetcher, cache, configDir):
        Log('OfflineBrowser init')
        self.cache = cache
        self.fetcher = fetcher
        self.parser = WebContentParser(configDir)
        self.mutex = Lock()
        self.rewriteMutex = Lock()
        Log('OfflineBrowser done initializing')

    def GetUrl(self, url, urlMapper, mimeHint = None, codingHint = -1, offline = False, refresh = False, original = False):
        Log('Request came in for ' + url)
        content = ''
        od = OutputDoc()
        if not refresh:
            urldata = self.cache.metadata.get(url, None)
        else:
            urldata = None

        if urldata:
            Log('Url is in cache')
            if urldata.failed and not CanRetry(urldata.httpCode):
                od.MakeError(urldata)
                return od
        elif not offline:
            Log('Url will be fetched')
            result = AsyncFetchResult()
            with self.mutex:
                self.counter += 1
                reqid = str(self.counter)
            print >>sys.stderr, 'Counter is ' + reqid
            print >>sys.stderr, repr(url)
            sys.stderr.flush()
            self.fetcher.Submit(url, reqid, result)
            while True:
                doc = result.WaitForCompletion(1000)
                if not doc:
                    continue
                Log('Zora response received')
                urldata, content = self.cache.StoreFetchedDoc(doc)
                break
        else:
            Log('Offline mode requested, will not fetch')

        if not urldata:
            od.MakeError('Fetching seems to be disabled', 500)
            return od

        if urldata.failed:
            od.MakeError(urldata)
            return od

        if not content:
            content = self.cache.ReadContent(urldata.contentHash)

        mimeType = urldata.mimeType or mimeHint or ''
        encoding = urldata.encoding
        if encoding < 0:
            encoding = codingHint
        if mimeType not in ('text/html', 'text/css'):
            od.content = content
            od.mimeType = mimeType
            return od

        with self.rewriteMutex:
            parseResult = self.parser.RewriteContent(content, urldata.finalUrl, mimeType, encoding, urlMapper)
        if not original:
            if parseResult.Error:
                od.MakeError('Failed to parse content: %s\n' % parseResult.ErrorMessage)
                return od
            od.content = parseResult.RewrittenContent
        else:
            od.content = content

        od.mimeType = mimeType
        od.encoding = GetEncodingName(parseResult.Encoding)
        return od

class UrlMapper(object):
    def __init__(self, refresh, offline, enc):
        self.params = {}
        if refresh:
            self.params['refresh'] = 1
        if offline:
            self.params['offline'] = 1
        if enc:
            self.params['coding'] = str(enc)

    def __call__(self, url):
        mimeType = None
        parts = url.split('\t')
        url = parts[0]
        if len(parts) > 1:
            mimeType = parts[1]
        result = '/fetch?url=' + urllib.quote(url)
        if mimeType:
            result += '&' + urllib.urlencode({'mime' : mimeType})
        if self.params:
            result += '&' + urllib.urlencode(self.params)
        return result

