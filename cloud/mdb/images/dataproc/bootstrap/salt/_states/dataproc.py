# -*- coding: utf-8 -*-
"""
States for working with dataproc
"""

import time
import logging

LOG = logging.getLogger(__name__)


def __virtual__():
    return 'dataproc'


def s3_available(name, timeout=300):
    for s3_prop in [
        'data:s3_bucket',
    ]:
        if not __salt__['pillar.get'](s3_prop, None):
            return {
                'name': name,
                'result': False,
                'comment': 'Property {} is not set'.format(s3_prop),
                'changes': {},
            }
    return {
        'name': name,
        'result': True,
        'comment': 'S3 should be available',
        'changes': {},
    }


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
        except Exception as e:
            LOG.warn(e)
        time.sleep(1)
    return {
        'name': name,
        'result': False,
        'comment': 'HDFS is not have free available space, {}'.format(stats),
        'changes': {},
    }


def hbase_rs_available(name, timeout=300):
    """
    Ensure that HBase has at least one live regionserver.
    """
    start_ts = time.time()
    stats = None
    while time.time() - start_ts < timeout:
        try:
            stats = __salt__['ydputils.get_hbase_stats']()
            alive = stats.get('numRegionServers', 0)
            if alive > 0:
                return {
                    'name': name,
                    'result': True,
                    'comment':
                        'HBase has at least {} alive regionservers'.format(alive),
                    'changes': {},
                }
            LOG.debug('HBase rs alive: {}, stats: {}'.format(alive, stats))
        except Exception as e:
            LOG.warn("HBase rs failed", e)
        time.sleep(1)

    return {
        'name': name,
        'result': False,
        'comment': 'Failed to wait alive HBase region servers, {}'.format(stats),
        'changes': {},
    }


def hbase_master_available(name, timeout=300):
    """
    Ensure that HBase master alive
    """
    start_ts = time.time()
    stats = None
    while time.time() - start_ts < timeout:
        try:
            stats = __salt__['ydputils.get_hbase_stats']()
            return {
                'name': name,
                'result': True,
                'comment':
                    'HBase Master alive, stats: {}'.format(stats),
                'changes': {},
            }
        except Exception as e:
            LOG.warn("HBase Master failed", e)
        time.sleep(1)

    return {
        'name': name,
        'result': False,
        'comment': 'Failed to wait alive HBase master, {}'.format(stats),
        'changes': {},
    }

def hbase_rest_available(name, timeout=300):
    """
    Ensure that HBase REST available
    """
    start_ts = time.time()
    stats = None
    while time.time() - start_ts < timeout:
        try:
            status = __salt__['ydputils.get_hbase_cluster_status']()
            return {
                'name': name,
                'result': True,
                'comment':
                    'HBase REST alive, status: {}'.format(status),
                'changes': {},
            }
        except Exception as e:
            LOG.warn("HBase REST failed", e)
        time.sleep(1)

    return {
        'name': name,
        'result': False,
        'comment': 'Failed to wait alive HBase REST, {}'.format(stats),
        'changes': {},
    }

def oozie_available(name, timeout=300):
    """
    Ensure that Oozie available
    """
    start_ts = time.time()
    resp = None
    while time.time() - start_ts < timeout:
        try:
            resp = __salt__['ydputils.get_oozie_status']()
            LOG.info('Oozie alive, stats: {}'.format(resp))
            return {
                'name': name,
                'result': True,
                'comment':
                    'Oozie alive, stats: {}'.format(resp),
                'changes': {},
            }
        except Exception as e:
            LOG.warn("Oozie is not alive", e)
        time.sleep(1)

    return {
        'name': name,
        'result': False,
        'comment': 'Failed to wait alive Oozie, {}'.format(resp),
        'changes': {},
    }


def zeppelin_available(name, timeout=300):
    """
    Ensure that Zeppelin available
    """
    start_ts = time.time()
    resp = None
    while time.time() - start_ts < timeout:
        try:
            resp = __salt__['ydputils.get_zeppelin_status']()
            LOG.info('Zeppelin alive, stats: {}'.format(resp))
            if resp.get('status') != 'OK':
                continue
            return {
                'name': name,
                'result': True,
                'comment':
                    'Zeppelin alive, stats: {}'.format(resp),
                'changes': {},
            }
        except Exception as e:
            LOG.warn("Zeppelin is not alive", e)
        time.sleep(1)

    return {
        'name': name,
        'result': False,
        'comment': 'Failed to wait alive Zeppelin, {}'.format(resp),
        'changes': {},
    }

def livy_available(name, timeout=300):
    """
    Ensure that Livy available
    """
    start_ts = time.time()
    resp = None
    while time.time() - start_ts < timeout:
        try:
            resp = __salt__['ydputils.get_livy_ping']()
            if not 'pong' in resp:
                continue
            return {
                'name': name,
                'result': True,
                'comment':
                    'Livy alive, stats: {}'.format(resp),
                'changes': {},
            }
        except Exception as e:
            LOG.warn("Livy is not alive", e)
        time.sleep(1)

    return {
        'name': name,
        'result': False,
        'comment': 'Failed to wait alive Livy, {}'.format(resp),
        'changes': {},
    }
