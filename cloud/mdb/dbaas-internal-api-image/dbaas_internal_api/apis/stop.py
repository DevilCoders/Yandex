# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster stop
"""

from flask.views import MethodView

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..core.exceptions import DbaasNotImplementedError, UnsupportedHandlerError
from ..utils import metadb
from ..utils.cluster.get import locked_cluster_info
from ..utils.idempotence import supports_idempotence
from ..utils.identity import get_folder_by_cluster_id
from ..utils.register import DbaasOperation, Resource, get_request_handler
from ..utils.types import VTYPE_COMPUTE, ClusterStatus
from ..utils.validation import all_cluster_hosts_has_vtype, check_cluster_not_in_status
from .operations import render_operation_v1
from .schemas.operations import OperationSchemaV1


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<cluster_id>:stop')
class StopClusterV1(MethodView):
    """
    Stop database cluster
    """

    @parse_kwargs.with_resource(Resource.CLUSTER, DbaasOperation.STOP)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.CLUSTER,
        operation=DbaasOperation.STOP,
    )
    @supports_idempotence
    def post(self, cluster_id: str, cluster_type: str, **db_specific_params) -> dict:
        """
        Stop cluster
        """
        with metadb.commit_on_success():
            # lock cluster, cause don't want
            # concurrent stop calls
            cluster_info = locked_cluster_info(cluster_id=cluster_id, cluster_type=cluster_type)

            try:
                handler = get_request_handler(cluster_type, Resource.CLUSTER, DbaasOperation.STOP)
            except UnsupportedHandlerError:
                raise DbaasNotImplementedError('Stop for {0} not implemented'.format(cluster_type))

            check_cluster_not_in_status(cluster_info, ClusterStatus.stopped)

            if not all_cluster_hosts_has_vtype(cluster_id, VTYPE_COMPUTE):
                raise DbaasNotImplementedError('Stop for {0} not implemented in that installation'.format(cluster_type))

            operation = handler(cluster_info, **db_specific_params)
            metadb.complete_cluster_change(cluster_id)

            return render_operation_v1(operation)
