"""
Base types
"""
from datetime import datetime
from enum import Enum, unique
from typing import Any, Dict, List, NamedTuple, Optional, Tuple

CID = str


@unique
class OperationStatus(Enum):
    """
    Operation status
    Should match code.operation_status
    """

    pending = 'PENDING'
    running = 'RUNNING'
    done = 'DONE'
    failed = 'FAILED'

    def is_terminal(self) -> bool:
        """
        Return if it's terminal operation status
        """
        return self in (OperationStatus.done, OperationStatus.failed)


class Operation(NamedTuple):
    """
    Operation
    """

    id: str
    target_id: str
    cluster_type: str
    environment: str
    operation_type: str
    created_by: str
    created_at: datetime
    started_at: datetime
    modified_at: datetime
    status: OperationStatus
    metadata: dict
    hidden: bool
    errors: Optional[List[dict]]


def make_operation(op_dict: dict) -> Operation:
    """
    Make operation from dict
    """
    status = OperationStatus(op_dict['status'])
    return Operation(status=status, **dict((k, op_dict[k]) for k in op_dict if k != 'status'))


@unique
class ResponseType(Enum):
    """
    Operation response type
    """

    empty = 'empty'
    cluster = 'cluster'
    backup = 'backup'
    database = 'database'
    user = 'user'
    subcluster = 'subcluster'
    shard = 'shard'
    hadoop_job = 'hadoop_job'
    ml_model = 'ml_model'
    format_schema = 'format_schema'
    shard_group = 'shard_group'
    alert_group = 'alert_group'


class OperationMetadata(NamedTuple):
    """
    OperationResponse is helper for operation metadata construction
    """

    annotation: str
    metadata: dict


class OperationResponse(NamedTuple):
    """
    OperationResponse is helper for operation response construction
    """

    annotation: str
    type: ResponseType
    key: Tuple


class OperationDescription(NamedTuple):
    """
    OperationDescription is helper for operation definition in our responses
    """

    metadata: OperationMetadata
    description: str
    response: OperationResponse


class OperationEvent(NamedTuple):
    """
    Operation specific meta for cloud event construction
    """

    event_type: str  # is a EventMetadata.event_type
    details: Dict[str, Any]  # is a operation specific EventDetails
    request_parameters: Optional[Dict[str, Any]]  # IMPORTANT: it shouldn't contain sensitive information
