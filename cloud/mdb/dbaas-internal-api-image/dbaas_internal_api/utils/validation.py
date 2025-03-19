# -*- coding: utf-8 -*-
"""
DBaaS Internal API validation functions
"""

import copy
import datetime
import logging
import math
import re
from abc import ABC, abstractmethod
from collections.abc import Iterable, Mapping
from typing import List, Dict, Optional, Sequence

import humanfriendly
import marshmallow.validate  # noqa
import requests
from flask import current_app, g, request
from flask_restful import abort
from marshmallow import post_load, pre_dump
from marshmallow.exceptions import ValidationError
from marshmallow.fields import Int
from marshmallow.validate import Range
from requests import RequestException

from dbaas_internal_api.utils.quota import DB_TO_SPEC_QUOTA_MAPPING, db_quota_limit_field, db_quota_usage_field

from dbaas_common import tracing
from . import config, metadb, register
from ..core.auth import AuthError
from ..core.exceptions import (
    DbaasClientError,
    HostNotExistsError,
    LastHostInHaGroupError,
    MalformedReplicationChain,
    PreconditionFailedError,
    QuotaViolationHttpError,
    ResourcePresetIsDecommissioned,
    DiskTypeIdError,
)
from ..utils.request_context import get_x_request_id
from ..utils.types import DTYPE_LOCAL_SSD, ExistedHostResources, RequestedHostResources
from .feature_flags import ensure_feature_flag
from .logs import log_warn
from .time import datetime_to_rfc3339_utcoffset, rfc3339_to_datetime
from .types import ENV_PROD, VTYPE_COMPUTE, VTYPE_PORTO, ClusterInfo


def check_cluster_not_in_status(cluster, *forbidden_statuses):
    """
    Raise exception if cluster status is equal to forbidden one
    """
    if isinstance(cluster, ClusterInfo):
        status = cluster.status.value
    else:
        status = cluster['status']
    if status in [x.value for x in forbidden_statuses]:
        raise PreconditionFailedError('Operation is not allowed in current cluster status')


def get_flavor_by_name(flavor_name):
    """
    Get flavor by name. Raises abort if flavor does not exists
    """
    flavor = metadb.get_flavor_by_name(flavor_name)
    if not flavor:
        raise DbaasClientError("resourcePreset '%s' does not exist" % flavor_name)
    return flavor


def validate_hosts_count_only_max(*args):
    min_hosts, *args_cut = args
    return _validate_hosts_count(0, *args_cut)


def _validate_hosts_count(min_hosts, max_hosts, cluster_type, role, resource_preset_id, disk_type_id, hosts_count):
    """
    Validate allowed for resource preset and disk type hosts count
    """

    def _format_message(cluster_type, role, resource_preset_id, disk_type_id, needed_count, failure_type):
        entity = 'cluster'
        if cluster_type in ('clickhouse_cluster', 'mongodb_cluster', 'redis_cluster'):
            entity = 'shard'
            if role not in ('clickhouse_cluster', 'mongodb_cluster.mongod', 'redis_cluster'):
                entity = 'subcluster'

        roles_mapping = {
            'postgresql_cluster': 'PostgreSQL',
            'clickhouse_cluster': 'ClickHouse',
            'zk': 'Zookeeper',
            'mongodb_cluster.mongod': 'Mongod',
            'mongodb_cluster.mongos': 'Mongos',
            'mongodb_cluster.mongocfg': 'Mongocfg',
            'mongodb_cluster.mongoinfra': 'Mongoinfra',
            'hadoop_cluster.masternode': 'Hadoop master nodes',
            'hadoop_cluster.datanode': 'Hadoop data nodes',
            'hadoop_cluster.computenode': 'Hadoop compute nodes',
            'mysql_cluster': 'MySQL',
            'redis_cluster': 'Redis',
        }

        if failure_type == 'minimum':
            action = 'requires at least'
        else:
            action = 'allows at most'

        hosts = 'host' if needed_count == 1 else 'hosts'
        message = (
            "{role} {entity} with resource preset '{resource_preset_id}' "
            + "and disk type '{disk_type_id}' {action} {needed_count} {hosts}"
        )

        return message.format(
            role=roles_mapping.get(role, role),
            entity=entity,
            resource_preset_id=resource_preset_id,
            disk_type_id=disk_type_id,
            action=action,
            needed_count=needed_count,
            hosts=hosts,
        )

    message = None
    if hosts_count < min_hosts:
        message = _format_message(cluster_type, role, resource_preset_id, disk_type_id, min_hosts, 'minimum')
    if hosts_count > max_hosts:
        message = _format_message(cluster_type, role, resource_preset_id, disk_type_id, max_hosts, 'maximum')

    if message:
        raise PreconditionFailedError(message)


def _validate_disk_type(disk_type_id):
    """
    ensure that given disk_type_id is valid
    """
    # Cache valid_disk_types in g cause:
    # - we call it from validate_hosts_count and validate_host_resources_combination
    #   (modules may then in different order)
    # - modules call then on each host
    if not hasattr(g, 'valid_disk_types'):
        g.valid_disk_types = frozenset(metadb.get_space_quota_map())
    if disk_type_id not in g.valid_disk_types:
        abort(
            422,
            message=f"diskTypeId '{disk_type_id}' is not valid",
        )


def validate_hosts_count(
    cluster_type: str,
    role: str,
    resource_preset_id: Optional[str],
    disk_type_id: Optional[str],
    hosts_count: int,
    validate_func=None,
):
    """
    Validate allowed for resource preset and disk type hosts count
    """
    _validate_disk_type(disk_type_id)
    combinations = metadb.get_valid_resources(
        cluster_type=cluster_type,
        role=role,
        resource_preset_id=resource_preset_id,
        disk_type_ext_id=disk_type_id,
    )

    unique_combinations = {(i['min_hosts'], i['max_hosts']) for i in combinations}
    if len(unique_combinations) > 1:
        raise RuntimeError('Different hosts limits for different geos. This should never happen.')

    if not combinations:
        abort(
            422,
            message="resourcePreset '{resource_preset_id}' and diskTypeId "
            "'{disk_type_id}' are not available".format(
                resource_preset_id=resource_preset_id, disk_type_id=disk_type_id
            ),
        )
    validate_func = _validate_hosts_count if validate_func is None else validate_func
    for combination in combinations:
        validate_func(
            combination['min_hosts'],
            combination['max_hosts'],
            cluster_type,
            role,
            resource_preset_id,
            disk_type_id,
            hosts_count,
        )


def validate_host_resources_combination(cluster_type, role, resource_preset_id, geo, disk_type_id, disk_size):
    """
    Validate resource combination
    """
    _validate_disk_type(disk_type_id)
    combinations = metadb.get_valid_resources(
        cluster_type=cluster_type,
        role=role,
        geo=geo,
        resource_preset_id=resource_preset_id,
        disk_type_ext_id=disk_type_id,
    )

    if not combinations:
        abort(
            422,
            message="resourcePreset '{resource_preset_id}' and diskTypeId "
            "'{disk_type_id}' are not available "
            "in zoneId '{zone_id}'".format(
                resource_preset_id=resource_preset_id, disk_type_id=disk_type_id, zone_id=geo
            ),
        )
    assert len(combinations) == 1, 'get_valid_resources returned ' 'invalid combinations: {combinations}'.format(
        combinations=str(combinations)
    )
    combination = combinations[0]

    disk_sizes = combination['disk_sizes']
    disk_size_range = combination['disk_size_range']
    if disk_sizes and disk_size not in disk_sizes:
        raise PreconditionFailedError('Invalid disk_size, use one of {sizes}'.format(sizes=disk_sizes))
    if disk_size_range:
        if disk_size not in disk_size_range:
            max = disk_size_range.upper if disk_size_range.upper_inc else disk_size_range.upper - 1
            raise PreconditionFailedError(
                'Invalid disk_size, must be between '
                'or equal {lower} and {upper}'.format(lower=disk_size_range.lower, upper=max)
            )
        minimal_disk_unit = config.get_minimal_disk_unit()
        if disk_size % minimal_disk_unit != 0:
            raise PreconditionFailedError('Disk size must be a multiple of {unit} bytes'.format(unit=minimal_disk_unit))


def is_decommissioning_geo(geo):
    """
    Return true if geo is decommissioning
    """
    return geo in config.get_decommissioning_zones()


def is_decommissioning_flavor(flavor_name):
    """
    Return true if flavor_name is decommissioning
    """
    return flavor_name in config.get_decommissioning_flavors()


def validate_geo_availability(geo):
    """
    Validate geo availability
    """
    if is_decommissioning_geo(geo):
        raise PreconditionFailedError(f"No new resources could be created in zone '{geo}'")


def validate_host_create_resources(cluster_type, role, resource_preset_id, geo, disk_type_id, disk_size):
    """
    Validate resource combination and DC status for new host
    """
    validate_host_resources_combination(
        cluster_type=cluster_type,
        role=role,
        resource_preset_id=resource_preset_id,
        geo=geo,
        disk_type_id=disk_type_id,
        disk_size=disk_size,
    )
    validate_geo_availability(geo)


def check_change_compute_resource(
    host_objs: Sequence[Dict],
    cur_resource_obj: ExistedHostResources,
    new_resource_obj: Optional[RequestedHostResources] = None,
):
    """
    Check if compute resource change is allowed
    (raises on not allowed changes)
    """
    # No resource changes required
    if new_resource_obj is None:
        return

    compute_hosts = [host for host in host_objs if host['vtype'] == VTYPE_COMPUTE]
    if not compute_hosts:
        return

    if new_resource_obj.disk_size is not None:
        # network disk change down without feature flag
        if any(
            (
                host['space_limit'] > new_resource_obj.disk_size
                for host in compute_hosts
                if host['disk_type_id'] not in ['local-nvme', DTYPE_LOCAL_SSD]
            )
        ):
            ensure_feature_flag('MDB_NETWORK_DISK_TRUNCATE')
        # MDB-9031: forbid network-ssd-nonreplicated resize
        if any(
            (
                host['space_limit'] != new_resource_obj.disk_size
                for host in compute_hosts
                if host['disk_type_id'] == 'network-ssd-nonreplicated'
            )
        ):
            ensure_feature_flag('MDB_ALLOW_NETWORK_SSD_NONREPLICATED_RESIZE')

    instance_type_changed = (
        new_resource_obj.resource_preset_id is not None
        and cur_resource_obj.resource_preset_id != new_resource_obj.resource_preset_id
    )
    # any change on local-nvme or local-ssd disks without feature flag
    if any(
        (
            host['disk_type_id'] in ['local-nvme', DTYPE_LOCAL_SSD]
            and (host['space_limit'] != new_resource_obj.disk_size or instance_type_changed)
            for host in compute_hosts
        )
    ):
        ensure_feature_flag('MDB_LOCAL_DISK_RESIZE')


def check_resource_diffs(cloud_ext_id, diffs):
    """
    Check that resource diffs are not negative and
    fail with correct error message
    """
    failed = []
    violations = []
    for key, value in diffs.items():
        quota = value['quota']
        used = value['used']
        resource_required = used + value['change']
        if quota >= resource_required:
            continue

        pretty_value = int(math.ceil(resource_required - quota))
        if key not in ('cpu', 'gpu', 'clusters'):
            # Humanfriendly will append KiB or something like that
            pretty_value = humanfriendly.format_size(pretty_value, binary=True)

        if key == 'cpu':
            append = ' cores'
        elif key == 'gpu':
            append = ' cards'
        elif key == 'clusters':
            append = ' clusters'
        else:
            append = ''

        failed.append('{key}: {value}{unit}'.format(key=key, value=pretty_value, unit=append))

        violations.append(
            {
                'metric': {
                    'name': DB_TO_SPEC_QUOTA_MAPPING[key],
                    'limit': int(quota),
                    'usage': used,
                },
                'required': int(math.ceil(resource_required)),
            }
        )

    if violations:
        message = 'Quota limits exceeded, not enough {resources}'.format(resources=', '.join(sorted(failed)))
        raise QuotaViolationHttpError(message, cloud_ext_id, violations)


class QuotaValidator:
    """
    Class for accounting resource allocation requirements and performing
    cloud quota validation.
    """

    # pylint: disable=too-many-instance-attributes

    def __init__(self, new_cluster=True):
        self.cloud = g.cloud
        self.cpu_required = 0
        self.gpu_required = 0
        self.memory_required = 0
        self.network_required = 0
        self.io_required = 0
        self.ssd_space_required = 0
        self.hdd_space_required = 0
        self.new_cluster = new_cluster
        self._space_quota_map = None

    def add(self, flavor, node_count, volume_size, disk_type_id):
        """
        Add required amount of resources for "node_count" nodes with given
        "flavor" and "volume_size".
        """
        if not self._space_quota_map:
            self._space_quota_map = metadb.get_space_quota_map()
        if disk_type_id not in self._space_quota_map:
            raise DiskTypeIdError(disk_type_id, list(self._space_quota_map.keys()))
        elif self._space_quota_map[disk_type_id] == 'hdd':
            hdd_volume_size = volume_size
            ssd_volume_size = 0
        elif self._space_quota_map[disk_type_id] == 'ssd':
            hdd_volume_size = 0
            ssd_volume_size = volume_size
        else:
            raise RuntimeError(
                'Unexpected space quota type for disk_type_id {id}: {value}'.format(
                    id=disk_type_id, value=self._space_quota_map[disk_type_id]
                )
            )
        node_count = int(node_count)
        self.cpu_required += node_count * flavor['cpu_guarantee']
        self.gpu_required += node_count * flavor['gpu_limit']
        self.memory_required += node_count * flavor['memory_guarantee']
        self.ssd_space_required += node_count * int(ssd_volume_size)
        self.hdd_space_required += node_count * int(hdd_volume_size)

    def validate(self):
        """
        Perform cloud quota validation.
        """
        self.check_resources()

    def check_resources(self):
        """
        Ensure that cloud has enough quota to create new hosts.
        """
        cloud = self.cloud
        diffs = {
            'clusters': {
                'quota': self.cloud[db_quota_limit_field('clusters')],
                'used': self.cloud[db_quota_usage_field('clusters')],
                'change': int(self.new_cluster),
            },
            'cpu': {
                'quota': cloud[db_quota_limit_field('cpu')],
                'used': cloud[db_quota_usage_field('cpu')],
                'change': self.cpu_required,
            },
            'gpu': {
                'quota': cloud[db_quota_limit_field('gpu')],
                'used': cloud[db_quota_usage_field('gpu')],
                'change': self.gpu_required,
            },
            'memory': {
                'quota': cloud[db_quota_limit_field('memory')],
                'used': cloud[db_quota_usage_field('memory')],
                'change': self.memory_required,
            },
            'ssd_space': {
                'quota': cloud[db_quota_limit_field('ssd_space')],
                'used': cloud[db_quota_usage_field('ssd_space')],
                'change': self.ssd_space_required,
            },
            'hdd_space': {
                'quota': cloud[db_quota_limit_field('hdd_space')],
                'used': cloud[db_quota_usage_field('hdd_space')],
                'change': self.hdd_space_required,
            },
        }
        check_resource_diffs(self.cloud['cloud_ext_id'], diffs)


def validate_resource_preset(flavor):
    """
    Check if given resource preset is valid (i.e. exists and can be used for new hosts/updates)
    """
    if not isinstance(flavor, dict):
        raise RuntimeError('flavor should be dict, but {} given'.format(type(flavor)))
    if is_decommissioning_flavor(flavor['name']):
        raise ResourcePresetIsDecommissioned(flavor['name'])


def check_quota(flavor, node_count, volume_size, disk_type_id, new_cluster=False):
    """
    Function not raises if cloud has enough resources to create the required
    number of nodes with required flavor and volume_size.
    """
    validator = QuotaValidator(new_cluster)
    validator.add(flavor, node_count, volume_size, disk_type_id)
    validator.check_resources()


def volume_size_diffs(hosts, new_size):
    """
    Returns pair of ssd and hdd volume size diffs.
    """
    current_ssd_space = 0
    current_hdd_space = 0
    new_ssd_space = 0
    new_hdd_space = 0
    space_quota_map = metadb.get_space_quota_map()
    for host in hosts:
        if space_quota_map[host['disk_type_id']] == 'hdd':
            current_hdd_space += host['space_limit']
            new_hdd_space += new_size
        elif space_quota_map[host['disk_type_id']] == 'ssd':
            current_ssd_space += host['space_limit']
            new_ssd_space += new_size
        else:
            raise RuntimeError(
                'Unexpected space quota type for disk_type_id {id}: {value}'.format(
                    id=host['disk_type_id'], value=space_quota_map[host['disk_type_id']]
                )
            )
    return new_ssd_space - current_ssd_space, new_hdd_space - current_hdd_space


def check_quota_volume_size_diffs(ssd_diff, hdd_diff):
    """
    Function checks if cloud has enough resources
    to change volume_size for `hosts` hosts.
    """
    diff = {
        'ssd_space': {
            'quota': g.cloud[db_quota_limit_field('ssd_space')],
            'used': g.cloud[db_quota_usage_field('ssd_space')],
            'change': ssd_diff,
        },
        'hdd_space': {
            'quota': g.cloud[db_quota_limit_field('hdd_space')],
            'used': g.cloud[db_quota_usage_field('hdd_space')],
            'change': hdd_diff,
        },
    }
    check_resource_diffs(g.cloud['cloud_ext_id'], diff)


def check_quota_change_resources(resources):
    """
    Function checks if cloud has enough resources to change resources.
    `resources` arg is dict which contains resources that want to be
    changed and diff values.
    """
    diffs = {
        'cpu': {
            'quota': g.cloud[db_quota_limit_field('cpu')],
            'used': g.cloud[db_quota_usage_field('cpu')],
            'change': resources.get('cpu_guarantee', 0),
        },
        'gpu': {
            'quota': g.cloud[db_quota_limit_field('gpu')],
            'used': g.cloud[db_quota_usage_field('gpu')],
            'change': resources.get('gpu_limit', 0),
        },
        'memory': {
            'quota': g.cloud[db_quota_limit_field('memory')],
            'used': g.cloud[db_quota_usage_field('memory')],
            'change': resources.get('memory_guarantee', 0),
        },
    }

    check_resource_diffs(g.cloud['cloud_ext_id'], diffs)


def get_available_geo(force_filter_decommissioning: bool = False):
    """
    Get available geo

    All except dismissing zones
    """
    decommissioning_geos = config.get_decommissioning_zones(force_filter_decommissioning)
    return [g for g in metadb.get_all_geo() if g not in decommissioning_geos]


def geo_validator(geo_config):
    """
    Validates geo locations configuration
    """
    # use all geo for validate
    all_geo = metadb.get_all_geo()
    # avaliable geo for error message
    available_geo = [g for g in all_geo if not is_decommissioning_geo(g)]

    def _is_invalid_geo(geo):
        return geo not in all_geo

    if isinstance(geo_config, Mapping):
        if any(_is_invalid_geo(geo) for geo in geo_config):
            raise ValidationError('Invalid values, valid values are {values}'.format(values=available_geo), 'nodes')
    elif _is_invalid_geo(geo_config):
        raise ValidationError('Invalid value, valid value is one of {values}'.format(values=available_geo), 'geo')


def hostname_validator(hostname):
    """Validates hostname"""

    hostname = hostname.lower()

    if hostname.endswith('.'):
        hostname = hostname[:-1]
    if len(hostname) > 253:
        raise ValidationError('Hostname must be up to 253 symbols long')
    domains = hostname.split('.')

    # the TLD must be not all-numeric
    if domains[-1].isnumeric():
        raise ValidationError('TLD must be non-numeric')

    allowed = re.compile('^[a-z0-9_-]{1,63}$')
    for domain in domains:
        if domain.startswith('-') or domain.endswith('-'):
            raise ValidationError('Subdomains must not start nor end with ' 'hyphen')
        if not allowed.search(domain):
            raise ValidationError('Subdomains may only contain alphanumeric characters and ' '\'_\' or \'-\'')


class StrValidator:
    """
    Generic str validator
    """

    def __init__(self, obj_name: str, min_size: int, max_size: int, invalid_chars: str) -> None:
        self.obj_name = obj_name
        self.min_size = min_size
        self.max_size = max_size
        self.invalid_re = re.compile(invalid_chars)

    def __call__(self, value: str) -> None:
        if not isinstance(value, str):
            raise ValidationError('%s must be a string.' % self.obj_name.title())
        if len(value) < self.min_size or len(value) > self.max_size:  # pylint: disable=len-as-condition
            raise ValidationError('%s can be up to %d characters long' % (self.obj_name.title(), self.max_size))
        match = self.invalid_re.search(value)
        invalid_char = match.group(0) if match else None
        if invalid_char:
            raise ValidationError('Symbol \'%s\' not allowed in %s.' % (invalid_char, self.obj_name))


class NotNulStrValidator:
    """
    PostgreSQL-friendly str validator
    """

    def __call__(self, value):
        if isinstance(value, Iterable) and '\0' in value:
            raise ValidationError('Symbol NUL is not allowed')


def validate_cluster_type(cluster_type, schema_type):
    """
    Check that cluster_type is registered
    """
    allowed = register.get_cluster_types(schema_type)
    if cluster_type not in allowed:
        raise DbaasClientError(
            'Unknown cluster_type \'{type}\', must be one of {allowed}'.format(type=cluster_type, allowed=list(allowed))
        )


def validate_repl_source(hosts_repl_sources: Dict[str, Optional[str]], host_config: Dict[str, str]):
    """
    Get existing host:replication_source mapping and dict with changes.
    Checks that replication chain ends ont HA host (one having no replication_source).
    Checks if there are no loops in replication topology.
    """

    def _check_repl_loop(host, stack):
        if hosts_repl_sources[host] is None:
            return
        if host == hosts_repl_sources[host] or host in stack:
            raise MalformedReplicationChain()
        stack.append(host)
        _check_repl_loop(hosts_repl_sources[host], stack)

    changes = False
    for host in host_config:
        if host in hosts_repl_sources and hosts_repl_sources[host] == host_config[host]:
            continue
        changes = True
        if host_config[host] is not None and host_config[host] not in hosts_repl_sources:
            raise HostNotExistsError(host_config[host])
        hosts_repl_sources[host] = host_config[host]
        if None not in hosts_repl_sources.values():
            raise LastHostInHaGroupError()
        _check_repl_loop(host, [])

    return changes


class OneOf(marshmallow.validate.OneOf):
    """
    Class inherited from marshmallow.validate.OneOf
    with normal error message initialization
    """

    def __init__(self, choices):
        super().__init__(choices, error='Invalid value \'{input}\',' ' allowed values: {choices}')


class GrpcTimestamp(marshmallow.fields.Field):
    """
    GRPC like timestamp.

    In JSON representation it should be a string in RFC3339 format.
    """

    default_error_messages = {
        'empty-value': 'Empty timestamp',
        'invalid-format': 'Invalid timestamp format',
    }

    def _serialize(self, value, attr, obj):
        return datetime_to_rfc3339_utcoffset(value)

    def _deserialize(self, value, attr, data):
        if not value:
            raise self.fail('empty-value')
        # It's looks ugly, but marshmallow,
        # call us on `missing` valus
        if isinstance(value, datetime.datetime):
            return value
        try:
            return rfc3339_to_datetime(value)
        except ValueError:
            raise self.fail('invalid-format')


class TimeOfDay(marshmallow.Schema):
    """
    google.type.TimeOfDay compatible structure
    """

    hours = Int(missing=0, validate=Range(min=0, max=23))
    minutes = Int(missing=0, validate=Range(min=0, max=59))
    seconds = Int(missing=0, validate=Range(min=0, max=59))
    nanos = Int(missing=0, validate=Range(min=0, max=999999999))


class RestartSchema(marshmallow.Schema):
    """
    Class inherited from marshmallow.Schema
    with restart flags support
    """

    @post_load
    def fill_restart(self, data):
        """
        Hook to fill context['restart']
        """
        for name, field in self.fields.items():
            # Do nothing if any of child schemas already set restart
            if self.context.get('restart'):
                break

            attr_name = field.attribute if field.attribute else name
            if not data or attr_name not in data:
                continue

            # Handle direct fields
            if field.metadata.get('restart'):
                field.root.context['restart'] = True
                break

            # Handle direct nested schemas
            if getattr(field, 'schema', None):
                if field.schema.context.get('restart'):
                    field.root.context['restart'] = True
                    break

            # Handle iterable containers
            elif getattr(field, 'container', None):
                if field.container.metadata.get('restart'):
                    field.root.context['restart'] = True
                    break
                # Handle nested schemas in containers
                if getattr(field.container, 'schema', None):
                    if field.container.schema.context.get('restart'):
                        field.root.context['restart'] = True
                        break


class DynamicDefaultSchema(marshmallow.Schema):
    """
    Marshmallow Schema + dynamic default calculation support via a callable.
    """

    def __init__(self, *args, instance_type=None, version=None, disk_size=None, **kwargs):
        super().__init__(*args, **kwargs)
        # Use public attributes here because Marshmallow
        # apparently resets its own context (self.context)
        # in a very obscure fashion.
        self.instance_type = instance_type
        self.version = version
        self.disk_size = disk_size

    def _pass_context(self, schema_obj: marshmallow.Schema) -> None:
        """Pass instance types, version etc onto descendants"""
        schema_obj.instance_type = self.instance_type
        schema_obj.version = self.version

    @pre_dump
    def calculate_defaults(self, _) -> None:
        """
        Calculates defaults:
         - calls schema`s dump() method to fill nested structures
         - calls custom `variable_default` callable to establish
           defaults that depend on instance type and/or version
        """
        # Disable special behaviour if instance-type or version
        # are not passed.
        if self.instance_type is None and self.version is None:
            return
        # Save a few calls in case same schema is reused.
        visited = set()  # type: set
        for _, field in sorted(self.fields.items()):
            # This field is a descendant schema.
            if getattr(field, 'schema', None):
                #  pylint: disable=cell-var-from-loop
                schema = field.schema
                # Skip schema if it has defaults set already.
                if id(schema) in visited:
                    continue
                # Marshmallow called __init__() already, so pass on the
                # `special` attributes.
                self._pass_context(schema)
                # Set default: just creates a nested (but empty) structure.
                # If removed, will prevent @pre_dump firing when it does have
                # a default (see below)
                field.default = schema.dump({}).data
                visited.add(id(schema))
            # This is also a descendant, but in a container, e.g. List
            elif getattr(field, 'container', None):
                if getattr(field.container, 'schema', None):
                    schema_orig = field.container.schema
                    # Skip schema if it has defaults set already.
                    if id(schema_orig) in visited:
                        continue
                    # This copy() trickery here is to prevent caching.
                    # Somehow Marshmallow can cache nested container
                    # objects even across multiple instances
                    # -- or so it seems (see test_validation.py for more):
                    # (Pdb) test = SampleDefaultSchema()
                    # (Pdb) test2 = SampleDefaultSchema()
                    # (Pdb) test is test2
                    # False
                    # (Pdb) test.fields['containerized'].container.schema
                    # <Level1Schema(many=False, strict=False)>
                    # (Pdb) test.fields['containerized'].container.schema is \
                    # (Pdb) test2.fields['containerized'].container.schema
                    # True
                    # WTF???
                    #  pylint: disable=cell-var-from-loop
                    schema = copy.deepcopy(schema_orig)
                    self._pass_context(schema)
                    # Note the cast to a `list`
                    field.default = lambda: [schema.dump({}).data]
                    visited.add(id(schema_orig))
            # This is a plain field.
            else:
                default_getter = field.metadata.get('variable_default')
                if default_getter is not None:
                    field.default = default_getter(
                        instance_type=self.instance_type, version=self.version, disk_size=self.disk_size
                    )


class Schema(RestartSchema, DynamicDefaultSchema):
    """
    Schema with restart and dynamic default support.
    Unified in one class for convenience.
    """


def all_cluster_hosts_has_vtype(cid, vtype):
    """
    Return True if all hosts in cluster have given vtype
    """
    for host in metadb.get_hosts(cid=cid):
        if host['vtype'] != vtype:
            return False
    return True


class AbstractURIValidator(ABC):
    """
    Abstract URI validator.
    """

    def __init__(self, configuration: dict) -> None:
        self.config = configuration

    @abstractmethod
    def validate(self, url: str) -> None:
        """
        Validate URI.
        """


class URIValidator(AbstractURIValidator):
    """
    URI validator.
    """

    @tracing.trace('URIValidator')
    def validate(self, url: str) -> None:
        """
        Validate URI.
        """
        try:
            response = requests.get(
                url,
                headers={
                    'X-Request-Id': get_x_request_id(),
                },
                verify=self.config.get('ca_path', True),
                timeout=10,
                stream=True,
                allow_redirects=False,
            )

            if response.is_redirect:
                log_warn('Got redirect URI: \'%s\' redirects to \'%s\'', url, response.headers['location'])
                raise ValidationError('Redirect URI is not allowed')

        except RequestException:
            log_warn('URI is unreachable', exc_info=True)
            raise ValidationError('URI is unreachable')


def uri_validator() -> AbstractURIValidator:
    """
    Return configured URI validator.
    """
    validation_config = config.external_uri_validation()
    return validation_config.get('validator', URIValidator)(validation_config.get('validator_config', {}))


def validate_size_in_kb(size):
    if size % 1024 != 0:
        raise ValidationError('{0} should be divisible by 1024.'.format(size))
    return True


def validate_service_account(service_account_id):
    """
    Authenticate service account
    """
    logger = logging.LoggerAdapter(
        logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER']),
        extra={
            'request_id': get_x_request_id(),
        },
    )
    try:
        provider = current_app.config['AUTH_PROVIDER'](current_app.config)
        logger.info('Starting auth with %s', provider)
        token = request.auth_context['token']
        provider.authorize('iam.serviceAccounts.use', token, sa_id=service_account_id)
    except AuthError as exc:
        logger.info('Rejecting due to {exc!r}'.format(exc=exc))
        abort(
            403,
            message='You do not have permission to access the requested service account '
            'or service account does not exist',
        )


def validate_hosts(specs: List[dict], resources: RequestedHostResources, role: str, cluster_type: str):
    validate_hosts_count(
        cluster_type=cluster_type,
        role=role,  # pylint: disable=no-member
        resource_preset_id=resources.resource_preset_id,
        disk_type_id=resources.disk_type_id,
        hosts_count=len(specs),
    )

    for host in specs:
        validate_host_create_resources(
            cluster_type=cluster_type,
            role=host['type'],
            resource_preset_id=resources.resource_preset_id,
            geo=host['zone_id'],
            disk_size=resources.disk_size,
            disk_type_id=resources.disk_type_id,
        )


def assign_public_ip_changed(hostname: str, assign_public_ip: bool):
    host = metadb.get_host_info(hostname)
    return host.assign_public_ip != assign_public_ip


def validate_sox_flag(vtype: str, env: str, sox_audit: bool) -> None:
    """
    Validate sox_audit
    """
    if vtype.lower() != VTYPE_PORTO.lower() or env.lower() != ENV_PROD.lower():
        return

    if not sox_audit:
        raise DbaasClientError(
            'IDM flag is required. To use cluster without flag you should get an approval in MDBSUPPORT ticket.'
        )
