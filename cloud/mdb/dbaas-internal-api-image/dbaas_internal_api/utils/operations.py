# coding: utf-8
"""
Task utilities
"""

from google.protobuf.descriptor import Descriptor, FieldDescriptor
from google.protobuf.message import Message

from typing import Any, Callable, Dict, Mapping, Optional, Tuple, Type

from ..core.types import (
    Operation,
    OperationDescription,
    OperationEvent,
    OperationMetadata,
    OperationResponse,
    ResponseType,
)
from .backup_id import encode_global_backup_id
from .logs import log_warn
from .register import BaseOperationDescriber, register_operations_describer
from .types import ComparableEnum


def annotation_maker(module_path: str) -> Callable[[str], str]:
    """
    Return annotation function
    """

    def _make_annotation(local_annotation: str) -> str:
        return module_path + '.' + local_annotation

    return _make_annotation


def default_metadata(oper: Operation) -> dict:
    """
    Make default metadata from operation
    """
    return {'clusterId': oper.target_id}


def restore_metadata(oper: Operation) -> dict:
    """
    Make restore cluster metadata from operation
    """
    ret = default_metadata(oper)
    ret['backupId'] = encode_global_backup_id(
        cluster_id=oper.metadata['source_cid'],
        backup_id=oper.metadata['backup_id'],
    )
    return ret


def move_metadata(oper: Operation) -> dict:
    """
    Make move cluster metadata from operation
    """
    ret = default_metadata(oper)
    ret['sourceFolderId'] = oper.metadata['source_folder_id']
    ret['destinationFolderId'] = oper.metadata['destination_folder_id']
    return ret


def modify_hosts_metadata(oper: Operation) -> dict:
    """
    Make modify (add, delete, modify) host metadata
    """
    ret = default_metadata(oper)
    ret['hostNames'] = oper.metadata['host_names']
    return ret


def modify_user_metadata(oper: Operation) -> dict:
    """
    Make (add, delete, *permission) user metadata
    """
    ret = default_metadata(oper)
    ret['userName'] = oper.metadata['user_name']
    return ret


def modify_database_metadata(oper: Operation) -> dict:
    """
    Make create delete database metadata
    """
    ret = default_metadata(oper)
    ret['databaseName'] = oper.metadata['database_name']
    return ret


def modify_shard_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on shards
    """
    ret = default_metadata(oper)
    ret['shardName'] = oper.metadata['shard_name']
    return ret


def modify_subcluster_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on subclusters
    """
    ret = default_metadata(oper)
    ret['subclusterId'] = oper.metadata['subcid']
    return ret


def modify_hadoop_job_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on subclusters
    """
    ret = default_metadata(oper)
    ret['jobId'] = oper.metadata['jobId']
    return ret


def modify_ml_model_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on ML models
    """
    ret = default_metadata(oper)
    ret['mlModelName'] = oper.metadata['ml_model_name']
    return ret


def modify_format_schema_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on format schemas
    """
    ret = default_metadata(oper)
    ret['formatSchemaName'] = oper.metadata['format_schema_name']
    return ret


def modify_reschedule_maintenance_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on maintenance
    """
    ret = default_metadata(oper)
    ret['delayedUntil'] = oper.metadata['delayed_until']
    return ret


def modify_shard_group_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on format schemas
    """
    ret = default_metadata(oper)
    ret['shardGroupName'] = oper.metadata['shard_group_name']
    return ret


def mongodb_hosts_resetup_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on resetup hosts
    """
    ret = default_metadata(oper)
    ret['hostNames'] = oper.metadata['host_names']
    return ret


def mongodb_hosts_restart_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on restart hosts
    """
    ret = default_metadata(oper)
    ret['hostNames'] = oper.metadata['host_names']
    return ret


def mongodb_hosts_stepdown_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on stepdown hosts
    """
    ret = default_metadata(oper)
    ret['hostNames'] = oper.metadata['host_names']
    return ret


def delete_backup_metadata(oper: Operation) -> dict:
    """
    Make metadata for operations on delete backup
    """
    ret = default_metadata(oper)
    ret['backupID'] = oper.metadata['backup_id']
    return ret


# *_event_details same as _metadata helpers,
# but use snake_case in dictionaries


def default_event_details(oper: Operation) -> dict:
    """
    Make default event details from operation
    """
    return {'cluster_id': oper.target_id}


def restore_event_details(oper: Operation) -> dict:
    """
    Make restore cluster event details from operation
    """
    ret = default_event_details(oper)
    ret['backup_id'] = encode_global_backup_id(
        cluster_id=oper.metadata['source_cid'],
        backup_id=oper.metadata['backup_id'],
    )
    return ret


# currently in our spec we don't have folder ids in MoveEvent
move_event_details = default_event_details


def modify_hosts_event_details(oper: Operation) -> dict:
    """
    Make modify (add, delete, modify) host event details
    """
    ret = default_event_details(oper)
    ret['host_names'] = oper.metadata['host_names']
    return ret


def modify_user_event_details(oper: Operation) -> dict:
    """
    Make (add, delete, *permission) user event details
    """
    ret = default_event_details(oper)
    ret['user_name'] = oper.metadata['user_name']
    return ret


def modify_database_event_details(oper: Operation) -> dict:
    """
    Make create delete database event details
    """
    ret = default_event_details(oper)
    ret['database_name'] = oper.metadata['database_name']
    return ret


def modify_shard_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on shards
    """
    ret = default_event_details(oper)
    ret['shard_name'] = oper.metadata['shard_name']
    return ret


def modify_subcluster_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on subclusters
    """
    ret = default_event_details(oper)
    ret['subcluster_id'] = oper.metadata['subcid']
    return ret


def modify_hadoop_job_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on subclusters
    """
    ret = default_event_details(oper)
    ret['jobId'] = oper.metadata['jobId']
    return ret


def modify_ml_model_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on ML models
    """
    ret = default_event_details(oper)
    ret['ml_model_name'] = oper.metadata['ml_model_name']
    return ret


def modify_format_schema_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on format schemas
    """
    ret = default_event_details(oper)
    ret['format_schema_name'] = oper.metadata['format_schema_name']
    return ret


def modify_reschedule_maintenance_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on maintenance
    """
    ret = default_event_details(oper)
    ret['delayed_until'] = oper.metadata['delayed_until']
    return ret


def modify_shard_group_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on format schemas
    """
    ret = default_event_details(oper)
    ret['shard_group_name'] = oper.metadata['shard_group_name']
    return ret


def modify_alert_group_event_details(oper: Operation) -> dict:
    """
    Make event details for operations on format schemas
    """
    ret = default_event_details(oper)
    ret['alert_group_id'] = oper.metadata['alert_group_id']
    return ret


def cluster_response(oper: Operation) -> tuple:
    """
    Return cid
    """
    return (oper.target_id,)


def database_response(oper: Operation) -> tuple:
    """
    cid + database_name
    """
    return (oper.target_id, oper.metadata['database_name'])


def user_response(oper: Operation) -> tuple:
    """
    cid + user_name
    """
    return (oper.target_id, oper.metadata['user_name'])


def subcluster_response(oper: Operation) -> tuple:
    """
    cid + subcid
    """
    return (oper.target_id, oper.metadata['subcid'])


def shard_response(oper: Operation) -> tuple:
    """
    cid + shard_name
    """
    return (oper.target_id, oper.metadata['shard_name'])


def hadoop_job_response(oper: Operation) -> tuple:
    """
    cid + jobId
    """
    return (oper.target_id, oper.metadata['jobId'])


def ml_model_response(oper: Operation) -> tuple:
    """
    cid + ml_model_name
    """
    return (oper.target_id, oper.metadata['ml_model_name'])


def format_schema_response(oper: Operation) -> tuple:
    """
    cid + format_schema_name
    """
    return (oper.target_id, oper.metadata['format_schema_name'])


def shard_group_response(oper: Operation) -> tuple:
    """
    cid + shard_group_name
    """
    return (oper.target_id, oper.metadata['shard_group_name'])


def alert_group_response(oper: Operation) -> tuple:
    """
    cid + alert_group_name
    """
    return (oper.target_id, oper.metadata['alert_group_id'])


def empty_response(
    oper: Operation,  # pylint: disable=unused-argument
) -> tuple:
    """
    Empty response
    """
    return tuple()


DEFAULT_RESPONSES = {
    ResponseType.empty: empty_response,
    ResponseType.cluster: cluster_response,
    # for create-backup request
    # we should return backup,
    # but right now there are no way to
    # link operation with created backup
    ResponseType.backup: empty_response,
    ResponseType.database: database_response,
    ResponseType.user: user_response,
    ResponseType.subcluster: subcluster_response,
    ResponseType.shard: shard_response,
    ResponseType.hadoop_job: hadoop_job_response,
    ResponseType.ml_model: ml_model_response,
    ResponseType.format_schema: format_schema_response,
    ResponseType.shard_group: shard_group_response,
    ResponseType.alert_group: alert_group_response,
}

CE = ComparableEnum

Request2DictT = Callable[[dict], Dict[str, Any]]
Operation2DictT = Callable[[Operation], dict]
Operation2TupleT = Callable[[Operation], Optional[tuple]]
EMPTY_RESPONSE = (ResponseType.empty, '')


class OperationHandle:
    """
    OperationHandle holds info about metadata and events construction

    Attributes:
        description (str): human-readable description for operation

        response_annotation (str): GRPC type for response. e.g. "yandex.cloud.mdb.clickhouse.v1.FormatSchema"
        response_type (ResponseType): enum. Used to choose proper response renderer (refer to DEFAULT_RESPONSES and api/operations_responser.py)

        metadata_annotation (str): GRPC type for metadata. e.g. "type.googleapis.com/yandex.cloud.mdb.sqlserver.v1.CreateClusterMetadata"
        metadata: function that converts Operation to dictionary. Dictionary format matches `matadata_annotation` type.

        event_type (str): GRPC type for cloud events (audit trails). e.g. "yandex.cloud.events.mdb.mysql.CreateCluster"
        event_details: function that converts Operation to dictionary. Dictionary format matches `event_type` type.

        request_parameters_annotation (str): GRPC type for request. e.g. "yandex.cloud.events.mdb.mysql.CreateCluster.RequestParameters"
        request_parameters: function that converts Operation to dictionary. Dictionary format matches `request_parameters_annotation` type.
    """

    def __init__(
        self,
        description: str,
        response: Tuple[ResponseType, str],
        metadata_annotation: str,
        event_type: str,
        request_parameters_annotation: Optional[str] = None,
        metadata: Operation2DictT = default_metadata,
        event_details: Operation2DictT = default_event_details,
        request_parameters: Request2DictT = lambda req: {},
    ) -> None:
        self.description = description
        self.response_type, self.response_annotation = response
        self.metadata_annotation = metadata_annotation
        self.metadata = metadata
        self.event_type = event_type
        self.event_details = event_details
        self.request_parameters_annotation = request_parameters_annotation
        self.request_parameters = request_parameters

    def response_key(self, oper: Operation) -> tuple:
        """
        Create response keys from operation
        """
        key_handler = DEFAULT_RESPONSES[self.response_type]
        return key_handler(oper)


class Event:
    """
    Event construct operation specific from operation
    """


HandlesMap = Mapping[CE, OperationHandle]  # pylint: disable=unsubscriptable-object


class OperationDescriber(BaseOperationDescriber):
    """
    Simple operation describer
    """

    def __init__(self, cluster_type: str, operation_types: Type[CE], handles: HandlesMap) -> None:
        self.cluster_type = cluster_type
        self.operation_types = operation_types
        self.handles = handles

    def _get_handler(self, oper: Operation) -> Optional[OperationHandle]:
        try:
            oper_type = self.operation_types.from_string(oper.operation_type)
        except KeyError as exc:
            log_warn('%s describe operation got operation (%r) ' 'with unknown type: %s', self.cluster_type, oper, exc)
            return None

        return self.handles[oper_type]

    def get_description(self, oper: Operation) -> Optional[OperationDescription]:
        """
        Describe operation
        """
        handler = self._get_handler(oper)
        if handler is None:
            return None

        return OperationDescription(
            metadata=OperationMetadata(
                annotation=handler.metadata_annotation,
                metadata=handler.metadata(oper),
            ),
            description=handler.description,
            response=OperationResponse(
                key=handler.response_key(oper),
                type=handler.response_type,
                annotation=handler.response_annotation,
            ),
        )

    def get_event(self, operation: Operation, req: Optional[dict] = None) -> Optional[OperationEvent]:
        """
        Construct operation event from operation
        """
        handler = self._get_handler(operation)
        if handler is None:
            return None
        return OperationEvent(
            event_type=handler.event_type,
            details=handler.event_details(operation),
            request_parameters=handler.request_parameters(req) if req is not None else None,
        )


def describe_operations(cluster_type: str, operation_types: Type[CE], handles: HandlesMap) -> None:
    """
    Shortcut for create and register operations describer
    """
    describer = OperationDescriber(
        cluster_type=cluster_type,
        operation_types=operation_types,
        handles=handles,
    )
    register_operations_describer(cluster_type)(describer)


FIELD_NAME_EXCEPTIONS = {
    'postgresql_config_10': 'postgresqlConfig_10',
    'postgresql_config_10_1c': 'postgresqlConfig_10_1c',
    'postgresql_config_11': 'postgresqlConfig_11',
    'postgresql_config_11_1c': 'postgresqlConfig_11_1c',
    'postgresq_config_12': 'postgresqlConfig_12',
    'postgresql_config_12_1c': 'postgresqlConfig_12_1c',
    'postgresql_config_13': 'postgresqlConfig_13',
    'postgresql_config_13_1c': 'postgresqlConfig_13_1c',
    'postgresql_config_14': 'postgresqlConfig_14',
    'postgresql_config_14_1c': 'postgresqlConfig_14_1c',
}


def request_to_event_request(message_descriptor: Descriptor, data: dict) -> Dict[str, Any]:
    """
    This method that takes :message_descriptor and :data and produces dictionary with fields defined in protobuf.
    Any fields that not present in protobuf message won't be copied (so, sensitive information won't leak to Cloud Events)
    """

    def is_map(desc: FieldDescriptor):
        # https://github.com/protocolbuffers/protobuf/blob/v3.19.4/python/google/protobuf/json_format.py#L168
        return (
            desc.type == FieldDescriptor.TYPE_MESSAGE
            and desc.message_type.has_options
            and desc.message_type.GetOptions().map_entry
        )

    result = {}
    for descriptor in message_descriptor.fields:
        # protobuf is in snake_case
        # REST is in camelCase (json) + camelCase (query) + snake_case (path)
        # result is snake_case
        snake_name = descriptor.name
        camel_name = snake_case_to_camel_case(descriptor.name)

        if snake_name in FIELD_NAME_EXCEPTIONS:
            camel_name = FIELD_NAME_EXCEPTIONS[snake_name]

        value = None
        if camel_name in data:
            value = data[camel_name]
        elif snake_name in data:
            value = data[snake_name]

        if value is not None:
            if descriptor.message_type is not None:
                converted: Any
                if "google/protobuf/wrappers" in descriptor.message_type.file.name:
                    converted = value
                elif "google.protobuf.Timestamp" == descriptor.message_type.full_name:
                    converted = value
                elif is_map(descriptor):
                    converted = value if isinstance(value, dict) else None
                elif descriptor.label == FieldDescriptor.LABEL_REPEATED:
                    converted = [request_to_event_request(descriptor.message_type, rec) for rec in value]
                else:
                    converted = request_to_event_request(descriptor.message_type, value)
                result[snake_name] = converted
            else:
                result[snake_name] = value
    return result


def operation_request_to_dict_factory(proto: Message) -> Request2DictT:
    """
    Factory that produces converters that build dictionaries that match protobuf message structure
    """

    def fn(req) -> dict:
        if req is None:
            return {}
        return request_to_event_request(proto.DESCRIPTOR, req)

    return fn


def snake_case_to_camel_case(s: str) -> str:
    splitted = s.split('_')
    return ''.join(
        map(
            lambda x: x[1] if x[0] == 0 else x[1].capitalize(),
            enumerate(splitted),
        )
    )
