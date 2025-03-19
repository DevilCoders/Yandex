# -*- coding: utf-8 -*-
"""
DBaaS Internal API support endpoints
"""

from flask import g
from flask.views import MethodView
from flask_restful import abort

from . import API, marshal, parse_kwargs
from ..apis.schemas.fields import Str
from ..core.auth import check_auth
from ..core.exceptions import DbaasClientError, OperationNotExistsError
from ..utils import config, metadb
from ..utils.cluster.get import get_cluster_info
from ..utils.identity import get_folder_by_cluster_id, get_folder_by_operation_id
from ..utils.register import DbaasOperation, Resource, get_request_handler, get_response_schema
from .info import render_typed_cluster
from .operations import get_cluster_type_by_operation, render_operation_v1
from .schemas.operations import OperationSchemaV1
from .schemas.support import (
    ClusterInfoRequestSchemaV1,
    QuotaAction,
    QuotaInfoSchema,
    UpdateQuotaClustersRequestSchemaV1,
    UpdateQuotaHddSpaceRequestSchemaV1,
    UpdateQuotaResourcesRequestSchemaV1,
    UpdateQuotaSsdSpaceRequestSchemaV1,
)

DISPENSER_USED_ERROR = 'This environment uses dispenser for quota management'


def _get_cloud(cloud_ext_id):
    """
    Get cloud by id
    """
    return metadb.get_cloud(cloud_ext_id=cloud_ext_id)


def _upsert_cloud(cloud_ext_id, cloud):
    """
    Create new cloud if not exists (with empty quota)
    """
    if cloud is None:
        cloud = metadb.create_cloud(
            cloud_ext_id,
            {
                'cpu_quota': 0,
                'gpu_quota': 0,
                'memory_quota': 0,
                'ssd_space_quota': 0,
                'hdd_space_quota': 0,
                'clusters_quota': 0,
            },
        )
    return cloud


@API.resource('/mdb/<version>/support/quota/<string:cloud_id>')
class QuotaInfo(MethodView):
    """Get information about quota in cloud"""

    @marshal.with_schema(QuotaInfoSchema)
    @check_auth(explicit_action='mdb.all.support', entity_type='cloud')
    def get(self, version, cloud_id):  # pylint: disable=unused-argument
        """
        Get quota in cloud
        """
        cloud = metadb.get_cloud(cloud_ext_id=cloud_id)
        g.metadb.commit()
        return cloud


@API.resource('/mdb/<version>/support/quota/<string:cloud_id>/clusters')
class QuotaClusters(MethodView):
    """Update clusters quota in cloud"""

    @parse_kwargs.with_schema(UpdateQuotaClustersRequestSchemaV1)
    @marshal.with_schema(QuotaInfoSchema)
    @check_auth(explicit_action='mdb.all.support', entity_type='cloud')
    # pylint: disable=unused-argument
    def post(self, version, cloud_id, action, clusters_quota):
        """
        Update clusters quota in cloud
        """
        cloud = _get_cloud(cloud_id)
        add_clusters_count = clusters_quota
        if action == QuotaAction.add:
            cloud = _upsert_cloud(cloud_id, cloud)
        else:
            add_clusters_count = -1 * add_clusters_count
            validate_cloud_quota(cloud)
            validate_quota_change('clusters', cloud['clusters_used'], cloud['clusters_quota'], clusters_quota)

        res = metadb.update_cloud_quota(cloud_ext_id=cloud_id, add_clusters=add_clusters_count)

        g.metadb.commit()
        return res


@API.resource('/mdb/<version>/support/quota/<string:cloud_id>/ssd_space')
class QuotaSsdSpace(MethodView):
    """Update ssd space quota in cloud"""

    @parse_kwargs.with_schema(UpdateQuotaSsdSpaceRequestSchemaV1)
    @marshal.with_schema(QuotaInfoSchema)
    @check_auth(explicit_action='mdb.all.support', entity_type='cloud')
    # pylint: disable=unused-argument
    def post(self, version, cloud_id, action, ssd_space_quota):
        """
        Update ssd space quota in cloud
        """
        if config.is_dispenser_used():
            raise DbaasClientError(DISPENSER_USED_ERROR)

        cloud = _get_cloud(cloud_id)
        add_space_quota = ssd_space_quota
        if action == QuotaAction.add:
            _upsert_cloud(cloud_id, cloud)
        else:
            add_space_quota = -1 * add_space_quota

            validate_cloud_quota(cloud)
            validate_quota_change('space', cloud['ssd_space_used'], cloud['ssd_space_quota'], ssd_space_quota)

        res = metadb.update_cloud_quota(cloud_ext_id=cloud_id, add_ssd_space=add_space_quota)

        g.metadb.commit()
        return res


@API.resource('/mdb/<version>/support/quota/<string:cloud_id>/hdd_space')
class QuotaHddSpace(MethodView):
    """Update hdd space quota in cloud"""

    @parse_kwargs.with_schema(UpdateQuotaHddSpaceRequestSchemaV1)
    @marshal.with_schema(QuotaInfoSchema)
    @check_auth(explicit_action='mdb.all.support', entity_type='cloud')
    # pylint: disable=unused-argument
    def post(self, version, cloud_id, action, hdd_space_quota):
        """
        Update hdd space quota in cloud
        """
        if config.is_dispenser_used():
            raise DbaasClientError(DISPENSER_USED_ERROR)

        cloud = _get_cloud(cloud_id)
        add_space_quota = hdd_space_quota
        if action == QuotaAction.add:
            _upsert_cloud(cloud_id, cloud)
        else:
            add_space_quota = -1 * add_space_quota

            validate_cloud_quota(cloud)
            validate_quota_change('space', cloud['hdd_space_used'], cloud['hdd_space_quota'], hdd_space_quota)

        res = metadb.update_cloud_quota(cloud_ext_id=cloud_id, add_hdd_space=add_space_quota)

        g.metadb.commit()
        return res


@API.resource('/mdb/<version>/support/quota/<string:cloud_id>/resources')
class QuotaResources(MethodView):
    """Update resources quota in cloud"""

    @parse_kwargs.with_schema(UpdateQuotaResourcesRequestSchemaV1)
    @marshal.with_schema(QuotaInfoSchema)
    @check_auth(explicit_action='mdb.all.support', entity_type='cloud')
    # pylint: disable=unused-argument
    def post(self, version, cloud_id, action, preset_id, count):
        """
        Update resources quota in cloud
        """
        if config.is_dispenser_used():
            raise DbaasClientError(DISPENSER_USED_ERROR)

        if count == 0:
            abort(422, message="count must be non-zero")

        preset = metadb.get_flavor_by_name(preset_id)
        if preset is None:
            abort(422, message="unknown resource preset")

        cpu_quota = count * preset['cpu_guarantee']
        gpu_quota = count * preset['gpu_limit']
        memory_quota = count * preset['memory_guarantee']
        cloud = _get_cloud(cloud_id)

        add_quota_sign = 1
        if action == QuotaAction.add:
            cloud = _upsert_cloud(cloud_id, cloud)
        else:
            add_quota_sign = -1

            # todo looks like it should be validated in spite of quota sign
            validate_cloud_quota(cloud)
            validate_quota_change('cpu', cloud['cpu_used'], cloud['cpu_quota'], cpu_quota)
            validate_quota_change('gpu', cloud['gpu_used'], cloud['gpu_quota'], gpu_quota)
            validate_quota_change('memory', cloud['memory_used'], cloud['memory_quota'], memory_quota)

        res = metadb.update_cloud_quota(
            cloud_ext_id=cloud_id,
            add_cpu=add_quota_sign * cpu_quota,
            add_gpu=add_quota_sign * gpu_quota,
            add_memory=add_quota_sign * memory_quota,
        )

        g.metadb.commit()
        return res


def validate_cloud_quota(cloud):
    """Validate existence of cloud quota"""

    if cloud is None:
        abort(422, message="cloud does not have any quota, can't subtract anything")


def validate_quota_change(name, current_used, current_quota, quota_change):
    """Validate cloud quota change"""

    if current_quota - quota_change < current_used:
        abort(
            422,
            message="{name} quota would become less than {name} used".format(name=name),
        )


@API.resource('/mdb/<version>/support/operations/<string:operation_id>')
class HiddenOperationStatusV1(MethodView):
    """
    Get information about particular operation (hidden-included handle)
    """

    @marshal.with_schema(OperationSchemaV1)
    @check_auth(explicit_action='mdb.all.support', folder_resolver=get_folder_by_operation_id)
    def get(
        self,
        operation_id: str,
        version: str,  # pylint: disable=unused-argument
    ):
        """
        Get a single operation (allows getting hidden operations)
        """
        with metadb.commit_on_success():
            operation = metadb.get_operation_by_id(operation_id=operation_id, include_hidden=True)

            if operation is None:
                raise OperationNotExistsError(operation_id)

            cluster_type = get_cluster_type_by_operation(operation)

            return render_operation_v1(operation=operation, cluster_type=cluster_type, expose_all=True)


def get_folder_by_support_search_query(
    *_args, cluster_id=None, vtype_id=None, fqdn=None, shard_id=None, subcid=None, **_kwargs
):
    """
    Get folder id by search params
    """
    return get_folder_by_cluster_id(
        cluster_id=get_cid_by_support_search_query(
            cluster_id=cluster_id, vtype_id=vtype_id, fqdn=fqdn, shard_id=shard_id, subcid=subcid
        )
    )


def get_cid_by_support_search_query(cluster_id=None, vtype_id=None, fqdn=None, shard_id=None, subcid=None):
    """
    Get cluster id by search params
    """
    num_params = len([x for x in (vtype_id, fqdn, shard_id, subcid, cluster_id) if x is not None])
    if num_params == 0:
        raise DbaasClientError('No search params')
    if num_params > 1:
        raise DbaasClientError('Multiple search params')
    if cluster_id is not None:
        return cluster_id
    return metadb.get_cluster_id_by_host_attrs(vtype_id=vtype_id, fqdn=fqdn, shard_id=shard_id, subcid=subcid)


@API.resource('/mdb/<version>/support/clusters/search')
class SupportClusterSearchV1(MethodView):
    """
    Find cluster
    """

    @parse_kwargs.with_schema(ClusterInfoRequestSchemaV1)
    @check_auth(explicit_action='mdb.all.support', folder_resolver=get_folder_by_support_search_query)
    def get(self, vtype_id=None, fqdn=None, shard_id=None, subcid=None, cluster_id=None, **_):
        """
        Get cluster info by host propery or cid
        """
        cluster_id = get_cid_by_support_search_query(
            cluster_id=cluster_id, vtype_id=vtype_id, fqdn=fqdn, shard_id=shard_id, subcid=subcid
        )
        cluster = metadb.get_cluster(cid=cluster_id)
        cluster_info = get_cluster_info(cluster_type=cluster['type'], cluster_id=cluster['cid'])
        typed = render_typed_cluster(cluster_info._asdict())
        out_schema = get_response_schema(cluster['type'], Resource.CLUSTER, DbaasOperation.INFO)()
        ret = out_schema.dump(typed).data
        ret['cloudId'] = g.cloud['cloud_ext_id']
        hosts_handler = get_request_handler(cluster['type'], Resource.HOST, DbaasOperation.LIST)

        # We do a little schema change at runtime to allow passing instance id here
        hosts_schema = get_response_schema(cluster['type'], Resource.HOST, DbaasOperation.LIST)()
        host_schema = hosts_schema.fields['hosts'].nested()
        host_schema.declared_fields.update({'instanceId': Str(attribute='vtype_id')})
        hosts_schema.fields['hosts'].nested = host_schema
        ret.update(hosts_schema.dump(hosts_handler(cluster=cluster)).data)

        return ret
