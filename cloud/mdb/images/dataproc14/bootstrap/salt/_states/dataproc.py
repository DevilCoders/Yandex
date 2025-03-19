# -*- coding: utf-8 -*-
"""
States for working with dataproc
"""

import time
import logging

LOG = logging.getLogger(__name__)


def __virtual__():
    return 'dataproc'


def hdfs_available(name, timeout=300):
    """
    Ensure that HDFS has at least one live datanode.
    """
    start_ts = time.time()
    stats = None
    while time.time() - start_ts < timeout:
        try:
            stats = __salt__['ydputils.get_hdfs_stats']()
            free = stats.get('Free', 0)
            if free > 0:
                return {
                    'name': name,
                    'result': True,
                    'comment':
                        'HDFS has at least {} bytes available'.format(free),
                    'changes': {},
                }
            LOG.debug('HDFS free space: {}, stats: {}'.format(free, stats))
            time.sleep(1)
        except Exception as e:
            LOG.warn(e)
    return {
        'name': name,
        'result': False,
        'comment': 'HDFS is not have free available space, {}'.format(stats),
        'changes': {},
    }
