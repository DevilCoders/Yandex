import argparse
import json
import logging
import os
import random
import signal
import socket
import threading
import time
import uuid
import library.python.bstr.compress as compress
from datetime import datetime
import parsedatetime

import six
if six.PY2:
    import subprocess32 as subprocess
else:
    import subprocess
    long = int
from six import iteritems, ensure_text
from library.python.bstr.common import init, ml, dump_json, get_yt_token
from library.python.bstr.stages_logger import StagesLogger
from library.python.yt import expiration_time, set_ttl

import yt.wrapper as yw
from yt.wrapper.errors import YtRequestRateLimitExceeded, YtRequestQueueSizeLimitExceeded
import yt.logger


yt.logger.LOGGER = logging.getLogger('yt')

logger = logging.getLogger(__name__)

B = 1
KB = 1024 * B
MB = 1024 * KB


def str2seconds(s):
    cal = parsedatetime.Calendar()
    diff = cal.parseDT(s, sourceTime=datetime.min)[0] - datetime.min
    return int(diff.total_seconds())


def do_share(path):
    cwd = os.path.dirname(path)

    if not cwd:
        cwd = None

    for l in subprocess.check_output(['/usr/local/bin/sky', 'share', os.path.basename(path)], shell=False, cwd=cwd).split('\n'):
        if l.startswith('rbtorrent:'):
            return l

    raise Exception('shit happened, cannot share %s' % path)


def share_torrent(rbtorrent, file_info=None, bstr_info=None):
    logger.debug('will share existing torrent %s', rbtorrent)
    if not bstr_info:
        if not file_info:
            out = subprocess.check_output(['/usr/local/bin/sky', 'files', '--json', rbtorrent], shell=False)
            file_info = json.loads(out)
        assert len(file_info) == 1, 'Expected exactly one file to share, got {}'.format(len(file_info))
        st = file_info[0]
        assert st['type'] == 'file', 'Can\'t share {}, file expected'.format(st['type'])
        bstr_info = {
            'mtime': long(time.time()),
            'fsize': long(st['size']),
            'name': st['name'],
        }

    return dict(bstr_info, **{
        'torrent': ensure_text(rbtorrent),
        'origin': socket.getfqdn(),
        'now': time.time(),
    })


def parse_yt_path(path):
    cluster, _, yt_path_ext = path.partition('://')
    yt_path, _, filename = yt_path_ext.partition('[')
    filename = filename.strip(']"\'')
    yt_path = '//' + yt_path
    return cluster, yt_path, filename


def share_blob_table(path, token, blob_table_ttl=None):
    cluster, yt_path, filename = parse_yt_path(path)
    logger.debug('will share blob table %s @ %s', yt_path, cluster)

    client = yw.YtClient(proxy=cluster, token=token)
    try:
        bstr_info = yw.get(os.path.join(yt_path, '@_bstr_attrs'), client=client)
        if not filename:
            filename = bstr_info['filename']
        bstr_info['name'] = bstr_info['filename']
        del bstr_info['filename']
    except Exception as e:
        logger.warning('failed to get _bstr_attrs attribute, will treat it as empty; got exception: %s', e)
        bstr_info = None
    rbtorrent = yw.sky_share('{}{}'.format(yt_path, '["{}"]'.format(filename) if filename else ''), client=client)
    if blob_table_ttl:
        set_ttl(client, yt_path, seconds=blob_table_ttl)
    return share_torrent(rbtorrent, bstr_info=bstr_info)


def share_file(path, spec):
    logger.debug('will share local file %s', path)
    if spec['compress']:
        tmpfile = '{path}.{method}'.format(path=path, method=spec['compress'])
        compress.safe_compress(
            method=spec['compress'],
            infile=path,
            outfile=tmpfile,
            threads=spec['compress-threads'],
            level=spec['compress-level'],
        )
        rbtorrent = do_share(tmpfile)
        # os.unlink(tmpfile)
    else:
        rbtorrent = do_share(path)
    info = share_file_common(path, rbtorrent)
    if spec['compress']:
        info['compress'] = spec['compress']
    return info


def share_file_common(path, rbtorrent):
    st = os.stat(path)
    return {
        'torrent': ensure_text(rbtorrent),
        'mtime': long(st.st_mtime),
        'fsize': long(st.st_size),
        'origin': socket.getfqdn(),
        'name': os.path.basename(path),
        'now': time.time(),
    }


def iter_file_for_blob_table(f, filename, chunk_size=4 * MB):
    chunk_no = 0
    while True:
        chunk = f.read(chunk_size)
        if not chunk:
            break
        yield {
            "filename": filename,
            "part_index": chunk_no,
            "data": chunk
        }
        chunk_no += 1


def write_and_share_blob_table(file_path,
                               table_path,
                               token,
                               yt_heavy_request_timeout,
                               blob_table_ttl=None,
                               content_encoding=None,
                               retries=0):
    cluster, yt_path, _ = parse_yt_path(table_path)
    logger.debug('will share local file %s via blob table %s @ %s', file_path, yt_path, cluster)

    client = yw.YtClient(proxy=cluster, token=token, config={
        'proxy': {
            'heavy_request_timeout': yt_heavy_request_timeout,
            'content_encoding': content_encoding or 'gzip'
        },
        'write_retries': {
            'enable': False,
            'backoff': {
                'policy': 'rounded_up_to_request_timeout'
            }
        }
    })
    filename = os.path.basename(file_path)
    st = os.stat(file_path)
    bstr_attrs = dict(
        mtime=long(st.st_mtime),
        fsize=long(st.st_size),
        filename=filename
    )

    if client.exists(yt_path):
        try:
            bstr_info = client.get(os.path.join(yt_path, '@_bstr_attrs'))
        except Exception as e:
            logger.error('failed to get bstr_attrs from table, got exception: %s', e)
            bstr_info = {}
        for k, v in iteritems(bstr_attrs):
            if k not in bstr_info or bstr_info[k] != v:
                raise Exception('table at path {} @ {} exists and does not correspond file to be shared, aborting'.format(yt_path, cluster))
        logger.warn('table at path {} @ {} already exists, will share it as is'.format(yt_path, cluster))
    else:
        write_start = time.time()
        dir_attributes = {}
        if blob_table_ttl:
            dir_attributes['expiration_time'] = expiration_time(seconds=blob_table_ttl)
        client.create('map_node', os.path.dirname(yt_path), recursive=True, ignore_existing=True, attributes=dir_attributes)
        for attempt_no in range(retries + 1):
            try:
                with client.Transaction():
                    client.create(
                        "table",
                        yt_path,
                        attributes={
                            "enable_skynet_sharing": True,
                            "optimize_for": "scan",
                            "schema": [
                                {"group": "meta", "name": "filename", "type": "string", "sort_order": "ascending"},
                                {"group": "meta", "name": "part_index", "type": "int64", "sort_order": "ascending"},
                                {"group": "meta", "name": "sha1", "type": "string"},
                                {"group": "meta", "name": "md5", "type": "string"},
                                {"group": "meta", "name": "data_size", "type": "int64"},
                                {"group": "data", "name": "data", "type": "string"}
                            ],
                            "_bstr_attrs": bstr_attrs
                        }
                    )
                    logger.debug('table created')
                    with open(file_path, 'rb') as f:
                        client.write_table(yt_path, iter_file_for_blob_table(f, filename))
                logger.info('writing table complete in %s seconds', int(time.time() - write_start))
                if attempt_no:
                    logger.warning('writing table failed attempts count is %d', attempt_no)
                break
            except Exception:
                logger.exception("attempt #%d: writing table failed", attempt_no + 1)
    share_start = time.time()
    rbtorrent = yw.sky_share('{}["{}"]'.format(yt_path, filename), client=client, enable_fastbone=True)
    logger.info('yt sky_share complete in %s seconds', int(time.time() - share_start))
    if blob_table_ttl:
        set_ttl(client, os.path.dirname(yt_path), seconds=blob_table_ttl)

    return share_file_common(file_path, rbtorrent)


def send_int(proc):
    proc.send_signal(signal.SIGINT)


class Cleanup(object):
    def __init__(self):
        self._lst = []

    def register(self, func):
        self._lst.append(func)

    def at_exit(self):
        for x in reversed(self._lst):
            try:
                logger.debug('will run %s', x)
                x()
            except Exception as e:
                logger.exception('at axit: %s', e)

    def register_proc(self, proc):
        def shut_up():
            send_int(proc)
            proc.wait()

        self._lst.append(shut_up)

    def popen(self, *args, **kwargs):
        proc = subprocess.Popen(*args, **kwargs)

        self.register_proc(proc)

        return proc


def calc_once(f):
    def w():
        try:
            f.__result__
        except AttributeError:
            f.__result__ = f()

        return f.__result__

    return w


@calc_once
def at_exit():
    return Cleanup()


def iter_tout(initial=1.0, max=20.0):
    tout = initial

    while True:
        yield tout * (0.5 + random.random())
        tout = min(tout * 1.5, max)


def compactify(lst, expiration_time):
    v = set()
    c = time.time()
    m = expiration_time

    for x in sorted(lst, key=lambda x: (x['name'], -x['mtime'])):
        if c - x['mtime'] > m:
            logger.info('drop stale %s', x)

            continue

        if x['name'] in v:
            logger.info('drop old %s', x)
        else:
            v.add(x['name'])

            yield x


def get_base_info_from_spec(spec):
    return {'name': os.path.basename(spec['file']), 'extra': spec['extra_info'], 'process_id': spec['process_id']}


def log_stages(function):
    def wrapper(spec, stages_logger=None):
        if stages_logger is None:
            stages_logger = StagesLogger()

        def event_callback(event_type, error=None, info=None):
            try:
                stages_logger.log_stage(
                    base_info=get_base_info_from_spec(spec),
                    process_type='Push',
                    event_type=event_type,
                    status='Fail' if error else 'OK',
                    error=error,
                    info=info,
                )
            except Exception as e:
                logger.error('Exception occured during event callback: {}'.format(e))

        def start_callback(**kwargs):
            return event_callback('Start', **kwargs)

        def finish_callback(**kwargs):
            return event_callback('Finish', **kwargs)

        try:
            start_callback()
            function(spec)
            finish_callback()
        except Exception as e:
            finish_callback(error=str(e))
            raise e

    return wrapper


@log_stages
def run(spec):
    assert 'file' in spec
    assert 'token' in spec
    assert ('root' in spec) or ('cypress_dir' in spec), 'Expected --root or --cypress-dir'

    ml(logger.info, 'spec: %s' % dump_json(spec))

    path_to_share = spec['file']
    if path_to_share.startswith('rbtorrent:'):
        # rbtorrent:012345abcd
        info = share_torrent(path_to_share, file_info=spec.get('file_info'))
    elif '://' in path_to_share:
        # hahn://tmp/myblobtable["file.bin"]
        info = share_blob_table(path_to_share, token=spec['token'], blob_table_ttl=spec.get('set_blob_table_ttl'))
    else:
        # path/to/file
        if spec.get('write_to_blob_table'):
            info = write_and_share_blob_table(
                path_to_share,
                spec['write_to_blob_table'],
                token=spec['token'],
                yt_heavy_request_timeout=spec.get('yt_heavy_request_timeout') or spec['timeout'],
                blob_table_ttl=spec.get('set_blob_table_ttl'),
                content_encoding=spec.get('blob_table_write_encoding'),
                retries=spec.get('blob_table_write_retries')
            )
        else:
            info = share_file(path_to_share, spec)

    if spec.get('force') and not spec.get('force_real_mtime'):
        info['mtime'] = long(time.time())

    if spec.get('force_mtime'):
        info['mtime'] = long(spec.get('force_mtime'))

    if 'freeze_time' in spec:
        info['freeze_timestamp'] = info['now'] + spec['freeze_time']

    info['priority'] = spec['priority']

    if spec['extra_info'] is not None:
        info['extra'] = spec['extra_info']

    file_ttl_seconds = str2seconds(spec['file_ttl'])

    ml(logger.info, 'share: %s' % dump_json(info))

    client = yw.YtClient(proxy=spec['proxy'], token=spec['token'])

    def try_store(store):
        try:
            store()
            return True
        except (YtRequestRateLimitExceeded, YtRequestQueueSizeLimitExceeded):
            pass
        except Exception as e:
            if 'lock is taken by concurrent transaction' not in str(e):
                if 'is locked by concurrent transaction' not in str(e):
                    if 'Timed out while waiting' not in str(e):
                        raise e

            if spec['waitable_lock']:
                logger.debug('will retry %s', e)
            else:
                logger.debug('will retry %s, after %s seconds', e, tout)

        return False

    def store_dir():
        # raise Exception('lock is taken by concurrent transaction')
        # raise YtError("Timed out while waiting")

        if 'cypress_dir' not in spec:
            return

        with client.Transaction():
            dir_path = spec['cypress_dir']
            path = dir_path + '/' + info['name']
            path_info = path + '/@_bstr_info'

            client.create(
                'document', path, recursive=True, ignore_existing=True,
                attributes={'_bstr_info': {}, 'expiration_time': expiration_time(seconds=file_ttl_seconds)}
            )
            client.lock(path, waitable=spec['waitable_lock'], wait_for=spec['lock_timeout'])

            old_info = client.get(path_info, attributes=['_bstr_info'])
            ml(logger.info, 'old_info: %s' % dump_json(old_info))

            if (
                spec.get('force') or (
                    (info['now'] > old_info.get('freeze_timestamp', 0)) and
                    (spec.get('force_real_mtime') or ("mtime" not in old_info) or (old_info['mtime'] < info['mtime']) or
                     (spec.get('ignore_mtime_check') and old_info['torrent'] != info['torrent']))
                )
            ):
                client.create(
                    'document', path, recursive=True, ignore_existing=False, force=True,
                    attributes={'_bstr_info': info, 'expiration_time': expiration_time(seconds=file_ttl_seconds)}
                )
            else:
                logger.warn('skip due to mtime/freeze_time check')

    def store():
        # raise Exception('lock is taken by concurrent transaction')
        # raise YtError("Timed out while waiting")

        if 'root' not in spec:
            return

        with client.Transaction():
            path = spec['root']
            client.create('document', path, recursive=True, ignore_existing=True)
            client.lock(path, waitable=spec['waitable_lock'], wait_for=spec['lock_timeout'])

            data = client.get(path)

            if not data:
                data = []

            client.set(path, list(compactify(data + [info], expiration_time=file_ttl_seconds)))

    success_store = False
    success_store_dir = False
    for tout in iter_tout(max=60):
        if success_store and success_store_dir:
            break

        if not success_store_dir:
            success_store_dir = try_store(store_dir)

        if not success_store:
            success_store = try_store(store)

        if not spec['waitable_lock']:
            time.sleep(tout)

    if spec['quorum'] == 0:
        logger.info('complete share')
        return
    else:
        logger.info('complete share, will wait for quorum')

    start_ts = time.time()

    def complete():
        tracker_info = json.loads(subprocess.check_output(
            ['/skynet/tools/skybone-ctl', '-f', 'json', 'resource-info', info['torrent']],
            timeout=20, shell=False)
        )
        complete_hosts = max(tracker_info[info['torrent']]['seeders'] - 1, 0)

        logger.debug('complete: %s', complete_hosts)

        if complete_hosts >= spec['quorum']:
            logger.info('have quorum(%s)', complete_hosts)

            return True
        else:
            logger.info('no quorum(have %s hosts)', complete_hosts)

    for tout in iter_tout():
        if (time.time() - start_ts) > spec['timeout']:
            raise Exception('quorum gathering timeout')

        try:
            if complete():
                return
        except Exception as e:
            logger.error('cannot fetch tracker info: %s', e)

        time.sleep(tout)


def main(args, **settings):
    at_exit()

    # for YT
    os.environ['PATH'] = '/skynet/tools:/usr/local/bin:' + os.environ.get('PATH', '/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin')
    os.environ['YT_ALLOW_HTTP_REQUESTS_TO_YT_FROM_JOB'] = '1'
    yt.logger.LOGGER.setLevel(os.environ.get('YT_LOG_LEVEL', 'ERROR'))

    parser_parents = filter(lambda x: x, [settings.get('init_parser')])
    parser = argparse.ArgumentParser(parents=parser_parents)

    parser.add_argument('--proxy', default='locke', help='YT cluster for push/pull communication')
    parser.add_argument('-r', '--root', help='Cypress path for push/pull communication')
    parser.add_argument('-s', '--spec', help='Config file, in json format')
    parser.add_argument('-t', '--timeout', help='Timeout for transfer. Bstr will wait for quorum of machines with shared file, or for timeout, which comes first')
    parser.add_argument('-p', '--priority', type=float, default=1.0, help='Weight multiplier for bstr pull mode')
    parser.add_argument('-q', '--quorum', help='See --timeout')
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('--quiet', action='store_true')
    parser.add_argument('-f', '--file', help='File to transfer')
    parser.add_argument('--file-info', type=json.loads, help='`sky files --json <rbtorrent>` output of file to transfer')
    parser.add_argument('--file-ttl', type=str, default='7d', help='File TTL. TTL is a string should be parsable by parsedatetime module.')
    parser.add_argument('--cypress-dir', help='Cypress directory for push/pull communication')
    parser.add_argument('--force', action='store_true', help='Force transfer, even for old files')
    parser.add_argument('--force-real-mtime', action='store_true', help='Publish file with its real mtime, even if it is old')
    parser.add_argument('--freeze-time', type=int, help='Set this argument to suppress futher push publications of file for a given time (in seconds)')
    parser.add_argument('--force-mtime', type=long, help='Set specified mtime')
    parser.add_argument('--ignore-mtime-check', action='store_true', help='Ignore mtime check when publishing base file')
    parser.add_argument('--token', help='YT token')
    parser.add_argument('--ts', action='store_true', help='Enable log timestamps')
    parser.add_argument('--waitable-lock', action='store_true', help='Use waitable YT lock')
    parser.add_argument('--lock-timeout', type=int, default=300000, help='YT waitable lock timeout (milliseconds)')
    parser.add_argument('--extra-info', type=json.loads, help='JSON with extra info attributes')
    parser.add_argument('--write-to-blob-table', type=str, help='If set, bstr will write file to YT blob table at given path and then share it')
    parser.add_argument('--set-blob-table-ttl', type=int, help='TTL (in seconds) for blob table to be shared')
    parser.add_argument('--blob-table-write-encoding', type=str, default='gzip', help='YT client write_table content encoding')
    parser.add_argument('--blob-table-write-retries', type=int, default=2, help='Number of retries to create + write YT blob table (in case something with YT goes wrong)')
    parser.add_argument('--yt-heavy-request-timeout', type=int, help='YT client heavy_request_timeout config value used for writing blob table (milliseconds)')
    parser.add_argument('--compress', default=None, help='set compression method (zstd, ...)')
    parser.add_argument('--compress-level', type=int, default=4, help='set compression level')
    parser.add_argument('--compress-threads', type=int, default=0, help='set count of threads to compress file (0=autodetect, default)')

    args = parser.parse_args(args)
    spec = init(args, init_logger=settings.get("init_logger", True))

    spec['proxy'] = args.proxy

    spec['priority'] = args.priority

    if args.force:
        spec['force'] = True

    if args.force_real_mtime:
        spec['force_real_mtime'] = True

    if args.freeze_time:
        spec['freeze_time'] = args.freeze_time

    if args.force_mtime:
        spec['force_mtime'] = args.force_mtime

    if args.ignore_mtime_check:
        spec['ignore_mtime_check'] = args.ignore_mtime_check

    if args.root:
        spec['root'] = args.root

    if args.cypress_dir:
        spec['cypress_dir'] = args.cypress_dir

    if args.timeout:
        spec['timeout'] = long(args.timeout)

    if 'timeout' not in spec:
        spec['timeout'] = 1000000000

    if args.quorum:
        spec['quorum'] = long(args.quorum)

    if 'quorum' not in spec:
        spec['quorum'] = 0

    if args.file:
        spec['file'] = args.file

    if args.file_info:
        if not spec['file'].startswith('rbtorrent:'):
            raise Exception('--file-info can be used only if rbtorrent specified for --file argument')
        spec['file_info'] = args.file_info
    spec['file_ttl'] = args.file_ttl

    spec['compress'] = args.compress
    spec['compress-level'] = args.compress_level
    spec['compress-threads'] = args.compress_threads

    global_timeout = int(spec['timeout'] * 1.2 + 30)

    def timeout_signal_handler(*args):
        raise Exception('global timeout occured ({} s)'.format(global_timeout))

    if isinstance(threading.current_thread(), threading._MainThread):
        signal.signal(signal.SIGALRM, timeout_signal_handler)
    signal.alarm(global_timeout)

    spec['waitable_lock'] = args.waitable_lock
    spec['lock_timeout'] = args.lock_timeout

    if args.extra_info:
        spec['extra_info'] = args.extra_info
    else:
        spec['extra_info'] = None

    spec['process_id'] = str(uuid.uuid4())

    if args.write_to_blob_table:
        spec['write_to_blob_table'] = args.write_to_blob_table

    if args.set_blob_table_ttl:
        spec['set_blob_table_ttl'] = args.set_blob_table_ttl

    spec['yt_heavy_request_timeout'] = args.yt_heavy_request_timeout
    spec['blob_table_write_encoding'] = args.blob_table_write_encoding
    spec['blob_table_write_retries'] = args.blob_table_write_retries

    stages_logger = settings.get('stages_logger')
    try:
        spec['token'] = get_yt_token(args)

        run(spec, stages_logger=stages_logger)
    finally:
        at_exit().at_exit()
