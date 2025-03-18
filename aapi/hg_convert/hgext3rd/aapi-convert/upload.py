import threading
import logging
import Queue
import traceback
import time

import grpc

import aapi.lib.py_common.store as local_store_utils
import yt_client as yt
import aapi.lib.py_common.consts as consts
import aapi.lib.proto.vcs_pb2 as vcs_pb2
import aapi.lib.proto.vcs_pb2_grpc as vcs_pb2_grpc
import vcs_helpers
import struct


def dump_object(_type, data):
    if _type == consts.NODE_TREE:
        data = vcs_helpers.decompress(data)

    return data + struct.pack('B', _type)


class VcsUploadThread(threading.Thread):

    def __init__(self, proxy_addr, in_q, store_path, state, dry_run=False):
        super(VcsUploadThread, self).__init__()
        self._channel = grpc.insecure_channel(proxy_addr, [('grpc.max_send_message_length', 32 * 2**20)])
        self._vcs_client = vcs_pb2_grpc.VcsStub(self._channel)
        self._in_q = in_q
        self._store = local_store_utils.Store2(store_path)
        self._vcs_name = proxy_addr
        self._state = state
        self._dry_run = dry_run

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                x = self._in_q.get(timeout=2)

                if x is None:
                    # Sentinel
                    return

                rev1, hash1, rev2, hash2, objects_hashes, changeset_hash = x

            except Queue.Empty:
                continue

            logging.info('Will upload %s(%s) -> %s(%s) objects to VCS_API %s', str(rev1), hash1, str(rev2), hash2, self._vcs_name)

            def objects_iterator():
                # Meta
                meta = vcs_pb2.TObjects()
                meta.Data.append('Hg ' + str(rev1) + ' -> ' + str(rev2))
                yield meta

                objects = vcs_pb2.TObjects()
                objects_size = 0
                objects_max_size = 4 * 1024 * 1024  # 4MB

                for key_bin in objects_hashes + [changeset_hash]:
                    _type, data = self._store.get(key_bin)
                    objects.Data.append(key_bin)
                    objects.Data.append(dump_object(_type, data))
                    objects_size += len(data)
                    if objects_size > objects_max_size:
                        yield objects
                        objects = vcs_pb2.TObjects()
                        objects_size = 0

                if objects_size:
                    yield objects

            if not self._dry_run:
                try:
                    self._vcs_client.Push(objects_iterator())
                except Exception as e:
                    logging.warning('Can\'t upload %s(%s) -> %s(%s) objects to VCS_API %s: %s', str(rev1), hash1, str(rev2), hash2, self._vcs_name, str(e))
                    logging.warning(traceback.format_exc())
                    continue

            logging.info('Uploaded %s(%s) -> %s(%s) to VCS_API %s', str(rev1), hash1, str(rev2), hash2, self._vcs_name)
            break


class YdbUploadThread(threading.Thread):

    def __init__(self, ydb_client, in_q, out_q, store_path, state, dry_run=False):
        super(YdbUploadThread, self).__init__()
        self._ydb_client = ydb_client
        self._in_q = in_q
        self._out_q = out_q
        self._state = state
        self._store = local_store_utils.Store2(store_path)
        self._dry_run = dry_run

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                x = self._in_q.get(timeout=2)

                if x is None:
                    # Sentinel
                    return

                rev1, hash1, rev2, hash2, objects_hashes, changeset_hash = x

            except Queue.Empty:
                continue

            logging.info('Will upload %s(%s) -> %s(%s) objects to YDB', str(rev1), hash1, str(rev2), hash2)

            while True:
                chunk = []
                chunk_size = 0
                chunk_max_size = 32 * 1024 * 1024  # 32MB
                uploading = None

                try:
                    for key_bin in objects_hashes:
                        _type, data = self._store.get(key_bin)
                        assert _type in (consts.NODE_TREE, consts.NODE_BLOB)

                        chunk.append((key_bin, _type, data))
                        chunk_size += len(data)

                        if chunk_size > chunk_max_size:
                            if not self._dry_run:
                                if uploading is not None:
                                    uploading()
                                uploading = self._ydb_client.put_many(chunk)
                            chunk = []
                            chunk_size = 0

                    if uploading is not None:
                        uploading()

                    if chunk:
                        if not self._dry_run:
                            uploading = self._ydb_client.put_many(chunk)
                            uploading()

                    cs_type, cs_data = self._store.get(changeset_hash)
                    assert cs_type == consts.NODE_HG_CHANGESET
                    if not self._dry_run:
                        self._ydb_client.put_item(changeset_hash, cs_type, cs_data)

                except Exception as e:
                    logging.warning('Can\'t upload %s(%s) -> %s(%s) objects to YDB: %s', str(rev1), hash1, str(rev2), hash2, str(e))
                    logging.warning(traceback.format_exc())
                    self._ydb_client.reset_session()
                    time.sleep(1)
                    continue

                self._out_q.put(changeset_hash)
                logging.info('Uploaded %s(%s) -> %s(%s) to YDB', str(rev1), hash1, str(rev2), hash2)
                break


class YtUploadThread(threading.Thread):

    def __init__(self, yt_client, yt_table, in_q, out_q, store_path, state, dry_run=False):
        super(YtUploadThread, self).__init__()
        self._yt_client = yt_client
        self._yt_table = yt_table
        self._in_q = in_q
        self._out_q = out_q
        self._state = state
        self._store = local_store_utils.Store2(store_path)
        self._dry_run = dry_run

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                x = self._in_q.get(timeout=2)

                if x is None:
                    # Sentinel
                    return

                rev1, hash1, rev2, hash2, objects_hashes, changeset_hash = x

            except Queue.Empty:
                continue

            logging.info('Will upload %s(%s) -> %s(%s) objects to YT', str(rev1), hash1, str(rev2), hash2)

            while True:
                chunk = []
                chunk_size = 0
                chunk_max_size = 64 * 1024 * 1024  # 64MB

                try:
                    for key_bin in objects_hashes:
                        _type, data = self._store.get(key_bin)
                        assert _type in (consts.NODE_TREE, consts.NODE_BLOB)

                        chunk.append({'hash': key_bin, 'type': yt.node_ui64(_type), 'data': data})
                        chunk_size += len(data)

                        if chunk_size > chunk_max_size:
                            if not self._dry_run:
                                self._yt_client.insert_rows(self._yt_table, chunk)
                            chunk = []
                            chunk_size = 0

                    if chunk:
                        if not self._dry_run:
                            self._yt_client.insert_rows(self._yt_table, chunk)

                    cs_type, cs_data = self._store.get(changeset_hash)
                    assert cs_type == consts.NODE_HG_CHANGESET
                    if not self._dry_run:
                        self._yt_client.insert_rows(self._yt_table, [{'hash': changeset_hash, 'type': yt.node_ui64(cs_type), 'data': cs_data}])

                except Exception as e:
                    logging.warning('Can\'t upload %s(%s) -> %s(%s) objects to YT: %s', str(rev1), hash1, str(rev2), hash2, str(e))
                    logging.warning(traceback.format_exc())
                    time.sleep(1)
                    continue

                self._out_q.put(changeset_hash)
                logging.info('Uploaded %s(%s) -> %s(%s) to YT', str(rev1), hash1, str(rev2), hash2)
                break
