# -*- coding: utf-8 -*-
"""
DBaaS Internal API task apis
"""

from datetime import datetime
from typing import Any, Dict, List, Sequence  # noqa

from flask import g
from flask.views import MethodView

from dbaas_common import tracing

from . import API, marshal, parse_kwargs
from ..core.auth import AuthError, check_action, check_auth
from ..core.exceptions import OperationNotExistsError, OperationTypeMismatchError
from ..core.types import Operation, OperationStatus
from ..utils import metadb, pagination
from ..utils.cluster.get import get_cluster_info
from ..utils.config import get_task_expose
from ..utils.identity import get_folder_by_cluster_id, get_folder_by_operation_id
from ..utils.logs import log_warn
from ..utils.register import BaseOperationDescriber, get_operations_describer, get_traits, Resource, DbaasOperation
from .operations_responser import OperationsResponser
from .schemas.common import ListRequestSchemaV1
from .schemas.operations import ListOperationsRequestSchema, OperationListSchemaV1, OperationSchemaV1


def _annotate_dict(type_name: str, obj_dict) -> dict:
    """
    Annotate dict with type

    GRPC require @type annotation for Any fields
    """
    return {
        '@type': type_name,
        **obj_dict,
    }


def _get_auth_task_expose() -> bool:
    """
    Returns True if user has mdb.all.support action permission
    """
    cached = getattr(g, 'auth_task_expose_cached', None)
    if cached is None:
        try:
            check_action(action='mdb.all.support', folder_ext_id=g.folder['folder_ext_id'])
            g.auth_task_expose_cached = True
        except AuthError:
            g.auth_task_expose_cached = False
    return g.auth_task_expose_cached


@tracing.trace('Render Operation')
def _render_operation_v1(
    operation: Operation, describer: BaseOperationDescriber, responser: OperationsResponser, expose_all: bool = False
) -> dict:
    """Render an operation object ready to be fed to schema"""

    tracing.set_tag('operation.id', operation.id)
    tracing.set_tag('operation.target.id', operation.target_id)
    tracing.set_tag('operation.status', operation.status)
    tracing.set_tag('operation.hidden', operation.hidden)
    tracing.set_tag('render.expose.all', expose_all)

    op_dict = operation._asdict()
    op_dict.pop('metadata')
    ret = {
        **op_dict,
        'done': operation.status.is_terminal(),
    }  # type: Dict[str, Any]

    desc = describer.get_description(operation)
    if desc is not None:
        ret['description'] = desc.description
        ret['metadata'] = _annotate_dict(desc.metadata.annotation, desc.metadata.metadata)
    else:
        log_warn('%r describer return None for operation %r', describer, operation)

    if operation.status.is_terminal():
        if operation.status == OperationStatus.failed:
            ret['error'] = {'code': 2, 'message': 'Unknown error', 'details': []}
            if operation.errors:
                is_support = _get_auth_task_expose()
                for error in operation.errors:
                    if expose_all or get_task_expose() or error['exposable'] or is_support:
                        ret['error']['code'] = error['code']
                        ret['error']['message'] = error['message']
                        ret['error']['details'].append({x: y for x, y in error.items() if x != 'exposable'})
        else:
            if desc is not None:
                resp = responser.get(desc.response.type, desc.response.key)
                if resp:
                    # annotate response with type
                    # if it defined
                    resp = _annotate_dict(desc.response.annotation, resp)
                ret['response'] = resp or {}
    return ret


@tracing.trace('Render Operation Sequence')
def _render_operations_seq_v1(
    operations: Sequence[Operation], cluster_type: str, expose_all: bool = False
) -> List[dict]:
    tracing.set_tag('cluster.type', cluster_type)
    tracing.set_tag('render.expose.all', expose_all)
    describer = get_operations_describer(cluster_type)
    responser = OperationsResponser(cluster_type)
    return [_render_operation_v1(o, describer, responser, expose_all) for o in operations]


def render_operation_v1(operation: Operation, cluster_type: str = None, expose_all: bool = False) -> dict:
    """Render an operation object ready to be fed to schema"""

    # *_cluster_delete operation don't return cluster_type,
    # cause cluster deleted before operation.
    # Callers should pass it directly
    op_cluster_type = cluster_type or operation.cluster_type
    if op_cluster_type is None:
        raise RuntimeError('Unable to define operation {0} cluster_type'.format(operation))
    describer = get_operations_describer(op_cluster_type)
    responser = OperationsResponser(op_cluster_type)
    return _render_operation_v1(operation, describer, responser, expose_all)


def get_operation_by_idem_id(idempotence_id):
    """Resolve operation id by idempotence id"""
    op_data = metadb.get_task_by_idempotence_id(idempotence_id=idempotence_id) or {}
    if op_data:
        return op_data['id'], bytes(op_data['request_hash'])
    return None, b''


def get_operation(operation_id, expose_all):
    """
    Get individual operation properties
    """
    operation = metadb.get_operation_by_id(operation_id=operation_id)
    if not operation:
        raise OperationNotExistsError(operation_id)
    return render_operation_v1(operation, expose_all)


_operations_paging = pagination.supports_pagination(
    items_field='operations',
    columns=(
        # The order is important! (defines parsing)
        pagination.Column(field='created_at', field_type=pagination.date),
        pagination.Column(field='id', field_type=str),
    ),
)


def _asset_cluster_has_given_type(cluster_id: str, cluster_type: str) -> None:
    get_cluster_info(cluster_id=cluster_id, cluster_type=cluster_type)


@API.resource('/mdb/<ctype:cluster_type>/<version>/operations')
class FolderOperationsListV1(MethodView):
    """Get information about folder operations"""

    @parse_kwargs.with_schema(ListOperationsRequestSchema, parse_kwargs.Locations.query)
    @marshal.with_schema(OperationListSchemaV1)
    @check_auth(resource=Resource.OPERATION, operation=DbaasOperation.LIST)
    @_operations_paging
    def get(
        self,
        cluster_type: str,
        folder_id: str,  # pylint: disable=unused-argument
        version: str,  # pylint: disable=unused-argument
        limit: int,
        page_token_id: str = None,
        page_token_created_at: datetime = None,
    ):
        """
        Get folder operataions for given cluster_type
        """
        with metadb.commit_on_success():
            operations = metadb.get_operations(
                cluster_type=cluster_type,
                limit=limit,
                page_token_id=page_token_id,
                page_token_created_at=page_token_created_at,
            )
            return _render_operations_seq_v1(operations, cluster_type)


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>/operations')
class ClusterOperationsListV1(MethodView):
    """Get information about cluster operations"""

    @parse_kwargs.with_schema(ListRequestSchemaV1, parse_kwargs.Locations.query)
    @marshal.with_schema(OperationListSchemaV1)
    @check_auth(resource=Resource.OPERATION, operation=DbaasOperation.LIST, folder_resolver=get_folder_by_cluster_id)
    @_operations_paging
    def get(
        self,
        cluster_type: str,
        cluster_id: str,
        version: str,  # pylint: disable=unused-argument
        limit: int,
        page_token_id: str = None,
        page_token_created_at: datetime = None,
    ):
        """
        Get not finished and finished < 24h tasks on cluster
        """
        with metadb.commit_on_success():
            _asset_cluster_has_given_type(cluster_id, cluster_type)

            operations = metadb.get_operations(
                cluster_id=cluster_id,
                limit=limit,
                page_token_id=page_token_id,
                page_token_created_at=page_token_created_at,
            )
            return _render_operations_seq_v1(operations, cluster_type)


def get_cluster_type_by_operation(operation: Operation) -> str:
    """
    Return cluster type for operation
    """
    if operation.cluster_type is not None:
        return operation.cluster_type

    # probably we got delete cluster task,
    # try define operation.cluster_type from enums lookup
    for supported_cluster_type, traits in get_traits().items():
        if traits.tasks.has_value(operation.operation_type):
            return supported_cluster_type

    raise OperationTypeMismatchError(operation.id)


def _assert_operation_match_cluster(operation: Operation, cluster_type: str) -> None:
    if get_cluster_type_by_operation(operation) == cluster_type:
        return

    raise OperationTypeMismatchError(operation.id)


@API.resource('/mdb/<ctype:cluster_type>/<version>/operations/<string:operation_id>')
class OperationStatusV1(MethodView):
    """Get information about particular operation"""

    @marshal.with_schema(OperationSchemaV1)
    @check_auth(resource=Resource.OPERATION, operation=DbaasOperation.INFO, folder_resolver=get_folder_by_operation_id)
    def get(
        self,
        operation_id: str,
        cluster_type: str,
        version: str,  # pylint: disable=unused-argument
    ):
        """Get a single operation definition"""
        with metadb.commit_on_success():
            operation = metadb.get_operation_by_id(operation_id=operation_id)

            if operation is None:
                raise OperationNotExistsError(operation_id)

            _assert_operation_match_cluster(operation, cluster_type)

            return _render_operation_v1(
                operation=operation,
                describer=get_operations_describer(cluster_type),
                responser=OperationsResponser(cluster_type),
            )


@API.resource('/mdb/<version>/operations/<string:operation_id>')
class CommonOperationStatusV1(MethodView):
    """
    Get information about particular operation (common handle)
    """

    @marshal.with_schema(OperationSchemaV1)
    @check_auth(explicit_action='mdb.all.read', folder_resolver=get_folder_by_operation_id)
    def get(
        self,
        operation_id: str,
        version: str,  # pylint: disable=unused-argument
    ):
        """Get a single operation definition"""
        with metadb.commit_on_success():
            operation = metadb.get_operation_by_id(operation_id=operation_id)

            if operation is None:
                raise OperationNotExistsError(operation_id)

            cluster_type = get_cluster_type_by_operation(operation)

            return _render_operation_v1(
                operation=operation,
                describer=get_operations_describer(cluster_type),
                responser=OperationsResponser(cluster_type),
            )
