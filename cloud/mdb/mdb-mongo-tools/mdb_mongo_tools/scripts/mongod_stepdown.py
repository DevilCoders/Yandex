"""
MongoDB stepdown script

stepdown behavior: https://docs.mongodb.com/manual/reference/command/replSetStepDown/
"""

import argparse
import logging
import socket
import sys
from enum import Enum

from mdb_mongo_tools.exceptions import (StepdownPrimaryWaitTimeoutExceeded, StepdownSamePrimaryElected,
                                        StepdownTimeoutExceeded, StepdownUnexpectedError)
from mdb_mongo_tools.helpers import host_shutdown_allowed, stepdown
from mdb_mongo_tools.mongodb import (MONGO_DEFAULT_CONNECT_TIMEOUT_MS, MONGO_DEFAULT_SERVER_SELECTION_TIMEOUT_MS,
                                     MONGO_DEFAULT_SOCKET_TIMEOUT_MS, MongoConnection, ReplSetCtl)
from mdb_mongo_tools.reviewers.replset import ReplSetOpReviewer
from mdb_mongo_tools.util import load_config, setup_logging

DEFAULT_CONFIG_PATH = '/etc/yandex/mdb-mongo-tools/mdb-mongo-tools.conf'

DEFAULT_CONFIG = {
    'stepdown': {
        'replSetStepDown': 60,  # The number of seconds to step down the primary, during which time
        #  the stepdown member is ineligible for becoming primary
        'secondaryCatchUpPeriodSecs': 10,  # The number of seconds that the mongod will wait for an electable secondary
        #  to catch up to the primary.
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
            'fresh_secondaries': {
                'enabled': True,
                'warn_only': False,
                'hosts_required': 1,
                'lag_threshold': {
                    'seconds': 30,  # TODO: fix update config, now merge adds values for dicts
                },
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


class MdbStepDownExitCodes(Enum):
    """
    Exit code enum
    """
    DRY_RUN = 0
    SUCCEEDED = 0
    NOT_PRIMARY = 0
    ACTION_NOT_ALLOWED = 10
    STEPDOWN_TIMED_OUT = 20
    ELECTION_TIMED_OUT = 30
    SAME_PRIMARY_ELECTED = 40
    UNEXPECTED_ERROR = 50


def stepdown_mongod(conf: dict, dry_run: bool, force: bool) -> MdbStepDownExitCodes:
    """
    Perform checks and run resetup if node is stalled
    """

    host_port = '{host}:{port}'.format(**conf['mongodb'])
    logging.debug('Starting check of mongod on %s', host_port)
    mongo_conn = MongoConnection(conf['mongodb'])
    replset_ctl = ReplSetCtl(conf['mongodb'], mongo_conn)
    replset_info = replset_ctl.get_info()

    stepdown_conf = conf['stepdown']

    if replset_info.primary_host_port != host_port:
        logging.info('Primary is {node}. No need to step down. Exit now')
        return MdbStepDownExitCodes.NOT_PRIMARY

    rs_op_reviewer = ReplSetOpReviewer(replset_info, stepdown_conf['replset_checks'])
    if not host_shutdown_allowed(rs_op_reviewer, host_port):
        if not force:
            logging.error('Stepdown is not allowed. Exit now')
            return MdbStepDownExitCodes.ACTION_NOT_ALLOWED

        logging.debug('Stepdown is not allowed, but --force flag is passed, will initiate stepdown anyway.')

    if dry_run:
        logging.debug('Will not not perform resetup because of dry run. Exit now')
        return MdbStepDownExitCodes.DRY_RUN

    try:
        stepdown(
            host_port,
            replset_ctl,
            server_election_timeout=mongo_conn.server_selection_timeout,
            step_down_secs=stepdown_conf['replSetStepDown'],
            secondary_catch_up_period_secs=stepdown_conf['secondaryCatchUpPeriodSecs'],
            ensure_new_primary=True)
        exit_code = MdbStepDownExitCodes.SUCCEEDED
        logging.info('mongod stepped down successfully')

    except StepdownTimeoutExceeded:
        logging.critical('Stepdown timeout exceeded', exc_info=True)
        exit_code = MdbStepDownExitCodes.STEPDOWN_TIMED_OUT

    except StepdownPrimaryWaitTimeoutExceeded:
        logging.error('Primary election timeout exceeded', exc_info=True)
        exit_code = MdbStepDownExitCodes.ELECTION_TIMED_OUT

    except StepdownUnexpectedError:
        logging.critical('Unexpected error occurred during resetup', exc_info=True)
        exit_code = MdbStepDownExitCodes.UNEXPECTED_ERROR

    except StepdownSamePrimaryElected:
        logging.critical('Same primary elected after stepdown', exc_info=True)
        exit_code = MdbStepDownExitCodes.SAME_PRIMARY_ELECTED

    return exit_code


def main():
    """
    Main
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', type=str, default=DEFAULT_CONFIG_PATH, dest='config', help='Config file')
    parser.add_argument('-n', '--dry-run', action='store_true', dest='dryrun', help='Dry run')
    parser.add_argument('-f', '--force', action='store_true', dest='force', help='Force stepdown, skip safety checks')
    args = parser.parse_args()

    conf = load_config(args.config, DEFAULT_CONFIG)
    setup_logging(conf['logging'])
    exit_code = stepdown_mongod(conf, args.dryrun, args.force)
    sys.exit(exit_code.value)  # TODO: report status and short description to stdout


if __name__ == '__main__':
    main()
