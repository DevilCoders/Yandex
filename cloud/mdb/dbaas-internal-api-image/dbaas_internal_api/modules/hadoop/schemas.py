# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster options schema
"""

from marshmallow.fields import Boolean, DateTime, Dict, Int, List, Nested, Str, Url
from marshmallow.validate import Length, Range, URL

from ...apis.schemas.cluster import (
    ClusterConfigSchemaV1,
    CreateClusterRequestSchemaV1,
    TypedClusterSchemaV1,
    UpdateClusterRequestSchemaV1,
)
from ...apis.schemas.common import ListResponseSchemaV1
from ...apis.schemas.console import ClustersConfigAvailableVersionSchemaV1, StringValueV1
from ...apis.schemas.fields import Environment, MappedEnum
from ...apis.schemas.objects import ResourcePresetSchemaV1, ResourcesSchemaV1, ResourcesUpdateSchemaV1
from ...utils.register import (
    DbaasOperation,
    Resource,
    register_config_schema,
    register_request_schema,
    register_response_schema,
)
from ...utils.traits import ServiceAccountId, LogGroupId, VersionPrefix
from ...utils.validation import Schema
from .constants import (
    COMPUTE_SUBCLUSTER_TYPE,
    DATA_SUBCLUSTER_TYPE,
    MASTER_SUBCLUSTER_TYPE,
    MY_CLUSTER_TYPE,
)
from .traits import ClusterService, HadoopClusterTraits, HostHealth, JobStatus
from .utils import generate_cluster_name


class HadoopHostType(MappedEnum):
    """
    'type' field.
    """

    def __init__(self, **kwargs):
        super().__init__(
            {
                'MASTERNODE': MASTER_SUBCLUSTER_TYPE,
                'DATANODE': DATA_SUBCLUSTER_TYPE,
                'COMPUTENODE': COMPUTE_SUBCLUSTER_TYPE,
            },
            **kwargs,
            skip_description=True
        )


class HadoopService(MappedEnum):
    """
    Hadoop service enum field.
    """

    def __init__(self, **kwargs):
        super().__init__(
            {
                'HDFS': ClusterService.hdfs,
                'YARN': ClusterService.yarn,
                'MAPREDUCE': ClusterService.mapreduce,
                'HIVE': ClusterService.hive,
                'TEZ': ClusterService.tez,
                'ZOOKEEPER': ClusterService.zookeeper,
                'HBASE': ClusterService.hbase,
                'SQOOP': ClusterService.sqoop,
                'FLUME': ClusterService.flume,
                'SPARK': ClusterService.spark,
                'ZEPPELIN': ClusterService.zeppelin,
                'OOZIE': ClusterService.oozie,
                'LIVY': ClusterService.livy,
            },
            **kwargs,
            skip_description=True
        )


class HadoopServiceDependency(Schema):
    """
    Hadoop schema for description that service depends on dependencies
    """

    service = HadoopService(attribute='service')
    dependencies = List(HadoopService, attribute='deps')


class HadoopServiceInfo(Schema):
    """
    Hadoop schema for description service info
    """

    service = HadoopService(attribute='service')
    dependencies = List(HadoopService, attribute='deps')
    version = Str()
    default = Boolean(missing=False)


class InitializationAction(Schema):
    """
    Cluster initialization action attributes
    """

    uri = Str(validate=URL(schemes={"http", "https", "s3a", "hdfs"}, require_tld=False), attribute='uri', required=True)
    args = List(Str(), missing=[], attribute='args')
    timeout = Int(missing=0, attribute='timeout')


@register_config_schema(MY_CLUSTER_TYPE, '1.0')
class HadoopCreateConfig(Schema):
    """
    Hadoop specific config options schema
    """

    services = List(HadoopService())
    properties = Dict(keys=Str(), values=Str())
    sshPublicKeys = List(Str, attribute='ssh_public_keys')
    initializationActions = List(Nested(InitializationAction), attribute='initialization_actions')


class AutoscalingSubclusterConfig(Schema):
    """
    Schema for autoscaling subcluster config
    """

    maxHostsCount = Int(validate=Range(min=0), missing=1, attribute='max_hosts_count')
    measurementDuration = Int(validate=Range(min=60, max=600), missing=60, attribute='measurement_duration')
    warmupDuration = Int(validate=Range(min=0, max=600), missing=60, attribute='warmup_duration')
    stabilizationDuration = Int(validate=Range(min=60, max=1800), missing=120, attribute='stabilization_duration')
    preemptible = Boolean(attribute='preemptible', missing=False)
    cpuUtilizationTarget = Int(validate=Range(min=0, max=100), missing=0, attribute='cpu_utilization_target')
    decommissionTimeout = Int(attribute='decommission_timeout', missing=60, validate=Range(min=0, max=86400))


class AutoscalingSubclusterModifyConfig(Schema):
    """
    Schema for autoscaling subcluster config
    """

    maxHostsCount = Int(validate=Range(min=1), attribute='max_hosts_count')
    measurementDuration = Int(validate=Range(min=60, max=600), attribute='measurement_duration')
    warmupDuration = Int(validate=Range(min=0, max=600), attribute='warmup_duration')
    stabilizationDuration = Int(validate=Range(min=60, max=1800), attribute='stabilization_duration')
    preemptible = Boolean(attribute='preemptible')
    cpuUtilizationTarget = Int(validate=Range(min=0, max=100), attribute='cpu_utilization_target')
    decommissionTimeout = Int(attribute='decommission_timeout', validate=Range(min=0, max=86400))


@register_request_schema(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.CREATE)
class SubclusterCreateConfig(Schema):
    """
    Hadoop subcluster config schema.
    """

    role = HadoopHostType(required=True)
    name = Str(validate=HadoopClusterTraits.cluster_name.validate, missing='')
    resources = Nested(ResourcesUpdateSchemaV1, missing={})
    hostsCount = Int(validate=Range(min=0), missing=1, attribute='hosts_count')
    subnetId = Str(attribute='subnet_id')
    rootDiskQuota = Int(attribute='root_disk_quota')
    assignPublicIp = Boolean(missing=False, attribute='assign_public_ip')
    autoscalingConfig = Nested(AutoscalingSubclusterConfig(), attribute='autoscaling_config')


class SubclusterBillingConfig(SubclusterCreateConfig):
    """
    Hadoop subcluster config schema for billing
    """

    name = Str(validate=HadoopClusterTraits.cluster_name.validate)
    subnetId = Str(attribute='subnet_id')


@register_response_schema(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.INFO)
class SubclusterInfoConfig(Schema):
    """
    Hadoop subcluster config info schema.
    """

    id = Str(attribute='subcid')
    clusterId = Str(attribute='cluster_id', required=True)
    createdAt = DateTime(attribute='created_at', required=True)
    name = Str(validate=HadoopClusterTraits.cluster_name.validate, required=True)
    role = HadoopHostType(required=True)
    resources = Nested(ResourcesSchemaV1, required=True)
    hostsCount = Int(validate=Range(min=1), missing=1, attribute='hosts_count')
    autoscalingConfig = Nested(AutoscalingSubclusterConfig(), attribute='autoscaling_config')
    instanceGroupId = Str(attribute='instance_group_id')
    subnetId = Str(required=True, attribute='subnet_id')
    rootDiskQuota = Int(attribute='root_disk_quota')
    assignPublicIp = Boolean(missing=False, attribute='assign_public_ip')


@register_request_schema(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.MODIFY)
class SubclusterModifyConfigV1(Schema):
    """
    Hadoop subcluster modify schema.
    """

    name = Str(validate=HadoopClusterTraits.cluster_name.validate)
    resources = Nested(ResourcesUpdateSchemaV1)
    hostsCount = Int(attribute='hosts_count', validate=Range(min=1))
    rootDiskQuota = Int(attribute='root_disk_quota')
    decommissionTimeout = Int(attribute='decommission_timeout', default=0, validate=Range(min=0, max=86400))
    autoscalingConfig = Nested(AutoscalingSubclusterModifyConfig(), attribute='autoscaling_config')


class SubclusterModifyConfig(Schema):
    """
    Hadoop subcluster modify schema.
    """

    id = Str(attribute='subcid', required=True)
    name = Str(validate=HadoopClusterTraits.cluster_name.validate)
    resources = Nested(ResourcesUpdateSchemaV1)
    hostsCount = Int(attribute='hosts_count')
    rootDiskQuota = Int(attribute='root_disk_quota')
    autoscalingConfig = Nested(AutoscalingSubclusterModifyConfig(), attribute='autoscaling_config')


class HadoopConfigSchemaSpecV1(Schema):
    """
    Hadoop cluster config schema
    """

    versionId = Str(attribute='version_id', validate=VersionPrefix().validate)
    hadoop = Nested(HadoopCreateConfig)
    subclustersSpec = List(
        Nested(SubclusterCreateConfig), attribute='subclusters', required=True, validate=Length(min=1)
    )
    imageId = Str(attribute='image_id')


class HadoopBillingConfigSchemaV1(HadoopConfigSchemaSpecV1):
    """
    Hadoop cluster config schema for Billing
    """

    subclustersSpec = List(Nested(SubclusterBillingConfig), attribute='subclusters', required=True)


class HadoopModifyConfigSchemaV1(Schema):
    """
    Hadoop cluster config schema
    """

    versionId = Str(attribute='version_id')
    hadoop = Nested(HadoopCreateConfig)
    subclustersSpec = List(Nested(SubclusterModifyConfig), attribute='subclusters')


class HadoopClusterSchemaV1(ClusterConfigSchemaV1):
    """
    Hadoop cluster config schema.
    """

    versionId = Str(attribute='version_id')
    hadoop = Nested(HadoopCreateConfig, attribute='hadoop', required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
class HadoopClusterInfoSchemaV1(TypedClusterSchemaV1):
    """
    Hadoop cluster schema.
    """

    config = Nested(HadoopClusterSchemaV1, required=True)
    serviceAccountId = Str(validate=ServiceAccountId().validate, attribute='service_account_id')
    logGroupId = Str(validate=LogGroupId().validate, attribute='log_group_id')
    bucket = Str(attribute='bucket')
    labels = Dict()  # type: ignore
    uiProxy = Boolean(attribute='ui_proxy')
    deletionProtection = Boolean(attribute='deletion_protection', default=False)


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.LIST)
class HadoopListClustersResponseSchemaV1(ListResponseSchemaV1):
    """
    Hadoop cluster list schema.
    """

    clusters = Nested(HadoopClusterInfoSchemaV1, many=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.LIST)
class HadoopListSubclusterResponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for list Hadoop subclusters.
    """

    subclusters = List(Nested(SubclusterInfoConfig), attribute='subclusters')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
class HadoopCreateClusterRequestSchemaV1(CreateClusterRequestSchemaV1):
    """
    Schema for create Hadoop cluster request.
    """

    name = Str(validate=HadoopClusterTraits.cluster_name.validate, missing=generate_cluster_name())
    configSpec = Nested(HadoopConfigSchemaSpecV1, attribute='config_spec', required=True)
    zoneId = Str(required=True, attribute='zone_id')
    bucket = Str(attribute='bucket')
    environment = Environment(required=False, missing='PRODUCTION')
    labels = Dict(missing={})  # type: ignore
    serviceAccountId = Str(required=True, attribute='service_account_id', validate=ServiceAccountId().validate)
    uiProxy = Boolean(missing=False, attribute='ui_proxy')
    hostGroupIds = List(Str(), attribute='host_group_ids')
    logGroupId = Str(missing='', attribute='log_group_id', validate=LogGroupId().validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
class HadoopBillingClusterRequestSchemaV1(HadoopCreateClusterRequestSchemaV1):
    """
    Schema for create Hadoop cluster request.
    """

    name = Str()
    configSpec = Nested(HadoopBillingConfigSchemaV1, attribute='config_spec', required=True)
    serviceAccountId = Str(attribute='service_account_id', validate=ServiceAccountId().validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
class HadoopDeleteClusterRequestSchemaV1(Schema):
    """
    Schema for stop Hadoop cluster request.
    """

    decommissionTimeout = Int(attribute='decommission_timeout', default=0, validate=Range(min=0, max=86400))


@register_request_schema(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.DELETE)
class HadoopDeleteSubclusterRequestSchemaV1(Schema):
    """
    Schema for stop Hadoop cluster request.
    """

    decommissionTimeout = Int(attribute='decommission_timeout', default=0, validate=Range(min=0, max=86400))


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
class HadoopStopClusterRequestSchemaV1(Schema):
    """
    Schema for stop Hadoop cluster request.
    """

    decommissionTimeout = Int(attribute='decommission_timeout', default=0, validate=Range(min=0, max=86400))


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
class HadoopUpdateClusterRequestSchemaV1(UpdateClusterRequestSchemaV1):
    """
    Schema for update Hadoop cluster request.
    """

    name = Str(validate=HadoopClusterTraits.cluster_name.validate)
    serviceAccountId = Str(attribute='service_account_id', validate=ServiceAccountId().validate)
    configSpec = Nested(HadoopModifyConfigSchemaV1, attribute='config_spec')
    bucket = Str(attribute='bucket')
    labels = Dict()  # type: ignore
    decommissionTimeout = Int(attribute='decommission_timeout', default=0, validate=Range(min=0, max=86400))
    uiProxy = Boolean(attribute='ui_proxy')
    logGroupId = Str(attribute='log_group_id', validate=LogGroupId().validate)


class HadoopHostSchemaV1(Schema):
    """
    Hadoop host schema.
    """

    name = Str(attribute='name')
    subclusterId = Str(attribute='subcluster_id')
    health = MappedEnum(
        {
            'UNKNOWN': HostHealth.unknown,
            'ALIVE': HostHealth.alive,
            'DEAD': HostHealth.dead,
            'DEGRADED': HostHealth.degraded,
        },
        attribute='health',
    )
    computeInstanceId = Str(attribute='compute_instance_id')
    instanceGroupId = Str(attribute='instance_group_id')
    role = MappedEnum(
        {
            'MASTERNODE': MASTER_SUBCLUSTER_TYPE,
            'DATANODE': DATA_SUBCLUSTER_TYPE,
            'COMPUTENODE': COMPUTE_SUBCLUSTER_TYPE,
        },
        attribute='role',
    )


@register_response_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
class HadoopListHostsResponseSchemaV1(ListResponseSchemaV1):
    """
    Hadoop host list schema.
    """

    hosts = Nested(HadoopHostSchemaV1, many=True, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.INFO)
class HadoopResourcePresetSchemaV1(ResourcePresetSchemaV1):
    """
    Hadoop resource preset schema.
    """

    hostTypes = List(HadoopHostType(), attribute='roles')


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.LIST)
class HadoopListResourcePresetsSchemaV1(ListResponseSchemaV1):
    """
    Hadoop resource preset list schema.
    """

    resourcePresets = Nested(HadoopResourcePresetSchemaV1, many=True, attribute='resource_presets', required=True)


class HadoopConsoleClustersConfigDiskSizeRangeSchemaV1(Schema):
    """
    Hadoop disk size range schema.
    """

    min = Int()
    max = Int()


class HadoopConsoleClustersConfigDiskTypesSchemaV1(Schema):
    """
    Hadoop available disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    diskSizeRange = Nested(HadoopConsoleClustersConfigDiskSizeRangeSchemaV1(), attribute='disk_size_range')
    minHosts = Int(attribute='min_hosts')
    maxHosts = Int(attribute='max_hosts')


class HadoopConsoleClustersConfigZoneSchemaV1(Schema):
    """
    Hadoop available zone schema.
    """

    zoneId = Str(attribute='zone_id')
    diskTypes = Nested(HadoopConsoleClustersConfigDiskTypesSchemaV1, many=True, attribute='disk_types')


class HadoopConsoleClustersConfigResourcePresetSchemaV1(Schema):
    """
    Hadoop available resource preset schema.
    """

    presetId = Str(attribute='preset_id')
    cpuLimit = Int(attribute='cpu_limit')
    cpuFraction = Int(attribute='cpu_fraction')
    gpuLimit = Int(attribute='gpu_limit')
    memoryLimit = Int(attribute='memory_limit')
    type = Str(attribute='type')
    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    decommissioning = Boolean(attribute='decommissioning')
    zones = Nested(HadoopConsoleClustersConfigZoneSchemaV1, many=True)


class HadoopConsoleClustersConfigDefaultResourcesSchemaV1(Schema):
    """
    Hadoop default resources schema.
    """

    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    resourcePresetId = Str(attribute='resource_preset_id')
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size')


class HadoopConsoleClustersConfigHostTypeSchemaV1(Schema):
    """
    Hadoop available host type schema.
    """

    type = HadoopHostType()
    resourcePresets = Nested(HadoopConsoleClustersConfigResourcePresetSchemaV1, many=True, attribute='resource_presets')
    defaultResources = Nested(HadoopConsoleClustersConfigDefaultResourcesSchemaV1, attribute='default_resources')


class HadoopConsoleClustersConfigHostCountLimitsSchemaV1(Schema):
    """
    Hadoop host count limits schema.
    """

    minHostCount = Int(attribute='min_host_count')
    maxHostCount = Int(attribute='max_host_count')


class HadoopConsoleClustersConfigDecommissionTimeoutSchemaV1(Schema):
    """
    Hadoop decommission timeout schema.
    """

    min = Int(attribute='min')
    max = Int(attribute='max')


class HadoopImage(Schema):
    """
    Hadoop image schema.
    """

    version = Str(attribute='name')
    services = Nested(HadoopServiceInfo(), many=True)
    availableServices = List(HadoopService(), attribute='available_services')
    serviceDependencies = Nested(HadoopServiceDependency(), attribute='service_deps', many=True)
    minSize = Int(attribute='min_size')


@register_response_schema(MY_CLUSTER_TYPE, Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
class HadoopConsoleClustersConfigSchemaV1(Schema):
    """
    Hadoop console clusters config schema.
    """

    clusterName = Nested(StringValueV1(), attribute='cluster_name')
    hostCountLimits = Nested(HadoopConsoleClustersConfigHostCountLimitsSchemaV1(), attribute='host_count_limits')
    hostTypes = Nested(HadoopConsoleClustersConfigHostTypeSchemaV1, many=True, attribute='host_types')
    versions = List(Str())
    images = Nested(HadoopImage(), many=True, attribute='images')
    availableVersions = Nested(ClustersConfigAvailableVersionSchemaV1, many=True, attribute='available_versions')
    defaultVersion = Str(attribute='default_version')
    decommissionTimeout = Nested(
        HadoopConsoleClustersConfigDecommissionTimeoutSchemaV1,
        attribute='decommission_timeout',
    )
    defaultResources = Nested(
        ResourcesSchemaV1,
        attribute='default_resources',
    )


class MapReduceJobSpec(Schema):
    """
    Hadoop MapReduce job specification
    """

    args = List(Str())
    jarFileUris = List(Str(), attribute='jar_file_uris')
    fileUris = List(Str(), attribute='file_uris')
    archiveUris = List(Str(), attribute='archive_uris')
    properties = Dict(keys=Str(), values=Str())
    mainJarFileUri = Str(attribute='main_jar_file_uri')
    mainClass = Str(attribute='main_class')


class SparkJobSpec(Schema):
    """
    Hadoop Spark job specification
    """

    args = List(Str())
    jarFileUris = List(Str(), attribute='jar_file_uris')
    fileUris = List(Str(), attribute='file_uris')
    archiveUris = List(Str(), attribute='archive_uris')
    properties = Dict(keys=Str(), values=Str())
    mainJarFileUri = Str(attribute='main_jar_file_uri')
    mainClass = Str(attribute='main_class')
    packages = List(Str(), attribute='packages')
    repositories = List(Str(), attribute='repositories')
    excludePackages = List(Str(), attribute='exclude_packages')


class PySparkJobSpec(Schema):
    """
    Hadoop PySpark job specification
    """

    args = List(Str())
    jarFileUris = List(Str(), attribute='jar_file_uris')
    fileUris = List(Str(), attribute='file_uris')
    archiveUris = List(Str(), attribute='archive_uris')
    properties = Dict(keys=Str(), values=Str())
    mainPythonFileUri = Str(attribute='main_python_file_uri')
    pythonFileUris = List(Str(), attribute='python_file_uris')
    packages = List(Str(), attribute='packages')
    repositories = List(Str(), attribute='repositories')
    excludePackages = List(Str(), attribute='exclude_packages')


class HiveQueryListSpec(Schema):
    """
    Hadoop Hive query list specification
    """

    queries = List(Str())


class HiveJobSpec(Schema):
    """
    Hadoop Hive job specification
    """

    properties = Dict(keys=Str(), values=Str())
    continueOnFailure = Boolean(missing=False, attribute='continue_on_failure')
    scriptVariables = Dict(keys=Str(), values=Str(), attribute='script_variables')
    jarFileUris = List(Str(), attribute='jar_file_uris')
    queryFileUri = Str(attribute='query_file_uri')
    queryList = Nested(HiveQueryListSpec(), attribute='query_list')


class HadoopJobSpec(Schema):
    """
    Hadoop job specification
    """

    mapreduceJob = Nested(MapReduceJobSpec(), attribute='mapreduce_job')
    sparkJob = Nested(SparkJobSpec(), attribute='spark_job')
    pysparkJob = Nested(PySparkJobSpec(), attribute='pyspark_job')
    hiveJob = Nested(HiveJobSpec(), attribute='hive_job')


class ApplicationAttempt(Schema):
    """
    YARN Application attempt attributes
    """

    id = Str(attribute='id')
    amContainerId = Str(attribute='am_container_id')


class ApplicationInfo(Schema):
    """
    YARN Application attributes
    """

    id = Str(attribute='id')
    applicationAttempts = List(Nested(ApplicationAttempt), attribute='application_attempts')


@register_request_schema(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.CREATE)
class HadoopJobCreateSchema(Schema):
    """
    Hadoop job create schema.
    """

    name = Str(missing=None)
    mapreduceJob = Nested(MapReduceJobSpec(), attribute='mapreduce_job')
    sparkJob = Nested(SparkJobSpec(), attribute='spark_job')
    pysparkJob = Nested(PySparkJobSpec(), attribute='pyspark_job')
    hiveJob = Nested(HiveJobSpec(), attribute='hive_job')


@register_response_schema(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.INFO)
class HadoopJobSchema(Schema):
    """
    Hadoop job schema.
    """

    id = Str(required=True, attribute='job_id')
    clusterId = Str(attribute='cid', required=True)
    applicationInfo = Nested(ApplicationInfo(), attribute='application_info')

    mapreduceJob = Nested(MapReduceJobSpec(), attribute='mapreduce_job')
    sparkJob = Nested(SparkJobSpec(), attribute='spark_job')
    pysparkJob = Nested(PySparkJobSpec(), attribute='pyspark_job')
    hiveJob = Nested(HiveJobSpec(), attribute='hive_job')

    name = Str()
    status = MappedEnum(
        {
            'PROVISIONING': JobStatus.PROVISIONING,
            'PENDING': JobStatus.PENDING,
            'RUNNING': JobStatus.RUNNING,
            'ERROR': JobStatus.ERROR,
            'DONE': JobStatus.DONE,
            'CANCELLING': JobStatus.CANCELLING,
            'CANCELLED': JobStatus.CANCELLED,
        }
    )
    createdAt = DateTime(attribute='create_ts', required=True)
    startedAt = DateTime(attribute='start_ts')
    finishedAt = DateTime(attribute='end_ts')
    createdBy = Str(attribute='created_by')


@register_request_schema(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.CANCEL)
class HadoopJobCancelRequestSchema(Schema):
    """
    Hadoop job cancel schema.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.LIST)
class HadoopListJobsResponseSchema(ListResponseSchemaV1):
    """
    Hadoop jobs list schema.
    """

    jobs = Nested(HadoopJobSchema, many=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.HADOOP_JOB, DbaasOperation.GET_HADOOP_JOB_LOG)
class HadoopGetJobLogResponseSchema(Schema):
    """
    Hadoop job log response schema
    """

    content = Str()
    nextPageToken = Str(attribute='next_page_token')


class HadoopUILinkSchemaV1(Schema):
    """
    Hadoop UILink schema.
    """

    name = Str()
    url = Url()


@register_response_schema(MY_CLUSTER_TYPE, Resource.HADOOP_UI_LINK, DbaasOperation.LIST)
class HadoopListUILinksResponseSchemaV1(Schema):
    """
    List UILinks response schema.
    """

    links = Nested(HadoopUILinkSchemaV1, many=True, required=True)
