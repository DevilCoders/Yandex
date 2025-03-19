"""
Simple script for fixing ext dns after unexpected reallocation in compute
"""

import logging
from queue import Queue

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi
from cloud.mdb.dbaas_worker.internal.providers.dns import DnsApi, Record


def fix_ext_dns():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('hosts', type=str, nargs='*', help='FQDNs of hosts to fix')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')

    task = {
        'task_id': 'dns-fix',
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'test',
    }
    config = get_config(args.config)
    compute_api = ComputeApi(config, task, Queue())
    dns_api = DnsApi(config, task, Queue())

    for host in args.hosts:
        if host.endswith(config.postgresql.managed_zone):
            raise RuntimeError(f'Managed hostname {host} used')
        setup_address = compute_api.get_instance_setup_address(host)
        managed_records = [
            Record(
                address=setup_address,
                record_type='AAAA',
            ),
        ]
        dns_api.set_records(f'{host.split(".")[0]}.{config.postgresql.managed_zone}', managed_records)
        public_records = []
        for address, version in compute_api.get_instance_public_addresses(host):
            public_records.append(Record(address=address, record_type=('AAAA' if version == 6 else 'A')))
        dns_api.set_records(host, public_records)
