# -*- coding: utf-8 -*-
"""
DBaaS Internal API cluster modify
"""

from datetime import timedelta
from dbaas_internal_api.modules.redis.constants import MY_CLUSTER_TYPE
from flask import g
from flask.views import MethodView
from flask_restful import abort
from typing import Optional

from . import API, marshal, parse_kwargs
from ..core.auth import check_auth
from ..core.exceptions import NoChangesError
from ..core.types import Operation
from ..utils import metadb
from ..utils.cluster.get import lock_cluster
from ..utils.cluster.update import update_cluster_labels, update_cluster_metadb_parameters, update_cluster_name
from ..utils.idempotence import supports_idempotence
from ..utils.identity import get_folder_by_cluster_id
from ..utils.metadata import ModifyClusterMetadata
from ..utils.operation_creator import create_finished_operation, create_operation
from ..utils.register import ClusterTraits, DbaasOperation, Resource, get_request_handler, get_cluster_traits
from ..utils.types import ClusterInfo, ClusterStatus, LabelsDict, MaintenanceWindowDict
from ..utils.validation import check_cluster_not_in_status
from .operations import render_operation_v1
from .schemas.operations import OperationSchemaV1


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>')
class UpdateClusterV1(MethodView):
    """Modify database cluster"""

    @parse_kwargs.with_resource(Resource.CLUSTER, DbaasOperation.MODIFY)
    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.CLUSTER,
        operation=DbaasOperation.MODIFY,
    )
    @supports_idempotence
    def patch(
        self,
        cluster_type,
        cluster_id,
        labels: LabelsDict = None,
        description: str = None,
        name: Optional[str] = None,
        maintenance_window: Optional[MaintenanceWindowDict] = None,
        deletion_protection: Optional[bool] = None,
        **db_specific_params
    ):
        """
        Facilitate changes in cluster.
        """
        # NOTE: see also utils/modify.py

        # Here we lock cloud row in Exclusive mode as top-level entry
        # point of any cluster changes (to prevent any race conditions
        # with concurrent updates of pillar, hosts etc, because
        # now we initially read pillar from db to analyze and
        # replace his value).
        # Also flavor may be changed in this
        # request and we need to update cloud resources.
        # Maybe we need to weaken this lock and apply it explicitly
        # on individual resources, such as pillar and others.
        if not metadb.lock_cloud():
            # HTTP 423 -- Locked
            # https://tools.ietf.org/html/rfc4918#section-9.9.4
            abort(423, message='Conflicting operation in progress, try again later')

        # Lock cluster, cause we want initiate cluster change
        cluster_obj = lock_cluster(cluster_id, cluster_type)

        handler = get_request_handler(cluster_type, Resource.CLUSTER, DbaasOperation.MODIFY)

        metadata_changes = False
        # redis cluster does not support metadata operation on rename
        if cluster_type != MY_CLUSTER_TYPE:
            # need run metadata operation
            metadata_changes = self._update_cluster_metadata(cluster_obj=cluster_obj, name=name, labels=labels)

        # update common metadb fields:
        have_metadb_changes = update_cluster_metadb_parameters(
            cluster=cluster_obj,
            description=description,
            maintenance_window=maintenance_window,
            deletion_protection=deletion_protection,
        )

        try:
            operation: Operation = handler(
                cluster_obj,  # required positional argument
                labels=labels,
                description=description,
                name=name,
                maintenance_window=maintenance_window,
                deletion_protection=deletion_protection,
                **db_specific_params
            )
        except NoChangesError as err:
            cluster_trait = get_cluster_traits(cluster_type)
            if metadata_changes:
                operation = self._create_operation(
                    cluster_obj=cluster_obj,
                    cluster_trait=cluster_trait,
                    operation_type='metadata',
                    task_type='metadata',
                )
            elif have_metadb_changes:
                operation = create_finished_operation(
                    operation_type=cluster_trait.operations['modify'],
                    metadata=ModifyClusterMetadata(),
                    cid=cluster_obj['cid'],
                )
            else:
                raise err

        metadb.complete_cluster_change(cluster_id)
        g.metadb.commit()
        return render_operation_v1(operation)

    def _update_cluster_metadata(self, cluster_obj: dict, name: Optional[str], labels: Optional[LabelsDict]) -> bool:
        labels_updated = update_cluster_labels(cluster_obj, labels)
        name_updated = update_cluster_name(cluster_obj, name)
        return labels_updated or name_updated

    def _create_operation(
        self, cluster_obj: dict, cluster_trait: ClusterTraits, operation_type: str, task_type: str
    ) -> Operation:
        check_cluster_not_in_status(ClusterInfo.make(cluster_obj), ClusterStatus.stopped)

        return create_operation(
            task_type=cluster_trait.tasks[task_type],
            operation_type=cluster_trait.operations[operation_type],
            metadata=ModifyClusterMetadata(),
            cid=cluster_obj['cid'],
            time_limit=timedelta(hours=1),
        )
