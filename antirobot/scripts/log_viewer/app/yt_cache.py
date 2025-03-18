import hashlib
import os
import StringIO
import re
import sys
import traceback
import json
import datetime
import yt_client


reHash = re.compile(r'^[a-f, 0-9]{40}$')
def HashValid(hash):
    return reHash.match(hash)


class CacheKey(object):
    def __init__(self, basePath, keyStr):
        if not HashValid(keyStr):
            raise Exception, "Invalid cache string"

        self.result = keyStr
        self.keyRootPath = os.path.join(basePath, keyStr)

    @property
    def RootPath(self):
        return self.keyRootPath

    @property
    def MetaPath(self):
        return os.path.join(self.keyRootPath, 'meta')

    @property
    def DataPath(self):
        return os.path.join(self.keyRootPath, 'data')

    @property
    def LockPath(self):
        return os.path.join(self.keyRootPath, 'lock')

    def __str__(self):
        return self.result


class YTCache:
    def __init__(self, clusterRoot, ytInst):
        self.ytInst = ytInst
        self.basePath = os.path.join(clusterRoot, 'slow_search_cache')

    def MakeKey(self, logId, date, keyType, keyValue):
        if not (date and logId):
            raise Exception, 'date and tableKey must be specified'

        S = lambda x: str(x) if x else ''

        return CacheKey(self.basePath, hashlib.sha1(''.join((date.strftime('%Y%m%d'), logId, S(keyType), S(keyValue)))).hexdigest())

    def MakeKeyFromPendingId(self, pendingId):
        return CacheKey(self.basePath, pendingId)

    @staticmethod
    def _MakeStream(data):
        return StringIO.StringIO(str(data))

    @staticmethod
    def _CheckKeyType(key):
        if not isinstance(key, CacheKey):
            raise Exception, 'Invalid key type'

    def Exists(self, key):
        self._CheckKeyType(key)
        return self.ytInst.exists(key.RootPath)

    def IsLocked(self, key):
        self._CheckKeyType(key)
        if not self.ytInst.exists(key.RootPath) or not self.ytInst.exists(key.LockPath):
            return False

        with inst1.Transaction():
            try:
                self.ytInst.lock(key.LockPath)
            except:
                return True
            else:
                return False

    def _Lock(self, key, transaction):
        if not self.ytInst.exists(key.LockPath):
            self.ytInst.create('file', key.LockPath)
        self.ytInst.lock(key.LockPath, 'exclusive')

    def WriteKey(self, key, getValueCallable):
        """
            getValueCallable must return tuple (meta, table_name)
            meta
                it is some arbitrary data (json usually)

            table_name
                is a table where table data stored. Can be None.
                If not None, the table will be moved into cache key folder.
        """
        self._CheckKeyType(key)
        self.ytInst.create('map_node', key.RootPath, ignore_existing=True)
        expiration_time = datetime.datetime.now() + datetime.timedelta(days=7)
        self.ytInst.set(key.RootPath + '/@expiration_time', expiration_time.isoformat())

        with self.ytInst.Transaction() as tr:
            try:
                self._Lock(key, tr)
            except:
                print >>sys.stderr, 'Cache key is locked by another instance, exiting'
            else:
                # we create second Yt instance to make possible
                # reading the content of Meta in other transactions
                ytInstSecond = yt_client.CloneInstance(self.ytInst)
                ytInstSecond.write_file(key.MetaPath, json.dumps({'status': 'initializing'}, None))
                #ytInstSecond.write_file(key.MetaPath, json.dumps({'status': 'running', 'progress': {}}))

                meta, tbl = getValueCallable(ytInstSecond)
                ytInstSecond.write_file(key.MetaPath, json.dumps(meta))

                if tbl:
                    ytInstSecond.move(tbl, key.DataPath)

    def ReadKeyMeta(self, key):
        self._CheckKeyType(key)
        if not self.ytInst.exists(key.RootPath) or not self.ytInst.exists(key.MetaPath):
            return None

        with self.ytInst.Transaction():
            self.ytInst.lock(key.MetaPath, 'snapshot')
            return json.loads(self.ytInst.read_file(key.MetaPath).read())

    def KeyIsReady(self, key):
        self._CheckKeyType(key)
        meta = self.ReadKeyMeta(key)
        return meta and meta.get('status') == 'ready'

    def DeleteKey(self, key):
        self._CheckKeyType(key)

        with self.ytInst.Transaction() as tr:
            try:
                self._Lock(key, tr)
                self.ytInst.remove(key.RootPath, force=True, recursive=True)
            except: # Do nothing if cache key is locked
                pass

    def ClearAll(self):
        with self.ytInst.Transaction():
            self.ytInst.lock(self.basePath, 'exclusive')
            files = self.ytInst.list(self.basePath)
            for fname in files:
                try:
                    self.ytInst.remove(os.path.join(self.basePath, fname), force=True, recursive=True)
                except:
                    pass
