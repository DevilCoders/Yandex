# -*- coding: utf-8 -*-

from core.settings import STORAGE_ROOT

from core.hard import steam_pb2
from core.hard import filestorage as fs


class Storage:

    FILESTORAGE = None

    def __init__(self):
        if not Storage.FILESTORAGE:
            Storage.FILESTORAGE = fs.TFileStorage(STORAGE_ROOT)

    @staticmethod
    def store_snip_pool(filename, is_json_pool, is_rca_pool):
        filename = str(filename)
        if is_json_pool:
            return Storage.FILESTORAGE.StoreJSONSnipPool(filename, is_rca_pool)
        else:
            return Storage.FILESTORAGE.StoreSnipPool(filename)

    @staticmethod
    def store_raw_file(filename):
        return Storage.FILESTORAGE.StoreRawFile(str(filename))

    @staticmethod
    def store_serp(filename):
        return Storage.FILESTORAGE.StoreSerp(str(filename))

    @staticmethod
    def get_raw_file(file_id):
        return Storage.FILESTORAGE.GetRawFile(str(file_id))

    @staticmethod
    def delete_file(file_id):
        Storage.FILESTORAGE.DeleteFile(str(file_id))

    @staticmethod
    def snippet_iterator(pool_id, begin=0, end=None, search_request=''):
        if search_request:
            return FilteringSnippetIterator(Storage.FILESTORAGE, pool_id,
                                            begin, end, search_request)
        return SnippetIterator(Storage.FILESTORAGE, pool_id, begin, end)


class SnippetIterator(object):
    def __init__(self, storage, pool_id, begin=0, end=None):
        pool_id = str(pool_id)
        if end is None:
            self.it = fs.TSnippetIterator(storage, pool_id, begin)
        else:
            self.it = fs.TSnippetIterator(storage, pool_id, begin, end)
        self.current = begin - 1

    def __iter__(self):
        return self

    def next(self):
        if not self.it.Valid():
            raise StopIteration
        snip_proto = steam_pb2.TSnippet()
        snip_proto.ParseFromString(self.it.Value())
        self.it.Next()
        self.current += 1
        return snip_proto

    def count(self):
        return self.it.GetSnipCount()

    def snip_id(self):
        return self.current


class FilteringSnippetIterator(SnippetIterator):
    def __init__(self, storage, pool_id, begin=0, end=None, substr=''):
        substr = substr.lower()
        super(FilteringSnippetIterator, self).__init__(storage, pool_id)
        self.results = []
        self.total_count = 0
        while True:
            try:
                snippet = super(FilteringSnippetIterator, self).next()
            except StopIteration:
                break
            if (
                substr in snippet.TitleText.decode('utf-8').lower() or
                any(substr in fragment.Text.decode('utf-8').lower()
                    for fragment in snippet.Fragments)
            ):
                if self.total_count >= begin and (end is None or
                                                  self.total_count < end):
                    self.results.append(
                        (snippet,
                         super(FilteringSnippetIterator, self).snip_id())
                    )
                self.total_count += 1
        self.current_result = 0

    def next(self):
        self.current_result += 1
        try:
            return self.results[self.current_result - 1][0]
        except IndexError:
            raise StopIteration

    def count(self):
        return self.total_count

    def snip_id(self):
        return self.results[self.current_result - 1][1]


if 'storage' in __name__:
    Storage()
