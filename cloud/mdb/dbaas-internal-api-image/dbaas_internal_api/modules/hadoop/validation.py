# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop input validation
"""

import ipaddress
import re
from typing import cast, List, Optional, Set  # noqa
from urllib.parse import urlparse

from flask import current_app, g
import semver
from sshpubkeys import InvalidKeyError, SSHKey, TooLongKeyError, TooShortKeyError

from ...utils.iam_token import get_iam_client
from ...core.exceptions import DbaasClientError, ParseConfigError, PreconditionFailedError
from ...core.auth import AuthError
from ...utils import validation
from ...utils.dataproc_manager import dataproc_manager_client
from ...utils.feature_flags import has_feature_flag
from ...utils.logging_service import get_logging_service, LogGroupNotFoundError, LogGroupPermissionDeniedError
from ...utils.resource_manager import service_account_roles
from ...utils.types import ClusterHealth
from ...utils.network import (
    get_provider as get_network_provider,
    SecurityGroup,
    SecurityGroupRule,
    SecurityGroupRuleDirection,
)
from ...utils.validation import validate_service_account as validate_can_use_service_account
from . import constants
from .pillar import HadoopPillar
from .traits import ClusterService, DATAPROC_JOB_SERVICES
from .types import HadoopSubclusterSpec
from .utils import fetch_subnet


def validate_cluster(cluster_config: dict, zone_id: str, image_config: dict, version: str) -> None:
    """
    Validate cluster config
    """

    hadoop_cfg = cluster_config.get('hadoop', {})
    validate_public_ssh_keys(hadoop_cfg.get('ssh_public_keys', []))

    services = hadoop_cfg.get('services', [])
    validate_services(services, image_config, version)

    all_subclusters = cluster_config.get('subclusters', [])
    subclusters_main = [sub for sub in all_subclusters if sub.get('role') == constants.MASTER_SUBCLUSTER_TYPE]
    if not subclusters_main:
        raise DbaasClientError('You must specify exactly one main subcluster')
    if len(subclusters_main) > 1:
        raise DbaasClientError('Only one main subcluster allowed')

    names = set()  # type: Set[str]
    for subcluster in all_subclusters:
        if subcluster['name'] in names:
            raise DbaasClientError('Subclusters names must be unique')
        if subcluster['name']:
            names.add(subcluster['name'])
        try:
            subcluster_spec = HadoopSubclusterSpec(subcluster)
            validate_subcluster(subcluster_spec, zone_id, image_config, version)
        except ValueError as err:
            raise ParseConfigError(err)


def validate_labels(labels):
    """
    Validate labels value
    """
    full_exception_message = ''
    re_keys_pattern = re.compile(r'^[a-z][-_./\\@0-9a-z]*$')
    re_values_pattern = re.compile(r'[-_./\\@0-9a-z]+$')
    for internal_labels_dict in labels.values():
        pairs_count = len(internal_labels_dict)
        if pairs_count > 64:
            full_exception_message += (
                'No more than 64 label\'s pair per resource was allowed.\n'
                f'But you have `{pairs_count}` pairs of labels.\n'
            )

        for key, value in internal_labels_dict.items():
            key_len = len(key)
            value_len = len(value)
            if key_len < 1 or key_len > 63:
                full_exception_message += (
                    'The string length in characters for each key must be `1-63`.\n'
                    f'But key `{key}` has length `{key_len}`.\n'
                )
            if value_len < 1 or value_len > 63:
                full_exception_message += (
                    'The string length in characters for each value must be `1-63`.\n'
                    f'But value `{value}` has length `{value_len}`.\n'
                )
            if not re_keys_pattern.match(key):
                full_exception_message += (
                    'Each key must match the regular expression `[a-z][-_./\\@0-9a-z]`.\n'
                    f'But key `{key}` does not match pattern.\n'
                )
            if not re_values_pattern.match(value):
                full_exception_message += (
                    'Each value must match the regular expression `[-_./\\@0-9a-z]`.\n'
                    f'But value `{value}` does not match pattern.\n'
                )

    if full_exception_message:
        raise DbaasClientError(full_exception_message)


def validate_subcluster(spec: HadoopSubclusterSpec, zone_id: str, image_config: dict, version: str) -> None:
    """
    Validate subcluster config
    """
    if spec.get_role() == constants.MASTER_SUBCLUSTER_TYPE:
        validate_main_subcluster(spec)
    resources = spec.get_resources()

    hosts_count = spec.get_hosts_count()
    validation.validate_hosts_count(
        cluster_type=constants.MY_CLUSTER_TYPE,
        role=spec.get_role(),
        resource_preset_id=resources.resource_preset_id,
        disk_type_id=resources.disk_type_id,
        hosts_count=hosts_count,
    )
    if hosts_count == 0 and not spec.is_autoscaling():
        raise PreconditionFailedError(
            'Setting `hosts count` to 0 is allowed only for autoscaling subclusters '
            '(when `max hosts` property is set and is bigger than `hosts count` property)'
        )
    validation.validate_host_create_resources(
        cluster_type=constants.MY_CLUSTER_TYPE,
        role=spec.get_role(),
        resource_preset_id=resources.resource_preset_id,
        geo=zone_id,
        disk_type_id=resources.disk_type_id,
        disk_size=resources.disk_size,
    )

    validate_subcluster_disk_size(spec, image_config, version)


def validate_main_subcluster(spec: HadoopSubclusterSpec) -> None:
    """
    Validate main subcluster config
    """
    if spec.get_hosts_count() != 1:
        raise DbaasClientError("Hosts count for main subcluster must be exactly one")


def validate_subcluster_disk_size(spec: HadoopSubclusterSpec, image_config: dict, version_prefix: str) -> None:
    """
    Validate root size of disk
    """
    resources = spec.get_resources()
    image_min_size = image_config['imageMinSize']
    if resources.disk_size < image_config['imageMinSize']:
        raise DbaasClientError(
            f'Disk size for image `{version_prefix}` should be greater or equal than {image_min_size} bytes.'
        )


def has_lightweight_spark_support(version):
    """
    Validate cluster image support lightweight configuration
    """
    return semver.compare(str(version), constants.DATAPROC_LIGHTWEIGHT_SPARK_VERSION) >= 0


def has_lightweight_hive_support(version):
    """
    Validate cluster image support lightweight configuration
    """
    return semver.compare(str(version), constants.DATAPROC_LIGHTWEIGHT_HIVE_VERSION) >= 0


def validate_services(services: dict, image: dict, version: str) -> None:
    """
    Validate hadoop cluster services
    """
    services = ClusterService.to_enums([s.name for s in services])
    for service in services:
        service_name = ClusterService.to_string(service)
        if service not in ClusterService.to_enums(image['services']):
            raise DbaasClientError(f"Service '{service_name}' is not available in image '{image['name']}'")
        service_dependencies = ClusterService.to_enums(image['services'][service_name].get('deps', []))

        # Add HDFS as dependecy for images earlier than DATAPROC_LIGHTWEIGHT_SPARK_VERSION
        if service in [ClusterService.spark, ClusterService.yarn] and not has_lightweight_spark_support(version):
            service_dependencies.append(ClusterService.hdfs)

        # Add dependecies for images earlier than DATAPROC_LIGHTWEIGHT_HIVE_VERSION
        if service == ClusterService.hive and not has_lightweight_hive_support(version):
            service_dependencies.append(ClusterService.yarn)

        for dep in service_dependencies:
            if dep not in services:
                dep_name = ClusterService.to_string(dep)
                if (
                    service in [ClusterService.spark, ClusterService.yarn]
                    and dep == ClusterService.hdfs
                    and not has_lightweight_spark_support(version)
                ):
                    raise DbaasClientError(
                        f"To use service `{service_name}` without `{dep_name}` "
                        f"dataproc image version must be at least "
                        f"`{constants.DATAPROC_LIGHTWEIGHT_SPARK_VERSION}`"
                    )
                if (
                    service == ClusterService.hive
                    and dep == ClusterService.yarn
                    and not has_lightweight_hive_support(version)
                ):
                    raise DbaasClientError(
                        f"To use service `{service_name}` without `{dep_name}` "
                        f"dataproc image version must be at least "
                        f"`{constants.DATAPROC_LIGHTWEIGHT_HIVE_VERSION}`"
                    )
                raise DbaasClientError(f"Service '{service_name}' needs '{dep_name}' to work")


def validate_pillar(pillar: HadoopPillar) -> None:
    """
    Validate hadoop pillar
    """
    validate_subclusters_and_services(pillar)
    validate_properties_values(pillar)
    validate_properties_by_services(pillar)
    validate_labels(pillar.labels)
    validate_s3_bucket(pillar)


def validate_subclusters_and_services(pillar) -> None:
    """
    Validate set of required subclusters with desired services
    """
    services = set(pillar.services)
    subclusters = pillar.subclusters
    has_data_subclusters = bool([sub for sub in subclusters if sub.get('role') == constants.DATA_SUBCLUSTER_TYPE])
    data_services = [ClusterService.hdfs, ClusterService.oozie]
    given_data_services = sorted(services.intersection(set(data_services)), key=data_services.index)
    has_data_services = bool(given_data_services)

    # Forbid these services without datanodes
    if has_data_services and not has_data_subclusters:
        raise DbaasClientError(
            "Creating cluster without datanodes and with services {services} is forbidden.".format(
                services=', '.join(ClusterService.to_strings(given_data_services))
            )
        )
    # Forbid using datanodes if hdfs or hive not in services
    if has_data_subclusters and not has_data_services:
        raise DbaasClientError(
            "Creating cluster with datanodes without any of services {service} is forbidden.".format(
                service=', '.join(ClusterService.to_strings(data_services))
            )
        )

    # Forbid creacting subclusters without services
    for sub in subclusters:
        if not sub['services']:
            raise DbaasClientError("There is no services for subcluster: `{}`".format(sub['name']))


def validate_properties_values(pillar) -> None:
    """
    Validate set of properties with subclusters
    """
    # Skip validation for lightweight clusters
    if pillar.is_lightweight():
        return

    subclusters = pillar.subclusters
    datanodes_count = sum(
        sub['hosts_count'] for sub in subclusters if sub.get('role') == constants.DATA_SUBCLUSTER_TYPE
    )

    dfs_replication_value = pillar.properties.get('hdfs:dfs.replication', '1')
    try:
        dfs_replication = int(dfs_replication_value)
    except ValueError:
        raise DbaasClientError(
            f'Property `hdfs:dfs.replication` has value `{dfs_replication_value}`, '
            f'but should be integer in range [1; datanodes count `{datanodes_count}`]'
        )

    if dfs_replication > datanodes_count or dfs_replication < 1:
        raise DbaasClientError(
            f'Property `hdfs:dfs.replication` has value `{dfs_replication_value}`, '
            f'but should be integer in range [1; datanodes count `{datanodes_count}`]'
        )


def validate_properties_by_services(pillar) -> None:
    """
    Validate properties by services
    """
    invalid_property_names_list = []
    missing_services_list = []
    installed_services = set(ClusterService.to_strings(pillar.services))
    installed_services.add('core')
    installed_services.add('dataproc')
    installed_services.add('conda')
    installed_services.add('pip')
    installed_services.add('monitoring')
    # add additional prefixes for hive service
    # for configs hivemetastore-site.xml and hiveserver2-site.xml
    if ClusterService.hive in pillar.services:
        installed_services.add('hivemetastore')
        installed_services.add('hiveserver2')
    if ClusterService.yarn in pillar.services:
        installed_services.add('capacity-scheduler')
        installed_services.add('resource-types')
        installed_services.add('node-resources')
    for property_name in pillar.properties.keys():
        service_name, service_property = property_name.split(':')
        if service_name not in installed_services:
            invalid_property_names_list.append(service_property)
            missing_services_list.append(service_name)
    invalid_property_names = ', '.join(invalid_property_names_list)
    missing_services = ', '.join(missing_services_list)
    if invalid_property_names:
        raise DbaasClientError(
            f'Properties `{invalid_property_names}` for services `{missing_services}` has been set, '
            f'but the next services is missing: `{missing_services}`. '
            f'Please, create cluster with this services.'
        )


def validate_public_ssh_keys(public_keys: List[str]) -> None:
    """
    Validate ssh public keys before creating cluster
    """
    if not public_keys:
        raise DbaasClientError('Ssh public keys are empty')
    for public_key in public_keys:
        try:
            ssh = SSHKey(public_key, strict=True, skip_option_parsing=True)
            ssh.parse()
        except (TooShortKeyError, TooLongKeyError):
            pass
        except (InvalidKeyError, NotImplementedError):
            raise DbaasClientError(f'Invalid ssh public key for record `{public_key}`')


def validate_initialization_actions(version: str) -> None:
    """
    Validate image version for initialization actions
    """
    threshold_version = constants.DATAPROC_INIT_ACTIONS_VERSION
    comparison = semver.compare(version, threshold_version)
    if comparison == -1:
        raise DbaasClientError(
            f'To specify initialization actions dataproc image version must be at least `{threshold_version}`'
        )


def validate_service_account(service_account_id: str, folder_id: str) -> None:
    """
    Validate service account before creating or modifying a cluster
    """
    validate_can_use_service_account(service_account_id)
    if not {'mdb.dataproc.agent', 'dataproc.agent'} & service_account_roles(folder_id, service_account_id):
        raise DbaasClientError(
            f'Service account {service_account_id} should have role `dataproc.agent` for specified folder {folder_id}'
        )


def validate_log_group(log_group_id: Optional[str], service_account_id: str, cluster_folder_id: str) -> None:
    """
    Validate log group before creating or modifying a cluster
    """

    logging_service = get_logging_service()
    try:
        logging_service.work_as_user_specified_account(service_account_id=service_account_id)
        if log_group_id == '':
            log_group = logging_service.get_default(folder_id=cluster_folder_id)
            log_group_id = log_group.data.id
            log_group_folder_id = cluster_folder_id
        else:
            log_group = logging_service.get(log_group_id=log_group_id)
            log_group_folder_id = log_group.data.folder_id
    except (LogGroupNotFoundError, LogGroupPermissionDeniedError):
        raise DbaasClientError(
            f'Can not find log group {log_group_id}. '
            f'Or service account {service_account_id} does not have permission to get log group {log_group_id}. '
            f'Grant role "logging.writer" to service account {service_account_id} for the folder of this log group.'
        )

    access_service = current_app.config['AUTH_PROVIDER'](current_app.config)
    try:
        access_service.authorize(
            'logging.records.write',
            logging_service.user_service_account_token,
            folder_id=log_group_folder_id,
        )
    except AuthError:
        raise DbaasClientError(
            f'Service account {service_account_id} does not have permission to write to log group {log_group_id}. '
            f'Grant role "logging.writer" to service account {service_account_id} for folder {log_group_folder_id}.'
        )


def validate_service_account_for_instance_groups(service_account_id: str, folder_id: str) -> None:
    """
    Validate service account permissions to work with instance groups
    """

    needed_permissions = [
        'compute.instanceGroups.create',
        'compute.instanceGroups.delete',
        'compute.instanceGroups.update',
        'compute.instanceGroups.stop',
        'compute.instanceGroups.start',
        'compute.instanceGroups.use',
        'iam.serviceAccounts.use',
        'monitoring.data.read',
        'monitoring.sensors.get',
    ]
    access_service = current_app.config['AUTH_PROVIDER'](current_app.config)
    user_service_account_token = get_iam_client().issue_iam_token(service_account_id=service_account_id)

    # TODO use BulkAuthorize(BulkAuthorizeRequest) here to check all permissions with one request
    for permission in needed_permissions:
        try:
            access_service.authorize(
                permission,
                user_service_account_token,
                folder_id=folder_id,
            )
        except AuthError:
            error_message = (
                f'Service account {service_account_id} must have role '
                'such as `dataproc.provisioner` and `monitoring.viewer` '
                f'for specified folder {folder_id} to create and operate instance group for autoscaling subcluster.'
            )

            if g.folder['folder_ext_id'] != folder_id:
                error_message += '\nYC CLI example:\n'
                'yc resource-manager folder add-access-binding --role dataproc.provisioner'
                f' --service-account-id {service_account_id} --id {folder_id}\n'
                'yc resource-manager folder add-access-binding --role monitoring.viewer'
                f' --service-account-id {service_account_id} --id {folder_id}\n'

            raise DbaasClientError(error_message)


def validate_service_account_for_folder(service_account_id: str, folder_id: str) -> None:
    """
    Validate that service account has roles vpc.user and dns.editor roles for another folder in the cloud
    This is needed to created an instance group based subcluster using subnet from another folder
    See CLOUDSUPPORT-80986 for example
    """

    needed_permissions = [
        'resource-manager.folders.get',
        'vpc.subnets.use',
        'vpc.subnets.get',
        'vpc.subnets.list',
        'vpc.securityGroups.use',
        'vpc.securityGroups.get',
        'vpc.securityGroups.list',
        'vpc.operations.get',
        'dns.zones.updateRecords',
        'dns.zones.get',
        'dns.zones.create',
        'dns.zones.delete',
        'dns.zones.update',
        'dns.zones.use',
    ]
    access_service = current_app.config['AUTH_PROVIDER'](current_app.config)
    user_service_account_token = get_iam_client().issue_iam_token(service_account_id=service_account_id)

    # TODO use BulkAuthorize(BulkAuthorizeRequest) here to check all permissions with one request
    for permission in needed_permissions:
        try:
            access_service.authorize(
                permission,
                user_service_account_token,
                folder_id=folder_id,
            )
        except AuthError:
            raise DbaasClientError(
                f'Service account {service_account_id} must have roles such as vpc.user and dns.editor'
                f' to use subnets and edit DNS records for specified folder {folder_id}.'
                ' Please add such roles or choose another service account for cluster. YC CLI example:\n'
                'yc resource-manager folder add-access-binding --role vpc.user'
                f' --service-account-id {service_account_id} --id {folder_id}\n'
                'yc resource-manager folder add-access-binding --role dns.editor'
                f' --service-account-id {service_account_id} --id {folder_id}'
            )


def validate_job(job_spec: dict, pillar: HadoopPillar) -> None:
    """
    Validate job specification before add
    """
    validate_job_services(job_spec, pillar)
    validate_hive_job(job_spec)
    validate_mapreduce_job(job_spec)
    validate_health_on_job_submit(pillar)
    validate_lightweight_jobs(job_spec, pillar)


def validate_job_services(job_spec: dict, pillar: HadoopPillar) -> None:
    """
    Validate job on cluster services
    """
    if len(job_spec) > 1 or not job_spec:
        raise DbaasClientError('You need to specify exactly one job spec')
    job_type = list(job_spec.keys())[0]
    cluster_services = set(pillar.services)
    job_services = set(DATAPROC_JOB_SERVICES[job_type])
    missing_services = job_services - cluster_services
    if missing_services:
        missing_services_str = ', '.join(sorted(ClusterService.to_strings(missing_services)))
        raise DbaasClientError('To create job of this type you need cluster with services: ' + missing_services_str)


def validate_lightweight_jobs(job_spec: dict, pillar: HadoopPillar):
    """
    Validate lightweight jobs
    """
    if not pillar.is_lightweight():
        return
    for job_type in ['spark_job', 'pyspark_job']:
        spec = job_spec.get(job_type)
        if not spec:
            continue
        if spec.get('file_uris'):
            raise DbaasClientError('Using fileUris is forbidden on lightweight clusters')


def validate_hive_job(job_spec: dict):
    """
    Validate hive job specification
    """
    hive_job_spec = job_spec.get('hive_job')
    if hive_job_spec is not None:
        query_args_count = 0

        query_file_uri = hive_job_spec.get('query_file_uri', '')
        if len(query_file_uri) > 0:
            query_args_count += 1

        query_list = hive_job_spec.get('query_list', {'queries': []})
        queries = query_list.get('queries', [])
        if len(queries) > 0:
            query_args_count += 1

        if query_args_count != 1:
            raise DbaasClientError(
                'Exactly one of attributes "query_file_uri"' ' or "query_list" must be specified for hive job'
            )


def validate_mapreduce_job(job_spec: dict):
    """
    Validate mapreduce job specification
    """
    mapreduce_job_spec = job_spec.get('mapreduce_job')
    if mapreduce_job_spec is not None:
        driver_args_count = 0

        main_jar_file_uri = mapreduce_job_spec.get('main_jar_file_uri', '')
        if len(main_jar_file_uri) > 0:
            driver_args_count += 1

        main_class = mapreduce_job_spec.get('main_class', '')
        if len(main_class) > 0:
            driver_args_count += 1

        if driver_args_count != 1:
            raise DbaasClientError(
                'Exactly one of attributes "main_jar_file_uri"' ' or "main_class" must be specified for mapreduce job'
            )


def validate_health_on_job_submit(pillar: HadoopPillar):
    """
    On job submit check that dataproc-agent is connected to control-plane and HDFS is not in safemode
    """
    extended_health = dataproc_manager_client().cluster_health(pillar.agent_cid)
    if extended_health.health == ClusterHealth.unknown:
        subcluster = pillar.subcluster_main
        subnet = fetch_subnet(subcluster['subnet_id'], subcluster.get('assign_public_ip', False), pillar.zone_id)
        if not subnet.get('egressNatEnable', False):
            raise DbaasClientError(
                'In order to run jobs subnet of the main subcluster should have NAT feature turned on.'
            )

    if extended_health.hdfs_in_safemode:
        raise DbaasClientError('Unable to run jobs while HDFS is in Safemode')


def validate_s3_bucket(pillar: HadoopPillar) -> None:
    """
    Validate s3_bucket
    """
    if pillar.is_lightweight():
        if not pillar.user_s3_bucket:
            raise DbaasClientError('Unable to use lightweight cluster without s3 bucket')


def validate_cluster_is_alive(cluster_id: str):
    if has_feature_flag('MDB_DATAPROC_MANAGER'):
        extended_health = dataproc_manager_client().cluster_health(cluster_id)
        if extended_health.health != ClusterHealth.alive:
            raise DbaasClientError(
                f'Unable to modify cluster while its health status is {extended_health.health.humanize()}:'
                f' {extended_health.explanation}'
            )


def check_version_is_allowed(image_config: dict, version_prefix: str) -> None:
    if image_config.get('deprecated'):
        if feature_flag := image_config.get('allow_deprecated_feature_flag'):
            if not has_feature_flag(feature_flag):
                raise PreconditionFailedError(f"Can't create cluster, version '{version_prefix}' is deprecated")
    if feature_flag := image_config.get('feature_flag'):
        if not has_feature_flag(feature_flag):
            raise PreconditionFailedError(f"Version '{version_prefix}' is not available yet")


class ServiceEndpoint:
    hostname: str
    ip: Optional[str]
    port: int
    purpose: str

    # Here we understand following formats:
    # * storage.yandexcloud.net
    # * dataproc-manager.api.cloud.yandex.net:443
    # * https://dataproc-ui.yandexcloud.net/
    def __init__(self, uri_or_hostname: str, purpose: str, default_port: int = 443):
        if '://' in uri_or_hostname:
            uri = urlparse(uri_or_hostname)
            self.hostname = cast(str, uri.hostname)
            if uri.port:
                self.port = uri.port
            elif uri.scheme == 'http':
                self.port = 80
            elif uri.scheme == 'https':
                self.port = 443
            else:
                raise RuntimeError(f'Unsupported scheme {uri.scheme} for service endpoint {uri_or_hostname}')
        else:
            if ':' in uri_or_hostname:
                hostname, port = uri_or_hostname.split(":")
                self.hostname = hostname
                self.port = int(port)
            else:
                self.hostname = uri_or_hostname
                self.port = default_port
        self.purpose = purpose


def validate_dataproc_security_groups(
    pillar: HadoopPillar, nat_enabled: bool, got_assign_public_ip: bool, security_groups_ids: List[str] = None
) -> None:
    """
    Validate security group rules for dataproc cluster
    """
    if not security_groups_ids:
        if got_assign_public_ip:
            raise DbaasClientError('Public ip on subcluster requires at least one security group specified.')
        else:
            return
    security_groups = [get_network_provider().get_security_group(sg) for sg in security_groups_ids]
    do_validate_dataproc_security_groups(pillar, nat_enabled, security_groups)


def do_validate_dataproc_security_groups(
    pillar: HadoopPillar, nat_enabled: bool, security_groups: List[SecurityGroup]
) -> None:
    """
    Validate security group rules for dataproc cluster
    """
    rules = []
    for sg in security_groups:
        rules.extend(sg.rules)
    # Check widest self rules for ingress and egress traffic
    ingress, egress = False, False
    for rule in rules:
        if (
            ((rule.ports_from == 0 and rule.ports_to == 65535) or (rule.ports_from is None and rule.ports_to is None))
            and rule.protocol_name in ('ANY', 'TCP')
            and ('0.0.0.0/0' in rule.v4_cidr_blocks or rule.predefined_target == 'self_security_group')
        ):
            if rule.direction == SecurityGroupRuleDirection.INGRESS.value:
                ingress = True
            elif rule.direction == SecurityGroupRuleDirection.EGRESS.value:
                egress = True
            else:
                # SG has widest double-sided rule
                ingress, egress = True, True
                break

    if not ingress or not egress:
        raise DbaasClientError(
            'Specified set of security_groups rules doesn\'t have enough permissions for working dataproc correctly.'
        )

    if not nat_enabled:
        # If NAT is not enabled than service endpoints will not be accessible anyway
        return

    service_endpoints = get_service_endpoints(pillar)
    dns_cache = current_app.config['DNS_CACHE']
    for service_endpoint in service_endpoints:
        service_endpoint.ip = dns_cache.get(service_endpoint.hostname)
    service_endpoints = [se for se in service_endpoints if se.ip is not None]
    validate_service_endpoints_accessible(service_endpoints, rules)


S3_HOSTNAME = 'storage.yandexcloud.net'
MANAGER_HOSTNAME = 'dataproc-manager.api.cloud.yandex.net'
MONITORING_HOSTNAME = 'monitoring.api.cloud.yandex.net'
UI_PROXY_HOSTNAME = 'dataproc-ui.yandexcloud.net'
GATEWAY_URL = 'api.cloud.yandex.net'
LOGGING_INGESTER_URL = 'ingester.logging.yandexcloud.net'


def get_service_endpoints(pillar: HadoopPillar) -> List[ServiceEndpoint]:
    service_endpoints = []
    s3_hostname = pillar.s3.get('endpoint_url', S3_HOSTNAME)
    service_endpoints.append(ServiceEndpoint(s3_hostname, 'required to access Object Storage'))

    # health and jobs
    manager_url = pillar.agent.get('manager_url', f'{MANAGER_HOSTNAME}:443')
    service_endpoints.append(
        ServiceEndpoint(manager_url, 'required for reporting of service health and running Data Proc jobs')
    )

    # monitoring and autoscaling
    monitoring_hostname = pillar.monitoring.get('hostname', MONITORING_HOSTNAME)
    service_endpoints.append(
        ServiceEndpoint(monitoring_hostname, 'required for Data Proc service monitoring and autoscaling')
    )

    # logging
    service_endpoints.append(
        ServiceEndpoint(
            pillar.logging.get('url', GATEWAY_URL),
            'required for Data Proc service cloud logging',
        )
    )
    service_endpoints.append(
        ServiceEndpoint(
            pillar.logging.get('ingester_url', LOGGING_INGESTER_URL),
            'required for Data Proc service cloud logging',
        )
    )

    # UI Proxy
    ui_proxy_url = pillar.agent.get('ui_proxy_url', f'https://{UI_PROXY_HOSTNAME}/')
    service_endpoints.append(ServiceEndpoint(ui_proxy_url, 'used by Data Proc UI Proxy'))

    return service_endpoints


def validate_service_endpoints_accessible(service_endpoints: List[ServiceEndpoint], rules: List[SecurityGroupRule]):
    not_allowed_service_endpoints = []
    for service_endpoint in service_endpoints:
        allowed = False
        for rule in rules:
            if rule.direction != SecurityGroupRuleDirection.EGRESS.value:
                continue

            if rule.ports_from is not None and rule.ports_to is not None:
                port_allowed = rule.ports_from <= service_endpoint.port <= rule.ports_to
            else:
                port_allowed = rule.ports_from is None and rule.ports_to is None
            if not port_allowed:
                continue

            if rule.protocol_name not in ('ANY', 'TCP'):
                continue

            for subnet in rule.v4_cidr_blocks:
                if ipaddress.ip_address(service_endpoint.ip) in ipaddress.ip_network(subnet):
                    allowed = True
                    break
            if allowed:
                break
        if not allowed:
            not_allowed_service_endpoints.append(service_endpoint)
    if not_allowed_service_endpoints:
        endpoints = ', '.join(sorted([f'{e.ip}:{e.port} ({e.purpose})' for e in not_allowed_service_endpoints]))
        raise DbaasClientError(
            f'It is required that security groups allow egress TCP traffic to the following list of Yandex Cloud'
            f' service endpoints for the Data Proc cluster to function properly: {endpoints}'
        )
