"""
Script for task-run
"""

import logging
import socket
from datetime import datetime
from logging.config import dictConfig
from queue import Queue


from ..internal.config import get_config, worker_args_parser
from ..internal.providers.base_metadb import BaseMetaDBProvider
from ..internal.query import execute
from ..internal.runners import resolve_args
from ..internal.tasks import get_executor


def conf_logging():
    LOGFILE = '/var/log/task-run-{dt}.log'.format(dt=datetime.now().isoformat())
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


def get_worker_id() -> str:
    """
    Return worker_id
    """
    return socket.getfqdn()


def poll(task_id, txn):
    """
    Try to lock task in queue
    """
    cursor = txn.cursor()
    ret = execute(cursor, 'get_task', task_id=task_id)
    return ret[0]


def task_run():
    """
    Console entry-point
    """
    parser = worker_args_parser(
        description='\n'.join(
            [
                'Example: ./task-run <task_id>',
            ]
        )
    )

    parser.add_argument('task_id', type=str, help='target host FQDN')
    args = parser.parse_args()

    conf_logging()
    config = get_config(args.config)
    list_task = {
        'cid': 'list-cids',
        'task_id': 'list-cids',
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'test',
    }

    queue = Queue(maxsize=10**6)

    metadb = BaseMetaDBProvider(config, list_task, queue)
    with metadb.get_master_conn() as conn:
        with conn:
            task = poll(args.task_id, conn)
            argsTask = resolve_args(conn, task)
            task['changes'] = []
            if not task['context']:
                task['context'] = {}
            task['feature_flags'] = task['task_args']['feature_flags']
            task['folder_id'] = task['ext_folder_id']
            task['task_id'] = ""
            task['timeout'] = 2 * 3600
            executor = get_executor(task, config, queue, argsTask)
            try:
                executor.run()
            except Exception:
                raise
