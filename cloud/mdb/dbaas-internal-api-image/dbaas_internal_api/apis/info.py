# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster list
"""
from typing import List

from flask import current_app, g
from flask.views import MethodView

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..core.exceptions import UnsupportedHandlerError
from ..health import MDBH
from ..utils import metadb
from ..utils.cluster.get import get_all_clusters_essence_in_folder, get_cluster_info
from ..utils.filters_parser import Filter
from ..utils.identity import get_folder_by_cluster_id
from ..utils.infra import Flavors
from ..utils.pagination import Column, supports_pagination
from ..utils.register import DbaasOperation, Resource, get_cluster_traits, get_request_handler, VersionsColumn
from ..utils.types import ClusterStatus, MaintenanceOperation, MaintenanceWindow
from .filters import get_name_filter
from .schemas.cluster import ListClustersRequestSchemaV1, ListGenericClustersResponseSchemaV1


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>')
class GetClusterV1(MethodView):
    """Get specific cluster info."""

    @marshal.with_resource(Resource.CLUSTER, DbaasOperation.INFO)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.CLUSTER,
        operation=DbaasOperation.INFO,
    )
    def get(
        self,
        cluster_type: str,
        cluster_id: str,
        version: str = None,  # pylint: disable=unused-argument
    ):
        """Get cluster info."""
        cluster = get_cluster_info(cluster_type=cluster_type, cluster_id=cluster_id)
        return render_typed_cluster(cluster._asdict())


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters')
class ListClustersV1(MethodView):
    """Get list of clusters."""

    @parse_kwargs.with_schema(ListClustersRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_resource(Resource.CLUSTER, DbaasOperation.LIST)
    @check_auth(resource=Resource.CLUSTER, operation=DbaasOperation.LIST)
    @supports_pagination(items_field='clusters', columns=(Column(field='name', field_type=str),))
    def get(
        self,
        cluster_type: str,
        limit: int,
        page_token_name: str = None,
        filters: List[Filter] = None,
        folder_id: str = None,  # pylint: disable=unused-argument
        version: str = None,  # pylint: disable=unused-argument
    ):
        """Get list of existing database clusters"""
        return prepare_clusters_list(filters, limit, page_token_name, render_typed_cluster, cluster_type)


@API.resource('/mdb/<version>/clusters')
class ListGenericClustersV1(MethodView):
    """Get generic list of clusters."""

    @parse_kwargs.with_schema(ListClustersRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_schema(ListGenericClustersResponseSchemaV1)
    @check_auth(explicit_action='mdb.all.read')
    @supports_pagination(items_field='clusters', columns=(Column(field='name', field_type=str),))
    def get(
        self,
        limit: int,
        page_token_name: str = None,
        filters: List[Filter] = None,
        folder_id: str = None,  # pylint: disable=unused-argument
        version: str = None,  # pylint: disable=unused-argument
    ):
        """Get list of existing database clusters"""
        return prepare_clusters_list(filters, limit, page_token_name, render_generic_cluster, cluster_type=None)


def prepare_clusters_list(filters, limit, page_token_name, render_func, cluster_type):
    """Effectivly prepare list of clusters with all params"""
    cluster_name = get_name_filter(filters)
    # single builk request to metadb
    clusters = get_all_clusters_essence_in_folder(
        cluster_type, limit=limit, page_token_name=page_token_name, cluster_name=cluster_name
    )
    versions = None
    cluster_pillar = None
    if cluster_type:
        pillar = metadb.get_cluster_type_pillar(cluster_type)
        cluster_pillar = pillar['data'] if pillar else None
        traits = get_cluster_traits(cluster_type)
        cids = [cluster['cid'] for cluster in clusters]
        if traits.versions_component == VersionsColumn.cluster:
            versions = metadb.get_clusters_versions(traits.versions_component, cids)
        elif traits.versions_component == VersionsColumn.subcluster:
            versions = metadb.get_subclusters_versions(traits.versions_component, cids)
    fqdns = []
    for cluster in clusters:
        if versions:
            cluster['versions'] = versions[cluster['cid']]
        if cluster_pillar:
            cluster['cluster_pillar_default'] = cluster_pillar
        for info in cluster['hosts_info']:
            fqdns.append(info['fqdn'])

    flavors = Flavors()
    return [render_func(c, flavors) for c in clusters]


def render_typed_cluster(cluster, flavors=None):
    """
    Assemble cluster specification for specific DB type.
    """
    charts_handler = get_request_handler(cluster['type'], Resource.CLUSTER, DbaasOperation.CHARTS)
    monitoring = charts_handler(cluster, cluster['value'])
    config_handler = get_request_handler(cluster['type'], Resource.CLUSTER, DbaasOperation.INFO)
    config = config_handler(cluster, flavors)

    render = render_generic_cluster(cluster, flavors)
    render['monitoring'] = monitoring
    render['config'] = config

    try:
        extra_info_handler = get_request_handler(cluster['type'], Resource.CLUSTER, DbaasOperation.EXTRA_INFO)
        extra_info = extra_info_handler(cluster)
        render.update(extra_info)
    except UnsupportedHandlerError:
        pass

    return render


def render_generic_cluster(cluster, flavors=None):
    """
    Assemble generic cluster specification.
    """
    # Unmanaged clusters are not storing health in MDB health
    if cluster['type'] in current_app.config['UNMANAGED_CLUSTER_TYPES']:
        health_handler = get_request_handler(cluster['type'], Resource.CLUSTER, DbaasOperation.HEALTH)
        health = health_handler(cluster)
    else:
        health = MDBH.health_by_cid(cluster['cid'])

    render = render_cluster(cluster)
    render['type'] = get_cluster_traits(cluster['type']).name
    render['health'] = health
    return render


def render_cluster(cluster: dict):
    """
    Assemble basic cluster specification.
    """

    res = {
        'id': cluster['cid'],
        'labels': cluster['labels'],
        'description': cluster['description'] or '',
        'folder_id': g.folder['folder_ext_id'],
        'created_at': cluster['created_at'],
        'name': cluster['name'],
        'environment': cluster['env'],
        'network_id': cluster['network_id'],
        'deletion_protection': cluster['deletion_protection'],
    }

    if 'maintenance_window' in cluster:
        res['maintenance_window'] = cluster['maintenance_window']
    else:
        res['maintenance_window'] = MaintenanceWindow.make(cluster)

    if 'planned_operation' in cluster:
        res['planned_operation'] = cluster['planned_operation']
    else:
        res['planned_operation'] = MaintenanceOperation.make(cluster, res['maintenance_window'])

    if 'user_sgroup_ids' in cluster:
        res['user_sgroup_ids'] = cluster['user_sgroup_ids']

    if 'host_group_ids' in cluster:
        res['host_group_ids'] = cluster['host_group_ids']

    if 'status' in cluster:
        res['status'] = cluster['status']

    return res


def deduce_cluster_health(hosts, status, hosthenum, clusterhenum):
    """
    Calculates cluster health using cluster-specific enums
    """

    # MDB-3622, MDB-4021
    if status not in [ClusterStatus.running, ClusterStatus.modifying]:
        return clusterhenum.unknown

    aggregate = {
        hosthenum.alive: 0,
        hosthenum.degraded: 0,
        hosthenum.dead: 0,
        hosthenum.unknown: 0,
    }

    for host in hosts:
        aggregate[host.get('health', hosthenum.unknown)] += 1

    # If for some reason len(hosts) == 0, than report 'unknown'
    if aggregate[hosthenum.unknown] == len(hosts):
        return clusterhenum.unknown
    if aggregate[hosthenum.alive] == len(hosts):
        return clusterhenum.alive
    if aggregate[hosthenum.dead] == len(hosts):
        return clusterhenum.dead

    return clusterhenum.degraded
