"""
MongoDB resetup script

Only mongod is supported for now
"""

import argparse
import logging
import socket
import sys
from enum import Enum

from mdb_mongo_tools.exceptions import (LocalLockNotAcquired, PredictorOperationNotAllowed, ResetupTimeoutExceeded,
                                        ResetupUnexpectedError)
from mdb_mongo_tools.mongo_ctl import MongodCtl
from mdb_mongo_tools.mongodb import (MONGO_DEFAULT_CONNECT_TIMEOUT_MS, MONGO_DEFAULT_SERVER_SELECTION_TIMEOUT_MS,
                                     MONGO_DEFAULT_SOCKET_TIMEOUT_MS, MongoConnection, ReplSetInfo)
from mdb_mongo_tools.resetup import get_resetup_instance
from mdb_mongo_tools.reviewers.replset import ReplSetOpReviewer
from mdb_mongo_tools.util import load_config, local_lock, setup_logging

DEFAULT_CONFIG_PATH = '/etc/yandex/mdb-mongo-tools/mdb-mongo-tools.conf'

DEFAULT_CONFIG = {
    'control': {
        'service_ctl': 'sysvinit',  # supervisor supported
        'wd_stop_file': '/var/run/wd-mongodb.stop',
    },
    'resetup': {
        'local_lock': '/var/run/mdb_mongod_resetup.in_progress',
        'resetup_type': 'initial_sync',
        'initial_sync': {
            'timeout': {  # timedelta
                'hours': 12,
            },
            'status_check_delay': {  # timedelta
                'minutes': 1,
            },
        },
        'replset_checks': {
            'quorum': {
                'enabled': True,
                'warn_only': False,
                'reserved_votes': 0,
            },
            'write_concern': {
                'enabled': False,
                'warn_only': False,
                'required': 'majority',
            },
            'eta_acceptable': {
                'enabled': True,
                'warn_only': True,
                'multiplier': 1.2,
                'net_bps': 4194304,
            },
        },
    },
    'mongodb': {
        'host': socket.gethostname(),
        'port': 27018,
        'auth': True,
        'username': 'user',
        'password': 'password',
        'database': 'admin',
        'options': {
            'connectTimeoutMS': MONGO_DEFAULT_CONNECT_TIMEOUT_MS,
            'socketTimeoutMS': MONGO_DEFAULT_SOCKET_TIMEOUT_MS,
            'serverSelectionTimeoutMS': MONGO_DEFAULT_SERVER_SELECTION_TIMEOUT_MS,
        },
    },
    'logging': {
        'log_file': '/var/log/mongodb/mdb-mongo-tools.log',
        'log_level_root': 'DEBUG',
        'log_format': '%(asctime)s [%(levelname)s] %(process)d %(module)s:\t%(message)s',
    },
}


class MdbResetupExitCodes(Enum):
    """
    Exit code enum
    """
    NOT_STALE = 0
    DRY_RUN = 0
    SUCCEEDED = 0
    NOTHING_TO_CONTINUE = 0
    ALREADY_RUNNING = 10
    ACTION_NOT_ALLOWED = 11
    CANNOT_OBTAIN_REMOTE_LOCK = 12
    TIMED_OUT = 50
    UNEXPECTED_ERROR = 51


def host_shutdown_allowed(rs_op_reviewer: ReplSetOpReviewer, host_port: str) -> bool:
    """
    Host shutdown check Wrapper
    """

    try:
        rs_op_reviewer.shutdown_host(host_port)
        return True
    except PredictorOperationNotAllowed:
        return False


def watch_for_running_resetup(_, conf):
    '''
    Watch for currently running resetup
    '''
    # TODO: Too much copypaste, need to think, how to improve
    host_port = '{host}:{port}'.format(**conf['mongodb'])
    logging.debug('Starting check of mongod on %s', host_port)

    mongo_conn = MongoConnection(conf['mongodb'])
    replset_info = ReplSetInfo(conf['mongodb'], mongo_conn)
    replset_info.update()

    resetup_conf = conf['resetup']
    if replset_info.host_is_in_startup2_state(host_port):
        logging.debug(
            'mongod is in STARTUP2 state and --continue flag is passed, will watch for current resetup process.')
        resetup_type = resetup_conf['resetup_type']
        mongo_ctl = MongodCtl(conf['control'], mongo_conn.dbpath)
        resetup_exc = get_resetup_instance(resetup_type, resetup_conf[resetup_type], mongo_conn, mongo_ctl)
        try:
            resetup_exc.fake_start()
            resetup_exc.wait()
            exit_code = MdbResetupExitCodes.SUCCEEDED
        except ResetupTimeoutExceeded:
            logging.error('Timeout expired', exc_info=True)
            exit_code = MdbResetupExitCodes.TIMED_OUT
        except ResetupUnexpectedError:
            logging.critical('Unexpected error occurred during resetup', exc_info=True)
            exit_code = MdbResetupExitCodes.UNEXPECTED_ERROR
        logging.info('Resetup duration was %s', resetup_exc.duration)

    else:
        logging.debug('Flag --continue is passed, but mongod not in STARTUP2 state, exiting.')
        exit_code = MdbResetupExitCodes.NOTHING_TO_CONTINUE

    return exit_code


def resetup_stalled(args, conf: dict) -> MdbResetupExitCodes:
    """
    Perform checks and run resetup if node is stalled
    """

    host_port = '{host}:{port}'.format(**conf['mongodb'])
    logging.debug('Starting check of mongod on %s', host_port)

    mongo_conn = MongoConnection(conf['mongodb'])
    replset_info = ReplSetInfo(conf['mongodb'], mongo_conn)
    replset_info.update()

    if not replset_info.host_is_stale(host_port):
        if args.force:
            logging.debug('mongod is not stale, but --force flag is passed, will initiate resetup anyway.')
        else:
            logging.debug('mongod is not stale. Exit now.')
            return MdbResetupExitCodes.NOT_STALE
    else:
        logging.debug('mongod is stale')

    resetup_conf = conf['resetup']
    rs_op_reviewer = ReplSetOpReviewer(replset_info, resetup_conf['replset_checks'])
    if not host_shutdown_allowed(rs_op_reviewer, host_port):
        logging.error('Resetup is not allowed. Exit now.')
        return MdbResetupExitCodes.ACTION_NOT_ALLOWED

    if args.dryrun:
        logging.debug('Will not not perform resetup because of dry run. Exit now.')
        return MdbResetupExitCodes.DRY_RUN

    # TODO: acquire REMOTE lock
    # then check again under lock:

    rs_op_reviewer.update()
    if not host_shutdown_allowed(rs_op_reviewer, host_port):
        logging.error('Resetup is not allowed. Exit now')
        return MdbResetupExitCodes.ACTION_NOT_ALLOWED

    mongo_ctl = MongodCtl(conf['control'], mongo_conn.dbpath)
    resetup_type = resetup_conf['resetup_type']
    resetup_exc = get_resetup_instance(resetup_type, resetup_conf[resetup_type], mongo_conn, mongo_ctl)

    try:
        with local_lock(resetup_conf['local_lock']):
            resetup_exc.start()
            resetup_exc.wait()
        exit_code = MdbResetupExitCodes.SUCCEEDED

    except LocalLockNotAcquired:
        logging.critical('Can not acquire local lock: assume resetup is already in progress. Exit now')
        return MdbResetupExitCodes.ALREADY_RUNNING

    except ResetupTimeoutExceeded:
        logging.error('Timeout expired', exc_info=True)
        exit_code = MdbResetupExitCodes.TIMED_OUT
    except ResetupUnexpectedError:
        logging.critical('Unexpected error occurred during resetup', exc_info=True)
        exit_code = MdbResetupExitCodes.UNEXPECTED_ERROR

    logging.info('Resetup duration was %s', resetup_exc.duration)
    return exit_code


def main():
    """
    Main
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', type=str, default=DEFAULT_CONFIG_PATH, dest='config', help='Config file')
    parser.add_argument('-n', '--dry-run', action='store_true', dest='dryrun', help='Dry run')
    parser.add_argument(
        '-f', '--force', action='store_true', dest='force', help='Start resetup even if node is not stalled')
    parser.add_argument(
        '--continue',
        action='store_true',
        dest='continue_flag',
        help='Continue previosly started resetup if any, do not try to perform new resetup.')
    args = parser.parse_args()

    conf = load_config(args.config, DEFAULT_CONFIG)
    setup_logging(conf['logging'])
    if args.continue_flag:
        exit_code = watch_for_running_resetup(args, conf)
    else:
        exit_code = resetup_stalled(args, conf)
    sys.exit(exit_code.value)


if __name__ == '__main__':
    main()
