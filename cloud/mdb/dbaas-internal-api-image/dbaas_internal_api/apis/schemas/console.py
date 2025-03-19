"""
Defines common schemas like listing or base objects.
"""

from marshmallow import Schema
from marshmallow.fields import Bool, DateTime, Dict, Int, List, Nested

from .fields import CloudId, Environment, FolderId, Str


class StringValueV1(Schema):
    """
    Schema for string value requirements.
    """

    regexp = Str()
    min = Int()
    max = Int()
    blacklist = List(Str())


class BillingMetric(Schema):
    """
    Schema for billing metrics
    """

    folder_id = Str()
    schema = Str()
    tags = Dict()


class ListClusterTypesByFolderRequestSchemaV1(Schema):
    """
    Schema for list cluster types by folder request.
    """

    folderId = FolderId(required=True)


class ListClusterTypesByCloudRequestSchemaV1(Schema):
    """
    Schema for list cluster types by cloud request.
    """

    cloudId = CloudId(required=True)


class EstimateCreateResponseSchemaV1(Schema):
    """
    Schema for create cluster billing estimate response.
    """

    metrics = Nested(BillingMetric, many=True, required=True)


class ClusterTypeResponseSchemaV1(Schema):
    """
    Schema for description of cluster type.
    """

    type = Str(required=True)
    versions = List(Str)


class ClustersConfigAvailableVersionSchemaV1(Schema):
    """
    Database available versions schema
    """

    id = Str()
    name = Str()
    deprecated = Bool()
    updatableTo = List(Str, attribute='updatable_to', default=[])
    versionedConfigKey = Str(attribute='versioned_config_key')


class ListClusterTypesResponseSchemaV1(Schema):
    """
    Schema for list cluster types response.
    """

    clusterTypes = Nested(ClusterTypeResponseSchemaV1, many=True, attribute='cluster_types')


class ListClustersConfigRequestSchemaV1(Schema):
    """
    Schema for cluster config request.
    """

    folderId = FolderId(required=True)


class ClustersStatsRequestSchemaV1(Schema):
    """
    Schema for cluster stats request.
    """

    folderId = FolderId(required=True)


class ClustersStatsResponseSchemaV1(Schema):
    """
    Schema for cluster stats response.
    """

    clustersCount = Int(attribute='clusters_count')


class EstimateHostCreateRequestSchemaV1(Schema):
    """
    Schema for host creation estimation request.
    """

    folderId = FolderId(required=True)


class RestoreHintResource(Schema):
    """
    Resource in restore hint
    """

    diskSize = Int(attribute='disk_size')
    resourcePresetId = Str(attribute='resource_preset_id')


class RestoreHintResponseSchemaV1(Schema):
    """
    Schema for restore hint
    """

    resources = Nested(RestoreHintResource, attribute='resources')
    networkId = Str(attribute='network_id')
    version = Str(attribute='version')
    environment = Environment(attribute='env')


class RestoreHintWithTimeResponseSchemaV1(RestoreHintResponseSchemaV1):
    """
    Schema for restore hint with time
    """

    time = DateTime(attribute='time')
