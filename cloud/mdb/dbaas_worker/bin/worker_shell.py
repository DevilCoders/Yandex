# -*- coding: utf-8 -*-
"""
DBaaS Worker
"""

from queue import Queue
from logging.config import dictConfig

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.internal.python.ipython_repl import UserNamespace, start_repl
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi
from cloud.mdb.dbaas_worker.internal.providers.solomon_client.client import SolomonApiV2
from cloud.mdb.dbaas_worker.internal.providers.vpc import VPCProvider
from cloud.mdb.dbaas_worker.internal.providers.resource_manager import ResourceManagerApi
from cloud.mdb.dbaas_worker.internal.providers.disk_placement_group import DiskPlacementGroupProvider


def get_task() -> dict:
    return {  # noqa
        'cid': 'localhost',
        'task_id': 'worker-shell',
        'timeout': 24 * 3600,
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'test',
    }


def get_queue() -> Queue:
    return Queue(maxsize=10**6)


def get_compute_api(config) -> ComputeApi:
    return ComputeApi(config, get_task(), get_queue())


def get_solomon_api(config) -> ComputeApi:
    return SolomonApiV2(config, get_task(), get_queue())


def get_vpc(config) -> VPCProvider:
    return VPCProvider(config, get_task(), get_queue())


def get_disk_placement_group_provider(config) -> DiskPlacementGroupProvider:
    return DiskPlacementGroupProvider(config, get_task(), get_queue())


def get_resource_manager_api(config) -> ResourceManagerApi:
    return ResourceManagerApi(config, get_task(), get_queue())


def worker_shell():
    """
    Shell interactive entry-point
    """
    parser = worker_args_parser()
    args = parser.parse_args()

    dictConfig(
        {
            'version': 1,
            'disable_existing_loggers': True,
            'formatters': {
                'json': {
                    '()': 'cloud.mdb.internal.python.logs.format.json.JsonFormatter',
                },
            },
            'handlers': {
                'streamhandler': {
                    'level': 'DEBUG',
                    'class': 'logging.StreamHandler',
                    'formatter': 'json',
                    'stream': 'ext://sys.stderr',
                },
                'null': {
                    'level': 'DEBUG',
                    'class': 'logging.NullHandler',
                },
            },
            'loggers': {
                '': {
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

    config = get_config(args.config)  # noqa

    start_repl(
        [
            UserNamespace(config, 'config'),
            UserNamespace(get_queue, 'get_queue'),
            UserNamespace(get_task, 'get_task'),
            UserNamespace(get_compute_api, 'get_compute_api', 'compute = get_compute_api(config)'),
            UserNamespace(get_vpc, 'get_vpc', 'vpc = get_vpc(config)'),
            UserNamespace(
                get_resource_manager_api, 'get_resource_manager_api', 'rm = get_resource_manager_api(config)'
            ),
            UserNamespace(
                get_disk_placement_group_provider,
                'get_disk_placement_group_provider',
                'dpg = get_disk_placement_group_provider(config)',
            ),
            UserNamespace(get_solomon_api, 'get_solomon_api', 'solomon = get_solomon_api(config)'),
        ]
    )
