import re2
import argparse
import hashlib
import json
import logging
import math
import os
import random
import shutil
import threading
import time
import uuid
import yaml
import copy
import traceback
import library.python.bstr.compress as compress

from collections import defaultdict
from functools import reduce

from humanfriendly import format_size
from tabulate import tabulate
from marisa_trie import Trie

import watchdog.observers
import yt.wrapper as yw

from library.python.bstr.common import init, ml, dump_json, iter_lines, get_yt_token, BaseWeightEstimator, UidStat
from library.python.bstr.stages_logger import StagesLogger
from library.python.bstr.agent import SolomonAgent, Kind, DEFAULT_AGENT_PORT, avg, DummyAgent

import six
from six.moves import queue as Queue
if six.PY2:
    import subprocess32 as subprocess
else:
    import subprocess
    long = int


logger = logging.getLogger(__name__)

B = 1
KB = B * 1024
MB = KB * 1024
GB = MB * 1024


def check_direct_io():
    def get_config_value(cfg, key):
        return reduce(lambda c, k: c[k], key.split('.'), cfg)

    try:
        with open('/Berkanavt/supervisor/skycore/var/skynet/skybone/conf/configuration.yaml', 'r') as f:
            sky_conf = yaml.load(f)
            return get_config_value(sky_conf, 'subsections.skynet.subsections.services.subsections.copier.config.config.seed_direct_io')
    except Exception as e:
        logger.debug('in check_direct_io(): %s', e)

    return False


def notify_copier(paths):
    """
    Accepts a list of deleted files or directories. Makes copier daemon stop seeding
    resources that contain these files/directories.
    """
    cmd = ['/skynet/tools/skybone-ctl', 'notify']
    # Copier expects absolute dom0 paths, while we might be inside a container chroot.
    # Insert a wildcard in the beginning to make paths match.
    cmd.extend('*' + os.path.realpath(path) for path in paths)
    try:
        subprocess.check_output(cmd, stderr=subprocess.STDOUT, timeout=5)
        return True
    except subprocess.CalledProcessError as e:
        logger.warning(
            'skybone-ctl exited with code %s: %s',
            e.returncode, e.output
        )
    except Exception as e:
        logger.warning('Failed to call skybone-ctl: %s', e)
    return False


def check_copier_notifier():
    if not notify_copier(['nonexistent_path']):
        logger.warning('Disabling copier notifications')
        return False

    return True


def str_md5(data):
    return hashlib.md5(six.ensure_binary(data)).hexdigest()


def struct_md5(data):
    return str_md5(json.dumps(data, sort_keys=True))


def hard_exit():
    os._exit(100)


def safe_rm(path):
    if os.path.isdir(path):
        try:
            shutil.rmtree(path)
        except Exception as e:
            logger.warning('cannot remove %s: %s', path, e)
    else:
        try:
            os.unlink(path)
        except Exception as e:
            logger.warning('cannot remove %s: %s', path, e)


def rm_many(lst, root, copier_notifications_enabled=False):
    removed = []
    for path in sorted(frozenset(lst), key=lambda x: (-x.count('/'), -len(x), x)):
        if path.startswith(root):
            logger.warning('purge %s', path)
            safe_rm(path)
            removed.append(path)
        else:
            logger.warning('shit happened, cannot delete \'%s\' cause path not startwith \'%s\'', path, root)

    if removed and copier_notifications_enabled:
        notify_copier(removed)


def calc_size(path):
    res = 0

    for a, b, c in os.walk(path):
        for d in c:
            res += os.stat(os.path.join(a, d)).st_size

    return res


def start_watchers(infos):
    wlogger = logger.getChild('watchdog')

    def calc_md5(data):
        def iter_lines():
            for f in data.split('\n'):
                f = f.strip()

                if f:
                    yield f

        return struct_md5(sorted(frozenset(iter_lines())))

    def do_run_watch(info):
        path = info['path']
        ev_q = Queue.Queue()

        class WD(object):
            def dispatch(self, event):
                wlogger.debug('watchdog on %s', path)
                ev_q.put(event)

        observer = watchdog.observers.Observer()

        observer.schedule(WD(), os.path.dirname(path))
        observer.start()

        while True:
            try:
                ev_q.get(True, 30 * (random.random() + 0.5))
            except Queue.Empty:
                pass

            with open(path, 'r') as f:
                new_data = f.read()
                old_data = info['data']

                if calc_md5(new_data) != calc_md5(old_data):
                    def fmt(x):
                        return dump_json(x.split('\n'))

                    ml(wlogger.warning, 'old %s: %s' % (path, fmt(old_data)))
                    ml(wlogger.warning, 'new %s: %s' % (path, fmt(new_data)))

                    raise Exception('Should restart')

            time.sleep(1)

    def run_watch(info):
        try:
            do_run_watch(info)
        except Exception as e:
            wlogger.error('in run_watch(): %s', e)
        finally:
            hard_exit()

    def create_watch(info):
        return lambda: run_watch(info)

    for info in infos:
        wlogger.info('start watch on %s', info['path'])

        thr = threading.Thread(target=create_watch(info))
        thr.daemon = True
        thr.start()


def safe_link(res_name, fin_name):
    tmp_name = fin_name + '.' + str(random.random())

    try:
        logger.debug('link %s -> %s', res_name, tmp_name)
        os.link(res_name, tmp_name)
    except Exception:
        logger.debug('copy %s -> %s', res_name, tmp_name)
        shutil.copyfile(res_name, tmp_name)

    try:
        logger.debug('rename %s -> %s', tmp_name, fin_name)
        os.rename(tmp_name, fin_name)
    finally:
        if os.path.exists(tmp_name):
            logger.debug('unlink %s', tmp_name)
            os.unlink(tmp_name)


def prepare_solomon_agent(spec):
    if not spec.get('enable_monitoring'):
        return DummyAgent()

    agent = SolomonAgent(port=spec['agent_port'], yabs_mode=spec['yabs_mode'])

    agent.set_grid(spec['metric_grid'])
    agent.ignore_sensors(spec['ignore_sensors'])
    agent.set_aggr_method("progress.speed_in_bytes", avg)
    agent.set_aggr_method("completed.duration", avg)
    agent.set_aggr_method("completed.retry_attempts", sum)
    agent.set_aggr_method("completed.successes", sum)
    agent.set_aggr_method("completed.errors", sum)
    return agent


def run(spec, file_weight_estimator, stages_logger, overdue_checker):
    assert 'dir' in spec
    assert 'token' in spec
    assert ('root' in spec) or ('cypress_dir' in spec), 'Expected --root or --cypress-dir'

    ml(logger.info, 'spec: %s' % dump_json(spec))

    if not spec['files']:
        raise Exception('nothing to sync')

    def log_pretty(logger_f, string, *args):
        if not spec['pretty']:
            return logger_f(string, *args)
        return ml(logger_f, string % args)

    trie, patterns = None, None
    if spec['use_trie']:
        trie = Trie(spec['files'])
    else:
        patterns = [re2.compile(pattern) for pattern in spec['files']]

    def match_files(_f):
        if spec['use_trie']:
            return _f in trie
        else:
            return any(p.match(_f) for p in patterns)

    res_dir = spec['dir']
    shadow_dir = spec['shadow_dir']

    try:
        os.makedirs(shadow_dir)
    except OSError:
        pass

    start_watchers(spec['watch'])

    solomon_agent = prepare_solomon_agent(spec)
    solomon_agent.start()

    def iter_not_satisfactory():
        oc = overdue_checker()
        for x in os.listdir(res_dir):
            path = os.path.join(res_dir, x)

            if x == 'shadow':
                continue
            elif not match_files(x):
                yield path
            elif oc.is_overdue(path):
                logger.debug('overdue detected forr %s', path)
                yield path
            else:
                logger.debug('skip %s', path)

    def iter_old():
        limit = spec.get('limit', 0)

        if not limit:
            return

        limit = limit * GB
        bad = []

        def iter_infos():
            for x in os.listdir(shadow_dir):
                path = os.path.join(shadow_dir, x)

                if os.path.isdir(path):
                    try:
                        yield {
                            'path': path,
                            'size': calc_size(path),
                            'mtime': os.stat(path).st_mtime,
                        }
                    except Exception as e:
                        logger.warning('in iter_infos(): %s', e)
                        bad.append(path)
                else:
                    bad.append(path)

        infos = sorted(iter_infos(), key=lambda x: x['mtime'])
        sum_size = sum((x['size'] for x in infos), 0)

        logger.debug('limit = %s, size = %s', limit, sum_size)

        while sum_size > limit:
            yield infos[0]['path']

            sum_size -= infos[0]['size']
            infos = infos[1:]

        for x in bad:
            yield x

    rm_many(list(iter_not_satisfactory()), res_dir)
    rm_many(list(iter_old()), shadow_dir, spec['copier_notifications_enabled'])

    q = Queue.Queue()
    in_fly = {}

    state = {
        'state': {
        },
    }

    state_lock = threading.Lock()

    watch_q = Queue.Queue()

    def run_watch_impl():
        while True:
            watch_q.get()

            try:
                rm_many(list(iter_not_satisfactory()), res_dir)
                rm_many(list(iter_old()), shadow_dir, spec['copier_notifications_enabled'])
            except Exception as e:
                logger.warning('in run_watch(): %s', e)

    def run_watch():
        try:
            run_watch_impl()
        finally:
            hard_exit()

    logger.info('spawn disk watcher thread')

    watch_thr = threading.Thread(target=run_watch)
    watch_thr.daemon = True
    watch_thr.start()

    def sync_state_impl():
        client = yw.YtClient(proxy=spec['proxy'], token=spec['token'])

        def calc_state():

            state_from_dir = {}
            if 'cypress_dir' in spec:
                try:
                    state_from_dir = dict(
                        (x.attributes['_bstr_info']['name'], x.attributes['_bstr_info'])
                        for x in client.list(
                            spec['cypress_dir'],
                            read_from='cache',
                            attributes=['_bstr_info'],
                            cache_sticky_group_size=spec['cache_sticky_group_size']
                        )
                        if '_bstr_info' in x.attributes and ('name' in x.attributes['_bstr_info']) and match_files(x.attributes['_bstr_info']['name'])
                    )
                except Exception as e:
                    logger.warn('Read from ' + spec['cypress_dir'] + ' failed: %s', e)

            state_from_root = {}
            if 'root' in spec:
                try:
                    state_from_root = dict((x['name'], x) for x in client.get(spec['root'], read_from='cache') if match_files(x['name']))
                except Exception as e:
                    logger.warn('Read from ' + spec['root'] + ' failed: %s', e)

            for key, value in state_from_dir.items():
                if (key not in state_from_root) or (value['mtime'] > state_from_root[key]['mtime']):
                    state_from_root[key] = value

            return state_from_root

        while True:
            try:
                new_state = calc_state()

                with state_lock:
                    old_state = state.get('state', {})

                    if struct_md5(old_state) != struct_md5(new_state):
                        log_pretty(logger.debug, 'new state: %s', dump_json(new_state, pretty=spec['pretty']))
                        state['state'] = new_state
                        q.put(None)
            except Exception as e:
                logger.warning('in sync_state_1(): %s', e)

            time.sleep(spec['poll_period'] * (0.5 + random.random()))

    def sync_state():
        try:
            sync_state_impl()
        except Exception as e:
            logger.error('in sync_state_outer(): %s', e)
        finally:
            hard_exit()

    def get_state():
        with state_lock:
            return state['state'].copy()

    name_to_mtime = {}
    high_prio_files = {}

    def file_mtime(path):
        try:
            return long(os.stat(path).st_mtime)
        except OSError:
            return 0

    def safe_mtime(path, ftime):
        return name_to_mtime.get(os.path.basename(path), ftime)

    def get_full_state():
        for f, info in get_state().items():
            info = info.copy()

            fpath = os.path.join(res_dir, f)

            info['ftime'] = file_mtime(fpath)
            info['ltime'] = safe_mtime(fpath, info['ftime'])
            info['uid'] = info['name'] + '_' + info['torrent']
            info['prev_now'] = high_prio_files.get(info['uid'])
            info['descr'] = '(name = ' + info['name'] + ', torrent = ' + info['torrent'] + ')'
            info['process_id'] = str(uuid.uuid4())

            yield info

    logger.info('spawn state updater thread')

    sync_thr = threading.Thread(target=sync_state)
    sync_thr.daemon = True
    sync_thr.start()

    def base_shadow_dir(base_uid):
        return os.path.join(shadow_dir, base_uid.replace(':', '_'))

    def base_complete_marker_path(base_shadow_dir):
        return os.path.join(base_shadow_dir, 'complete')

    def fetch(info):
        tlogger = logger.getChild(info['uid'][-15:])
        fetch_state = dict(
            name=info['name'],
            source=info['torrent'],
            network=spec.get('network'),
            start_ts=None,
            start_ts_int=None,  # To join logs with other systems
            attempts=None,
            sky_exit_code=None,
            time_total=None,    # May be add times by stage and attempts later
            done_bytes=None,
            total_bytes=None,
        )

        def show_err(err_parts):
            if err_parts:
                tlogger.error(
                    'Got non-JSON output from sky process for %s. It seems that an error has been occured.' +
                    'Folowing lines were given:\n%s', info['descr'], '\n'.join(err_parts)
                )
                err_parts[:] = []

        stale_speed = spec.get('stale_speed')
        stale_time = spec.get('stale_time')
        try:
            where = base_shadow_dir(info['uid'])
            ddir = os.path.join(where, 'download')

            try:
                log_pretty(tlogger.info, 'will fetch %s', dump_json(info, pretty=spec['pretty']))

                def iter_args():
                    common = [
                        '/usr/local/bin/sky',
                        'get',
                        '-u',
                        '-d', ddir,
                        '--progress',
                        '--progress-format', 'json',
                        '--progress-version', '1',
                    ]

                    for c in common:
                        yield c

                    speed = spec.get('speed')

                    if speed:
                        if info.get('no_speed_limit', False):
                            logger.warning('%s: speed limit is turned off for this file', info['descr'])
                        else:
                            yield '--max-dl-speed'
                            yield str(speed)

                    def iter_opts():
                        if spec.get('subproc'):
                            yield 'subproc: 1'

                        if spec.get('skybit'):
                            yield 'use_skybit_data: 1'

                        if spec.get('directio'):
                            yield 'direct_write: 1'
                            yield 'direct_read: 1'

                        sync_writes_period = spec.get('sync_writes_period')
                        if sync_writes_period:
                            yield 'sync_writes_period: {}'.format(sync_writes_period)

                        yield 'mtime: ' + str(long(info['mtime']))

                    opts = list(iter_opts())

                    if opts:
                        yield '--opts'
                        yield '\n'.join(opts) + '\n'

                    net = spec.get('network')

                    if net:
                        yield '-N'
                        yield net

                    yield info['torrent']

                cmd = list(iter_args())

                tlogger.info('will spawn %s', ' '.join(cmd))

                # Set common labels for file being pulled
                file_labels = info.get('custom_labels', {})

                start = time.time()
                now = start
                normal_speed_time = start
                done_bytes_time = start
                done_bytes = 0

                interval_start = start
                interval_done_bytes = 0

                fetch_state['start_ts'] = start
                fetch_state['start_ts_int'] = int(start)
                retry_attempts = 0
                stale_speed_attempts = 0
                while True:
                    proc = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                    err_lines = []

                    for l in iter_lines(proc.stdout):
                        try:
                            if l.startswith('{'):
                                show_err(err_lines)
                                data = json.loads(l)
                            else:
                                err_lines.append(l)
                                continue

                            if data.get('stage', '') == 'get_resource':
                                now = time.time()
                                speed = (data['done_bytes'] - done_bytes) / max(now - done_bytes_time, 1)
                                elapsed = int(now - start)
                                percent = 100 * data['done_bytes'] // max(data['total_bytes'], 1)
                                tlogger.debug(
                                    '%s: complete %s%%, %s from %s (%s / %s), avg. speed %s/s, current speed %s/s',
                                    info['name'],
                                    percent,
                                    format_size(data['done_bytes'], binary=True),
                                    format_size(data['total_bytes'], binary=True),
                                    data['done_bytes'],
                                    data['total_bytes'],
                                    format_size(data['done_bytes'] / max(now - start, 1)),
                                    format_size(speed)
                                )
                                fetch_state['done_bytes'] = data['done_bytes']
                                fetch_state['total_bytes'] = data['total_bytes']
                                if stale_speed and speed > stale_speed:
                                    normal_speed_time = now
                                interval_duration = now - interval_start
                                if stale_time and interval_duration > stale_time:
                                    interval_avg_speed = (data['done_bytes'] - interval_done_bytes) / interval_duration
                                    interval_done_bytes = data['done_bytes']
                                    interval_start = now
                                    tlogger.info(
                                        '%s: complete %s%%, %s from %s (%s / %s), avg. speed for last %s s: %s/s, current speed %s/s',
                                        info['name'],
                                        percent,
                                        format_size(data['done_bytes'], binary=True),
                                        format_size(data['total_bytes'], binary=True),
                                        data['done_bytes'],
                                        data['total_bytes'],
                                        interval_duration,
                                        format_size(interval_avg_speed),
                                        format_size(speed)
                                    )
                                    if stale_speed and interval_avg_speed < stale_speed / 10:
                                        tlogger.warning(
                                            (
                                                '[ABORT] {desc} : {torrent}, stale for last {interval}s (avg.speed: {avg_speed}/s)'
                                                'at progress {percent}% after {elapsed}s, last speed {speed}/s, terminate'
                                            ).format(
                                                desc=info['name'],
                                                torrent=info['torrent'],
                                                interval=interval_duration,
                                                avg_speed=format_size(interval_avg_speed),
                                                percent=percent,
                                                elapsed=elapsed,
                                                speed=format_size(speed)
                                            )
                                        )
                                        proc.terminate()
                                        stale_speed_attempts += 1
                                if stale_time and now - normal_speed_time > stale_time:
                                    tlogger.warning(
                                        '[ABORT] {desc} : {torrent}, stale for {stale_for}s at progress {percent}% after {elapsed}s, last speed {speed}/s, terminate'.format(
                                            desc=info['name'],
                                            torrent=info['torrent'],
                                            stale_for=now - normal_speed_time,
                                            percent=percent,
                                            elapsed=elapsed,
                                            speed=format_size(speed)
                                        )
                                    )
                                    proc.terminate()
                                    stale_speed_attempts += 1
                                if info['name'] in in_fly and in_fly[info['name']]['uid'] != info['uid']:
                                    tlogger.warning(
                                        '[ABORT] {desc} : {torrent}, new torrent is in fly for this name (uid={new_uid}, priority={new_priority}), terminate'.format(
                                            desc=info['name'],
                                            torrent=info['torrent'],
                                            new_uid=in_fly[info['name']]['uid'],
                                            new_priority=in_fly[info['name']]['priority']
                                        )
                                    )
                                    proc.terminate()

                                solomon_agent.push_metric('progress.size_in_bytes', file_labels, value=data['done_bytes'], kind=Kind.IGAUGE)
                                solomon_agent.push_metric('progress.speed_in_bytes', file_labels, value=speed)

                                done_bytes = data['done_bytes']
                                done_bytes_time = now
                        except Exception:
                            tlogger.error('info: %s, exception: %s', l, traceback.format_exc())

                    show_err(err_lines)
                    proc.wait()
                    etime = time.time() - start
                    fetch_state['sky_exit_code'] = proc.returncode
                    fetch_state['time_total'] = etime
                    fetch_state['attempts'] = retry_attempts + 1
                    fetch_state['stale_speed_attempts'] = stale_speed_attempts
                    info['finish_ts'] = int(time.time())

                    if proc.returncode:
                        if proc.returncode == 7 and spec.get('retry-after-exit-code-7', False):
                            if retry_attempts >= 1:     # May be add other codes and number of attempts later
                                raise Exception('got %s retcode from sky get for file %s (attempts %s)' %
                                                (proc.returncode, info['name'], retry_attempts))
                            retry_attempts += 1
                            solomon_agent.push_metric('completed.retry_attempts', file_labels, value=1, kind=Kind.COUNTER)
                            continue

                        solomon_agent.push_metric('completed.successes', file_labels, value=0, kind=Kind.IGAUGE)
                        solomon_agent.push_metric('completed.errors', file_labels, value=1, kind=Kind.IGAUGE)

                        raise Exception('got %s retcode from sky get for file %s' % (proc.returncode, info['name']))
                    else:
                        break

                solomon_agent.push_metric('completed.duration', file_labels, value=fetch_state['time_total'], kind=Kind.DGAUGE)
                solomon_agent.push_metric('completed.successes', file_labels, value=1, kind=Kind.IGAUGE)
                solomon_agent.push_metric('completed.errors', file_labels, value=0, kind=Kind.IGAUGE)

                tlogger.info('%s: elapsed %s seconds, total avg. speed %s/s', info['descr'], etime, (format_size(info['fsize'] / etime)) if etime > 0 else '??')
                if 'fetch-state-file' in spec:
                    with open(spec['fetch-state-file'], 'a') as f:
                        json.dump(fetch_state, f)
                        f.write('\n')

                def get_file_path():
                    for x in os.listdir(ddir):
                        xx = os.path.join(ddir, x)

                        if os.path.isfile(xx):
                            return xx

                    raise Exception('empty torrent %s', info['torrent'])

                new_shadow_path = get_file_path()
                target_path = os.path.join(res_dir, info['name'])

                if 'compress' in info:
                    compress.safe_decompress(
                        method=info['compress'],
                        infile=new_shadow_path,
                        outfile=target_path,
                        threads=spec['decompress-threads']
                    )
                else:
                    safe_link(new_shadow_path, target_path)

                with open(os.path.join(where, 'meta.json'), 'w') as f:
                    f.write(json.dumps(info, indent=4, sort_keys=True))

                with open(base_complete_marker_path(where), 'w'):
                    pass

                os.utime(where, None)

                if spec.get('callback'):
                    callback_substitutions = [('%p', target_path), ('%P', new_shadow_path), ('%r', info['torrent'])]
                    cb = spec['callback']
                    for old, new in callback_substitutions:
                        cb = cb.replace(old, new)

                    tlogger.info('will run %s', cb)

                    try:
                        out = subprocess.check_output(['/bin/bash', '-c', cb], stderr=subprocess.STDOUT, shell=False)

                        if out:
                            ml(tlogger.info, 'from callback:\n%s' % out)
                    except subprocess.CalledProcessError as e:
                        if e.output:
                            ml(tlogger.warning, 'from callback:\n%s' % e.output)
                        tlogger.warning('in user callback: %s', e)
                    except Exception as e:
                        tlogger.warning('in user callback: %s', e)
            except Exception as e:
                info['exc'] = e
                info['traceback'] = traceback.format_exc()
        finally:
            q.put(info)
            watch_q.put(None)

    def gen_fetch(info):
        return lambda: fetch(info)

    def iter_complete():
        n = 0

        try:
            while True:
                yield q.get_nowait()
                n += 1
        except Queue.Empty:
            pass

        try:
            if not n:
                yield q.get(True, 5 * (random.random() + 0.5))
        except Queue.Empty:
            pass

    bad_uids = defaultdict(UidStat)

    def on_bad_uid(uid, publication_time):
        if len(bad_uids) > 10000:
            bad_uids.clear()

        bad_uids[(uid, publication_time)].inc()

    on_bad_uid('', 0)

    def on_priority_file(uid, publication_time):
        if len(high_prio_files) > 10000:
            high_prio_files.clear()

        high_prio_files[uid] = publication_time

    last_bad_res = {}

    def store_err(res):
        err_res = copy.deepcopy(res)
        e = err_res['exc']
        err_res['exc'] = str(e)
        last_bad_res.update({
            'file_info': err_res,
            'err': str(e),
            'err_type': type(e).__name__,
            'traceback': err_res.get('traceback'),
            'ts': time.time()
        })

    def process_complete(callback):
        for res in iter_complete():
            if not res:
                continue

            del in_fly[res['name']]

            if 'exc' in res:
                logger.error('%s error: %s', res['descr'], res['exc'])
                logger.error(res['traceback'])
                store_err(res)
                on_bad_uid(res['uid'], res['now'])
            else:
                name_to_mtime[res['name']] = res['mtime']
                log_pretty(logger.debug, 'mtimes: %s', dump_json(name_to_mtime, pretty=spec['pretty']))
                if res.get('priority') > 1.0:
                    on_priority_file(res['uid'], res['now'])
                logger.info('%s: download complete', res['descr'])

            callback(base_info=res, error=res.get('exc'))

    def next_file():
        fw = file_weight_estimator(bad_uids)

        def iter_infos():
            for info in get_full_state():
                if info['name'] not in in_fly or (
                    in_fly[info['name']]['uid'] != info['uid'] and
                    info.get('priority', 0) > in_fly[info['name']]['priority']
                ):
                    weight = fw.get_final_weight(info)
                    info['custom_labels'] = fw.get_custom_labels(info)

                    if weight is not None:
                        info['weight'] = weight
                        yield info

        lst = sorted(iter_infos(), key=lambda x: -x['weight'])

        lst_log_data = [[
            x['name'],
            x['torrent'],
            "{:.0f}".format(x['mtime']),
            format_size(x['fsize']),
            "{:.0f}".format(x['now']),
            x['prev_now'] and "{:.0f}".format(x['prev_now']),
            x['weight'],
            x['priority'],
            "{:.0f}".format(x['ftime'])
        ] for x in lst]

        if spec.get('dump_state'):
            with open('bstr_state.json', 'w') as state_f:
                json.dump({
                    'in_fly': in_fly,
                    'queue': lst,
                    'last_err': last_bad_res,
                }, state_f, indent=4)

        logger.info('Current file queue:\n%s', tabulate(
            lst_log_data,
            headers=[
                'Filename',
                'Torrent',
                'MTime',
                'Size',
                'Published at',
                'Prev. publ.',
                'Weight',
                'BSTR Priority',
                'Cur. mtime'
            ],
            tablefmt='orgtbl')
        )

        log_pretty(logger.debug, 'stats: %s', dump_json(lst, pretty=spec['pretty']))
        log_pretty(logger.debug, 'files_prioritization_strategy: %s', file_weight_estimator.__name__)
        log_pretty(logger.info, 'in_fly files: %s', dump_json(in_fly, pretty=spec['pretty']))

        if lst:
            lst[0]['start_ts'] = int(time.time())
            if fw.no_limit_condition(lst, spec):
                lst[0]['no_speed_limit'] = True
                logger.info('set no_speed_limit=True for the next file')
            return lst[0]

    def event_callback(event_type, base_info, error=None, info=None):
        try:
            stages_logger.log_stage(
                base_info=base_info,
                process_type='Pull',
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

    while True:
        process_complete(finish_callback)

        while len(in_fly) < spec['threads']:
            info = next_file()

            if not info:
                break

            stage_info = 'Weight={weight}, priority={priority}'.format(
                weight=info['weight'],
                priority=info['priority']
            )
            start_callback(base_info=info, info=stage_info)

            thr = threading.Thread(target=gen_fetch(info))
            thr.daemon = True
            thr.start()

            in_fly[info['name']] = {
                'uid': info['uid'],
                'bstr_start_timestamp': info.get('start_ts', -1),
                'priority': info.get('priority', 1),
            }


class DefaultWeightEstimator(BaseWeightEstimator):
    def __init__(self, bad_uids):
        self.bad_uids = bad_uids

    def get_final_weight(self, info):
        weight = (info['mtime'] - info['ltime']) * (5.0 + random.random()) * info.get('priority', 1.0) / (self.bad_uids.get((info['uid'], info['now']), UidStat()).count + 6.0)
        if weight > 1:
            weight_denominator = max(1, info.get('fsize', 2 * MB) // MB)
            return (weight ** 0.1) / (1 + math.log(weight_denominator)) * 100

    def get_custom_labels(self, info):
        return {'filename': info['name']}


class DefaultOverdueChecker(object):
    def is_overdue(self, path):
        return False


def main(args, **settings):
    parser_parents = filter(lambda x: x, [settings.get('init_parser')])
    parser = argparse.ArgumentParser(parents=parser_parents)

    parser.add_argument('--proxy', default='locke', help='YT cluster for push/pull communication')
    parser.add_argument('-r', '--root', help='Cypress path for push/pull communication')
    parser.add_argument('-s', '--spec', help='Config file, in json format')
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('--quiet', action='store_true')
    parser.add_argument('--log-file', type=str)
    parser.add_argument('--log-file-level', type=str)
    parser.add_argument('--rotate-log-file', action='store_true')
    parser.add_argument('-d', '--dir', help='Directory for downloaded files')
    parser.add_argument('-S', '--shadow-dir', help="Shadow directory for downloaded files")
    parser.add_argument('-j', '--threads', help='Maximim number of parallel downloads')
    parser.add_argument('-l', '--limit', help='Size limit for downloaded files, in gigabytes')
    parser.add_argument('-c', '--callback', help='Command to run on new files. Supports the following expansions: '
                                                 '%%p(for path of the updated file), '
                                                 '%%P(new shadow path of that file), '
                                                 '%%r(file rbtorrent)')
    parser.add_argument('-f', '--file', action='append', default=[], help='File name to download')
    parser.add_argument('-p', '--list', action='append', default=[], help='File with \\n-delimited list of files to '
                                                                          'download')
    parser.add_argument('-N', '--network', help='Which network to use for download, Fastbone|Backbone')
    parser.add_argument('--cypress-dir', help='Cypress directory for push/pull communication')
    parser.add_argument('--speed', help='Speed limit, process-wide')
    parser.add_argument('--skybit', action='store_true')
    parser.add_argument('--directio', action='store_true', help='Use skynet.copier direct io')
    parser.add_argument('--sync-writes-period', type=int, default=8, help="Force skynet.copier to flush page cache to disk on every 'period' pieces writes (not used if directio is enabled) ")
    parser.add_argument('--subproc', action='store_true', help='Use skynet.copier dedicated suprocess mode')
    parser.add_argument('--token', help='YT token')
    parser.add_argument('--ts', action='store_true', help='Enable timestamps in log')
    parser.add_argument('--cache_sticky_group_size', type=int, default=3, help='change yt.read cache_sticky_group_size')
    parser.add_argument('--dump-state', action='store_true', help='Dump current state (queue, in_fly files) to file bstr_state.json')
    parser.add_argument('--decompress-threads', type=int, default=0, help='set count of threads to decompress file (0=autodetect, default)')
    parser.add_argument('--retry-after-exit-code-7', action='store_true', help='retry one time sky get operation if it fails with error code 7')
    parser.add_argument('--fetch-state-file', help='file name to store fetch states in json format')
    parser.add_argument('--stale-speed', help="stale speed limit, disabled by default", type=int, default=None)
    parser.add_argument('--stale-time', help="Stale time limit, seconds", type=int, default=None)
    parser.add_argument('--enable-monitoring', action='store_true', help="Enable solomon-agent")
    parser.add_argument('--agent-port', type=int, default=DEFAULT_AGENT_PORT,
                        help="Port to run solomon-agent (default: {})".format(DEFAULT_AGENT_PORT))
    parser.add_argument('--ignore-sensors', nargs='*', default=[], help="Ignore sensor types. They will not be pushed to Solomon.")
    parser.add_argument('--metric-grid', type=int, default=1, help="Aggregation grid for metrics in seconds.")
    parser.add_argument('--yabs-mode', action='store_true', help="Yabs mode run (used for metrics only).")
    parser.add_argument('--use-trie', action='store_true', help="Use trie instead of regexps for files search in locke.")
    parser.add_argument('--disable-pretty-output', action='store_true', help="Use less pretty output formatting for better performance.")
    parser.add_argument('--poll-period', type=int, help='avg. time in seconds between two sequential scans of cypress directory')
    parser.add_argument('--disable-copier-notifications', action='store_true', help="Do not notify skynet.copier daemon when deleting files")

    args = parser.parse_args(args)
    spec = init(args, init_logger=settings.get("init_logger", True))
    logger.info('START bstr in pull mode')

    # Update spec with provided value from command line or default value
    # Order of precedence is cmdline, spec, default.
    # This allows to get all the configuration from the spec alone or
    # override spec with command line arguments.
    def set_spec(name, value, default=None):
        # Cannot use 'x or y or z' pattern because it does not differentiate between
        # '', 0 and None
        value = next((x for x in [value, spec.get(name), default] if x is not None), None)
        if value is not None:
            spec[name] = value

    set_spec('proxy', args.proxy)
    set_spec('cache_sticky_group_size', args.cache_sticky_group_size)
    set_spec('stale_speed', args.stale_speed)
    set_spec('stale_time', args.stale_time)
    set_spec('network', args.network)
    set_spec('callback', args.callback)
    set_spec('skybit', args.skybit)
    set_spec('directio', args.directio)
    set_spec('sync_writes_period', args.sync_writes_period)
    # monitoring specs
    set_spec('enable_monitoring', args.enable_monitoring)
    set_spec('agent_port', args.agent_port)
    set_spec('ignore_sensors', args.ignore_sensors)
    set_spec('metric_grid', args.metric_grid)
    set_spec('yabs_mode', args.yabs_mode)
    set_spec('use_trie', args.use_trie)
    set_spec('pretty', not args.disable_pretty_output)

    if spec.get('directio'):
        if check_direct_io():
            logger.info('direct io check OK')
        else:
            logger.error('direct io check FAILED, add "seed_direct_io: True" to skynet host config')

    set_spec('subproc', args.subproc)
    set_spec('speed', args.speed)
    set_spec('dir', args.dir, os.path.abspath('.'))
    set_spec('shadow_dir', args.shadow_dir, os.path.join(spec['dir'], 'shadow'))

    if args.limit:
        set_spec('limit', long(args.limit))
    else:
        logger.warning('no size limit on file cache, consider --limit option')

    set_spec('threads', long(args.threads) if args.threads else 4)
    set_spec('root', args.root)
    set_spec('cypress_dir', args.cypress_dir)
    set_spec('poll_period', args.poll_period, 10)

    set_spec(
        'copier_notifications_enabled',
        not args.disable_copier_notifications and check_copier_notifier()
    )

    spec['watch'] = []

    def iter_files():
        for f in spec.get('files', []):
            yield f

        for f in args.file:
            yield f

        for l in args.list:
            with open(l, 'r') as ff:
                data = ff.read()

                spec['watch'].append({
                    'data': data,
                    'path': l,
                })

                for f in data.split('\n'):
                    f = f.strip()

                    if f:
                        yield f

    spec['files'] = sorted(frozenset(iter_files()))
    spec['token'] = get_yt_token(args)
    if args.dump_state:
        spec['dump_state'] = True
    set_spec('decompress-threads', args.decompress_threads)

    set_spec('retry-after-exit-code-7', args.retry_after_exit_code_7)
    set_spec('fetch-state-file', args.fetch_state_file)

    file_weight_estimator = settings.get('file_weight_estimator', DefaultWeightEstimator)
    overdue_checker = settings.get('overdue_checker', DefaultOverdueChecker)
    stages_logger = settings.get('stages_logger', StagesLogger())

    run(spec, file_weight_estimator, stages_logger, overdue_checker)
