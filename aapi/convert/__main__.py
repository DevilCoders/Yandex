import os
import sys
import time
import logging
import threading
import struct
import optparse
import json
import Queue
import shutil
import traceback
import subprocess as sp
import flask
import grpc

import subvertpy.repos

import vcs_helpers

import yt_client as yt

import aapi.lib.convert.convert as convert_utils
import aapi.convert.store as store_utils
import aapi.lib.proto.vcs_pb2 as vcs_pb2
import aapi.lib.proto.vcs_pb2_grpc as vcs_pb2_grpc
import aapi.lib.py_common.consts as consts
import aapi.lib.py_common.store as local_store_utils
import aapi.lib.ydb.ydb as ydb


def parse_args():
    parser = optparse.OptionParser()
    parser.add_option('-c', '--config', help='Config json path')
    opts, args = parser.parse_args()

    if not opts.config:
        print>>sys.stderr, 'Config must be specified'
        sys.exit(1)

    return opts


class State(object):

    def __init__(self):
        self._stopped = False

    def stop(self):
        self._stopped = True

    def stopped(self):
        return self._stopped


def get_svn_head_from_yt(yt_client, svn_head_yt_path):
    return int(yt_client.get(svn_head_yt_path))


def create_sensors(svn_local_repo, yt_client, cfg):
    app = flask.Flask('sensors')
    fs = subvertpy.repos.Repository(svn_local_repo).fs()

    @app.route('/sensors')
    def sensors():
        logging.info('Sensors request')

        try:
            real_svn_head = fs.youngest_revision()
            yt_svn_head = get_svn_head_from_yt(yt_client, cfg['svn_head_yt_path'])
            svn_head_gap = real_svn_head - yt_svn_head
        except Exception as e:
            logging.error('Error while getting sensors: %s', e)
            return '{}'

        sensors = {
            'sensors': [
                {
                    'labels': {'sensor': 'svn_head_gap'},
                    'value': svn_head_gap
                },
            ]
        }

        return json.dumps(sensors)

    return app


class SvnPoolingThread(threading.Thread):

    def __init__(self, svn_local_repo, svn_head, out_q, state):
        super(SvnPoolingThread, self).__init__()
        self._svn_local_repo = svn_local_repo
        self._fs = subvertpy.repos.Repository(self._svn_local_repo).fs()
        self._svn_head = svn_head
        self._out_q = out_q
        self._state = state

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                svn_head = self._fs.youngest_revision()
            except Exception as e:
                logging.error('Can\'t get svn head from %s: %s', self._svn_repo, str(e))
                time.sleep(3)
                continue

            if self._svn_head > svn_head:
                logging.warning('Svn head(%d) is less than known one(%d)', svn_head, self._svn_head)
                time.sleep(1)
                continue

            for r in xrange(self._svn_head, svn_head):
                self._out_q.put(r)
                self._svn_head += 1

            time.sleep(1)


def rmtree(path):
    try:
        shutil.rmtree(path)
        return
    except Exception as e:
        logging.error(e)

    sp.check_call(['rm', '-rf', path])


class ConvertingThread(threading.Thread):

    def __init__(self, svn_local_repo, vcs_store, garbage_dir, in_q, out_q, blacklist, state):
        super(ConvertingThread, self).__init__()
        self._svn_local_repo = svn_local_repo
        self._fs = subvertpy.repos.Repository(self._svn_local_repo).fs()
        self._vcs_store = vcs_store
        self._garbage_dir = garbage_dir
        self._blacklist = blacklist
        self._in_q = in_q
        self._out_q = out_q
        self._state = state
        self._converted = 0

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                rev = self._in_q.get(timeout=2)
            except Queue.Empty:
                continue

            while True:
                objects_dir = os.path.join(self._garbage_dir, str(rev + 1) + '-objects')

                if os.path.exists(objects_dir):
                    rmtree(objects_dir)

                os.makedirs(objects_dir)

                substore = self._vcs_store.substore(objects_dir)

                try:
                    convert_utils.update(self._fs, substore, rev, self._blacklist)

                except Exception as e:
                    logging.warning('Can\'t convert %s -> %s: %s', str(rev), str(rev + 1), e)
                    logging.warning(traceback.format_exc())
                    rmtree(objects_dir)
                    time.sleep(1)
                    continue

                self._out_q.put((rev, objects_dir, sorted(substore.get_all_puts())))
                self._converted += 1
                break

            if self._converted >= 2500:
                self._state.stop()
                return


class DirRefcounted(object):

    def __init__(self, path, refs):
        self.path = path
        self._refs = refs
        self._lock = threading.Lock()

    def unref(self):
        with self._lock:
            assert self._refs > 0
            self._refs -= 1

            if self._refs == 0:
                rmtree(self.path)


class DistributorThread(threading.Thread):

    def __init__(self, coversion_q, yt_upload_q, ydb_upload_q, vcs_upload_qs, state):
        super(DistributorThread, self).__init__()
        self._coversion_q = coversion_q
        self._yt_upload_q = yt_upload_q
        self._ydb_upload_q = ydb_upload_q
        self._vcs_upload_qs = vcs_upload_qs
        self._state = state

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                rev, objects_dir, keys = self._coversion_q.get(timeout=2)
            except Queue.Empty:
                continue

            logging.info('Converted %s -> %s', str(rev), str(rev + 1))
            objects_dir_refconted = DirRefcounted(objects_dir, 1 + 1 + len(self._vcs_upload_qs))

            self._yt_upload_q.put((rev, objects_dir_refconted, keys))
            self._ydb_upload_q.put((rev, objects_dir_refconted, keys))
            for q in self._vcs_upload_qs:
                q.put((rev, objects_dir_refconted, keys))


class TerminatorThread(threading.Thread):

    def __init__(self, deadline, state):
        super(TerminatorThread, self).__init__()
        self._deadline = deadline
        self._state = state

    def run(self):
        start = time.time()

        while True:
            if self._state.stopped():
                return

            time.sleep(5)

            if time.time() - start > self._deadline:
                self._state.stop()
                return


class YdbUploadThread(threading.Thread):

    def __init__(self, ydb_client, in_q, state):
        super(YdbUploadThread, self).__init__()
        self._ydb_client = ydb_client
        self._in_q = in_q
        self._state = state

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                rev, objects_dir_refcounted, keys = self._in_q.get(timeout=2)
            except Queue.Empty:
                continue

            objects_dir = objects_dir_refcounted.path
            local_store = local_store_utils.Store2(objects_dir)

            logging.info('Will upload %s -> %s objects to YDB', str(rev), str(rev + 1))

            while True:
                chunk = []
                chunk_size = 0
                chunk_max_size = 32 * 1024 * 1024  # 32MB
                uploading = None

                try:
                    for key_bin in keys:
                        _type, data = local_store.get(key_bin)

                        chunk.append((key_bin, _type, data))
                        chunk_size += len(data)

                        if chunk_size > chunk_max_size:
                            if uploading is not None:
                                uploading()
                            uploading = self._ydb_client.put_many(chunk)
                            chunk = []
                            chunk_size = 0

                    if uploading is not None:
                        uploading()

                    if chunk:
                        uploading = self._ydb_client.put_many(chunk)
                        uploading()

                    self._ydb_client.put_svn_head(str(rev + 1))

                except Exception as e:
                    logging.warning('Can\'t upload %s -> %s objects to YDB: %s', str(rev), str(rev + 1), str(e))
                    logging.warning(traceback.format_exc())
                    self._ydb_client.reset_session()
                    time.sleep(1)
                    continue

                logging.info('Uploaded %s -> %s to YDB', str(rev), str(rev + 1))
                break

            objects_dir_refcounted.unref()


class YtUploadThread(threading.Thread):

    def __init__(self, yt_client, yt_table, yt_svn_head_path, in_q, state):
        super(YtUploadThread, self).__init__()
        self._yt_client = yt_client
        self._yt_table = yt_table
        self._yt_svn_head_path = yt_svn_head_path
        self._in_q = in_q
        self._state = state

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                rev, objects_dir_refcounted, keys = self._in_q.get(timeout=2)
            except Queue.Empty:
                continue

            objects_dir = objects_dir_refcounted.path
            local_store = local_store_utils.Store2(objects_dir)

            logging.info('Will upload %s -> %s objects to YT', str(rev), str(rev + 1))

            while True:
                chunk = []
                chunk_size = 0
                chunk_max_size = 64 * 1024 * 1024  # 64MB

                try:
                    for key_bin in keys:
                        _type, data = local_store.get(key_bin)

                        chunk.append({'hash': key_bin, 'type': yt.node_ui64(_type), 'data': data})
                        chunk_size += len(data)

                        if chunk_size > chunk_max_size:
                            self._yt_client.insert_rows(self._yt_table, chunk)
                            chunk = []
                            chunk_size = 0

                    if chunk:
                        self._yt_client.insert_rows(self._yt_table, chunk)

                    self._yt_client.set(self._yt_svn_head_path, str(rev + 1))

                except Exception as e:
                    logging.warning('Can\'t upload %s -> %s objects to YT: %s', str(rev), str(rev + 1), str(e))
                    logging.warning(traceback.format_exc())
                    time.sleep(1)
                    continue

                logging.info('Uploaded %s -> %s to YT', str(rev), str(rev + 1))
                break

            objects_dir_refcounted.unref()


def dump_object(_type, data):
    if _type == consts.NODE_TREE:
        data = vcs_helpers.decompress(data)

    return data + struct.pack('B', _type)


class VcsUploadThread(threading.Thread):

    def __init__(self, vcs_client, in_q, vcs_name, state):
        super(VcsUploadThread, self).__init__()
        self._vcs_client = vcs_client
        self._in_q = in_q
        self._vcs_name = vcs_name
        self._state = state

    def run(self):
        while True:
            if self._state.stopped():
                return

            try:
                rev, objects_dir_refcounted, keys = self._in_q.get(timeout=2)
            except Queue.Empty:
                continue

            objects_dir = objects_dir_refcounted.path
            local_store = local_store_utils.Store2(objects_dir)

            logging.info('Will upload %s -> %s objects to VCS_API %s', str(rev), str(rev + 1), self._vcs_name)

            def objects_iterator():
                # Meta
                meta = vcs_pb2.TObjects()
                meta.Data.append('Svn ' + str(rev) + ' -> ' + str(rev + 1))
                yield meta

                objects = vcs_pb2.TObjects()
                objects_size = 0
                objects_max_size = 4 * 1024 * 1024  # 14MB

                for key_bin in keys:
                    _type, data = local_store.get(key_bin)
                    objects.Data.append(key_bin)
                    objects.Data.append(dump_object(_type, data))
                    objects_size += len(data)
                    if objects_size > objects_max_size:
                        yield objects
                        objects = vcs_pb2.TObjects()
                        objects_size = 0

                if objects_size:
                    yield objects

            for i in xrange(5):
                objects_iter = objects_iterator()

                try:
                    self._vcs_client.Push(objects_iter)
                except Exception as e:
                    logging.warning('Can\'t upload %s -> %s objects to VCS_API %s: %s', str(rev), str(rev + 1), self._vcs_name, str(e))
                    logging.warning(traceback.format_exc())
                    time.sleep(1)
                    continue

                logging.info('Uploaded %s -> %s to VCS_API %s', str(rev), str(rev + 1), self._vcs_name)
                break

            objects_dir_refcounted.unref()


def main():
    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger('kikimr').setLevel(logging.WARNING)
    convert_utils.Wtf.disable()

    opts = parse_args()
    with open(opts.config) as f:
        cfg = json.load(f)

    yt_proxy = cfg['yt_proxy']
    yt_table = cfg['yt_table']
    yt_token = cfg['yt_token']
    yt_client = yt.Client(yt_proxy, token=yt_token)

    ydb_endpoint = cfg['ydb_endpoint']
    ydb_table = cfg['ydb_table']
    ydb_token = cfg['ydb_token']
    ydb_client = ydb.YdbClient(ydb_endpoint, ydb_table, ydb_token)

    svn_local_repo = cfg['svn_local_repo'].encode('utf8')
    svn_head_yt_path = cfg['svn_head_yt_path']
    svn_head = int(yt_client.get(svn_head_yt_path))

    vcs_api_services = cfg['vcs_api_services']  # host1:port1, host2:port2, ...
    vcs_api_clients = []
    for service in vcs_api_services:
        channel = grpc.insecure_channel(service, [('grpc.max_send_message_length', 32 * 2**20)])
        vcs_api_clients.append(vcs_pb2_grpc.VcsStub(channel))

    vcs_store_path = cfg['vcs_local_store_path']
    vcs_store = store_utils.Store(vcs_store_path, yt_client, yt_table, parent=None, local_store_types={consts.NODE_TREE, consts.NODE_SVN_CI})

    state = State()

    # Create threads objects
    svn_pooling_queue = Queue.Queue()
    svn_pooling_thread = SvnPoolingThread(svn_local_repo, svn_head, svn_pooling_queue, state)

    garbage_dir = cfg['garbage_dir']
    if not os.path.exists(garbage_dir):
        os.makedirs(garbage_dir)

    blacklist = []
    for p in cfg['blacklist']:
        blacklist.append('/' + p.encode('utf-8').strip('/') + '/')

    converting_queue = Queue.Queue()
    converting_thread = ConvertingThread(svn_local_repo, vcs_store, garbage_dir, svn_pooling_queue, converting_queue, blacklist, state)

    yt_upload_queue = Queue.Queue()
    ydb_upload_queue = Queue.Queue()
    vcs_api_upload_queues = [Queue.Queue() for _ in vcs_api_services]
    distributor_thread = DistributorThread(converting_queue, yt_upload_queue, ydb_upload_queue, vcs_api_upload_queues, state)

    yt_upload_thread = YtUploadThread(yt_client, yt_table, svn_head_yt_path, yt_upload_queue, state)
    ydb_upload_thread = YdbUploadThread(ydb_client, ydb_upload_queue, state)
    vcs_api_upload_threads = [VcsUploadThread(c, q, s, state) for (c, q, s) in zip(vcs_api_clients, vcs_api_upload_queues, vcs_api_services)]

    terminator_thread = TerminatorThread(1800, state)

    # Start threads
    svn_pooling_thread.start()
    converting_thread.start()
    distributor_thread.start()
    yt_upload_thread.start()
    ydb_upload_thread.start()
    for t in vcs_api_upload_threads:
        t.start()
    terminator_thread.start()

    # Join threads
    svn_pooling_thread.join()
    converting_thread.join()
    distributor_thread.join()
    yt_upload_thread.join()
    ydb_upload_thread.join()
    for t in vcs_api_upload_threads:
        t.join()
    terminator_thread.join()

    sys.exit(1)


if __name__ == '__main__':
    main()
