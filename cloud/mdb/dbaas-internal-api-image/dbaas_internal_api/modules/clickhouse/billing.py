# -*- coding: utf-8 -*-
"""
DBaaS Internal API clickhouse cluster billing estimate helpers
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
from ...utils.validation import get_flavor_by_name
from .constants import MY_CLUSTER_TYPE
from .create import process_host_specs
from .traits import ClickhouseRoles
from .utils import ClickhouseConfigSpec, get_zk_zones, split_host_specs
from .zookeeper import get_default_zk_resources


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CLUSTER, DbaasOperation.BILLING_CREATE)
def billing_create_clickhouse_cluster(config_spec: dict, host_specs: List[dict], **_):
    """
    Returns billing metrics for cost estimator
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_CONSOLE_API')

    try:
        # Load and validate config spec
        ch_config_spec = ClickhouseConfigSpec(config_spec)
    except ValueError as err:
        raise errors.ParseConfigError(err)

    ch_resources, zk_resources = ch_config_spec.resources
    ch_host_specs, zk_host_specs = split_host_specs(host_specs)

    ch_flavor = get_flavor_by_name(ch_resources.resource_preset_id)
    ch_cores = ch_flavor['cpu_limit'] * len(ch_host_specs)
    zk_zones = get_zk_zones(zk_host_specs)

    merged_zk_resources = get_default_zk_resources(ch_cores, zk_zones)
    merged_zk_resources.update(zk_resources)

    ch_host_specs, zk_host_specs = process_host_specs(
        ch_host_specs, zk_host_specs, ch_resources, merged_zk_resources, ch_config_spec.embedded_keeper
    )

    for host in ch_host_specs:
        # pylint: disable=no-member
        yield generate_metric(
            host, ch_resources, MY_CLUSTER_TYPE, [ClickhouseRoles.clickhouse.value], g.folder['folder_ext_id']
        )

    for host in zk_host_specs:
        # pylint: disable=no-member
        yield generate_metric(
            host, merged_zk_resources, MY_CLUSTER_TYPE, [ClickhouseRoles.zookeeper.value], g.folder['folder_ext_id']
        )


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
def billing_create_clickhouse_hosts(billing_host_specs: List[dict], **_):
    """
    Returns billing metrics for cost estimator
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_CONSOLE_API')

    for spec in billing_host_specs:
        host = spec['host']
        # (billing_)host_specs here have additional resource information.
        # Resource data has to be provided to handle situations like addZookeeper,
        # where different parts of the cluster will have different configuration.
        resources = parse_resources(spec['resources'])
        host_type = host.get('type', ClickhouseRoles.clickhouse)
        yield generate_metric(host, resources, MY_CLUSTER_TYPE, [host_type.value], g.folder['folder_ext_id'])
