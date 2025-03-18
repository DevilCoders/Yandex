import os
import os.path
import hashlib
import json
from threading import Lock

class UrlData(object):
    url = ''
    finalUrl = ''
    encoding = -1
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
        if isinstance(self.failed, unicode):
            self.failed = not (self.failed == u'false')
        self.errorMessage = d['error_message'].encode('utf-8')
        self.contentHash = d['content_hash'].encode('utf-8')
        self.httpCode = d['http_code']

class Cache(object):
    cachedir = None
    metadata = None
    mutex = None
    iomutex = None

    def __init__(self, cachedir = '.cache'):
        self.cachedir = cachedir
        self.metadata = {}
        self.mutex = Lock()
        self.iomutex = Lock()

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

    def QueryMetadata(self, url):
        with self.mutex:
            if url in self.metadata:
                return self.metadata[url]
            else:
                return None

    def AppendCache(self, urldata):
        with self.mutex:
            with open(self.ManifestPath(), 'a') as f:
                print >>f, json.dumps(urldata.ToDict(), 'utf-8')
            self.metadata[urldata.url] = urldata

    def WriteContent(self, content):
        hshfn = hashlib.sha256()
        hshfn.update(content)
        hsh = hshfn.hexdigest()
        path = self.MkPath(hsh)
        with self.iomutex:
            #TODO: use write-rename
            if not os.path.exists(path):
                with open(path, 'w') as f:
                    f.write(content)
        return hsh

    def ReadContent(self, hsh):
        path = self.MkPath(hsh)
        if not os.path.isfile(path):
            return ''
        with self.iomutex:
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
        ud.encoding = fetchedDoc.Encoding
        ud.httpCode = fetchedDoc.HttpCode
        ud.failed = fetchedDoc.Failed
        ud.errorMessage = fetchedDoc.ErrorMessage
        self.AppendCache(ud)
        return (ud, fetchedDoc.Content)


