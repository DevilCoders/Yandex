"""
Utilities for worker
"""
from typing import Sequence


def format_zk_hosts(zk_host: Sequence[str]) -> str:
    """
    Format Zookeeper hosts in task_args format
    """
    return ','.join('{0}:2181'.format(h) for h in zk_host)
