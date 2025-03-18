import threading
import logging
import copy
import struct

import vcs_helpers

import grpc
import aapi.lib.proto.vcs_pb2 as vcs_pb2
import aapi.lib.proto.vcs_pb2_grpc as vcs_pb2_grpc

import aapi.lib.py_common.store as local_store_utils
import aapi.lib.py_common.consts as consts


class Store(object):

    def __init__(self, path, yt_client, yt_table, proxy_addr=None, parent=None, local_store_types=None):
        self._local_store = local_store_utils.Store2(path)
        self._yt_client = yt_client
        self._yt_table = yt_table

        self._proxy_addr = proxy_addr
        if proxy_addr is not None:
            self._channel = grpc.insecure_channel(proxy_addr, [('grpc.max_send_message_length', 32 * 2**20), ('grpc.max_receive_message_length', 32 * 2**20)])
            self._vcs_client = vcs_pb2_grpc.VcsStub(self._channel)

        self._parent = parent
        self._local_store_types = local_store_types

        self._lock = threading.Lock()  # for reinsurance
        self._all_puts = set()

    def substore(self, path, local_store_types=None):
        return Store(path, self._yt_client, self._yt_table, self._proxy_addr, parent=self, local_store_types=local_store_types)

    def put(self, key_bin, _type, data):
        if self._parent is not None:
            self._parent.put(key_bin, _type, data)

        if self._local_store_types is None or _type in self._local_store_types:
            self._local_store.put(key_bin, _type, data)

            with self._lock:
                self._all_puts.add(key_bin)

    def has(self, key_bin):
        if self._parent is not None and self._parent.has(key_bin):
            return True

        return self._local_store.has(key_bin)

    def get_all_puts(self):
        with self._lock:
            return copy.copy(self._all_puts)

    def reset_all_puts(self):
        with self._lock:
            self._all_puts = set()

    def get(self, key_bin, try_walk=False):
        try:
            return self._local_store.get(key_bin)
        except KeyError:
            pass

        if self._parent:
            try:
                return self._parent.get(key_bin, try_walk=try_walk)
            except KeyError:
                pass

        if try_walk and self._proxy_addr is not None:
            root = vcs_pb2.THash()
            root.Hash = key_bin
            logging.info('Walk %s', key_bin.encode('hex'))
            for dirs in self._vcs_client.Walk(root):
                for d in dirs.Directories:
                    type_ = struct.unpack('B', d.Blob[-1])[0]
                    assert type_ == consts.NODE_TREE
                    tree_fbs = d.Blob[:-1]
                    if not self.has(d.Hash):
                        self.put(d.Hash, type_, vcs_helpers.compress(tree_fbs))
            return self.get(key_bin, try_walk=False)

        rows = self._yt_client.lookup_rows(self._yt_table, [{'hash': key_bin}], columns=['type', 'data'])
        logging.info('Downloaded %s from YT', key_bin.encode('hex'))

        assert len(rows) <= 1

        if not rows:
            raise KeyError(key_bin.encode('hex'))

        _type = rows[0]['type']
        data = rows[0]['data']

        self.put(key_bin, _type, data)

        return _type, data
