# -*- coding: utf-8 -*-
"""
Simple script to check that cert api works well
"""

import logging
from queue import Queue

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.certificator import CertificatorApi


def cert_host():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('fqdns', type=str, nargs='+', help='target host FQDN and alt_names')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    config = get_config(args.config)

    host = args.fqdns[0]
    alt_names = args.fqdns[1:]
    task = {
        'task_id': f'cert_host-{host}',
        'timeout': 24 * 3600,
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'test',
    }
    queue = Queue(maxsize=10**6)
    cert_api = CertificatorApi(config, task, queue)

    result = cert_api.issue(host, alt_names)
    print(f'got certificate: {result!r}')
