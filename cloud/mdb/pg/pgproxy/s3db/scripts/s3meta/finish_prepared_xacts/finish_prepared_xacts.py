#!/usr/bin/env python
# encoding: utf-8

import logging
from optparse import OptionParser
import os
import psycopg2
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
from util.s3meta import S3MetaDB
from util.pgmeta import PgmetaDB
from util.helpers import init_logging, read_config


def parse_cmd_args():
    usage = """
    %prog [-d db_connstring] [-p pgmeta_connstring] [-u user] [-l log_level] [-D delay]
    """
    parser = OptionParser(usage=usage)
    parser.add_option("-l", "--log-level", dest="log_level", default="INFO",
                      help="Verbosity level (default: %default)")
    parser.add_option("-S", "--sentry-dsn", dest="sentry_dsn",
                      help="Sentry DSN, skip if don't use")
    parser.add_option("-p", "--pgmeta-connstring", dest="pgmeta_connstring", default="dbname=pgmeta",
                      help="Connection string to pgmeta database (default: %default)")
    parser.add_option("-d", "--db-connstring", dest="db_connstring", default="dbname=s3meta",
                      help="Connection string to s3db database (default: %default)")
    parser.add_option("-u", "--user", dest="user",
                      help="User which will be used for authentication in databases")
    parser.add_option("-D", "--delay", dest="delay", default="5 minutes",
                      help="Dont' touch prepared xacts younger than delay")
    return parser.parse_args()


def get_gid_key(gid):
    """ Makes key for gid, which the same to all dbs
    GID Examples:
        's3meta_move_180f3e36-857b-43bd-8440-3176a6b6adb9_2311_f1_t0'
        's3meta_split_2db96c6c-e596-4cd0-8ee0-4ee78393eeda_1094949'
        's3db_split_b2b726c8-c9e1-4500-a224-03ed70f5cc1e_71_75'
        's3meta_create'
        's3db_purge_c14c0323-1a48-4d37-a392-6c83ff69c26b_140'
    """
    if '_split_' in gid:
        return '_'.join(gid.split('_')[1:4])
    elif '_create' in gid or '_purge' in gid:
        return gid.split('_')[1]
    else:   # _move_
        return '_'.join(gid.split('_')[1:])


def to_finish_mover_tpc(s3meta, db_from, db_to, bid, cid):
    def check_completed_meta(meta, db_from, db_to, bid, cid):
        chunk_shard_on_meta = meta.get_shard_by_chunk(bid, cid)
        if chunk_shard_on_meta == db_from.shard_id:
            return False
        if chunk_shard_on_meta == db_to.shard_id:
            return True
        return None

    def check_completed_db_from(db_from, bid, cid):
        return not db_from.is_chunk_exists(bid, cid)

    def check_completed_db_to(db_to, bid, cid):
        return db_to.is_chunk_exists(bid, cid)

    gid_suffix = "{0}_{1}_{2}_f{3}_t{4}".format("move", bid, cid, db_from.shard_id, db_to.shard_id)
    prepared_meta = 's3meta_' + gid_suffix in s3meta.get_prepared_xacts()
    prepared_db_from = 's3db_' + gid_suffix in db_from.get_prepared_xacts()
    prepared_db_to = 's3db_' + gid_suffix in db_to.get_prepared_xacts()

    committed_meta = check_completed_meta(s3meta, db_from, db_to, bid, cid)
    committed_db_from = check_completed_db_from(db_from, bid, cid)
    committed_db_to = check_completed_db_to(db_to, bid, cid)

    logging.info('Finishing mover tpc, bid={bid}, cid={cid}, from {from_shard} to {to_shard} shard...'.
                 format(bid=bid, cid=cid, from_shard=db_from.shard_id, to_shard=db_to.shard_id))

    need_rollback = (not prepared_db_to and not committed_db_to) or (not prepared_meta and not committed_meta) or \
                    (not prepared_db_from and not committed_db_from)    # True if some transactions hasn't been prepared

    if need_rollback:
        if not prepared_db_to and not committed_db_to:
            logging.info('Not prepared and not committed on db_to => rollback all')
        if not prepared_meta and not committed_meta:
            logging.info('Not prepared and not committed on meta => rollback all')
        if not prepared_db_from and not committed_db_from:
            logging.info('Not prepared and not committed on db_from => rollback all')

    need_commit = not need_rollback and (committed_meta or committed_db_from or committed_db_to)
    if need_commit:
        if committed_db_to:
            logging.info('Committed on db_to => commit all')
        if committed_meta:
            logging.info('Committed on meta => commit all')
        if committed_db_from:
            logging.info('Committed on db_from => commit all')
    else:
        logging.info('Hasn\'t been committed anywhere => rollback all')

    finish_ordered = []
    if prepared_db_to:
        finish_ordered.append((db_to, 's3db_' + gid_suffix, 'db_to'))
    if prepared_meta:
        finish_ordered.append((s3meta, 's3meta_' + gid_suffix, 'meta'))
    if prepared_db_from:
        finish_ordered.append((db_from, 's3db_' + gid_suffix, 'db_from'))

    if not need_commit:
        finish_ordered = reversed(finish_ordered)

    for db, xact, target in finish_ordered:
        logging.info('{action} {xact} on {dbname}'.format(
            action='Commit' if need_commit else 'Rollback',
            xact=xact,
            dbname=target,
        ))
        db.finish_prepared_xact(xact, need_commit)
    logging.info('Done.')


def to_finish_splitter_tpc(s3meta, s3db, bid, cid):
    gid_suffix = "{0}_{1}_{2}".format("split", bid, cid)
    prepared_meta = 's3meta_' + gid_suffix in s3meta.get_prepared_xacts()
    prepared_db_xact = None
    for x in s3db.get_prepared_xacts():
        if x.startswith('s3db_' + gid_suffix):
            prepared_db_xact = x

    chunk_meta = s3meta.get_chunk(bid, cid)
    chunk_db = s3db.get_chunk(bid, cid)

    #              chunk bounds                 |   prepared_db   |    prepared_meta  |   prepared_db & prepared_meta
    #   -----------------------------------------------------------------------------------------------------------
    #   chunk_db.end_key = chunk_meta.end_key   |    Rollback     |     Rollback      |      Rollback all
    #   chunk_db.end_key < chunk_meta.end_key   |       X         |      Commit       |            X
    #   chunk_db.end_key > chunk_meta.end_key   |       X         |         X         |            X

    def compare_end_keys(first, second):
        if first == second:
            return 0
        elif (second is None) or (first is not None) and (first < second):
            return -1  # first < second
        else:
            return 1   # first > second

    compare_db_meta = compare_end_keys(chunk_db.end_key, chunk_meta.end_key)
    if compare_db_meta == 0:
        if prepared_meta:
            logging.info('Rollback s3meta_%s', gid_suffix)
            s3meta.finish_prepared_xact('s3meta_' + gid_suffix, commit=False)
        if prepared_db_xact:
            logging.info('Rollback %s on %s shard', prepared_db_xact, chunk_meta.shard_id)
            s3db.finish_prepared_xact(prepared_db_xact, commit=False)
    elif prepared_meta and compare_db_meta < 0:
        logging.info('Commit s3meta_%s', gid_suffix)
        s3meta.finish_prepared_xact('s3meta_' + gid_suffix, commit=True)
    else:
        logging.error('Unknown splitter state: %s on db, %s on meta; db.end_key %s meta.end_key',
                      'prepared' if prepared_db_xact else 'not prepared',
                      'prepared' if prepared_meta else 'not prepared',
                      '<' if compare_db_meta < 0 else '>')

    logging.info('Done.')


def to_finish_merger_tpc(s3meta, s3db, bid, cid):
    gid_suffix = "{0}_{1}_{2}".format("merge", bid, cid)
    prepared_meta = 's3meta_' + gid_suffix in s3meta.get_prepared_xacts()
    prepared_db_xact = None
    for x in s3db.get_prepared_xacts():
        if x.startswith('s3db_' + gid_suffix):
            prepared_db_xact = x

    chunk_meta = s3meta.get_chunk(bid, cid)
    chunk_db = s3db.get_chunk(bid, cid)

    if chunk_db is not None and prepared_meta and prepared_db_xact:
        logging.info('Rollback s3meta_%s', gid_suffix)
        s3meta.finish_prepared_xact('s3meta_' + gid_suffix, commit=False)
        logging.info('Rollback %s on %s shard', prepared_db_xact, chunk_meta.shard_id)
        s3db.finish_prepared_xact(prepared_db_xact, commit=False)
    elif prepared_meta and not prepared_db_xact and chunk_db is None:
        logging.info('Commit s3meta_%s', gid_suffix)
        s3meta.finish_prepared_xact('s3meta_' + gid_suffix, commit=True)
    else:
        logging.error('Unknown merger state: %s on db, %s on meta; chunk %s on db, exists on meta',
                      'prepared' if prepared_db_xact else 'not prepared',
                      'prepared' if prepared_meta else 'not prepared',
                      'exists' if chunk_db is not None else 'does not exist')
    logging.info('Done.')


def to_finish_creator_or_purger_tpc(key, s3meta, s3db=None):
    # key in {'create', 'purge'}
    prepared_meta = 's3meta_%s' % key in s3meta.get_prepared_xacts()
    prepared_db_xact = None
    if s3db:
        for x in s3db.get_prepared_xacts():
            if x.startswith('s3db_%s' % key):
                prepared_db_xact = x

    if prepared_meta:
        # We always rollback meta (because chunk remains at s3.chunks_create_queue / s3.chunks_delete_queue)
        logging.info('Rollback s3meta_%s' % key)
        s3meta.finish_prepared_xact('s3meta_%s' % key, commit=False)
    if prepared_db_xact:
        # We always commit db (because in next meta creator/purger run it will be just removed from queue)
        logging.info('Commit %s' % prepared_db_xact)
        s3db.finish_prepared_xact(prepared_db_xact, commit=True)
    logging.info('Done.')


def main():
    options, _ = parse_cmd_args()
    config = read_config(options=options)
    init_logging(config)

    logging.info("Starting %s", os.path.basename(__file__))
    try:
        s3meta = S3MetaDB(config['db_connstring'], autocommit=True)
    except psycopg2.OperationalError as e:
        logging.warning("Cannot connect to local server:\n%s" % str(e))
        sys.exit(1)
    if not s3meta.is_master():
        logging.info("My db is replica. Nothing to do. Exiting...")
        sys.exit(0)

    all_gid_keys = set()
    pgmeta = PgmetaDB(config['pgmeta_connstring'], autocommit=True)

    for gid in s3meta.get_prepared_xacts(config.get('delay')):
        logging.info('Found prepared xact on s3meta with gid %s', gid)
        all_gid_keys.add(get_gid_key(gid))

    shard_to_s3db = {}
    purger_or_creator_shard = None
    for shard_id in s3meta.get_all_shard_ids():
        shard_to_s3db[shard_id] = pgmeta.get_master('db', shard_id, user=config.get('user'), autocommit=True)
        for gid in shard_to_s3db[shard_id].get_prepared_xacts(config.get('delay')):
            logging.info('Found prepared xact on s3db shard %d with gid %s', shard_id, gid)
            all_gid_keys.add(get_gid_key(gid))
            if '_purge_' in gid or '_create_' in gid:
                purger_or_creator_shard = shard_id

    for gid_key in all_gid_keys:
        if gid_key.startswith('move'):
            _, bid, cid, from_shard, to_shard = gid_key.split('_')
            from_shard, to_shard = int(from_shard[1:]), int(to_shard[1:])
            shard_id = s3meta.get_shard_by_chunk(bid, cid)
            if shard_id is None:
                logging.warning('Chunk in other metadb')
            else:
                to_finish_mover_tpc(s3meta, shard_to_s3db[from_shard], shard_to_s3db[to_shard], bid, cid)
        elif gid_key.startswith('split'):
            _, bid, cid = gid_key.split('_')
            shard_id = s3meta.get_shard_by_chunk(bid, cid)
            if shard_id is None:
                logging.warning('Chunk in other metadb')
            else:
                to_finish_splitter_tpc(s3meta, shard_to_s3db[s3meta.get_shard_by_chunk(bid, cid)], bid, cid)
        elif gid_key.startswith('merge'):
            _, bid, cid = gid_key.split('_')
            shard_id = s3meta.get_shard_by_chunk(bid, cid)
            if shard_id is None:
                logging.warning('Chunk in other metadb or has been deleted')
            else:
                to_finish_merger_tpc(s3meta, shard_to_s3db[s3meta.get_shard_by_chunk(bid, cid)], bid, cid)
        elif gid_key in {'purge', 'create'}:
            to_finish_creator_or_purger_tpc(gid_key, s3meta, shard_to_s3db.get(purger_or_creator_shard))
        else:
            logging.error('Unknown gid_key %s', gid_key)

    logging.info("Stopping %s", os.path.basename(__file__))


if __name__ == '__main__':
    main()
