# -*- coding: utf-8 -*-
"""
DBaaS Internal API mongodb cluster billing estimate helpers
"""

from typing import List

from flask import g

from ...core import exceptions as errors
from ...utils.billing import generate_metric
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.types import parse_resources
from .constants import MONGOD_HOST_TYPE, MY_CLUSTER_TYPE
from .traits import MongoDBRoles
from .types import MongodbConfigSpec


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CLUSTER, DbaasOperation.BILLING_CREATE)
def billing_create_mongodb_cluster(config_spec: dict, host_specs: List[dict], **_):
    """
    Returns billing metrics for cost estimator
    """
    try:
        # Load and validate config spec
        mongo_config_spec = MongodbConfigSpec(config_spec)
    except ValueError as err:
        raise errors.ParseConfigError(err)

    resources = mongo_config_spec.get_resources(MONGOD_HOST_TYPE)

    for host in host_specs:
        # pylint: disable=no-member
        yield generate_metric(host, resources, MY_CLUSTER_TYPE, [MongoDBRoles.mongod.value], g.folder['folder_ext_id'])


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
def billing_create_mongodb_hosts(billing_host_specs: List[dict], **_):
    """
    Returns billing metrics for cost estimator
    """
    for spec in billing_host_specs:
        host = spec['host']
        # (billing_)host_specs here have additional resource information.
        # Resource data has to be provided to handle situations like enableSharding,
        # where different parts of the cluster will have different configuration.
        resources = parse_resources(spec['resources'])
        host_type = host.get('type', MongoDBRoles.mongod)
        yield generate_metric(host, resources, MY_CLUSTER_TYPE, [host_type.value], g.folder['folder_ext_id'])
