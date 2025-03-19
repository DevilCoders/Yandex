# -*- coding: utf-8 -*-
"""
DBaaS Internal API quota api mappings
"""
SPEC_TO_DB_QUOTA_MAPPING = {
    'mdb.hdd.size': 'hdd_space',
    'mdb.ssd.size': 'ssd_space',
    'mdb.memory.size': 'memory',
    'mdb.gpu.count': 'gpu',
    'mdb.cpu.count': 'cpu',
    'mdb.clusters.count': 'clusters',
}

DB_TO_SPEC_QUOTA_MAPPING = {v: k for k, v in SPEC_TO_DB_QUOTA_MAPPING.items()}


def db_quota_limit_field(metric):
    """
    Returns database quota limit field name for specified metric
    """
    return metric + '_quota'


def db_quota_usage_field(metric):
    """
    Returns database quota usage field name for specified metric
    """
    return metric + '_used'
