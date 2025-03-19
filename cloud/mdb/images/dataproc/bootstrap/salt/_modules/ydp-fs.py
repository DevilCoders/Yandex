# -*- coding: utf-8 -*-
"""
Module to provide methods to determine FS schema to be used for file/dir paths in configs
:platform: all
"""

from __future__ import absolute_import

import itertools
import logging
import os

log = logging.getLogger(__name__)

__virtualname__ = 'ydp-fs'


def __virtual__():
    """
    Determine whether or not to load this module
    """
    return __virtualname__


class DataprocWrongConfigurationException(Exception):
    pass


def ydp_fs_protocol():
    if 'hdfs' in __salt__['pillar.get']('data:services', []):
        return 'hdfs'
    if __salt__['pillar.get']('data:s3_bucket', None):
        return 's3'
    return 'file'


def dfs_enabled():
    return ydp_fs_protocol() != 'file'


def is_singlenode(pillar=None):
    pillar = pillar or __salt__['pillar.get']('data')
    cluster_hosts = itertools.chain.from_iterable(
        subcluster['hosts'] for subcluster in pillar['topology']['subclusters'].values()
    )
    return len(list(cluster_hosts)) == 1


def fs_url_for_path(
    path='/',
    hdfs_hostport=None,
    s3_prefix='dataproc/hadoop',
    allow_local=False,
    pillar=None,
):
    pillar = pillar or __salt__['pillar.get']('data')
    hdfs_available = 'hdfs' in pillar.get('services', [])
    s3_bucket = pillar.get('s3_bucket', None)

    if hdfs_available:
        return 'hdfs://{hdfs_path}'.format(hdfs_path=os.path.normpath('/'.join((hdfs_hostport or '', path))))
    elif s3_bucket:
        return 's3a://{s3_path}'.format(s3_path=os.path.normpath('/'.join((s3_bucket, s3_prefix or '', path))))
    elif allow_local or is_singlenode(pillar):
        return os.path.join('/', path)
    else:
        raise DataprocWrongConfigurationException(
            'Either provide HDFS service / S3 bucket, or allow local fs usage for this path'
        )
