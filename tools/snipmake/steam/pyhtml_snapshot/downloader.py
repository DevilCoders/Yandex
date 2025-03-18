#!/usr/bin/env python
import sys
import os.path
import hashlib
import heapq
import random
import css
import link_types
import argparse

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

    links = []

    def urlMapper(url):
        url, link_type = url.split('\t')
        links.append((url, link_type))
        return url

    def cssRewriter(css_text, baseurl, encoding, is_inline):
        def relative_url_fn(rel_url, content_hint):
            full_url = baseurl.MergeWithBase(rel_url)
            links.append((full_url, content_hint))
            return full_url
        enc_name = GetEncodingName(encoding)
        return css.rewrite_css(css_text, relative_url_fn, enc_name, is_inline)

    encodingHint, mimeHint = -1, ''
    if urldata.url in hints:
        encodingHint, typeHint = hints[urldata.url]
    elif urldata.finalUrl in hints:
        encodingHint, typeHint = hints[urldata.finalUrl]
    mimeType = urldata.mimeType
    if not mimeType:
        if typeHint == link_types.CSS:
            mimeType = 'text/css'
        elif typeHint == link_types.HTML:
            mimeType = 'text/html'
    encoding = urldata.encoding
    if encoding < 0:
        encoding = encodingHint
    if mimeType and not mimeType.startswith('text/'):
        return

    parseResult = parser.RewriteContent(content, urldata.finalUrl, mimeType, encoding, urlMapper, cssRewriter)
    if parseResult.Error:
        Log('Failed to parse %s: %s' % (urldata.finalUrl, parseResult.ErrorMessage))
        return
    encoding = parseResult.Encoding
    newUrlCount = 0
    for url, linkTypeHint in links:
        if url in visitedUrls:
            continue
        visitedUrls.add(url)
        heapq.heappush(queue, (PRIORITY_RESOURCE_MAX + random.randrange(PRIORITY_RESOURCE_RANGE), url, hops))
        hints[url] = (encoding, linkTypeHint)
        newUrlCount += 1
    if newUrlCount:
        Log('Enqueued %d new URLs, queue size is %d' % (newUrlCount, len(queue)))

def CanRetryWithConfig(urlcache, config):
    return CanRetry(urlcache.httpCode) or config.forceRetry

def DownloadUrls(urlList, config):
    visited_urls = set(urlList)
    result = AsyncFetchResult()
    zora = ZoraFetcher(config.zoraSource, config.zoraUserproxy, False, config.zoraTimeout);
    cache = Cache(config.cacheDir)
    cache.InitCache()
    cache.LoadCache()
    parser = WebContentParser(config.configDir)

    # hints: url -> (int encodingHint, str mimeTypeHint)
    hints = {}
    for url in urlList:
        hints[url] = (-1, link_types.HTML)

    MAX_INFLIGHT = config.maxInFlight
    queue = [(PRIORITY_DOC_MAX + random.randrange(PRIORITY_DOC_RANGE), url, 0) for idx, url in enumerate(visited_urls)]
    heapq.heapify(queue)

    inflight = 0
    ctr = 0
    failed = 0
    succeeded = 0
    while True:
        while inflight < MAX_INFLIGHT and queue:
            prio, url, hops = heapq.heappop(queue)
            hinted_type = hints.get(url, (None, None))[1]
            urlcache = cache.metadata.get(url, None)
            if urlcache and urlcache.failed and not CanRetryWithConfig(urlcache, config):
                continue
            elif urlcache and not urlcache.failed:
                content = cache.ReadContent(urlcache.contentHash)
                EnqueueLinkedUrls(parser, urlcache, content, queue, hints, visited_urls, hops+1)
                continue
            if config.imagesOnly and hinted_type == link_types.HTML:
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
            Log('%s: download failed with code %d: %s' % (fetchedDoc.Url, fetchedDoc.HttpCode, fetchedDoc.ErrorMessage))
            failed += 1
        else:
            Log('%s: download complete -> %s' % (fetchedDoc.Url, fetchedDoc.FinalUrl))
            succeeded += 1
        urlcache, content = cache.StoreFetchedDoc(fetchedDoc)
        if not urlcache.failed:
            EnqueueLinkedUrls(parser, urlcache, content, queue, hints, visited_urls, hops+1)
        Log('inflight = %d; queue size = %d; successes = %d; failures = %d' % (inflight, len(queue), succeeded, failed))

    Log('Aaaand we are done!')
    zora.Terminate()

if __name__ == '__main__':
    argparser = argparse.ArgumentParser(description='Download HTML with images/CSSes into the offline cache')
    argparser.add_argument('configFile', metavar='config-file-name', type=str, nargs=1, help='Config file name')
    args = argparser.parse_args()

    class Config(object):
        execfile(args.configFile[0])

    config = Config()

    if len(sys.argv) == 2:
        cachedir = sys.argv[1]

    urls = []
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        urls.append(line)

    DownloadUrls(urls, config)

