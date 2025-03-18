#!/usr/bin/env python

import logging
import sys
from pyhtml_snapshot import FetchedDoc, AsyncFetchResult, ZoraFetcher, WebContentParser, CanRetry, GetEncodingName

logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
log = logging.getLogger('fetcher')

zora = ZoraFetcher('snippets_images', False, False, 60000)

result = AsyncFetchResult()

urls = {}
for line in sys.stdin.readlines():
    line = line.strip()
    if not line:
        continue
    urls[line] = str(len(urls))

inflight = 0
for url, ident in urls.viewitems():
    zora.Submit(url, ident, result)
    inflight += 1


while inflight:
    fetchedDoc = result.WaitForCompletion(1000)
    if not fetchedDoc:
        log.info('still waiting')
        continue
    inflight -= 1
    log.info('Zora response received')
    log.info(fetchedDoc.FinalUrl)
    log.info(fetchedDoc.RawMimeType)
    log.info(fetchedDoc.HttpCode)
    log.info(fetchedDoc.ErrorMessage)
    filename = urls[fetchedDoc.Url]

    with open(filename + '.jpeg', 'w') as outf:
        outf.write(fetchedDoc.Content)

zora.Terminate()
