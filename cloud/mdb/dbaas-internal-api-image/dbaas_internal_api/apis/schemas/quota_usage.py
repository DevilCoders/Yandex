# -*- coding: utf-8 -*-
"""
DBaaS Internal API quota related schemas
"""
from marshmallow import Schema
from marshmallow.fields import Float, Int

from .fields import Str


class QuotaUsageSchema(Schema):
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
