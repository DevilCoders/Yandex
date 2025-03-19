# -*- coding: utf-8 -*-
"""
DBaaS Internal API register functions
"""

import enum
from abc import ABC, abstractmethod
from typing import Union  # noqa
from typing import Any, Callable, Dict, NamedTuple, Optional, Set, Sequence, Type, cast

from flask_restful import abort
from marshmallow import Schema
from typing_extensions import Protocol

from ..core.exceptions import FeatureUnavailableError, UnsupportedHandlerError, UnsupportedAuthActionError
from ..core.types import Operation, OperationDescription, OperationEvent
from ..utils.feature_flags import ensure_feature_flag


class BaseOperationDescriber(ABC):
    """
    Operation describer interface
    """

    @abstractmethod
    def get_description(self, oper: Operation) -> Optional[OperationDescription]:
        """
        get operation description
        """

    @abstractmethod
    def get_event(self, oper: Operation, req: Optional[dict]) -> Optional[OperationEvent]:
        """
        get operation event
        """


SCHEMAS = {}  # type: Dict[str, Union[Dict, Schema]]
HANDLERS = {}  # type: Dict[str, Union[Dict, Callable]]
CLUSTER_TRAITS = {}  # type: Dict[str, FeaturedObject]
OPERATIONS_DESCRIBERS: Dict[str, BaseOperationDescriber] = {}
AUTH_ACTIONS: Dict[str, Union[Dict, str]] = {}


@enum.unique
class DbaasOperation(enum.Enum):
    """
    DBaaS operation type
    """

    INFO = 'info'
    LIST = 'list'
    CREATE = 'create'
    CANCEL = 'cancel'
    BILLING_CREATE = 'billing-estimate-create'
    BILLING_CREATE_HOSTS = 'billing-estimate-create-host'
    MODIFY = 'modify'
    DELETE = 'delete'
    BATCH_CREATE = 'batch-create'
    BATCH_DELETE = 'batch-delete'
    BATCH_MODIFY = 'batch-modify'
    RESTORE = 'restore'
    RESTORE_HINTS = 'restore-hints'
    CHARTS = 'charts'
    HEALTH = 'health'
    BACKUPS = 'backups'
    CREATE_BACKUP = 'create-backup'
    GRANT_PERMISSION = 'grant-permission'
    REVOKE_PERMISSION = 'revoke-permission'
    START = 'start'
    STOP = 'stop'
    MOVE = 'move'
    ENABLE_SHARDING = 'enable-sharding'
    ADD_ZOOKEEPER = 'add-zookeeper'
    START_FAILOVER = 'start-failover'
    CREATE_DICTIONARY = 'create-dictionary'
    DELETE_DICTIONARY = 'delete-dictionary'
    EXTRA_INFO = 'extra-info'
    CONFIG_EXTRA_INFO = 'config-extra-info'
    REBALANCE = 'rebalance'
    SEARCH_ATTRIBUTES = 'search-attributes'
    GET_HADOOP_JOB_LOG = 'get-hadoop-job-log'
    RESCHEDULE = 'reschedule'
    ALERTS_TEMPLATE = 'get-alerts-template'


@enum.unique
class Resource(enum.Enum):
    """
    Resource type
    """

    CLUSTER = 'cluster'
    SUBCLUSTER = 'subcluster'
    SHARD = 'shard'
    SHARD_GROUP = 'shard-group'
    HOST = 'host'
    USER = 'user'
    DATABASE = 'database'
    BACKUP = 'backup'
    ALERT_GROUP = 'alert-group'
    RESOURCE_PRESET = 'resource-preset'
    CONSOLE_CLUSTERS_CONFIG = 'console-clusters-config'
    HADOOP_JOB = 'hadoop_job'
    HADOOP_UI_LINK = 'hadoop_ui_link'
    ML_MODEL = 'ml-model'
    FORMAT_SCHEMA = 'format-schema'
    MAINTENANCE = 'maintenance'
    OPERATION = 'operation'


class VersionsColumn(enum.Enum):
    cluster = 1
    subcluster = 2
    shard = 3


class ClusterTraits(Protocol):
    """
    Cluster traits type
    """

    name: str
    url_prefix: str
    service_slug: str
    roles: Any
    tasks: Any
    operations: Any
    versions_column: VersionsColumn
    versions_component: str
    auth_actions: Dict[Resource, Dict[DbaasOperation, str]]


FeaturedObject = NamedTuple('FeaturedObject', [('object', object), ('feature_flag', str)])


def _register_object(dictionary: Dict, path: Sequence, feature_flag: str = None) -> Callable:
    """
    Decorator for registering classes and functions in dictionary.
    """

    def _wrapper(obj):
        elem = dictionary
        # fall deep inside dictionary
        for key in path[:-1]:
            if key not in elem:
                elem[key] = {}
            elem = elem[key]
        elem[path[-1]] = FeaturedObject(obj, feature_flag)
        return obj

    return _wrapper


def _get_object(dictionary: Dict, path: Sequence, ignore_feature_flags=False) -> object:
    """
    Get registered object with feature flags support. Some callers
    may be outside of global context (for example URL converters)
    and operating with global `g` is impossible in this cases. This
    callers need to ignore feature flags and must set
    `ignore_feature_flags` argument.
    """
    elem = dictionary
    for key in path[:-1]:
        elem = elem[key]
    ret = elem[path[-1]]
    if ret.feature_flag is not None and not ignore_feature_flags:
        ensure_feature_flag(ret.feature_flag)

    return ret.object


def register_schema(cluster_type: str, schema_type='main', feature_flag: str = None) -> Callable[[Schema], Schema]:
    """
    Class decorator to register schema in SCHEMAS dictionary.
    """
    return _register_object(SCHEMAS, [cluster_type, schema_type], feature_flag)


def get_schema(cluster_type: str, schema_type='main') -> Schema:
    """
    Returns schema for specified cluster type
    """
    return cast(Schema, _get_object(SCHEMAS, [cluster_type, schema_type]))


def get_cluster_types(schema_type: str) -> Sequence[str]:
    """
    Returns available cluster_types for schema_type
    """
    ret = set()
    for cluster_type in SCHEMAS:
        try:
            _get_object(SCHEMAS, [cluster_type, schema_type])
            ret.add(cluster_type)
        except (FeatureUnavailableError, KeyError):
            pass

    return list(sorted(ret))


def register_request_schema(
    cluster_type: str, resource: Resource, operation: DbaasOperation, feature_flag: str = None
) -> Callable[[Type[Schema]], Type[Schema]]:
    """
    Class decorator for registering request schemas to use with @use_kwargs
    decorator.
    """
    return _register_object(SCHEMAS, [cluster_type, resource, operation, 'request'], feature_flag)


def get_request_schema(cluster_type: str, resource: Resource, operation: DbaasOperation) -> Schema:
    """
    Returns registered request schema.
    """
    return cast(Schema, _get_object(SCHEMAS, [cluster_type, resource, operation, 'request']))


def register_response_schema(
    cluster_type: str, resource: Resource, operation: DbaasOperation, feature_flag: str = None
) -> Callable[[Type[Schema]], Type[Schema]]:
    """
    Class decorator for registering request schemas to use with @marshal_with
    decorator.
    """
    return _register_object(SCHEMAS, [cluster_type, resource, operation, 'response'], feature_flag)


def get_response_schema(cluster_type: str, resource: Resource, operation: DbaasOperation) -> Schema:
    """
    Returns registered response schema.
    """
    return cast(Schema, _get_object(SCHEMAS, [cluster_type, resource, operation, 'response']))


def register_request_handler(
    cluster_type: str, resource: Resource, operation: DbaasOperation, feature_flag: str = None
) -> Callable[[Callable], Callable]:
    """
    Decorator for registering request handlers.
    """
    return _register_object(HANDLERS, [cluster_type, resource, operation], feature_flag)


def get_request_handler(cluster_type: str, resource: Resource, operation: DbaasOperation) -> Callable:
    """
    Returns registered request handler.
    """
    try:
        return cast(Callable, _get_object(HANDLERS, [cluster_type, resource, operation]))
    except KeyError:
        raise UnsupportedHandlerError('{0} does not have handler for {1} {2}'.format(cluster_type, resource, operation))


def register_config_schema(
    cluster_type: str, version, feature_flag: str = None
) -> Callable[[Type[Schema]], Type[Schema]]:
    """
    Class decorator to register config schema in SCHEMAS dictionary.
    """
    return _register_object(SCHEMAS, [cluster_type, 'options', version], feature_flag)


def get_config_schema(cluster_type: str, version: str) -> Type[Schema]:
    """
    Returns config schema for specified cluster type
    """
    return cast(Type[Schema], _get_object(SCHEMAS, [cluster_type, 'options', version]))


def register_cluster_traits(
    cluster_type: str, feature_flag: str = None
) -> Callable[[Type[ClusterTraits]], Type[ClusterTraits]]:
    """
    Class decorator to register cluster traits in CLUSTER_TRAITS dictionary.
    """
    return _register_object(CLUSTER_TRAITS, [cluster_type], feature_flag)


def get_cluster_traits(cluster_type: str) -> ClusterTraits:
    """
    Returns cluster traits for specified cluster type.
    """
    return cast(ClusterTraits, _get_object(CLUSTER_TRAITS, [cluster_type]))


def get_traits() -> Dict[str, ClusterTraits]:
    """
    Returns all traits of all clusters
    """
    ret = {}
    for cluster_type in CLUSTER_TRAITS:
        try:
            ret[cluster_type] = get_cluster_traits(cluster_type)
        except (FeatureUnavailableError, KeyError):
            pass

    return ret


def get_task_type(cluster_type: str, task_type: str) -> object:
    """
    Get task type name for given cluster type
    """
    try:
        return get_cluster_traits(cluster_type).tasks[task_type]
    except KeyError:
        raise NotImplementedError('{0} does not implement task {1}'.format(cluster_type, task_type))


def register_operations_describer(cluster_type):
    """
    Register operation describer
    """
    return _register_object(OPERATIONS_DESCRIBERS, [cluster_type])


def get_operations_describer(cluster_type: str) -> BaseOperationDescriber:
    """
    Get operation describer
    """
    return cast(BaseOperationDescriber, _get_object(OPERATIONS_DESCRIBERS, [cluster_type]))


def get_roles(cluster_type: str) -> object:
    """Get roles of clusters/subclusters from traits"""
    try:
        return get_cluster_traits(cluster_type).roles
    except KeyError:
        raise NotImplementedError('{0} does not implement roles'.format(cluster_type))


def get_cluster_type_by_url_prefix(url_prefix: str) -> str:
    """
    Returns cluster type by URL prefix
    """
    for cluster_type, feature in CLUSTER_TRAITS.items():
        traits = cast(ClusterTraits, feature.object)
        if url_prefix == traits.url_prefix:
            return cluster_type

    raise abort(404)


def get_supported_clusters() -> Set[str]:
    """
    Return cluster types supported in that daemon
    """
    return set(CLUSTER_TRAITS)


def get_auth_action(cluster_type: str, resource: Resource, operation: DbaasOperation) -> str:
    """
    Returns registered request handler.
    """
    try:
        return get_cluster_traits(cluster_type).auth_actions[resource][operation]
    except KeyError:
        raise UnsupportedAuthActionError(
            '{0} does not have auth action for {1} {2}'.format(cluster_type, resource, operation)
        )
