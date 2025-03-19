# -*- coding: utf-8 -*-
"""
DBaaS Internal API redis cluster billing estimate helpers
"""

from typing import List

from flask import g

from ...core import exceptions as errors
from ...utils.billing import generate_metric
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.types import parse_resources
from .constants import MY_CLUSTER_TYPE
from .traits import RedisRoles
from .types import RedisConfigSpec, _resolve_disk_type_id


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CLUSTER, DbaasOperation.BILLING_CREATE)
def billing_create_redis_cluster(config_spec: dict, host_specs: List[dict], **_):
    """
    Returns billing metrics for cost estimator
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CONSOLE_API')
    try:
        r_config_spec = RedisConfigSpec(config_spec)
    except ValueError as err:
        raise errors.ParseConfigError(err)
    resources = r_config_spec.get_resources()

    for host in host_specs:
        yield generate_metric(host, resources, MY_CLUSTER_TYPE, [MY_CLUSTER_TYPE], g.folder['folder_ext_id'])


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
def billing_create_redis_hosts(billing_host_specs: List[dict], **_):
    """
    Returns billing metrics for cost estimator
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CONSOLE_API')
    for spec in billing_host_specs:
        host = spec['host']
        resources = parse_resources(spec['resources'], disk_type_id_parser=_resolve_disk_type_id)
        host_type = host.get('type', RedisRoles.redis)
        yield generate_metric(host, resources, MY_CLUSTER_TYPE, [host_type.value], g.folder['folder_ext_id'])
