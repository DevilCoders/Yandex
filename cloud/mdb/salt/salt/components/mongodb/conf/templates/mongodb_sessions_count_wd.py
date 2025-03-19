#!/usr/bin/env python3
"""
Watchdog for mongodb sessions overflow
"""

import logging
import logging.handlers
import subprocess
import warnings

import pymongo

# Ignore ssl-related warnings if any
warnings.simplefilter("ignore")

MONGO_URI = '{{ uri }}'
DEFAULT_TIMEOUT_MS = 5000
SESSIONS_MAX_TIME_MS = 3000
SESSIONS_HARD_LIMIT = 950000
SESSIONS_OVERFLOW_MESSAGE = 'Unable to add session into the cache because the number of active sessions is too high'
LOG_PATH = '/var/log/mongodb/mdb-sessions-watchdog.log'


def check(log):
    """
    Check mongodb sessions count
    """
    client = pymongo.MongoClient(
        MONGO_URI,
        socketTimeoutMS=DEFAULT_TIMEOUT_MS,
        connectTimeoutMS=DEFAULT_TIMEOUT_MS,
        serverSelectionTimeoutMS=DEFAULT_TIMEOUT_MS,
        waitQueueTimeoutMS=DEFAULT_TIMEOUT_MS,
    )

    local_count = 0
    local_cur = client['config'].command(
        'aggregate', 1,
        pipeline=[{"$listLocalSessions": {"allUsers": True}}, {'$group': {'_id': '', 'count': {'$sum': 1}}}],
        cursor={},
        maxTimeMS=SESSIONS_MAX_TIME_MS,
    )['cursor']
    for doc in local_cur['firstBatch']:
        local_count = doc['count']
        break

    if local_count > SESSIONS_HARD_LIMIT:
        log.error('Local sessions count exceeds hard limit: %s > %s', local_count, SESSIONS_HARD_LIMIT)
        return False

    server_count = 0
    server_cur = client['config']['system.sessions'].aggregate(
        pipeline=[{'$listSessions': {'allUsers': True}}, {'$group': {'_id': '', 'count': {'$sum': 1}}}],
        maxTimeMS=SESSIONS_MAX_TIME_MS,
    )
    for doc in server_cur:
        server_count = doc['count']
        break

    if server_count > SESSIONS_HARD_LIMIT:
        log.error('Server sessions count exceeds hard limit: %s > %s', server_count, SESSIONS_HARD_LIMIT)
        return False

    log.info('Local count: %s, server count: %s', local_count, server_count)
    return True


def restart(log):
    """
    Gracefully restart mongodb
    """
    try:
        subprocess.check_call(['timeout', '900', '/usr/local/yandex/pre_restart.sh'])
        subprocess.check_call(['timeout', '900', '/usr/local/yandex/post_restart.sh'])
    except Exception as exc:
        log.error('Mongodb restart failed: %s', repr(exc))


def _main():
    handler = logging.handlers.WatchedFileHandler(LOG_PATH)
    formatter = logging.Formatter('%(asctime)s %(levelname)s:\t%(message)s')
    handler.setFormatter(formatter)
    log = logging.getLogger()
    log.addHandler(handler)
    log.setLevel(logging.INFO)
    try:
        if not check(log):
            restart(log)
    except pymongo.errors.OperationFailure as exc:
        if SESSIONS_OVERFLOW_MESSAGE in str(exc):
            log.error('Sessions overflow detected')
            restart(log)
        else:
            log.warning('Check failure: %s', repr(exc))
    except Exception as exc:
        log.warning('Check failure: %s', repr(exc))


if __name__ == '__main__':
    _main()
