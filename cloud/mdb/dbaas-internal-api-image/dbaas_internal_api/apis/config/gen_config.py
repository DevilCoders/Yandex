# -*- coding: utf-8 -*-
"""
DBaaS Internal API generic host config
"""

import psycopg2
from flask import g

from ...core.exceptions import UnknownClusterForHostError
from ...utils import metadb


def gen_config(fqdn, target_pillar_id=None, rev=None):
    """
    Get config and do commit

    AHTUNG: This function do COMMIT. don't use it in transaction scope!
    """
    # All queries here are read-only, but we still commit
    # to move connection into STATUS_READY state
    try:
        config = metadb.get_managed_config(fqdn, target_pillar_id, rev)
        g.metadb.commit()
    except psycopg2.Error as exc:
        if exc.pgcode == 'MDB01':
            return {}
        raise

    return config


def gen_config_unmanaged(fqdn, target_pillar_id=None, rev=None):
    """
    Get config for unmanaged db and do commit

    AHTUNG: This function do COMMIT. don't use it in transaction scope!
    """
    try:
        config = metadb.get_unmanaged_config(fqdn, target_pillar_id, rev)
    except psycopg2.Error as exc:
        if exc.pgcode == 'MDB01':
            raise UnknownClusterForHostError(fqdn)
        raise

    cfg = {}
    if config:
        cfg = {'data': config['data'].get('unmanaged')}
        if config.get('ssh_authorized_keys'):
            cfg['ssh_authorized_keys'] = config['ssh_authorized_keys']
    g.metadb.commit()
    return cfg
