import aapi.lib.py_common.store as local_store_utils
import threading
import copy


class Store(object):

    def __init__(self, path, yt_client, yt_table, parent=None, local_store_types=None):
        self._local_store = local_store_utils.Store2(path)
        self._yt_client = yt_client
        self._yt_table = yt_table
        self._parent = parent
        self._local_store_types = local_store_types

        self._lock = threading.Lock()  # for reinsurance
        self._all_puts = set()

    def substore(self, path, local_store_types=None):
        return Store(path, self._yt_client, self._yt_table, parent=self, local_store_types=local_store_types)

    def put(self, key_bin, _type, data):
        if self._parent is not None:
            self._parent.put(key_bin, _type, data)

        if self._local_store_types is None or _type in self._local_store_types:
            self._local_store.put(key_bin, _type, data)

            with self._lock:
                self._all_puts.add(key_bin)

    def get_all_puts(self):
        with self._lock:
            return copy.copy(self._all_puts)

    def reset_all_puts(self):
        with self._lock:
            self._all_puts = set()

    def get(self, key_bin):
        try:
            return self._local_store.get(key_bin)
        except KeyError:
            pass

        if self._parent:
            try:
                return self._parent.get(key_bin)
            except KeyError:
                pass

        rows = self._yt_client.lookup_rows(self._yt_table, [{'hash': key_bin}], columns=['type', 'data'])

        assert len(rows) <= 1

        if not rows:
            raise KeyError(key_bin.encode('hex'))

        _type = rows[0]['type']
        data = rows[0]['data']

        self.put(key_bin, _type, data)

        return _type, data
