"""
Script for host resetup
"""

import logging
import socket
from datetime import datetime
from logging.config import dictConfig
from queue import Queue

import time

from cloud.mdb.dbaas_worker.internal import environment
from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.tools.resetup import HostResetuper


def conf_logging():
    LOGFILE = '/var/log/resetup-{dt}.log'.format(dt=datetime.now().isoformat())
    dictConfig(
        {
            'version': 1,
            'disable_existing_loggers': True,
            'formatters': {
                'default': {
                    'class': 'logging.Formatter',
                    'datefmt': '%Y-%m-%d %H:%M:%S',
                    'format': '%(asctime)s %(name)-15s %(levelname)-10s %(message)s',
                },
            },
            'handlers': {
                'streamerrorhandler': {
                    'level': 'WARNING',
                    'class': 'logging.StreamHandler',
                    'formatter': 'default',
                    'stream': 'ext://sys.stdout',
                },
                'streamhandler': {
                    'level': 'DEBUG',
                    'class': 'logging.StreamHandler',
                    'formatter': 'default',
                    'stream': 'ext://sys.stdout',
                },
                'filehandler': {
                    'level': 'DEBUG',
                    'class': 'logging.FileHandler',
                    'formatter': 'default',
                    'filename': LOGFILE,
                },
                'null': {
                    'level': 'DEBUG',
                    'class': 'logging.NullHandler',
                },
            },
            'loggers': {
                '': {
                    'handlers': [
                        'streamerrorhandler',
                        'filehandler',
                    ],
                    'level': 'DEBUG',
                },
                'stages': {
                    'handlers': ['streamhandler'],
                    'level': 'DEBUG',
                },
                'retry': {
                    'handlers': ['null'],
                    'level': 'DEBUG',
                },
            },
        }
    )
    logging.getLogger('stages').info('log file at "tail -f %s"', LOGFILE)


def check_host_availability(fqdn):
    logger = logging.getLogger('stages')
    logger.debug("check if %s:22 is available", fqdn)
    connection_timeout = 2
    for _ in range(3):
        s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        s.settimeout(connection_timeout)
        try:
            s.connect((fqdn, 22))
            logger.debug('successfully connected')
            return True
        except socket.timeout:
            logger.debug('connection timed out (%d seconds)', connection_timeout)
        except OSError as exc:
            if 'No route to host' in str(exc):
                logger.debug('no route to host')
        finally:
            s.close()
        time.sleep(2)

    return False


def resetup_host():
    """
    Console entry-point
    """
    parser = worker_args_parser(
        description='\n'.join(
            [
                'Example: ./resetup-host sas-o2mexowi0ajkcx98.db.yandex.net',
                'If you are in PORTO, the dom0 must be switched off.',
            ]
        )
    )
    parser.add_argument(
        '-m',
        '--method',
        default='readd',
        const='readd',
        nargs='?',
        choices=['readd', 'restore'],
        help='Resetup method (readd or restore, default: %(default)s)',
    )
    parser.add_argument('-s', '--save', action='store_true', help='Save instance disks (if possible)')
    parser.add_argument('-o', '--offline', action='store_true', help='Operate on stopped cluster')
    parser.add_argument('-p', '--preserve', action='store_true', help='Skip destructive actions (in porto)')
    parser.add_argument('-i', '--ignore', type=str, nargs='*', help='Drop FQDNs from task args')
    parser.add_argument('-v', '--verbose', action='store_true', help='Log everything to stderr')
    parser.add_argument('host', type=str, help='target host FQDN')
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    else:
        conf_logging()

    config = get_config(args.config)
    worker_env = environment.get_env_name_from_config(config)

    if worker_env == environment.EnvironmentName.PORTO:
        if not args.preserve and check_host_availability(args.host):
            raise Exception(f"{args.host} is available. Resetup will provide duplicated container.")
    else:
        logging.getLogger('stages.is_available').debug("workers works at %s, don't need availability check", worker_env)

    resetuper = HostResetuper(
        config,
        task={},
        queue=Queue(maxsize=10**6),
    )

    if args.ignore and args.host in args.ignore:
        raise RuntimeError('Target host could not be ignored')
    if args.method == 'readd':
        resetuper.readd(args.host, args.ignore, args.save, args.offline, args.preserve)
    else:
        resetuper.restore_from_backup(args.host, args.ignore, args.save, args.offline, args.preserve)
