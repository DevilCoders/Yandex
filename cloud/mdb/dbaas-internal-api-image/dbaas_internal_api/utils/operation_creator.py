"""
Create operation helper
"""

import enum
from datetime import timedelta
from typing import Callable, List, Optional
import json

from flask import g
import opentracing
import opentracing.propagation

from . import metadb
from ..core import exceptions as errors
from ..core.types import CID, Operation
from .cluster.get import get_cluster_info_assert_exists
from .events import make_event
from .metadata import Metadata
from .metadb import IdempotenceData
from .request_context import get_cloud_ext_id_from_request, get_folder_ids_from_request, get_idempotence_from_request
from .search_renders import make_doc
from .types import ComparableEnum, FolderIds


def compose_task_args(task_args: dict = None) -> dict:
    """
    Fill feature_flags from request
    """
    args = task_args or {}
    args['feature_flags'] = g.cloud['feature_flags']
    return args


class OperationChecks(enum.Flag):
    """
    Checks that we run before operation
    """

    none = enum.auto()
    is_running = enum.auto()
    is_consistent = enum.auto()
    all = is_running | is_consistent


Operation2JsonT = Callable[[Operation], Optional[dict]]


def get_tracing() -> Optional[str]:
    """
    get tracing info encoded for operation insert
    """
    trace_data: dict[str, str] = {}
    tracer = opentracing.global_tracer()

    active_span = tracer.active_span
    if active_span is None:
        return None

    tracer.inject(span_context=active_span.context, format=opentracing.propagation.Format.TEXT_MAP, carrier=trace_data)

    return json.dumps(trace_data)


class OperationCreator:
    """
    Operation creator
    """

    @staticmethod
    def skip_search_queue(_: Operation) -> Optional[dict]:
        """
        search document render thant doesn't add documents to search_queue
        """
        return None

    @staticmethod
    def skip_event(_: Operation) -> Optional[dict]:
        """
        event render thant doesn't add documents to worker_queue_events
        """
        return None

    def __init__(
        self,
        cid: str,
        folder: FolderIds,
        cloud_ext_id: str,
        skip_checks: OperationChecks = OperationChecks.none,
        search_queue_render: Operation2JsonT = None,
        event_render: Operation2JsonT = None,
    ) -> None:
        self.cid = cid
        self.folder = folder
        self.cloud_ext_id = cloud_ext_id
        self._operation: Optional[Operation] = None
        self._checks: List[Callable] = []
        if not skip_checks & OperationChecks.is_running:
            self._checks.append(self._assert_no_running)
        if not skip_checks & OperationChecks.is_consistent:
            self._checks.append(self._assert_consistent_state)
        self._custom_sq_render = search_queue_render
        self._custom_event_render = event_render

    @classmethod
    def make_from_request(
        cls,
        cid: str,
        skip_checks: OperationChecks = OperationChecks.none,
        search_queue_render: Operation2JsonT = None,
        event_render: Operation2JsonT = None,
    ) -> 'OperationCreator':
        """
        construct OperationCreator with folder and cloud from request
        """
        return OperationCreator(
            cid=cid,
            folder=get_folder_ids_from_request(),
            cloud_ext_id=get_cloud_ext_id_from_request(),
            skip_checks=skip_checks,
            search_queue_render=search_queue_render,
            event_render=event_render,
        )

    def _assert_consistent_state(self) -> None:
        """
        check that there are no failed tasks
        """
        op_id = metadb.get_failed_task(self.cid)
        if op_id:
            raise errors.PreconditionFailedError(f'Cluster state is inconsistent due to operation {op_id} failure')

    def _assert_no_running(self) -> None:
        """
        check that there are no running tasks
        """
        op_id = metadb.get_running_task(self.cid)
        if op_id:
            raise errors.PreconditionFailedError(f'Conflicting operation {op_id} detected')

    def _verify_checks(self):
        for check in self._checks:
            check()

    def add_operation(
        self,
        task_type: ComparableEnum,
        operation_type: ComparableEnum,
        metadata: Metadata,
        task_args: dict,
        idempotence_data: Optional[IdempotenceData],
        time_limit: timedelta = None,
        hidden: bool = False,
        delay_by: timedelta = None,
        required_operation_id: str = None,
    ) -> Operation:
        """
        add operation to metadb
        """
        # pylint: disable=too-many-arguments
        self._verify_checks()
        operation = metadb.add_operation(
            task_type=task_type,
            operation_type=operation_type,
            metadata=metadata,
            cid=self.cid,
            folder_id=self.folder.folder_id,
            time_limit=time_limit,
            task_args=task_args,
            hidden=hidden,
            delay_by=delay_by,
            required_operation_id=required_operation_id,
            idempotance_data=idempotence_data,
            tracing=get_tracing(),
        )
        self._add_to_search_queue(operation)
        self._add_to_worker_queue_events(operation)
        return operation

    def add_unmanaged_operation(
        self,
        task_type: ComparableEnum,
        operation_type: ComparableEnum,
        metadata: Metadata,
        task_args: dict,
        idempotence_data: Optional[IdempotenceData],
        time_limit: timedelta = None,
        hidden: bool = False,
        delay_by: timedelta = None,
        required_operation_id: str = None,
    ) -> Operation:
        """
        add unmanaged operation to metadb
        """
        # pylint: disable=too-many-arguments
        self._verify_checks()
        operation = metadb.add_unmanaged_operation(
            task_type=task_type,
            operation_type=operation_type,
            metadata=metadata,
            cid=self.cid,
            folder_id=self.folder.folder_id,
            time_limit=time_limit,
            task_args=task_args,
            hidden=hidden,
            delay_by=delay_by,
            required_operation_id=required_operation_id,
            idempotance_data=idempotence_data,
            tracing=get_tracing(),
        )
        self._add_to_search_queue(operation)
        return operation

    def add_finished_operation(
        self, operation_type: ComparableEnum, metadata: Metadata, hidden: bool, idempotence_data: IdempotenceData = None
    ) -> Operation:
        """
        add finished operation to metadb
        """
        self._verify_checks()
        operation = metadb.add_finished_operation(
            operation_type=operation_type,
            metadata=metadata,
            folder_id=self.folder.folder_id,
            cid=self.cid,
            hidden=hidden,
            idempotence_data=idempotence_data,
        )
        self._add_to_search_queue(operation)
        self._add_to_worker_queue_events(operation)
        return operation

    def add_finished_operation_for_current_rev(
        self,
        operation_type: ComparableEnum,
        metadata: Metadata,
        hidden: bool,
        rev: int,
        idempotence_data: IdempotenceData = None,
    ) -> Operation:
        """
        add finished operation to metadb
        """
        self._verify_checks()
        operation = metadb.add_finished_operation_for_current_rev(
            operation_type=operation_type,
            metadata=metadata,
            folder_id=self.folder.folder_id,
            cid=self.cid,
            hidden=hidden,
            idempotence_data=idempotence_data,
            rev=rev,
        )
        self._add_to_search_queue(operation)
        self._add_to_worker_queue_events(operation)
        return operation

    def _build_search_doc(self, operation) -> dict:
        cluster = get_cluster_info_assert_exists(cluster_id=self.cid, cluster_type=None, include_deleted=True)
        return make_doc(
            cluster=cluster,
            timestamp=operation.created_at,
            folder_ext_id=self.folder.folder_ext_id,
            cloud_ext_id=self.cloud_ext_id,
        )

    def _add_to_search_queue(self, operation):
        """
        add doc to search_queue. create doc if it not specified
        """
        if self._custom_sq_render is not None:
            doc = self._custom_sq_render(operation)
        else:
            doc = self._build_search_doc(operation)
        if doc is not None:
            metadb.add_to_search_queue([doc])

    def _make_event(self, operation: Operation):
        if self._custom_event_render is not None:
            return self._custom_event_render(operation)
        return make_event(operation)

    def _add_to_worker_queue_events(self, operation: Operation):
        """
        create_event and add it to worker_queue
        """
        event_data = self._make_event(operation)
        if event_data is not None:
            metadb.add_to_worker_queue_events(data=event_data, task_id=operation.id)


def create_operation(
    task_type: ComparableEnum,
    operation_type: ComparableEnum,
    metadata: Metadata,
    cid: str,
    time_limit: timedelta = None,
    task_args: dict = None,
    hidden: bool = False,
    delay_by: timedelta = None,
    required_operation_id: str = None,
) -> Operation:
    """
    Creates new task in metaDB and returns it
    """
    # pylint: disable=too-many-arguments
    creator = OperationCreator.make_from_request(
        cid,
        search_queue_render=OperationCreator.skip_search_queue if hidden else None,
        event_render=OperationCreator.skip_event if hidden else None,
    )

    return creator.add_operation(
        task_type=task_type,
        operation_type=operation_type,
        metadata=metadata,
        time_limit=time_limit,
        task_args=compose_task_args(task_args),
        hidden=hidden,
        delay_by=delay_by,
        required_operation_id=required_operation_id,
        idempotence_data=get_idempotence_from_request(),
    )


def create_unmanaged_operation(
    task_type: ComparableEnum,
    operation_type: ComparableEnum,
    metadata: Metadata,
    cid: str,
    time_limit: timedelta = None,
    task_args: dict = None,
    hidden: bool = False,
    delay_by: timedelta = None,
    required_operation_id: str = None,
) -> Operation:
    """
    Creates new unmanaged task in metaDB and returns it
    """
    # pylint: disable=too-many-arguments
    creator = OperationCreator.make_from_request(
        cid,
        skip_checks=OperationChecks.is_running,
        search_queue_render=OperationCreator.skip_search_queue if hidden else None,
    )

    return creator.add_unmanaged_operation(
        task_type=task_type,
        operation_type=operation_type,
        metadata=metadata,
        time_limit=time_limit,
        task_args=compose_task_args(task_args),
        hidden=hidden,
        delay_by=delay_by,
        required_operation_id=required_operation_id,
        idempotence_data=get_idempotence_from_request(),
    )


def create_finished_operation(
    operation_type: ComparableEnum, metadata: Metadata, cid: CID, hidden: bool = False
) -> Operation:
    """
    Creates finished task in metaDB and returns this
    """
    creator = OperationCreator.make_from_request(
        cid,
        skip_checks=OperationChecks.all,
        search_queue_render=OperationCreator.skip_search_queue if hidden else None,
        event_render=OperationCreator.skip_event if hidden else None,
    )
    return creator.add_finished_operation(
        operation_type=operation_type,
        metadata=metadata,
        hidden=hidden,
        idempotence_data=get_idempotence_from_request(),
    )


def create_finished_operation_for_current_rev(
    operation_type: ComparableEnum, metadata: Metadata, cid: CID, rev: int, hidden: bool = False
) -> Operation:
    """
    Creates finished task in metaDB and returns this
    """
    creator = OperationCreator.make_from_request(
        cid,
        skip_checks=OperationChecks.all,
        search_queue_render=OperationCreator.skip_search_queue if hidden else None,
        event_render=OperationCreator.skip_event if hidden else None,
    )
    return creator.add_finished_operation_for_current_rev(
        operation_type=operation_type,
        metadata=metadata,
        hidden=hidden,
        rev=rev,
        idempotence_data=get_idempotence_from_request(),
    )
