# -*- coding: utf-8 -*-
"""
DBaaS Internal API postgresql cluster billing estimate helpers
"""

from typing import List

from flask import g

from ...core import exceptions as errors
from ...utils.billing import generate_metric
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from .constants import MY_CLUSTER_TYPE
from .types import PostgresqlConfigSpec


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CLUSTER, DbaasOperation.BILLING_CREATE)
def billing_create_postgresql_cluster(config_spec: dict, host_specs: List[dict], host_group_ids: List[str] = None, **_):
    """
    Returns billing metrics for cost estimator
    """
    try:
        # Load and validate config spec
        pg_config_spec = PostgresqlConfigSpec(config_spec)
    except ValueError as err:
        raise errors.ParseConfigError(err)

    resources = pg_config_spec.get_resources()
    on_dedicated_host = (host_group_ids is not None) and (len(host_group_ids) > 0)

    for host in host_specs:
        yield generate_metric(
            host,
            resources,
            MY_CLUSTER_TYPE,
            [MY_CLUSTER_TYPE],
            g.folder['folder_ext_id'],
            on_dedicated_host=on_dedicated_host,
        )
