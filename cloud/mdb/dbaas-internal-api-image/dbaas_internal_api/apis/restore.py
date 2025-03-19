# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster create
"""
from typing import Any

from flask.views import MethodView
from flask_restful import abort

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..core.exceptions import DbaasNotImplementedError, UnsupportedHandlerError
from ..utils import metadb
from ..utils.cluster.get import get_cluster_info_assert_exists
from ..utils.idempotence import supports_idempotence
from ..utils.register import DbaasOperation, Resource, get_request_handler
from ..utils.types import ClusterDescription
from ..utils.backups import get_cluster_id
from .backups import get_cluster_backup_by_id, get_folder_by_backup_id
from .operations import render_operation_v1
from .schemas.operations import OperationSchemaV1
from .types import LabelsDict


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters:restore')
class RestoreFromBackupV1(MethodView):
    """Create new database cluster from backup"""

    @parse_kwargs.with_resource(Resource.CLUSTER, DbaasOperation.RESTORE)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_backup_id,
        resource=Resource.CLUSTER,
        operation=DbaasOperation.RESTORE,
    )
    @supports_idempotence
    def post(  # pylint: disable=too-many-arguments
        self,
        cluster_type: str,
        version: str,  # pylint: disable=unused-argument
        _schema: Any,  # pylint: disable=unused-argument
        name: str,
        backup_id: str,
        environment: str,
        folder_id: str = None,  # pylint: disable=unused-argument
        description: str = None,
        labels: LabelsDict = None,
        **cluster_specific
    ) -> dict:
        """
        Create cluster from backup
        """

        with metadb.commit_on_success():
            try:
                handler = get_request_handler(cluster_type, Resource.CLUSTER, DbaasOperation.RESTORE)
            except UnsupportedHandlerError:
                raise DbaasNotImplementedError('Restore from backup for {0} not implemented'.format(cluster_type))

            source_cluster_id = get_cluster_id(backup_id)
            source_cluster = get_cluster_info_assert_exists(source_cluster_id, cluster_type, include_deleted=True)
            backup = get_cluster_backup_by_id(source_cluster, backup_id)

            # lock cloud, cause we need to update it's quota
            if not metadb.lock_cloud():
                abort(403)

            operation = handler(
                source_cluster=source_cluster,
                backup=backup.back,
                description=ClusterDescription(
                    name=name,
                    environment=environment,
                    description=description,
                    labels=labels,
                ),
                **cluster_specific
            )

            metadb.complete_cluster_change(operation.target_id)

            return render_operation_v1(operation)
