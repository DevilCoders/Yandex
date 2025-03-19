# -*- coding: utf-8 -*-
"""
DBaaS Internal API admin related schemas
"""
from enum import Enum, unique

from marshmallow import Schema
from marshmallow.fields import Float, Int

from .fields import MappedEnum, Str


@unique
class QuotaAction(Enum):
    """
    Quota update actions.
    """

    add = 'QuotaActionAdd'
    sub = 'QuotaActionSub'


class QuotaInfoSchema(Schema):
    """
    Cloud quota schema
    """

    cloudId = Str(required=True, attribute='cloud_ext_id')
    clustersQuota = Int(required=True, attribute='clusters_quota')
    clustersUsed = Int(required=True, attribute='clusters_used')
    cpuQuota = Float(required=True, attribute='cpu_quota')
    cpuUsed = Float(required=True, attribute='cpu_used')
    gpuQuota = Int(required=True, attribute='gpu_quota')
    gpuUsed = Int(required=True, attribute='gpu_used')
    memoryQuota = Int(required=True, attribute='memory_quota')
    memoryUsed = Int(required=True, attribute='memory_used')
    ssdSpaceQuota = Int(required=True, attribute='ssd_space_quota')
    hddSpaceQuota = Int(required=True, attribute='hdd_space_quota')
    ssdSpaceUsed = Int(required=True, attribute='ssd_space_used')
    hddSpaceUsed = Int(required=True, attribute='hdd_space_used')


class UpdateQuotaClustersRequestSchemaV1(Schema):
    """
    Update cloud clusters quota schema
    """

    clustersQuota = Int(required=True, attribute='clusters_quota')
    action = MappedEnum(
        required=True,
        mapping={
            'add': QuotaAction.add,
            'sub': QuotaAction.sub,
        },
    )


class UpdateQuotaSsdSpaceRequestSchemaV1(Schema):
    """
    Update cloud ssd space quota schema
    """

    ssdSpaceQuota = Int(required=True, attribute='ssd_space_quota')
    action = MappedEnum(
        required=True,
        mapping={
            'add': QuotaAction.add,
            'sub': QuotaAction.sub,
        },
    )


class UpdateQuotaHddSpaceRequestSchemaV1(Schema):
    """
    Update cloud hdd space quota schema
    """

    hddSpaceQuota = Int(required=True, attribute='hdd_space_quota')
    action = MappedEnum(
        required=True,
        mapping={
            'add': QuotaAction.add,
            'sub': QuotaAction.sub,
        },
    )


class UpdateQuotaResourcesRequestSchemaV1(Schema):
    """
    Update cloud resources quota schema
    """

    presetId = Str(required=True, attribute='preset_id')
    count = Int(required=True)
    action = MappedEnum(
        required=True,
        mapping={
            'add': QuotaAction.add,
            'sub': QuotaAction.sub,
        },
    )


class ClusterInfoRequestSchemaV1(Schema):
    """
    Support cluster info request schema
    """

    instanceId = Str(attribute='vtype_id')
    fqdn = Str()
    shardId = Str(attribute='shard_id')
    subclusterId = Str(attribute='subcid')
    clusterId = Str(attribute='cluster_id')
