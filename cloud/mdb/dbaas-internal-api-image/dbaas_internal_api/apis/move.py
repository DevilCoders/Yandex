# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster move
"""

from flask import g
from flask.views import MethodView

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..core.exceptions import DbaasClientError, DbaasNotImplementedError, UnsupportedHandlerError
from ..utils import metadb
from ..utils.cluster.get import locked_cluster_info
from ..utils.idempotence import supports_idempotence
from ..utils.identity import get_folder_by_cluster_id
from ..utils.register import DbaasOperation, Resource, get_request_handler
from ..utils.types import ClusterStatus
from ..utils.validation import check_cluster_not_in_status
from .operations import render_operation_v1
from .schemas.cluster import MoveClusterRequestSchemaV1
from .schemas.operations import OperationSchemaV1


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>:move')
class MoveClusterV1(MethodView):
    """
    Move cluster
    """

    @parse_kwargs.with_schema(MoveClusterRequestSchemaV1)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.CLUSTER,
        operation=DbaasOperation.MOVE,
    )
    @supports_idempotence
    def post(self, cluster_type: str, cluster_id: str, destination_folder_id: str, **_) -> dict:
        """
        Move cluster
        """
        with metadb.commit_on_success():
            # lock cluster, cause don't want
            # concurrent move calls
            cluster_info = locked_cluster_info(cluster_id=cluster_id, cluster_type=cluster_type)
            try:
                handler = get_request_handler(cluster_type, Resource.CLUSTER, DbaasOperation.MOVE)
            except UnsupportedHandlerError:
                raise DbaasNotImplementedError('Move for {0} not implemented'.format(cluster_type))

            check_cluster_not_in_status(cluster_info, ClusterStatus.stopped)

            if destination_folder_id == g.folder['folder_ext_id']:
                raise DbaasClientError('Cluster already in folder {0}'.format(destination_folder_id))

            metadb.complete_cluster_change(cluster_id)
            return render_operation_v1(handler(cluster_info, destination_folder_id))
