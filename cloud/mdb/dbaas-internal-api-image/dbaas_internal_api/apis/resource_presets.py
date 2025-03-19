# -*- coding: utf-8 -*-
"""
DBaaS Internal API flavor info
"""

from flask import g
from flask.views import MethodView
from flask_restful import abort

from . import API, marshal, parse_kwargs
from ..utils import metadb
from ..utils.config import get_decommissioning_flavors
from ..utils.pagination import Column, supports_pagination
from ..utils.register import DbaasOperation, Resource
from ..utils.validation import is_decommissioning_geo
from .schemas.common import ListRequestSchemaV1


def _remove_decommissioning_zones(resource_preset):
    """
    Remove decommissioning zones from resource preset
    """
    for zone_id in resource_preset['zone_ids'][:]:
        if is_decommissioning_geo(zone_id):
            resource_preset['zone_ids'].remove(zone_id)
    return resource_preset


def _is_visible_resource_preset(resource):
    """
    Return true is resource preset exists at some zones
    """
    if resource is None:
        return False
    return bool(resource['zone_ids'])


def _list_resource_presets(cluster_type, page_token_cores=None, page_token_id=None, limit=None, **_):
    """
    Get list of resource presets
    """
    resource_presets = metadb.get_resource_presets(
        cluster_type=cluster_type,
        page_token_cores=page_token_cores,
        page_token_id=page_token_id,
        decommissioning_flavors=get_decommissioning_flavors(),
        limit=limit,
    )
    g.metadb.commit()
    resource_presets = (_remove_decommissioning_zones(r) for r in resource_presets)
    return [r for r in resource_presets if _is_visible_resource_preset(r)]


def _get_resource_preset(cluster_type, resource_preset_id):
    """
    Return a single resource preset for cluster type
    """
    resource_preset_obj = metadb.get_resource_preset_by_id(cluster_type, resource_preset_id)
    if not resource_preset_obj:
        abort(404, message='Resource preset is unavailable for this cluster type')
    resource_preset_obj = _remove_decommissioning_zones(resource_preset_obj)
    g.metadb.commit()
    if not _is_visible_resource_preset(resource_preset_obj):
        abort(404, message='Resource preset is unavailable for this cluster type')
    return resource_preset_obj


@API.resource('/mdb/<ctype:cluster_type>/<version>/resourcePresets')
class ResourcePresetListV1(MethodView):
    """
    List resource presets for cluster type
    """

    @parse_kwargs.with_schema(ListRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_resource(Resource.RESOURCE_PRESET, DbaasOperation.LIST)
    @supports_pagination(
        items_field='resource_presets',
        columns=(
            # The order is important! (defines parsing)
            Column(field='cores', field_type=float),
            Column(field='id', field_type=str),
        ),
    )
    def get(self, cluster_type, **kwargs):
        """
        List all instance types
        """
        return _list_resource_presets(cluster_type, **kwargs)


@API.resource('/mdb/<ctype:cluster_type>/<version>/resourcePresets/' '<resource_preset_id>')
class ResourcePresetV1(MethodView):
    """
    Get resource preset for cluster type
    """

    @marshal.with_resource(Resource.RESOURCE_PRESET, DbaasOperation.INFO)
    def get(self, cluster_type, resource_preset_id, **_):
        """
        Single resource preset for cluster type
        """
        return _get_resource_preset(cluster_type, resource_preset_id)
