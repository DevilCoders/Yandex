import os
import sys
import optparse
import logging
import json
import subprocess32 as sp
import uuid
import shutil
import signal
import time
import traceback

import vcs_helpers
import yt_client as yt

from concurrent import futures


def parse_args():
    parser = optparse.OptionParser()
    parser.add_option('-c', '--config', help='Config json path')
    opts, args = parser.parse_args()

    if not opts.config:
        print>>sys.stderr, 'Config must be specified'
        sys.exit(1)

    return opts


def update_repo(repo_path, hg_bin, hg_user, hg_key):
    sp.check_call([hg_bin, 'pull', '--ssh', 'ssh -l {} -i {}'.format(hg_user, hg_key)], cwd=repo_path)


def list_all_local_repo_changesets(repo_path, hg_bin):
    p = sp.Popen([hg_bin, 'log', '-v', '--debug', '--template', '{node}\n'], stdout=sp.PIPE, stderr=sp.PIPE, cwd=repo_path)
    out, err = p.communicate()
    rc = p.wait()

    if rc != 0:
        raise Exception('Hg log failed.\nRc={}\nOut={}\nErr={}'.format(rc, out, err))

    changesets_raw = out.strip().split('\n')
    changesets = []
    for c in changesets_raw:
        if len(c) != 40:
            logging.error('Bad hg log output: %s', c)
            continue

        try:
            c = c.decode('hex')
        except TypeError as e:
            logging.error('Bad hg log output: %s (%s)', c, str(e))
            continue

        changesets.append(c)

    logging.info('Hg log returned {} changesets'.format(len(changesets)))

    return changesets


def get_repo_tip(repo_path, hg_bin):
    p = sp.Popen([hg_bin, 'log', '-r', 'tip', '-v', '--debug', '--template', '{rev}\n'], stdout=sp.PIPE, stderr=sp.PIPE, cwd=repo_path)
    out, err = p.communicate()
    rc = p.wait()

    if rc != 0:
        raise Exception('Hg log failed.\nRc={}\nOut={}\nErr={}'.format(rc, out, err))

    revs = []
    for line in out.strip().split('\n'):
        try:
            rev = int(line)
        except ValueError as e:
            logging.error('Bad hg log output: %s (%s)', line, str(e))
            continue

        revs.append(rev)

    if len(revs) != 1:
        raise Exception('Hg log returned strange output: {}'.format(out))

    return revs[0]


def get_uploaded_changesets(changesets, yt_client, yt_table, state):
    pool = futures.ThreadPoolExecutor(max_workers=8)

    logging.info('Checking {} changesets'.format(len(changesets)))

    changesets = sorted(changesets)
    begin = 0
    tasks = []
    while begin < len(changesets):
        size = min(1000, len(changesets) - begin)
        chunk = [{'hash': c} for c in changesets[begin:begin+size]]
        tasks.append(pool.submit(state.wrap_stopping, yt_client.lookup_rows, yt_table, chunk, columns=('hash', 'type')))

    uploaded_changesets = []
    for t in tasks:
        rows = t.result()  # throws
        for r in rows:
            uploaded_changesets.append(r['hash'])

    return uploaded_changesets


def load_changesets_hashes(path):
    with open(path) as f:
        data = vcs_helpers.decompress(f.read())

    assert len(data) % 20 == 0

    changesets = [data[i:i+20] for i in xrange(0, len(data), 20)]
    logging.info('Loaded {} changesets from {}'.format(len(changesets), path))
    return changesets


def dump_changesets_hashes(changesets, path):
    tmp_path = path + '_tmp' + str(uuid.uuid4())

    assert all(len(c) == 20 for c in changesets)
    data = vcs_helpers.compress(''.join(changesets))

    with open(tmp_path, 'wb') as f:
        f.write(data)

    os.rename(tmp_path, path)


def rmtree(path):
    try:
        shutil.rmtree(path)
        return
    except Exception as e:
        logging.error(e)

    sp.check_call(['rm', '-rf', path])


def terminate(p, timeout):
    p.terminate()
    logging.info('Terminate %s: sent SIGTERM', p.pid)
    start = time.time()

    while True:
        try:
            rc = p.wait(1)
            logging.info('Terminate %s: process terminated in %s seconds after first SIGTERM', p.pid, time.time() - start)
            return rc
        except sp.TimeoutExpired:
            logging.info('Terminate %s: process still runnung %s seconds after first SIGTERM', p.pid, time.time() - start)
            if time.time() - start < timeout:
                p.terminate()
                logging.info('Terminate %s: sent SIGTERM', p.pid)
                continue
            else:
                break

    logging.info('Terminate %s: process didn\'t finish in % seconds after first SIGTERM', p.pid, timeout)
    p.kill()
    start2 = time.time()
    logging.info('Terminate %s: sent SIGKILL', p.pid)
    rc = p.wait()
    logging.info('Terminate %s: process finished in %s seconds after first SIGTERM, %s seconds after SIGKILL', time.time() - start, time.time() - start2)
    return rc


def _convert(
        r1,
        r2,
        repo_path,
        hg_bin,
        store_path,
        yt_proxy,
        yt_table,
        yt_token,
        ydb_endpoint,
        ydb_database,
        ydb_token,
        aapi_proxies,
        already_converted_and_uploaded_changesets_path,
        garbage_dir,
        max_convert_count,
        state,
        dry_run
):
    session = 'conversion-' + str(uuid.uuid4())
    new_converted_changesets_path = os.path.join(garbage_dir, session + '-result')
    substore_path = os.path.join(garbage_dir, session + '-substore')

    cmd = [
        hg_bin,
        'aapi-convert',
        '--store',
        store_path,
        '--substore',
        substore_path,
        '--do-not-store-blobs-in-main-store',
        '-r',
        '{}:{}'.format(str(r1), str(r2)),
        '--yt-proxy',
        yt_proxy,
        '--yt-table',
        yt_table,
        '--ydb-endpoint',
        ydb_endpoint,
        '--ydb-database',
        ydb_database,
        '--aapi-proxies',
        aapi_proxies,
        '--all-converted-and-uploaded-changesets-path',
        already_converted_and_uploaded_changesets_path,
        '--new-converted-and-uploaded-changesets-out-path',
        new_converted_changesets_path,
        '--max-convert-count',
        str(max_convert_count),
    ]

    if dry_run:
        cmd += ['--dry-run']

    if yt_token:
        cmd += ['--yt-token', yt_token]

    if ydb_token:
        cmd += ['--ydb-token', ydb_token]

    p = sp.Popen(cmd, cwd=repo_path)
    logging.info('Converting process started(pid=%s, cwd=%s)', p.pid, repo_path)
    start = time.time()

    while True:
        try:
            rc = p.wait(5)
            logging.info('Converting process finished normally(pid=%s, time=%s, rc=%s)', p.pid, time.time() - start, rc)
            break
        except sp.TimeoutExpired:
            logging.info('Waiting converting process(pid=%s, wait_time=%s)', p.pid, time.time() - start)
            if state.stopped():
                rc = terminate(p, 60)
                logging.info('Converting process terminated(pid=%s, time=%s, rc=%s)', p.pid, time.time() - start, rc)
                break
            else:
                continue

    state.check_stopped()

    rmtree(substore_path)

    if rc != 0:
        converted_hashes = None
        if os.path.exists(new_converted_changesets_path):
            converted_hashes = load_changesets_hashes(new_converted_changesets_path)
            os.unlink(new_converted_changesets_path)
        logging.error('Aapi convert failed')
        return False, converted_hashes

    assert os.path.exists(new_converted_changesets_path)

    converted_hashes = load_changesets_hashes(new_converted_changesets_path)
    os.unlink(new_converted_changesets_path)

    return True, converted_hashes


def convert(
        r1,
        r2,
        repo_path,
        hg_bin,
        store_path,
        yt_proxy,
        yt_table,
        yt_token,
        ydb_endpoint,
        ydb_database,
        ydb_token,
        aapi_proxies,
        already_converted_and_uploaded_changesets_path,
        garbage_dir,
        state,
        dry_run,
):
    converted = []

    while True:
        r, h = _convert(
            r1,
            r2,
            repo_path,
            hg_bin,
            store_path,
            yt_proxy,
            yt_table,
            yt_token,
            ydb_endpoint,
            ydb_database,
            ydb_token,
            aapi_proxies,
            already_converted_and_uploaded_changesets_path,
            garbage_dir,
            1000,
            state,
            dry_run
        )

        if not r:
            raise Exception('Aapi convert failed')

        converted += h

        if len(h) < 1000:
            break

    return converted


class State(object):

    def __init__(self):
        self._stopped = False

    def stop(self):
        self._stopped = True

    def stopped(self):
        return self._stopped

    def check_stopped(self):
        if self.stopped():
            raise Exception('stopped')

    def wrap_stopping(self, func, *args, **kwargs):
        self.check_stopped()
        try:
            return func(*args, **kwargs)
        except Exception as e:
            logging.error(e)
            traceback.print_exc(file=sys.stderr)
            self.stop()
            raise e


def main():
    logging.basicConfig(level=logging.DEBUG)

    # Setup sigterm handler for soft stop(don't corrupt local hg repository)
    state = State()

    def sigterm_handler(signum, frame):
        state.stop()

    signal.signal(signal.SIGTERM, sigterm_handler)

    opts = parse_args()
    with open(opts.config) as f:
        cfg = json.load(f)

    yt_proxy = cfg['yt_proxy']
    yt_table = cfg['yt_table']
    yt_token = cfg.get('yt_token')
    yt_client = yt.Client(yt_proxy, token=yt_token)

    ydb_endpoint = cfg['ydb_endpoint']
    ydb_database = cfg['ydb_table']
    ydb_token = cfg['ydb_token']

    hg_repo_path = cfg['hg_repo_path']
    hg_bin = cfg['hg_binary_path']
    hg_user = cfg['hg_user']
    hg_key = cfg['hg_key']

    vcs_api_services = cfg['vcs_api_services']  # host1:port1, host2:port2, ...

    vcs_store_path = cfg['vcs_local_store_path']
    hg_uploaded_changesets_path = cfg['hg_uploaded_changesets_path']

    garbage_dir = cfg['garbage_dir']
    if not os.path.exists(garbage_dir):
        os.makedirs(garbage_dir)

    do_not_upload = bool(cfg.get('do_not_upload'))

    # Init
    # 1) Ensure cloned hg repo exists
    assert os.path.exists(os.path.join(hg_repo_path, '.hg'))  # TODO hg clone repo

    # 2) Ensure uploaded changesets path exists
    if not os.path.exists(hg_uploaded_changesets_path):
        local_changesets = list_all_local_repo_changesets(hg_repo_path, hg_bin)
        state.check_stopped()
        uploaded_changesets = get_uploaded_changesets(local_changesets, yt_client, yt_table, state)
        dump_changesets_hashes(uploaded_changesets, hg_uploaded_changesets_path)

    # 3) Ensure all local changesets are converted
    tip = get_repo_tip(hg_repo_path, hg_bin)
    ix = convert(
        0,
        tip,
        hg_repo_path,
        hg_bin,
        vcs_store_path,
        yt_proxy,
        yt_table,
        yt_token,
        ydb_endpoint,
        ydb_database,
        ydb_token,
        ','.join(vcs_api_services),
        hg_uploaded_changesets_path,
        garbage_dir,
        state,
        do_not_upload
    )
    logging.info('Initially converted and uploaded %s changesets', str(len(ix)))

    # Start conversion loop
    while True:
        state.check_stopped()
        update_repo(hg_repo_path, hg_bin, hg_user, hg_key)
        state.check_stopped()
        new_tip = get_repo_tip(hg_repo_path, hg_bin)
        state.check_stopped()

        if new_tip == tip:
            time.sleep(1.0)
            continue

        assert tip < new_tip

        ix = convert(
            0,
            new_tip,
            hg_repo_path,
            hg_bin,
            vcs_store_path,
            yt_proxy,
            yt_table,
            yt_token,
            ydb_endpoint,
            ydb_database,
            ydb_token,
            ','.join(vcs_api_services),
            hg_uploaded_changesets_path,
            garbage_dir,
            state,
            do_not_upload
        )

        logging.info('Converted %s -> %s (%s new changesets)', str(tip), str(new_tip), str(len(ix)))
        tip = new_tip


if __name__ == '__main__':
    main()
