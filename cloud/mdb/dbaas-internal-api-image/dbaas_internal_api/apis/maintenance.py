# -*- coding: utf-8 -*-
"""
DBaaS Internal API maintenance management
"""
from flask.views import MethodView
from typing import Any

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..core.exceptions import DbaasNotImplementedError, UnsupportedHandlerError
from ..utils import metadb
from ..utils.cluster.get import get_cluster_info_assert_exists
from ..utils.idempotence import supports_idempotence
from ..utils.identity import get_folder_by_cluster_id
from ..utils.register import DbaasOperation, Resource, get_request_handler
from .operations import render_operation_v1
from .schemas.operations import OperationSchemaV1
from .schemas.maintenance import RescheduleMaintenanceRequestSchemaV1


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<cluster_id>:rescheduleMaintenance')
class RescheduleMaintenanceClusterV1(MethodView):
    """
    Reschedule maintenance for cluster
    """

    @parse_kwargs.with_schema(RescheduleMaintenanceRequestSchemaV1)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.MAINTENANCE,
        operation=DbaasOperation.RESCHEDULE,
    )
    @supports_idempotence
    def post(
        self, cluster_id: str, cluster_type: str, version: Any, **db_specific  # pylint: disable=unused-argument
    ) -> dict:
        """
        Reschedule maintenance for cluster
        """
        with metadb.commit_on_success():
            try:
                handler = get_request_handler(cluster_type, Resource.MAINTENANCE, DbaasOperation.RESCHEDULE)
            except UnsupportedHandlerError:
                raise DbaasNotImplementedError('Rescheduling maintenance for {0} not implemented'.format(cluster_type))

            cluster_info = get_cluster_info_assert_exists(cluster_id, cluster_type)
            operation = handler(cluster_info, **db_specific)
            return render_operation_v1(operation)
