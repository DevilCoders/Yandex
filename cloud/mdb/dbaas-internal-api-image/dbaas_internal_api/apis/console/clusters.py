# -*- coding: utf-8 -*-
"""
DBaaS Internal API console endpoints
"""

from flask import g
from flask.views import MethodView

from .. import API, marshal, parse_kwargs
from ...core.auth import check_auth
from ...core.exceptions import DbaasNotImplementedError, UnsupportedHandlerError
from ...utils import config, metadb
from ...utils.apispec import schema_to_jsonschema
from ...utils.register import (
    DbaasOperation,
    Resource,
    get_cluster_traits,
    get_request_handler,
    get_request_schema,
    get_traits,
)
from ...utils.validation import is_decommissioning_geo
from ...utils.version import get_available_versions, get_default_version, get_valid_versions
from ..schemas.console import (
    ClustersStatsRequestSchemaV1,
    ClustersStatsResponseSchemaV1,
    ListClustersConfigRequestSchemaV1,
    ListClusterTypesByCloudRequestSchemaV1,
    ListClusterTypesByFolderRequestSchemaV1,
    ListClusterTypesResponseSchemaV1,
)


@API.resource('/mdb/<ctype:cluster_type>/<version>/console/clusters:stats')
class ClustersStatsInFolderV1(MethodView):
    """
    Get cluster stats.
    """

    @parse_kwargs.with_schema(ClustersStatsRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_schema(ClustersStatsResponseSchemaV1)
    @check_auth(explicit_action='mdb.all.read')
    def get(self, cluster_type, **_):
        """
        Get cluster stats.
        """
        # check that we have required feature_flags
        get_cluster_traits(cluster_type)
        count = metadb.get_clusters_count_in_folder(cluster_type=cluster_type, folder_id=g.folder['folder_id'])
        return {
            'clusters_count': count,
        }


def _build_cluster_type_resp(cluster_type: str) -> dict:
    """
    Constructs cluster type description response
    """
    traits = get_cluster_traits(cluster_type)
    return {
        'type': traits.name,
        'versions': get_valid_versions(cluster_type),
    }


@API.resource('/mdb/<version>/console/clusters:types')
class ClusterTypeByFolderListV1(MethodView):
    """
    List cluster types by folder
    """

    @parse_kwargs.with_schema(ListClusterTypesByFolderRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_schema(ListClusterTypesResponseSchemaV1)
    @check_auth(explicit_action='mdb.all.read')
    def get(self, **_):
        """
        List all cluster types by folder
        """

        return {
            'cluster_types': [_build_cluster_type_resp(cluster_type) for cluster_type in sorted(get_traits())],
        }


@API.resource('/mdb/<version>/console/cloud-clusters:types')
class ClusterTypeByCloudListV1(MethodView):
    """
    List cluster types by cloud
    """

    @parse_kwargs.with_schema(ListClusterTypesByCloudRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_schema(ListClusterTypesResponseSchemaV1)
    @check_auth(explicit_action='mdb.clusterInfo.read', entity_type='cloud')
    def get(self, **_):
        """
        List all cluster types by cloud
        """

        return {
            'cluster_types': [_build_cluster_type_resp(cluster_type) for cluster_type in sorted(get_traits())],
        }


@API.resource('/mdb/<ctype:cluster_type>/<version>/console/clusters:config')
class ClustersConfigV1(MethodView):
    """
    Get cluster creation config.
    """

    @parse_kwargs.with_schema(ListClustersConfigRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_resource(Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
    @check_auth(resource=Resource.CONSOLE_CLUSTERS_CONFIG, operation=DbaasOperation.INFO)
    def get(self, cluster_type, **_):
        """
        Return cluster creation config.
        """

        resource_presets = _get_presets(cluster_type)
        default_resources = config.get_console_default_resources(cluster_type)
        host_types = _normalize_config(resource_presets, default_resources)
        traits = get_cluster_traits(cluster_type)

        res = {
            'cluster_name': traits.cluster_name,
            'password': traits.password,
            'host_count_limits': _get_host_count_limits(resource_presets),
            'host_types': host_types,
            'versions': get_valid_versions(cluster_type),
            'available_versions': get_available_versions(cluster_type),
            'default_version': get_default_version(cluster_type),
        }

        if hasattr(traits, 'db_name'):
            res['db_name'] = traits.db_name

        if hasattr(traits, 'user_name'):
            res['user_name'] = traits.user_name

        # This is for those specs that do not have host types but only resource
        # presets. Marshmallow should sort it out which one to output
        # (host_types or resource presets)

        # TODO: remove mongodb hack after st/CLOUDFRONT-395
        if len(host_types) == 1 or cluster_type == 'mongodb_cluster':
            res['resource_presets'] = host_types[0]['resource_presets']
            res['default_resources'] = host_types[0]['default_resources']

        # Add cluster type specific fields
        try:
            handler = get_request_handler(cluster_type, Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
            handler(res)
        except UnsupportedHandlerError:
            pass

        return res


def generate_cluster_resource_configs(config_map):
    """
    Generates and registers console config api handles for cluster resources
    """
    for resource, operations in config_map.items():
        for operation in operations:
            add_handler(resource, operation)


def add_handler(resource, operation):
    """
    Generate and register console config api handle
    """
    camel_cased_operation = ''.join(x.capitalize() for x in str(operation.value).split('-'))
    camel_cased = '{resource}{operation}'.format(resource=str(resource.value), operation=camel_cased_operation)
    schema_name = '{resource}{operation}'.format(
        resource=str(resource.value).capitalize(), operation=camel_cased_operation
    )
    url = f'/mdb/<ctype:cluster_type>/<version>/console/clusters:{camel_cased}Config'

    @parse_kwargs.with_schema(ListClustersConfigRequestSchemaV1, parse_kwargs.Locations.query)
    @check_auth(explicit_action='mdb.all.read', resource=resource, operation=operation)
    def _handle(_self, cluster_type, **_):
        try:
            schema = get_request_schema(cluster_type, resource, operation)
        except KeyError:
            raise DbaasNotImplementedError(f'Config handler for {camel_cased} is not implemented')
        return schema_to_jsonschema(schema, schema_name)

    api_resource = type(f'AutoConsoleHandler-{resource}-{operation}', (MethodView,), {'get': _handle})

    API.add_resource(api_resource, url)


CONFIG_RESOURCE_MAP = {
    Resource.CLUSTER: [
        DbaasOperation.CREATE,
        DbaasOperation.MODIFY,
        DbaasOperation.CREATE_DICTIONARY,
    ],
    Resource.DATABASE: [DbaasOperation.CREATE, DbaasOperation.MODIFY],
    Resource.USER: [DbaasOperation.CREATE, DbaasOperation.MODIFY],
}

generate_cluster_resource_configs(CONFIG_RESOURCE_MAP)


def _get_host_count_limits(resource_presets):
    """
    Returns host count limits in suitable format

    This function is needed for compability with current format.
    It should be removed or heavily modified after CLOUDFRONT-920
    """
    min_host_count = min([i['min_hosts'] for i in resource_presets])
    max_host_count = max([i['max_hosts'] for i in resource_presets])

    disk_type_ids = set()
    for i in resource_presets:
        disk_type_ids.add(i['disk_type_id'])

    min_disks_count_for_disk_type = []
    for disk_type in disk_type_ids:
        min_hosts = min([i['min_hosts'] for i in resource_presets if i['disk_type_id'] == disk_type])
        if min_hosts > min_host_count:
            min_disks_count_for_disk_type.append(
                {
                    'disk_type_id': disk_type,
                    'min_host_count': min_hosts,
                }
            )

    return {
        'min_host_count': min_host_count,
        'max_host_count': max_host_count,
        'host_count_per_disk_type': min_disks_count_for_disk_type,
    }


def _get_presets(cluster_type):
    """
    Gets resource presets from MetaDB
    """
    presets = metadb.get_resource_presets_by_cluster_type(cluster_type=cluster_type)
    g.metadb.commit()
    # remove decommissioning geos
    generation_names = config.get_generation_names()
    ret = []
    for preset in presets:
        if not is_decommissioning_geo(preset['geo_name']):
            preset['generation_name'] = generation_names[preset['generation']]
            ret.append(preset)
    return ret


def _normalize_config(cfgs, defaults):
    """
    Normalizes resource presets.
    """
    host_types = {}
    for cfg in cfgs:
        _normalize_host_type(cfg, host_types)

    # Convert dicts to lists
    host_types = list(host_types.values())

    for host_type in host_types:
        host_type['resource_presets'] = list(host_type['resource_presets'].values())

        for preset in host_type['resource_presets']:
            preset['zones'] = list(preset['zones'].values())

        host_type['default_resources'] = defaults[host_type['type']]

    return host_types


def _normalize_host_type(cfg, host_types):
    """
    Normalizes host types.
    """
    host_type_id = cfg['role']
    host_type = host_types.get(host_type_id, None)
    if host_type is None:
        host_type = {
            'type': host_type_id,
            'resource_presets': {},
        }
        host_types[host_type_id] = host_type

    _normalize_preset(cfg, host_type)


def _normalize_preset(cfg, host_type):
    """
    Normalizes resource presets.
    """
    preset_id = cfg['preset_id']
    preset = host_type['resource_presets'].get(preset_id, None)
    if preset is None:
        preset = {
            'preset_id': str(preset_id),
            'cpu_limit': cfg['cpu_limit'],
            'cpu_fraction': cfg['cpu_fraction'],
            'gpu_limit': cfg['gpu_limit'],
            'memory_limit': cfg['memory_limit'],
            'type': cfg['type'],
            'generation': cfg['generation'],
            'generation_name': cfg['generation_name'],
            'zones': {},
            'decommissioning': str(preset_id) in config.get_decommissioning_flavors(),
        }
        host_type['resource_presets'][preset_id] = preset

    _normalize_zone(cfg, preset)


def _normalize_zone(cfg, preset):
    """
    Normalizes zones.
    """
    zone_id = cfg['geo_name']
    zone = preset['zones'].get(zone_id, None)
    if zone is None:
        zone = {
            'zone_id': str(zone_id),
            'disk_types': [],
        }
        preset['zones'][zone_id] = zone

    _normalize_disk_type(cfg, zone)


def _normalize_disk_type(cfg, zone):
    """
    Normalizes disk types.
    """
    disk_type = {
        'disk_type_id': str(cfg['disk_type_id']),
        'min_hosts': cfg['min_hosts'],
        'max_hosts': cfg['max_hosts'],
    }

    if cfg['disk_sizes']:
        disk_type['disk_sizes'] = {
            'sizes': cfg['disk_sizes'],
        }
    else:
        range = cfg['disk_size_range']
        disk_type['disk_size_range'] = {
            'min': (range.lower if range.lower_inc else range.lower + 1),
            'max': (range.upper if range.upper_inc else range.upper - 1),
        }

    zone['disk_types'].append(disk_type)
