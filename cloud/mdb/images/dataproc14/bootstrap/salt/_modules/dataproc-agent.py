# -*- coding: utf-8 -*-
"""
Module to provide methods for working with dataproc-agent daemon
:platform: all
"""

from __future__ import absolute_import, with_statement

import logging

log = logging.getLogger(__name__)

try:
    import os
    import shutil
    import tempfile
    import requests
    MODULES_OK = True
except ImportError:
    MODULES_OK = False


__virtualname__ = 'dataproc-agent'


def __virtual__():
    """
    Determine whether or not to load this module
    """
    if MODULES_OK:
        return __virtualname__
    return False


def get_ui_proxy_url(host, port):
    """
    Method returns ui-proxy url to <host>:<port>
    """
    ui_proxy = __salt__['pillar.get']('data:ui_proxy', False)
    ui_proxy_url = __salt__['pillar.get']('data:agent:ui_proxy_url', None)
    cluster_id = __salt__['pillar.get']('data:agent:cid', None)

    node = host.split('.')[0]

    if not ui_proxy or not ui_proxy_url or not cluster_id:
        return None
    return ui_proxy_url.replace('https://', 'https://ui-{cluster_id}-{node}-{port}.'.format(
           cluster_id=cluster_id,
            node=node,
            port=port,
        )
    )


def get_max_concurrent_jobs(reserved_memory_for_services, job_memory_footprint, pillar=None):
    """
    Calculate limit for maximum concurrent jobs number based on masternode hardware resources
    see MDB-12202 for reference
    """
    if not pillar:
        pillar = __salt__['pillar.get']('data')

    properties = pillar.get('properties', {})
    dataproc_properties = properties.get('dataproc', {})
    max_concurrent_jobs = int(dataproc_properties.get('max-concurrent-jobs', 0))
    if max_concurrent_jobs:
        return max_concurrent_jobs
    main_subcluster_id = pillar['subcluster_main_id']
    subclusters = pillar['topology']['subclusters']
    main_subcluster = subclusters[main_subcluster_id]
    masternode_memory_bytes = main_subcluster['resources']['memory']
    masternode_memory_for_jobs = masternode_memory_bytes - reserved_memory_for_services
    max_concurrent_jobs = max(1, int(masternode_memory_for_jobs / job_memory_footprint))
    return max_concurrent_jobs


def get_agent_repo():
    """
    Resolve repository of dataproc-agent
    """
    properties = __salt__['pillar.get']('data:properties:dataproc',{})
    return properties.get('repo', 'https://dataproc.storage.yandexcloud.net/agent')


def get_agent_version():
    """
    Resolve dataproc-agent binary version
    """
    properties = __salt__['pillar.get']('data:properties:dataproc',{})
    repo = get_agent_repo()
    version = properties.get('version', 'latest')
    if version not in ('latest', 'testing'):
        return version
    try:
        r = requests.get('{repo}/{version}'.format(repo=repo, version=version), timeout=5.0)
        r.raise_for_status()
        return r.text.strip()
    except Exception as err:
        log.info('Failed to get latest version of dataproc-agent', exc_info=err)
    return None


def get_current_version():
    """
    Resolve current verison of dataproc-agent
    """
    version_path = '/opt/yandex/dataproc-agent/version'
    if not os.path.exists(version_path):
        return None
    try:
        with open(version_path, 'r') as f:
            return f.read()
    except IOError as err:
        log.warning('Failed to determine installed version of dataproc-agent', exc_info=err)
    return None


def download_file(url, path):
    """
    Download agent to path
    """
    r = requests.get(url, timeout=15.0, stream=True)
    r.raise_for_status()
    with open(path, 'wb') as f:
        for chunk in r.iter_content(chunk_size=2<<20):
            f.write(chunk)


def update(dry_run=False):
    """
    Update dataproc-agent
    """
    repo = get_agent_repo()
    version = get_agent_version()
    if not version:
        return (False, 'Failed to get latest version of dataproc-agent')
    current_version = get_current_version()
    agent = '{repo}/dataproc-agent-{version}'.format(repo=repo, version=version)
    binary = 'dataproc-agent'
    installed = '/opt/yandex/dataproc-agent'

    has_changes = current_version != version
    changes = None
    if not current_version:
        current_version = 'unknown'
    if has_changes:
        changes = 'dataproc-agent will be updated to version {version} from {current_version}'.format(
            version=version,
            current_version=current_version,
        )

    if dry_run:
        return (has_changes, changes)

    try:
        log.debug("Starting update dataproc-agent from version {current_version} to {version}")
        tmpdir = tempfile.mkdtemp()
        try:
            with open('{tmpdir}/version'.format(tmpdir=tmpdir), 'w') as f:
                f.write(version)
            download_file(agent, '{tmpdir}/{binary}'.format(tmpdir=tmpdir, binary=binary))
            os.chmod('{tmpdir}/{binary}'.format(tmpdir=tmpdir, binary=binary), 755)
            shutil.move(
                '{tmpdir}/{binary}'.format(tmpdir=tmpdir, binary=binary),
                '{installed}/bin/{binary}'.format(installed=installed, binary=binary),
            )
            shutil.move('{tmpdir}/version'.format(tmpdir=tmpdir), '{installed}/version'.format(installed=installed))
        finally:
            shutil.rmtree(tmpdir)
    except Exception as err:
        log.warn('Failed to update dataproc-agent', exc_info=err)
        return (False, 'Failed to update dataproc-agent to version {version}'.format(version=version))
    return (
        has_changes,
        'dataproc-agent updated to version {version} from {current_version}'.format(
            version=version,
            current_version=current_version,
        )
    )
