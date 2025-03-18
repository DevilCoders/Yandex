import sys
import os
import uuid
import convert
import threading
import Queue
import collections
import traceback
import logging
import time

from mercurial import registrar, mdiff, manifest, logcmdutil
from mercurial.i18n import _

from concurrent import futures

import yt_client as yt
import store as store_utils
import upload as upload_utils

import vcs_helpers
from aapi.lib.py_common import consts
from aapi.lib.ydb import ydb


# Note for extension authors: ONLY specify testedwith = 'internal' for
# extensions which SHIP WITH MERCURIAL. Non-mainline extensions should
# be specifying the version(s) of Mercurial they are tested with, or
# leave the attribute unspecified.
testedwith = 'internal'


cmdtable = {}
cmd = registrar.command(cmdtable)


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


def iter_deltas(repo, revs, raw_data_reader, pool, state):
    revs = sorted(revs)
    revlog = repo.manifestlog._rootstore

    if not isinstance(revlog, manifest.manifestrevlog):
        raise RuntimeError('Not a revlog')

    revlog = revlog._revlog

    delta_parents = {}
    chain_parents = {}
    child_count = collections.defaultdict(int)
    all_parents = set()
    all_nodes = set()
    mf_revs = set()
    flags = {}
    linkrevs = {}

    for rev in revs:
        mf_hash = repo[rev].manifestnode()
        mf_rev = revlog.rev(mf_hash)
        mf_parent_rev = revlog.deltaparent(mf_rev)

        delta_parents[mf_rev] = mf_parent_rev
        chain_parent = mf_parent_rev
        if mf_parent_rev == -1:
            p1_rev = revlog.parentrevs(mf_rev)[0]
            if p1_rev != -1:
                assert p1_rev < mf_rev
                chain_parent = p1_rev
        chain_parents[mf_rev] = chain_parent

        child_count[chain_parent] += 1

        all_parents.add(chain_parent)
        all_nodes.add(mf_rev)
        mf_revs.add(mf_rev)

        for r in (mf_rev, mf_parent_rev, chain_parent):
            flags[r] = revlog.flags(r)
            linkrevs[r] = revlog.linkrev(r)

    parents_to_restore = all_parents.difference(all_nodes)
    mf_raw_data = {}

    def wrap_logging(msg, func, *args, **kwargs):
        res = func(*args, **kwargs)
        logging.info(msg)
        return res

    for r in sorted(parents_to_restore | mf_revs):
        if r in parents_to_restore:
            mf_raw_data[r] = raw_data_reader.submit(state.wrap_stopping, wrap_logging, 'Loaded {} revision'.format(r), revlog.revision, r, None, True)  # TODO cache temporary manifests lru
        else:
            mf_raw_data[r] = raw_data_reader.submit(state.wrap_stopping, wrap_logging, 'Loaded {} patch'.format(r), revlog._chunk, r)

    mf_revs = sorted(mf_revs)
    logging.info('Manifests to convert count: %s', len(mf_revs))
    logging.info('Restore revision: %s', len(parents_to_restore))
    logging.info('Restore patch: %s', len(mf_revs))

    segments = {}
    segments_order = []
    segments_ids = {}
    children = collections.defaultdict(list)

    for rev in mf_revs:
        chain_parent = chain_parents[rev]

        if child_count[chain_parent] > 1 or chain_parent == -1 or chain_parent in parents_to_restore:
            segments[rev] = [chain_parent, rev]
            segments_order.append(rev)
            segments_ids[rev] = rev
            children[chain_parent].append(rev)

        else:
            segments[segments_ids[chain_parent]].append(rev)
            segments_ids[rev] = segments_ids[chain_parent]

    q = Queue.Queue(256)
    lock = threading.Lock()
    tasks = [0]

    def iter_segment_deltas(base_r, base, deltas_segment):
        state.check_stopped()

        if isinstance(base, basestring):
            raw = base[:]
        else:
            logging.info('Waiting data for %s', base_r)
            raw = base.result()[:]

        mf = manifest.manifestdict(revlog._processflags(raw, flags[base_r], 'read', raw=False)[0])

        for r in deltas_segment:
            state.check_stopped()

            logging.info('Waiting data for %s', r)
            if delta_parents[r] == -1:
                raw2 = str(mf_raw_data[r].result())
            else:
                raw2 = str(mdiff.patches(raw, [mf_raw_data[r].result()]))

            mf2 = manifest.manifestdict(revlog._processflags(raw2, flags[r], 'read', raw=False)[0])

            delta = mf.diff(mf2)
            raw = raw2
            mf = mf2

            while True:
                try:
                    q.put((revlog.node(chain_parents[r]), chain_parents[r], linkrevs[chain_parents[r]], revlog.node(r), r, linkrevs[r], delta), timeout=2.5)
                    logging.info('Prepared conversion task for %s', r)
                    break
                except Queue.Full:
                    state.check_stopped()

        state.check_stopped()

        if child_count[deltas_segment[-1]] != 0:
            assert child_count[deltas_segment[-1]] > 1

            for c in children[deltas_segment[-1]]:
                assert c in segments
                state.check_stopped()
                pool.submit(state.wrap_stopping, iter_segment_deltas, deltas_segment[-1], raw, segments[c][1:])

            with lock:
                tasks[0] += len(children[deltas_segment[-1]]) - 1

        else:
            with lock:
                tasks[0] -= 1

    count = 0
    for seg_id in sorted(segments.keys()):
        seg = segments[seg_id]

        if seg[0] == -1:
            pool.submit(state.wrap_stopping, iter_segment_deltas, -1, '', seg[1:])
        elif seg[0] in parents_to_restore:
            pool.submit(state.wrap_stopping, iter_segment_deltas, seg[0], mf_raw_data[seg[0]], seg[1:])
        else:
            continue

        count += 1

    with lock:
        tasks[0] += count

    while True:
        with lock:
            if tasks[0] == 0 and q.empty():
                break

        if state.stopped():
            return

        try:
            yield q.get(timeout=2)
        except Queue.Empty:
            continue


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
    logging.info('Dumped {} changesets to {}'.format(len(changesets), path))


def get_revs(repo, opts, args):
    revs, _ = logcmdutil.getrevs(repo, args, opts)

    cache = opts.get('all_converted_and_uploaded_changesets_path')

    if cache and os.path.exists(cache):
        done = set(load_changesets_hashes(cache))
    else:
        done = set()

    filtered_revs = []
    for rev in revs:
        if repo[rev].node() in done:
            continue
        filtered_revs.append(rev)

    max_revs = opts.get('max_convert_count') or sys.maxint

    return done, filtered_revs[:max_revs]


@cmd('aapi-convert', [
    ('', 'yt-proxy', 'hahn', _('yt proxy')),
    ('', 'yt-table', '//home/devtools/vcs/data', _('yt table path')),
    ('', 'yt-token', '', _('yt token')),
    ('', 'ydb-endpoint', 'ydb-ru.yandex.net:2135', _('YDB endpoint')),
    ('', 'ydb-database', '/ru/devtools/prod/aapi', _('YDB database')),
    ('', 'ydb-token', '', _('YDB token')),
    ('s', 'store', 'store', _('local store path')),
    ('', 'substore', '', _('convert session substore')),
    ('r', 'rev', [], _('convert the specified revision or revset'), _('REV')),
    ('', 'do-not-store-blobs-in-main-store', None, _('subj')),
    ('', 'aapi-proxies', '', _('proxies to upload new objects directly')),
    ('c', 'all-converted-and-uploaded-changesets-path', '', _('subj')),
    ('o', 'new-converted-and-uploaded-changesets-out-path', '', _('subj')),
    ('m', 'max-convert-count', 0, _('max changesets to convert count')),
    ('', 'dry-run', None, _('do not modify anything, do not upload anything'))
])
def aapi_convert(ui, repo, *args, **opts):
    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger('kikimr').setLevel(logging.WARNING)

    yt_token = opts.get('yt_token') or None
    yt_client = yt.Client(opts.get('yt_proxy'), token=yt_token)
    yt_table = opts.get('yt_table')

    ydb_endpoint = opts.get('ydb_endpoint')
    ydb_database = opts.get('ydb_database')
    ydb_token = opts.get('ydb_token')
    ydb_client = ydb.YdbClient(ydb_endpoint, ydb_database, ydb_token)

    aapi_proxies = opts.get('aapi_proxies', '')
    if aapi_proxies:
        aapi_proxies = aapi_proxies.split(',')
    else:
        aapi_proxies = []

    store_path = opts.get('store')
    if opts.get('do_not_store_blobs_in_main_store'):
        store = store_utils.Store(
            store_path,
            yt_client,
            yt_table,
            aapi_proxies[0] if aapi_proxies else None,
            local_store_types={consts.NODE_HG_CHANGESET, consts.NODE_TREE}
        )
    else:
        store = store_utils.Store(
            store_path,
            yt_client,
            yt_table,
            aapi_proxies[0] if aapi_proxies else None,
            local_store_types={consts.NODE_HG_CHANGESET, consts.NODE_TREE, consts.NODE_BLOB}
        )

    dry_run = opts.get('dry_run') or False

    substore_path = opts.get('substore')
    if substore_path:
        substore = store.substore(substore_path)
    else:
        substore = store

    done_all, revs = get_revs(repo, opts, args)
    done_now = set()
    uploads = set()

    mf_to_revs = collections.defaultdict(list)
    for r in revs:
        mf_to_revs[repo[r].manifestnode()].append(r)

    state = State()
    cs_cache = opts.get('all_converted_and_uploaded_changesets_path')
    cs_out = opts.get('new_converted_and_uploaded_changesets_out_path')

    if cs_out:
        dump_changesets_hashes(done_now, cs_out)

    yt_upload_queue = Queue.Queue()
    ydb_upload_queue = Queue.Queue()
    checkpoint_queue = Queue.Queue()

    yt_upload_thread = upload_utils.YtUploadThread(
        yt_client,
        yt_table,
        yt_upload_queue,
        checkpoint_queue,
        substore._local_store._path,
        state,
        dry_run
    )

    ydb_upload_thread = upload_utils.YdbUploadThread(
        ydb_client,
        ydb_upload_queue,
        checkpoint_queue,
        substore._local_store._path,
        state,
        dry_run
    )

    aapi_upload_queues = [Queue.Queue() for _ in aapi_proxies]
    aapi_upload_threads = [upload_utils.VcsUploadThread(p, q, substore._local_store._path, state, dry_run) for p, q in zip(aapi_proxies, aapi_upload_queues)]

    def enqueue_upload(x):
        yt_upload_queue.put(x)
        ydb_upload_queue.put(x)
        for q in aapi_upload_queues:
            q.put(x)

    def checkpoint():
        done_all_saved_count = len(done_all)
        done_now_saved_count = len(done_now)

        while True:
            try:
                h = checkpoint_queue.get(timeout=1.5)
            except Queue.Empty:
                break

            if h not in uploads:
                uploads.add(h)
                continue
            else:
                uploads.remove(h)
                done_all.add(h)
                done_now.add(h)

        if cs_cache and len(done_all) > done_all_saved_count:
            dump_changesets_hashes(done_all, cs_cache)

        if cs_out and len(done_now) > done_now_saved_count:
            dump_changesets_hashes(done_now, cs_out)

    for t in aapi_upload_threads + [yt_upload_thread, ydb_upload_thread]:
        t.start()

    raw_data_reader = futures.ThreadPoolExecutor(max_workers=1)
    pool = futures.ThreadPoolExecutor(max_workers=32)
    pool2 = futures.ThreadPoolExecutor(max_workers=32)

    try:
        checkpoint_time = time.time()

        for i, (mf_par_hash, mf_par_rev, mf_par_linkrev, mf_hash, mf_rev, mf_linkrev, delta) in enumerate(iter_deltas(repo, revs, raw_data_reader, pool, state)):
            if mf_par_rev == -1:
                par_tree_hash = None
            else:
                par_cs = repo[mf_par_linkrev]
                assert par_cs.manifestnode() == mf_par_hash
                t, d = substore.get(repo[mf_par_linkrev].node())
                assert t == consts.NODE_HG_CHANGESET
                par_cs_obj = vcs_helpers.load_hg_changeset_fbs(vcs_helpers.decompress(d))
                par_tree_hash = par_cs_obj.tree

            par_cs_hash = repo[mf_par_linkrev].node().encode('hex')
            cs_hash = repo[mf_linkrev].node().encode('hex')

            new_tree_hash = convert.convert_delta(
                pool2,
                repo,
                substore,
                par_tree_hash,
                delta,
                state,
                'Convert {}({}) -> {}({}) (%s)'.format(mf_par_linkrev, par_cs_hash, mf_linkrev, cs_hash)
            )

            new_objects = sorted(substore.get_all_puts())
            new_changesets = []

            for r in mf_to_revs[mf_hash]:
                h = convert.add_changeset(repo, substore, new_tree_hash, r)
                new_changesets.append(h)
                logging.info('Converted %s (%s)', r, len(delta))

            enqueue_upload((mf_par_linkrev, par_cs_hash, mf_linkrev, cs_hash, new_objects, new_changesets[0]))
            for cs in new_changesets[1:]:
                enqueue_upload((mf_par_linkrev, par_cs_hash, mf_linkrev, cs_hash, new_objects, cs))

            if time.time() - checkpoint_time > 15 or i % 250 == 0:
                checkpoint()
                checkpoint_time = time.time()

            substore.reset_all_puts()

        enqueue_upload(None)  # Sentinel

        for t in aapi_upload_threads + [yt_upload_thread, ydb_upload_thread]:
            while t.isAlive():
                t.join(5)
                checkpoint()

    except KeyboardInterrupt:
        state.stop()
        raise

    finally:
        checkpoint()
