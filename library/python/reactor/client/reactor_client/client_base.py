from . import reactor_objects as r_objs  # noqa
from datetime import datetime  # noqa


class ReactorAPIException(Exception):
    def __init__(self, status_code, message):
        super(ReactorAPIException, self).__init__(message)
        self.status_code = status_code

    def __reduce__(self):
        message = self.args[0]
        return (type(self), (self.status_code, message))


class ReactorInternalError(ReactorAPIException):
    pass


class ReactorAPITimeout(ReactorAPIException):
    pass


class NoNamespaceError(ReactorAPIException):
    pass


class NoArtifactError(ReactorAPIException):
    pass


class NoReactionError(ReactorAPIException):
    pass


class ArtifactTypeEndpointBase(object):
    def get(self, artifact_type_id=None, artifact_type_key=None):
        """
        :type artifact_type_id: int
        :type artifact_type_key: str
        :rtype: r_objs.ArtifactType
        """
        raise NotImplementedError

    def list(self):
        """
        :rtype: list[r_objs.ArtifactType]
        """
        raise NotImplementedError


class DynamicTriggerEndpointBase(object):
    def add(self, reaction_identifier, triggers):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :type triggers: r_objs.DynamicTriggerList
        :return: list[r_objs.DynamicTriggerDescriptor]
        """
        raise NotImplementedError

    def remove(self, reaction_identifier, triggers):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :type triggers: List[r_objs.DynamicTriggerIdentifier]
        :return: {}
        """
        raise NotImplementedError

    def update(self, reaction_identifier, triggers, action):
        """
       :type reaction_identifier: r_objs.OperationIdentifier
       :type triggers: List[r_objs.DynamicTriggerIdentifier]
       :type action: r_objs.DynamicTriggerAction
       :return: {}
       """
        raise NotImplementedError

    def list(self, reaction_identifier):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :return: list[r_objs.DynamicTriggerDescriptor]
        """
        raise NotImplementedError


class ArtifactTriggerEndpointBase(object):
    def insert(self,  reaction_identifier,  artifact_instance_ids):
        """
        :type reaction_identifier: r_objs.OperationIdentifier
        :type artifact_instance_ids: List[int]
        :return: {}
        """
        raise NotImplementedError


class ArtifactEndpointBase(object):
    def get(self, artifact_id=None, namespace_identifier=None):
        """
        Use either `artifact_id` or `namespace_identifier`
        :type artifact_id: int
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :raises: ArtifactNotExists
        :rtype: r_objs.Artifact
        """
        raise NotImplementedError

    def create(self, artifact_type_identifier, artifact_identifier, description, permissions,
               create_parent_namespaces=False, create_if_not_exist=False):
        """
        :type artifact_type_identifier: r_objs.ArtifactTypeIdentifier
        :type artifact_identifier: r_objs.ArtifactIdentifier
        :type description: str
        :type permissions: r_objs.NamespacePermissions
        :type create_parent_namespaces: bool
        :type create_if_not_exist: bool
        :rtype: r_objs.ArtifactCreateResponse
        """
        raise NotImplementedError

    def check_exists(self, artifact_id=None, namespace_identifier=None):
        """
        Use either `artifact_id` or `namespace_identifier`
        :type artifact_id: int
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: r_objs.Artifact
        """
        raise NotImplementedError


class ArtifactInstanceEndpointBase(object):
    def instantiate(self, artifact_identifier=None, metadata=None, attributes=None, user_time=None,
                    create_if_not_exist=False):
        """
        :type artifact_identifier: ArtifactIdentifier
        :type metadata: Metadata
        :type attributes: Attributes
        :type user_time: datetime
        :type create_if_not_exist: bool
        :rtype: ArtifactInstanceInstantiateResponse
        """
        raise NotImplementedError

    def last(self, artifact_identifier):
        """
        :param artifact_identifier:
        :return:
        """
        raise NotImplementedError

    def range(self, filter_):
        """
        :type filter_: r_objs.ArtifactInstanceFilterDescriptor
        :rtype: list[r_objs.ArtifactInstance]
        """
        raise NotImplementedError

    def get_status_history(self, artifact_instance_id):
        """
        :type artifact_instance_id: int
        :rtype: list[r_objs.ArtifactInstanceStatusRecord]
        """
        raise NotImplementedError

    def deprecate(self, instances_to_deprecate, description):
        """
        :type instances_to_deprecate: List[int]
        :type description: str
        :rtype: Optional[int]
        """
        raise NotImplementedError


class ReactionEndpointBase(object):
    def create(self, operation_descriptor=None, create_if_not_exist=False):
        """
        :type operation_descriptor: r_objs.OperationDescriptor
        :type create_if_not_exist: bool
        :rtype: r_objs.ReactionReference
        """
        raise NotImplementedError

    def get(self, operation_identifier):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :raises: ReactionNotExists
        :rtype: r_objs.Operation
        """
        raise NotImplementedError

    def check_exists(self, operation_identifier):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :rtype: r_objs.Operation
        """
        raise NotImplementedError

    def update(self, status_update_list=None, reaction_start_configuration_update=None):
        """
        :type status_update_list: list[r_objs.ReactionStatusUpdate]
        :type reaction_start_configuration_update: list[r_objs.ReactionStartConfigurationUpdate]
        :return: {}
        """
        raise NotImplementedError


class ReactionInstanceEndpointBase(object):
    def list(self, reaction, bound_instance_id=None, filter_type=r_objs.FilterTypes.GREATER_THAN,
             order=r_objs.OrderTypes.DESCENDING, limit=100):
        """
        :type reaction: r_objs.OperationIdentifier
        :type bound_instance_id: int | None
        :type filter_type: r_objs.FilterTypes
        :type order: r_objs.OrderTypes
        :type limit: int
        :rtype: list[r_objs.OperationInstance]
        """
        raise NotImplementedError

    def get(self, reaction_instance_id):
        """
        :type reaction_instance_id: int
        :rtype: r_objs.OperationInstance
        """
        raise NotImplementedError

    def list_statuses(self, reaction, bound_instance_id=None, filter_type=r_objs.FilterTypes.GREATER_THAN,
                      order=r_objs.OrderTypes.DESCENDING, limit=100):
        """
        :type reaction: r_objs.OperationIdentifier
        :type bound_instance_id: int | None
        :type filter_type: r_objs.FilterTypes
        :type order: r_objs.OrderTypes
        :type limit: int
        :rtype: list[r_objs.OperationInstanceStatusView]
        """
        raise NotImplementedError

    def cancel(self, reaction_instance_id):
        """
        :type reaction_instance_id: int
        """
        raise NotImplementedError

    def get_status_history(self, reaction_instance_id):
        """
        :type reaction_instance_id: int
        """
        raise NotImplementedError


class NamespaceEndpointBase(object):
    def get(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :raise: NamespaceNotExists
        :rtype: r_objs.Namespace
        """
        raise NotImplementedError

    def create(self, namespace_identifier, description, permissions, create_parents=False, create_if_not_exist=True):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :type description: str
        :type  permissions: r_objs.NamespacePermissions
        :type create_parents: bool
        :type create_if_not_exist: bool
        :rtype: int
        """
        raise NotImplementedError

    def delete(self, namespace_identifier_list, delete_if_exist=False):
        """
        :type namespace_identifier_list: list[r_objs.NamespaceIdentifier]
        :type delete_if_exist: Bool
        :return: {}
        """
        raise NotImplementedError

    def check_exists(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: r_objs.Namespace
        """
        raise NotImplementedError

    def list(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: list[r_objs.Namespace]
        """
        raise NotImplementedError

    def list_names(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: list[str]
        """
        raise NotImplementedError

    def resolve(self, entity_id):
        """
        :type entity_id: int
        :rtype: str
        """
        raise NotImplementedError

    def resolve_path(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: str
        """
        raise NotImplementedError

    def move(self, source_identifier, destination_identifier):
        """
        :type source_identifier: r_objs.NamespaceIdentifier
        :type destination_identifier: r_objs.NamespaceIdentifier
        :return: {}
        """
        raise NotImplementedError


class NamespaceNotificationEndpointBase(object):
    def change(self, delete_id_list=None, notification_descriptor_list=None):
        """
        :type delete_id_list: list[int] | None
        :type notification_descriptor_list: list[r_objs.NotificationDescriptor] | None
        """
        raise NotImplementedError

    def list(self, namespace_identifier):
        """
        :type namespace_identifier: r_objs.NamespaceIdentifier
        :rtype: list[r_objs.Notification]
        """
        raise NotImplementedError

    def change_long_running(self, operation_identifier, options):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :type options: r_objs.LongRunningOperationInstanceNotificationOptions
        :return: {}
        """
        raise NotImplementedError

    def get_long_running(self, operation_identifier):
        """
        :type operation_identifier: r_objs.OperationIdentifier
        :rtype: r_objs.LongRunningOperationInstanceNotificationOptions | None
        """
        raise NotImplementedError


class QueueEndpointBase(object):
    def create(self, queue_descriptor, create_if_not_exist=False):
        """
        :type queue_descriptor: r_objs.QueueDescriptor
        :type create_if_not_exist: bool
        :rtype: r_objs.QueueReference
        """
        raise NotImplementedError

    def get(self, queue_identifier):
        """
        :type queue_identifier: r_objs.QueueIdentifier
        :rtype: r_objs.QueueReactions
        """
        raise NotImplementedError

    def update(self, queue_identifier, new_queue_capacity=None, remove_reactions=None, add_reactions=None, new_max_instances_in_queue=None, new_max_instances_per_reaction_in_queue=None):
        """
        :type queue_identifier: r_objs.QueueIdentifier
        :type new_queue_capacity: int
        :type remove_reactions: list[r_objs.OperationIdentifier]
        :type add_reactions: list[r_objs.OperationIdentifier]
        :type new_max_instances_in_queue: int
        :type new_max_instances_per_reaction_in_queue: int
        :return: {}
        """
        raise NotImplementedError


class MetricEndpointBase(object):
    def create(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None, custom_tags=None):
        """
        :type metric_type: r_objs.MetricType | r_objs.UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :type custom_tags: dict[str, str] | None
        :rtype: r_objs.MetricReference
        """
        raise NotImplementedError

    def list(self, artifact_id=None, queue_id=None, reaction_id=None):
        """
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :return: list[r_objs.MetricReference]
        """
        raise NotImplementedError

    def update(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None, new_tags=None):
        """
        :type metric_type: r_objs.MetricType | r_objs.UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :type new_tags: r_objs.Attributes | None
        :return: r_objs.MetricReference
        """
        raise NotImplementedError

    def delete(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None):
        """
        :type metric_type: r_objs.MetricType | r_objs.UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :return: {}
        """
        raise NotImplementedError


class NamespacePermissionEndpointBase(object):
    def change(self, namespace, revoke=None, grant=None):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :type revoke: List[str] | None
        :type grant: r_objs.NamespacePermissions | None
        :rtype: {}
        """
        raise NotImplementedError

    def list(self, namespace):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :rtype: r_objs.NamespacePermissions
        """
        raise NotImplementedError


class QuotaEndpointBase(object):
    def get(self, namespace):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :rtype: r_objs.CleanupStrategyDescriptor
        """
        raise NotImplementedError

    def update(self, namespace, cleanup_strategy):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :type cleanup_strategy: r_objs.CleanupStrategyDescriptor
        :rtype: {}
        """
        raise NotImplementedError

    def delete(self, namespace):
        """
        :type namespace: r_objs.NamespaceIdentifier
        :rtype: {}
        """
        raise NotImplementedError


class CalculationEndpointBase(object):
    def create(self, calculation, mappings=None, inactive=False):
        """
        :type calculation: r_objs.CalculationDescriptor
        :type mappings: list[YtPathMappingRule] | None
        :type inactive: bool
        :rtype: r_objs.CalculationReference
        """
        raise NotImplementedError

    def get(self, root_namespace):
        """
        :type root_namespace: r_objs.NamespaceIdentifier
        :rtype: r_objs.Calculation
        """
        raise NotImplementedError

    def list_metadata(self, root_namespaces):
        """
        :type root_namespaces: list[r_objs.NamespaceIdentifier]
        :rtype: dict[int, r_objs.CalculationRuntimeMetadata]
        """
        raise NotImplementedError

    def update(self, calculation, version, mappings=None):
        """
        :type calculation: r_objs.CalculationDescriptor
        :type version: int
        :type mappings: list[YtPathMappingRule] | None
        :rtype: r_objs.CalculationReference
        """
        raise NotImplementedError

    def delete(self, root_namespace):
        """
        :type root_namespace: r_objs.NamespaceIdentifier
        :rtype: {}
        """
        raise NotImplementedError

    def translate(self, yt_paths, root_namespace=None, mappings=None):
        """
        :type yt_paths: list[str]
        :type root_namespace: list[r_objs.NamespaceIdentifier] | None
        :type mappings: list[r_objs.YtPathMappingRule] | None
        :rtype: list[r_objs.YtPathMappingCandidates]
        """
        raise NotImplementedError


class ReactorAPIClientBase(object):
    @property
    def artifact(self):
        """
        :rtype: ArtifactEndpointBase
        """
        raise NotImplementedError

    @property
    def artifact_type(self):
        """
        :rtype: ArtifactTypeEndpointBase
        """
        raise NotImplementedError

    @property
    def artifact_trigger(self):
        """
        :rtype: ArtifactTriggerEndpointBase
        """
        raise NotImplementedError

    @property
    def artifact_instance(self):
        """
        :rtype: ArtifactInstanceEndpointBase
        """
        raise NotImplementedError

    @property
    def reaction(self):
        """
        :rtype: ReactionEndpointBase
        """
        raise NotImplementedError

    @property
    def reaction_instance(self):
        """
        :rtype: ReactionInstanceEndpointBase
        """
        raise NotImplementedError

    @property
    def metric(self):
        """
        :rtype: MetricEndpointBase
        """
        raise NotImplementedError

    @property
    def namespace(self):
        """
        :rtype: NamespaceEndpointBase
        """
        raise NotImplementedError

    @property
    def namespace_notification(self):
        """
        :rtype: NamespaceNotificationEndpointBase
        """
        raise NotImplementedError

    @property
    def queue(self):
        """
        :rtype: QueueEndpointBase
        """
        raise NotImplementedError

    @property
    def quota(self):
        """
        :rtype: QuotaEndpointBase
        """
        raise NotImplementedError

    @property
    def dynamic_trigger(self):
        """
        :rtype: ArtifactTriggerEndpointBase
        """
        raise NotImplementedError

    @property
    def calculation(self):
        """
        :rtype: CalculationEndpointBase
        """
        raise NotImplementedError

    @property
    def permission(self):
        """
        :rtype: NamespacePermissionEndpoint
        """
        raise NotImplementedError

    def greet(self):
        """
        :rtype: r_objs.GreetResponse
        """
        raise NotImplementedError
