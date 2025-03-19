# -*- coding: utf-8 -*-
"""
DBaaS Worker
"""
from contextlib import closing

from cloud.mdb.dbaas_worker.internal.config import worker_args_parser
from cloud.mdb.dbaas_worker.internal.main import DbaasWorker
from cloud.mdb.dbaas_worker.internal.metadb import get_master_conn


def dbaas_worker():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('-t', '--test', action='store_true', help='Check config against schema and exit')

    args = parser.parse_args()

    worker = DbaasWorker(args.config)
    if args.test:
        with closing(
            get_master_conn(
                worker.config.main.metadb_dsn,
                worker.config.main.metadb_hosts,
                worker.log,
            )
        ):
            pass
    else:
        worker.start()
