"""
MongoDB stepdown script

stepdown behavior: https://docs.mongodb.com/manual/reference/command/replSetStepDown/
"""

import argparse
import logging
import socket
import sys
from enum import Enum
from typing import Callable, Optional, Tuple

from mdb_mongo_tools.exceptions import GetterFailure, GetterUnexpectedError
from mdb_mongo_tools.getters import GetterExc, MdbMongoGetters
from mdb_mongo_tools.mongodb import (MONGO_DEFAULT_CONNECT_TIMEOUT_MS, MONGO_DEFAULT_SERVER_SELECTION_TIMEOUT_MS,
                                     MONGO_DEFAULT_SOCKET_TIMEOUT_MS)
from mdb_mongo_tools.util import load_config, retry, setup_logging

DEFAULT_CONFIG_PATH = '/etc/yandex/mdb-mongo-tools/mdb-mongo-tools.conf'

DEFAULT_CONFIG = {
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
    'getter': {
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
    'logging': {
        'log_file': '/var/log/mongodb/mdb-mongo-tools.log',
        'log_level_root': 'DEBUG',
        'log_format': '%(asctime)s [%(levelname)s] %(process)d %(module)s:\t%(message)s',
    },
}


class MdbExecCheckExitCodes(Enum):
    """
    Exit code enum
    """
    SUCCEEDED = 0
    FAILED = 10
    UNEXPECTED_ERROR = 50


def exec_getter(getter: MdbMongoGetters, conf: dict, try_time: Optional[int], try_num: Optional[int],
                try_delay: Optional[int]) -> Tuple[MdbExecCheckExitCodes, str]:
    """
    Execute getter_exc
    """
    host_port = '{host}:{port}'.format(**conf['mongodb'])
    logging.debug('Starting getter %s execution on %s', getter, host_port)

    getter_exc = GetterExc(conf)
    getter_func: Callable = getter_exc.perform
    if try_time or try_num:
        getter_func = retry(
            exception_types=(GetterFailure, GetterUnexpectedError),
            max_attempts=try_num,
            max_wait=try_time,
            max_interval=try_delay)(getter_exc.perform)

    try:
        return MdbExecCheckExitCodes.SUCCEEDED, getter_func(getter)
    except GetterFailure as exc:
        return MdbExecCheckExitCodes.FAILED, str(exc)
    except GetterUnexpectedError as exc:
        return MdbExecCheckExitCodes.UNEXPECTED_ERROR, str(exc)


def main() -> None:
    """
    Main
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('getter', help='Getter name', choices=tuple(t.value for t in MdbMongoGetters))
    parser.add_argument('-q', '--quiet', action='store_true', dest='quiet', help='Suppress normal output')
    parser.add_argument('-c', '--config', type=str, default=DEFAULT_CONFIG_PATH, dest='config', help='Config file')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-t', '--try-time', type=int, default=None, dest='try_time', help='Time to wait while trying')
    group.add_argument('-n', '--try-number', type=int, default=None, dest='try_num', help='Number of tries')
    parser.add_argument(
        '-w', '--try-wait', type=int, default=3, dest='try_delay', help='Delay between tries in seconds')

    args = parser.parse_args()

    conf = load_config(args.config, DEFAULT_CONFIG)
    setup_logging(conf['logging'])

    exit_code, report = exec_getter(MdbMongoGetters(args.getter), conf, args.try_time, args.try_num, args.try_delay)
    if not args.quiet:
        std_stream = sys.stderr if exit_code.value else sys.stdout
        print(report, file=std_stream)

    sys.exit(exit_code.value)


if __name__ == '__main__':
    main()
