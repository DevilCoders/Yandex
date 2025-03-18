#!/usr/bin/env python
import sys
import cgi
import BaseHTTPServer
from pyhtml_snapshot import FetchedDoc, AsyncFetchResult, ZoraFetcher, WebContentParser, CanRetry, GetEncodingName
import os.path
import hashlib
import json
import urllib
import socket
import css
import link_types

#TODO: infinite link sequences are possible; TODO: limit hop distance from the root url

def Log(s):
    print >>sys.stderr, s
    sys.stderr.flush()

class UrlData(object):
    url = ''
    finalUrl = ''
    encoding = 0
    mimeType = ''
    failed = False
    errorMessage = ''
    contentHash = ''
    httpCode = 0

    def ToDict(self):
        return {
            'url' : self.url,
            'http_code' : self.httpCode,
            'final_url' : self.finalUrl,
            'encoding' : self.encoding,
            'mime_type' : self.mimeType,
            'failed' : self.failed,
            'error_message' : self.errorMessage,
            'content_hash' : self.contentHash
        }

    def FromDict(self, d):
        self.url = d['url'].decode('utf-8')
        self.finalUrl = d['final_url'].encode('utf-8')
        self.encoding = d['encoding']
        self.mimeType = d['mime_type'].encode('utf-8')
        self.failed = d['failed']
        self.errorMessage = d['error_message'].encode('utf-8')
        self.contentHash = d['content_hash'].encode('utf-8')
        self.httpCode = d['http_code']

class Cache(object):
    cachedir = '.cache'
    metadata = None

    def __init__(self):
        self.metadata = {}

    def MkPath(self, hsh):
        return os.path.join(self.cachedir, hsh)

    def ManifestPath(self):
        return self.MkPath('manifest.json')

    def InitCache(self):
        if os.path.isdir(self.cachedir):
            return
        os.mkdir(self.cachedir)

    def LoadCache(self):
        self.metadata = {}
        if not os.path.exists(self.ManifestPath()):
            return
        with open(self.ManifestPath(), 'r') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                ud = UrlData()
                ud.FromDict(json.loads(line))
                self.metadata[ud.url] = ud

    def AppendCache(self, urldata):
        with open(self.ManifestPath(), 'a') as f:
            print >>f, json.dumps(urldata.ToDict(), 'utf-8')
        self.metadata[urldata.url] = urldata

    def WriteContent(self, content):
        hshfn = hashlib.sha256()
        hshfn.update(content)
        hsh = hshfn.hexdigest()
        path = self.MkPath(hsh)
        if not os.path.exists(path):
            with open(path, 'w') as f:
                f.write(content)
        return hsh

    def ReadContent(self, hsh):
        path = self.MkPath(hsh)
        if not os.path.isfile(path):
            raise Exception('Missing cache file: ' + hsh)
        with open(path, 'r') as f:
            return f.read()

    def StoreFetchedDoc(self, fetchedDoc):
        if fetchedDoc.Content:
            hsh = self.WriteContent(fetchedDoc.Content)
        else:
            hsh = ''
        ud = UrlData()
        ud.url = fetchedDoc.Url
        ud.finalUrl = fetchedDoc.FinalUrl
        ud.contentHash = hsh
        ud.mimeType = fetchedDoc.RawMimeType
        ud.httpCode = fetchedDoc.HttpCode
        ud.failed = fetchedDoc.Failed
        ud.errorMessage = fetchedDoc.ErrorMessage
        self.AppendCache(ud)
        return (ud, fetchedDoc.Content)


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

class OfflineBrowser(object):
    cache = None
    fetcher = None
    parser = None
    urlMapper = None
    timeout = 30000
    counter = 0

    def __init__(self, config):
        Log('OfflineBrowser init')
        self.cache = Cache()
        self.cache.InitCache()
        self.cache.LoadCache()
        self.fetcher = ZoraFetcher(config['zoraSource'], config['zoraUserproxy'], False, self.timeout)
        self.parser = WebContentParser(config['configDir'])
        Log('OfflineBrowser done initializing')

    def GetUrl(self, url, urlMapper, mimeHint = None, codingHint = None, offline = True, refresh = False, original = False):
        Log('Request came in for url=' + url)

        def cssRewriter(css_text, baseurl, encoding, is_inline):
            def relative_url_fn(rel_url, content_hint):
                full_url = baseurl.MergeWithBase(rel_url)
                return urlMapper(full_url + '\t' + content_hint)
            enc_name = GetEncodingName(encoding)
            return css.rewrite_css(css_text, relative_url_fn, enc_name, is_inline)

        if not refresh:
            urldata = self.cache.metadata.get(url, None)
        else:
            urldata = None

        content = ''
        od = OutputDoc()
        if urldata:
            Log('Url is in cache')
            if urldata.failed and not CanRetry(urldata.httpCode):
                od.MakeError(urldata)
                return od
        elif not offline:
            Log('Url will be fetched')
            result = AsyncFetchResult()
            self.counter += 1
            self.fetcher.Submit(url, str(self.counter), result)
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
        encoding = urldata.encoding or codingHint or 0
        if mimeType not in ('text/html', 'text/css'):
            od.content = content
            od.mimeType = mimeType
            return od

        parseResult = self.parser.RewriteContent(content, urldata.finalUrl, mimeType, -1, urlMapper, cssRewriter)
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
        mimeTypeHint = None
        parts = url.split('\t')
        url = parts[0]
        if len(parts) > 1:
            mimeTypeHint = parts[1]
        if mimeTypeHint == link_types.CSS:
            mimeType = 'text/css'
        elif mimeTypeHint == link_types.HTML:
            mimeType = 'text/html'
        result = '/fetch?url=' + urllib.quote(url)
        if mimeType:
            result += '&' + urllib.urlencode({'mime' : mimeType})
        if self.params:
            result += '&' + urllib.urlencode(self.params)
        return result

class Handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def sendOutDoc(self, od):
        self.send_response(od.httpCode, 'ugh')
        self.send_header("Content-Type", od.MakeContentType())
        self.send_header("Content-Security-Policy", "default-src 'self'; img-src 'self'; script-src 'none'; object-src 'none'; style-src 'self' 'unsafe-inline'")
        self.send_header("X-Content-Security-Policy", "default-src 'self'; img-src 'self'; script-src 'none'; object-src 'none'; style-src 'self' 'unsafe-inline'")
        self.send_header("X-WebKit-CSP", "default-src 'self'; img-src 'self'; script-src 'none'; object-src 'none'; style-src 'self' 'unsafe-inline'")
        self.end_headers()
        self.wfile.write(od.content)
        self.wfile.close()

    def getPathAndParams(self, fromPost = False):
        path = self.path;
        queryParams = {}

        parts = path.split('?', 1)
        if len(parts) > 1:
            path = parts[0]
            queryParams = cgi.parse_qs(parts[1], True)

        if fromPost:
            if 'content-type' not in self.headers:
                return (None, None)
            (contType, contParams) = cgi.parse_header(self.headers['content-type'])
            if contType != 'multipart/form-data':
                return (None, None)
            parsed = cgi.parse_multipart(self.rfile, contParams)
            queryParams.update(parsed)

        return (path, queryParams)

    def do_GET(self):
        (path, queryParams) = self.getPathAndParams()
        if path == '/' or not path.startswith('/') or 'url' not in queryParams:
            od = OutputDoc()
            od.MakeError('Not found. Try /fetch?url=http://...', 404)
            self.sendOutDoc(od)
            return
        url = queryParams['url'][0]
        codingHint = int(queryParams.get('coding', ['0'])[0])
        mimeHint = queryParams.get('mime', [''])[0]
        original = 'original' in queryParams
        offline = 'offline' in queryParams
        refresh = 'refresh' in queryParams
        mapper = UrlMapper(refresh, offline, codingHint)

        od = Handler.browser.GetUrl(url, mapper, codingHint=codingHint, mimeHint=mimeHint, original=original, refresh=refresh)
        self.sendOutDoc(od)
        sys.stdout.flush()

import heapq
import random

MAX_DEPTH = 3

def EnqueueLinkedUrls(parser, urldata, content, queue, hints):
    def urlMapper(url):
        return url.split('\t')[0]

    Log('Discovering links in %s' % urldata.finalUrl)
    encodingHint, mimeHint = 0, ''
    if urldata.url in hints:
        encodingHint, mimeHint = hints[urldata.url]
    elif urldata.finalUrl in hints:
        encodingHint, mimeHint = hints[urldata.finalUrl]
    mimeType = urldata.mimeType or mimeHint or ''
    encoding = urldata.encoding or encodingHint or 0
    if mimeType and not mimeType.startswith('text/'):
        Log('%s: will not look for links here (type=%s)' % (urldata.finalUrl, mimeType))
        return

    parseResult = parser.RewriteContent(content, urldata.finalUrl, mimeType, encoding, urlMapper)
    if parseResult.Error:
        Log('Failed to parse %s: %s' % (urldata.finalUrl, parseResult.ErrorMessage))
        return
    encoding = parseResult.Encoding
    for link in parseResult.Links:
        # Add a link with randomized high priority
        heapq.heappush(queue, (random.randint(0, 100), link.Url))
        hints[link.Url] = (encoding, link.ContentTypeHint)
        Log('Enqueued %s (with probable content type %s)' % (link.Url, link.ContentTypeHint))

def DownloadUrls(urlList, config):
    result = AsyncFetchResult()
    zora = ZoraFetcher(config['zoraSource'], config['zoraUserproxy'], False, 30000);
    cache = Cache()
    cache.InitCache()
    cache.LoadCache()
    parser = WebContentParser(config['configDir'])

    # url -> (int encodingHint, str mimeTypeHint)
    hints = {}
    LOW_PRIO = 1000000
    MAX_INFLIGHT = 3
    queue = [(LOW_PRIO + idx, url) for idx, url in enumerate(urlList)]
    heapq.heapify(queue)

    inflight = 0
    ctr = 0
    while True:
        while inflight < MAX_INFLIGHT and queue:
            url = heapq.heappop(queue)[1]
            urlcache = cache.metadata.get(url, None)
            if urlcache and urlcache.failed and not CanRetry(urlcache.httpCode):
                Log('%s: in cache, failed, cannot retry: %s' %  (url, urlcache.errorMessage))
                continue
            elif urlcache and not urlcache.failed:
                Log('%s: in cache, will load from cache' % url)
                content = cache.ReadContent(urlcache.contentHash)
                EnqueueLinkedUrls(parser, urlcache, content, queue, hints)
                continue
            ctr += 1
            inflight += 1
            Log('%s: submitted job %d' % (url, ctr))
            Log('inflight is now: %d' % inflight)
            zora.Submit(url, str(ctr), result)

        if not queue and not inflight:
            break

        fetchedDoc = result.WaitForCompletion(1000)
        if not fetchedDoc:
            Log('zzzz...')
            continue
        inflight -= 1
        if fetchedDoc.Failed:
            Log('%s: download failed: %s' % (fetchedDoc.Url, fetchedDoc.ErrorMessage))
        else:
            Log('%s: download complete -> %s' % (fetchedDoc.Url, fetchedDoc.FinalUrl))
        Log('inflight is now: %d' % inflight)
        urlcache, content = cache.StoreFetchedDoc(fetchedDoc, depth)
        if not urlcache.failed:
            EnqueueLinkedUrls(parser, urlcache, content, queue, hints)

    Log('Aaaand we are done!')
    zora.Terminate()

if __name__ == '__main__':
    config = {
        'configDir': ".",
        'port': 28044,
        'zoraSource': "RichContentEval",
        'zoraUserproxy': False,
    }

    def rtfm():
        print >>sys.stderr, 'Usage: %s <download|serve>' % sys.argv[0]
        sys.exit(1)

    if len(sys.argv) != 2:
        rtfm()

    if sys.argv[1] == 'download':
        urls = []
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            urls.append(line)
        DownloadUrls(urls, config)
    elif sys.argv[1] == 'serve':
        server_address = ('0.0.0.0', config['port'])
        Handler.browser = OfflineBrowser(config)
        Log('Starting server at ' + str(server_address))
        httpd = BaseHTTPServer.HTTPServer(server_address, Handler)
        httpd.serve_forever()
    else:
        rtfm()

