# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster delete
"""

from flask import g
from flask.views import MethodView

from . import API, marshal, parse_kwargs
from .operations import render_operation_v1
from .schemas.operations import OperationSchemaV1
from ..core.auth import AuthError, check_auth, check_action
from ..core.exceptions import ClusterDeleteProtectionError
from ..utils import metadb
from ..utils.cluster.get import get_subclusters, lock_cluster
from ..utils.identity import get_folder_by_cluster_id
from ..utils.register import DbaasOperation, Resource, get_request_handler


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>')
class DeleteClusterV1(MethodView):
    """Delete database cluster"""

    @parse_kwargs.with_resource(Resource.CLUSTER, DbaasOperation.STOP)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.CLUSTER,
        operation=DbaasOperation.DELETE,
    )
    def delete(self, cluster_type, cluster_id, **db_specific_params):
        """
        Delete cluster
        """
        cluster = lock_cluster(cluster_id, cluster_type)

        if cluster['deletion_protection']:
            try:
                check_action('mdb.all.forceDelete', g.folder['folder_ext_id'], g.cloud['cloud_ext_id'])
            except AuthError:
                raise ClusterDeleteProtectionError()

        subclusters = get_subclusters(cluster)
        handler = get_request_handler(cluster_type, Resource.CLUSTER, DbaasOperation.DELETE)
        hosts, undeleted_hosts = metadb.delete_cluster(cluster_id)
        operation = handler(
            cluster=cluster,
            subclusters=subclusters,
            hosts=hosts,
            undeleted_hosts=undeleted_hosts,
            **db_specific_params,
        )

        metadb.complete_cluster_change(cluster_id)
        g.metadb.commit()
        return render_operation_v1(operation, cluster_type)


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>/subclusters/<string:subcluster_id>')
class DeleteSubclusterV1(MethodView):
    """Delete database subcluster"""

    @parse_kwargs.with_resource(Resource.SUBCLUSTER, DbaasOperation.DELETE)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.SUBCLUSTER,
        operation=DbaasOperation.DELETE,
    )
    def delete(self, cluster_type, cluster_id, subcluster_id, **db_specific_params):
        """
        Delete subcluster
        """
        handler = get_request_handler(cluster_type, Resource.SUBCLUSTER, DbaasOperation.DELETE)
        cluster = lock_cluster(cluster_id, cluster_type)
        operation = handler(
            cluster=cluster,
            subcid=subcluster_id,
            **db_specific_params,
        )
        metadb.complete_cluster_change(cluster_id)
        g.metadb.commit()
        return render_operation_v1(operation, cluster_type)
