from enum import Enum
from datetime import datetime  # noqa


class ReactorDataObject(object):
    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.__dict__ == other.__dict__
        return False

    def __ne__(self, other):
        return not self == other

    def __repr__(self):
        return self.__class__.__name__ + "(\n**{}\n)".format(self.__dict__)


class UnknownEnumValue(ReactorDataObject):
    def __init__(self, name):
        """
        :type name: str
        """
        self._name = name

    @property
    def name(self):
        """
        :rtype: str
        """
        return self._name

    @property
    def value(self):
        """
        :rtype: str
        """
        return -1


class EntityReference(ReactorDataObject):
    def __init__(self, type_=None, entity_id=None):
        """
        :type type_: str
        :type entity_id: str
        """
        self.type_ = type_
        self.entity_id = entity_id

    def __hash__(self):
        return hash((self.type_, self.entity_id))


class NamespaceIdentifier(ReactorDataObject):
    def __init__(self, namespace_path=None, namespace_id=None):
        """
        :type namespace_path: str
        :type namespace_id: int
        """
        self.namespace_path = namespace_path
        self.namespace_id = namespace_id

    def __hash__(self):
        return hash((self.namespace_path, self.namespace_id))


class ArtifactIdentifier(ReactorDataObject):
    def __init__(self, artifact_id=None, namespace_identifier=None):
        """
        :type artifact_id: int | None
        :type namespace_identifier: NamespaceIdentifier | None
        """
        self.artifact_id = artifact_id
        self.namespace_identifier = namespace_identifier


class Artifact(ReactorDataObject):
    def __init__(self, artifact_id=None, artifact_type_id=None,
                 namespace_id=None, project_id=None):
        """
        :type artifact_id: int
        :type artifact_type_id: int
        :type namespace_id: int
        :type project_id: int | None
        """
        self.artifact_id = artifact_id
        self.artifact_type_id = artifact_type_id
        self.namespace_id = namespace_id
        self.project_id = project_id


class Metadata(ReactorDataObject):
    def __init__(self, type_, dict_obj=None):
        """
        Look here for available types and there fields:
        https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/ObshayaFormula/fml/ReactiveConveyor/Artifact/Artifact-type/#path
        :type type_: str | None
        :type dict_obj: dict | None
        """
        self.type_ = type_
        self.dict_obj = dict_obj


class ArtifactTypeIdentifier(ReactorDataObject):
    def __init__(self, artifact_type_id=None, artifact_type_key=None):
        """
        :type artifact_type_id: int | None
        :type artifact_type_key: str | None
        """
        self.artifact_type_id = artifact_type_id
        self.artifact_type_key = artifact_type_key


class ArtifactTypeStatus(Enum):
    ACTIVE = 0
    DEPRECATED = 1


class ArtifactType(ReactorDataObject):
    def __init__(self, artifact_type_id=None, key=None, name=None, description=None, status=None):
        """
        :type artifact_type_id: int
        :type key: str
        :type name: str
        :type description: str
        :type status: ArtifactTypeStatus
        """
        self.artifact_type_id = artifact_type_id
        self.key = key
        self.name = name
        self.description = description
        self.status = status


class NamespaceRole(Enum):
    READER = 0
    WRITER = 1
    RESPONSIBLE = 2


class NamespacePermissions(ReactorDataObject):
    def __init__(self, roles, version=0):
        """
        :type roles: dict[str, NamespaceRole]
        :type version: int = 0
        """
        self.roles = roles
        self.version = version


class ArtifactCreateRequest(ReactorDataObject):
    def __init__(self, artifact_type_identifier=None, artifact_identifier=None,
                 description=None, permissions=None,
                 create_parent_namespaces=False, create_if_not_exist=False,
                 project_identifier=None, cleanup_strategy=None):
        """
        :type artifact_type_identifier: ArtifactTypeIdentifier
        :type artifact_identifier: ArtifactIdentifier
        :type description: str
        :type permissions: NamespacePermissions
        :type create_parent_namespaces: bool
        :type create_if_not_exist: bool
        :type project_identifier: ProjectIdentifier
        :type cleanup_strategy: CleanupStrategyDescriptor
        """
        self.artifact_type_identifier = artifact_type_identifier
        self.artifact_identifier = artifact_identifier
        self.description = description
        self.permissions = permissions
        self.create_parent_namespaces = create_parent_namespaces
        self.create_if_not_exist = create_if_not_exist
        self.project_identifier = project_identifier
        self.cleanup_strategy = cleanup_strategy


class ArtifactCreateResponse(ReactorDataObject):
    def __init__(self, artifact_id=None, namespace_id=None):
        """
        :type artifact_id: int
        :type namespace_id: int
        """
        self.artifact_id = artifact_id
        self.namespace_id = namespace_id


class Attributes(ReactorDataObject):
    def __init__(self, key_value=None):
        """
        :type key_value: dict[str, str]
        """
        self.key_value = key_value


class ArtifactInstanceInstantiateRequest(ReactorDataObject):
    def __init__(self, artifact_identifier=None, metadata=None, attributes=None, user_time=None,
                 create_if_not_exist=False):
        """
        :type artifact_identifier: ArtifactIdentifier
        :type metadata: Metadata
        :type attributes: Attributes
        :type user_time: datetime
        :type create_if_not_exist: bool
        """
        self.artifact_identifier = artifact_identifier
        self.metadata = metadata
        self.attributes = attributes
        self.user_time = user_time
        self.create_if_not_exist = create_if_not_exist


class ArtifactInstanceInstantiateResponse(ReactorDataObject):
    def __init__(self, artifact_instance_id=None, creation_time=None):
        """
        :type artifact_instance_id: int
        :type creation_time: datetime
        """
        self.artifact_instance_id = artifact_instance_id
        self.creation_time = creation_time


class ArtifactInstanceCreateRequest(ReactorDataObject):
    def __init__(self, artifact_create_request, metadata, user_time=None, attributes=None):
        """
        :type artifact_create_request: ArtifactCreateRequest
        :type metadata: Metadata
        :type user_time: datetime | None
        :type attributes: Attributes | None
        """
        self.artifact_create_request = artifact_create_request
        self.metadata = metadata
        self.user_time = user_time
        self.attributes = attributes


class ArtifactInstanceStatus(Enum):
    CREATED = 0
    DEPRECATED = 1
    DELETED = 2
    ACTIVE = 3
    REPLACING = 4


class ArtifactInstanceSource(Enum):
    UNDEFINED = 1
    MANUAL = 2
    API = 3
    OPERATION = 4
    FROM_CODE = 5


class ArtifactInstance(ReactorDataObject):
    def __init__(self, instance_id=None, artifact_id=None, creator=None, metadata=None, attributes=None,
                 user_time=None, creation_time=None, status=None, source=None):
        """
        :type instance_id: int
        :type artifact_id: int
        :type creator: string
        :type metadata: Metadata
        :type attributes: Attributes
        :type user_time: datetime
        :type creation_time: datetime
        :type status: ArtifactInstanceStatus | UnknownEnumValue
        :type source: ArtifactInstanceSource | UnknownEnumValue
        """

        self.instance_id = instance_id
        self.artifact_id = artifact_id
        self.creator = creator
        self.metadata = metadata
        self.attributes = attributes
        self.user_time = user_time
        self.creation_time = creation_time
        self.status = status
        self.source = source


class ArtifactInstanceReference(ReactorDataObject):
    def __init__(self, artifact_type_id, artifact_id, artifact_instance_id, namespace_id):
        """
        :type artifact_type_id: int
        :type artifact_id: int
        :type artifact_instance_id: int
        :type namespace_id: int
        """
        self.artifact_type_id = artifact_type_id
        self.artifact_id = artifact_id
        self.artifact_instance_id = artifact_instance_id
        self.namespace_id = namespace_id


class NamespaceDescriptor(ReactorDataObject):
    def __init__(self, namespace_identifier=None, description=None, permissions=None, create_parent_namespaces=None):
        """
        :type namespace_identifier: NamespaceIdentifier
        :type description: str
        :type permissions: NamespacePermissions
        :type create_parent_namespaces: bool
        """
        self.namespace_identifier = namespace_identifier
        self.description = description
        self.permissions = permissions
        self.create_parent_namespaces = create_parent_namespaces


class OperationTypeKey(ReactorDataObject):
    def __init__(self, set_key=None, key=None, version=None):
        """
        :type set_key: str
        :type key: str
        :type version: str
        """
        self.set_key = set_key
        self.key = key
        self.version = version


class OperationTypeIdentifier(ReactorDataObject):
    def __init__(self, operation_type_id=None, operation_type_key=None):
        """
        :type operation_type_id: int
        :type operation_type_key: OperationTypeKey
        """
        self.operation_type_id = operation_type_id
        self.operation_type_key = operation_type_key


class MisfirePolicy(Enum):
    FIRE_ALL = 0
    FIRE_ONE = 1
    IGNORE = 2


class CronTrigger(ReactorDataObject):
    def __init__(self, cron_expression, misfire_policy=None):
        """
        :type cron_expression: str
        :type misfire_policy: MisfirePolicy | UnknownEnumValue
        """
        self.cron_expression = cron_expression
        self.misfire_policy = misfire_policy


class Relationship(Enum):
    AND = 0
    OR = 1
    USER_TIMESTAMP_EQUALITY = 2
    CONDITION = 3


class SelectedArtifactsTrigger(ReactorDataObject):
    def __init__(self, artifact_refs, relationship, expression=None):
        """
        :type artifact_refs: list[ArtifactReference]
        :type relationship: Relationship
        :type expression: Expression
        """

        self.artifact_refs = artifact_refs
        self.relationship = relationship
        self.expression = expression


class InputsTrigger(ReactorDataObject):
    def __init__(self, artifact_refs=None, any_input=False, all_inputs=False):
        """
        :type artifact_refs: list[ArtifactReference]
        :type any_input: bool
        :type all_inputs: bool
        """
        self.artifact_refs = artifact_refs
        self.all_inputs = all_inputs
        self.any_input = any_input


class DeltaPattern(ReactorDataObject):
    def __init__(self, artifact_reference, deltas_dict):
        """
        :type artifact_reference: ArtifactReference
        :param deltas_dict: key to timedelta in milliseconds
        :type deltas_dict: dict[str, int]
        """
        self.artifact_reference = artifact_reference
        self.deltas_dict = deltas_dict


class DeltaPatternTrigger(ReactorDataObject):
    def __init__(self, delta_pattern_list):
        """
        :type delta_pattern_list: list[DeltaPattern]
        """
        self.delta_pattern_list = delta_pattern_list


class DynamicTriggerPlaceholder(ReactorDataObject):
    def __init__(self):
        """
        """


class DemandTriggerPlaceholder(ReactorDataObject):
    def __init__(self):
        """
        """


class Triggers(ReactorDataObject):
    def __init__(self, cron_trigger=None, inputs_trigger=None,
                 selected_artifacts_trigger=None, delta_pattern_trigger=None, dynamic_trigger=None, demand_trigger=None):
        """
        :type cron_trigger: CronTrigger
        :type inputs_trigger: InputsTrigger
        :type selected_artifacts_trigger: SelectedArtifactsTrigger
        :type delta_pattern_trigger: DeltaPatternTrigger
        :type dynamic_trigger: DynamicTriggerPlaceholder
        :type demand_trigger: DemandTriggerPlaceholder
        """

        self.cron_trigger = cron_trigger
        self.inputs_trigger = inputs_trigger
        self.selected_artifacts_trigger = selected_artifacts_trigger
        self.delta_pattern_trigger = delta_pattern_trigger
        self.dynamic_trigger = dynamic_trigger
        self.demand_trigger = demand_trigger


class OperationGlobals(ReactorDataObject):
    def __init__(self, expression=None, global_variables=None):
        """
        :type expression: Expression
        :type global_variables: dict[str, ExpressionGlobalVariable]
        """
        self.expression = expression
        self.global_variables = global_variables


class LatestUpdateResolver(ReactorDataObject):
    pass  # no attrs


class EqualAttributesResolver(ReactorDataObject):
    def __init__(self, artifact_to_attr):
        """
        :type artifact_to_attr: dict[int, str]
        """
        self.artifact_to_attr = artifact_to_attr


class InputResolver(ReactorDataObject):
    def __init__(self, latest_update_resolver=None, equal_attr_resolver=None):
        """
        :type latest_update_resolver: LatestUpdateResolver
        :type equal_attr_resolver: EqualAttributesResolver
        """
        self.latest_update_resolver = latest_update_resolver
        self.equal_attr_resolver = equal_attr_resolver


class DeprecationSensitivity(Enum):
    NON_SENSITIVE = 0
    STOP = 1
    STOP_AND_RECALCULATE = 2
    TRIGGER_BY_REPLACE = 3


class DeprecationStrategy(ReactorDataObject):
    def __init__(self, artifact=None, sensitivity=None):
        """
        :type artifact: ArtifactIdentifier
        :type sensitivity: DeprecationSensitivity
        """
        self.artifact = artifact
        self.sensitivity = sensitivity


class StartConfiguration(ReactorDataObject):
    def __init__(self, triggers=None, inputs_resolver=None, operation_globals=None, deprecation_strategies=None):
        """
        :type triggers: Triggers
        :type inputs_resolver: InputResolver
        :type operation_globals: OperationGlobals
        :type deprecation_strategies: list[DeprecationStrategy]
        """
        self.triggers = triggers
        self.inputs_resolver = inputs_resolver
        self.operation_globals = operation_globals
        self.deprecation_strategies = deprecation_strategies


class ParametersValueLeaf(ReactorDataObject):
    def __init__(self, value=None, generic_value=None):
        """
        :param value: binary encoding of value
        :type value: str
        :type generic_value: Metadata
        """
        self.value = value
        self.generic_value = generic_value


class ParametersValueList(ReactorDataObject):
    def __init__(self, elements=None):
        """
        :type elements: list[ParametersValueElement]
        """
        self.elements = elements if elements is not None else []


class ParametersValueNode(ReactorDataObject):
    def __init__(self, nodes=None):
        """
        :type nodes: dict[str, ParametersValueElement]
        """
        self.nodes = nodes if nodes is not None else {}


class ParametersValueElement(ReactorDataObject):
    def __init__(self, value=None, node=None, list_=None):
        """
        :type value: ParametersValueLeaf
        :type node: ParametersValueNode
        :type list_: ParametersValueList
        """
        self.value = value
        self.node = node
        self.list_ = list_


class ParametersValue(ReactorDataObject):
    def __init__(self, root_node=None):
        """
        :type root_node: ParametersValueNode
        """
        self.root_node = root_node


class ArtifactReference(ArtifactIdentifier):
    def __init__(self, artifact_id=None, namespace_id=None, namespace_identifier=None):
        """
        :type artifact_id: int
        :type namespace_id: int
        :type namespace_identifier: NamespaceIdentifier
        """
        super(ArtifactReference, self).__init__(artifact_id, namespace_identifier)
        self.namespace_id = namespace_id


class InputsValueConst(ReactorDataObject):
    def __init__(self, const_value=None, generic_value=None):
        """
        :type const_value: str
        :type generic_value: Metadata
        """
        self.const_value = const_value
        self.generic_value = generic_value


class Expression(ReactorDataObject):
    def __init__(self, expression):
        """
        :type expression: str
        """
        self.expression = expression


class ExpressionVariable(ReactorDataObject):
    def __init__(self, var_name, artifact_type_id=None):
        self.var_name = var_name
        self.artifact_type_id = artifact_type_id


class InputsValueRef(ReactorDataObject):
    def __init__(self, artifact_reference=None, const_value=None, expression=None, expression_var=None):
        """
        :type artifact_reference: ArtifactReference
        :type const_value: InputsValueConst
        :type expression: Expression
        :type expression_var: ExpressionVariable
        """
        self.artifact_reference = artifact_reference
        self.const_value = const_value
        self.expression = expression
        self.expression_var = expression_var


class InputsValueElement(ReactorDataObject):
    def __init__(self, value=None, node=None, list_=None):
        """
        :type value: InputsValueRef
        :type node: InputsValueNode
        :type list_: InputsValueList
        """
        self.value = value
        self.node = node
        self.list_ = list_


class InputsValueList(ReactorDataObject):
    def __init__(self, elements=None):
        """
        :type elements: list[InputsValueElement]
        """
        self.elements = elements if elements is not None else []


class InputsValueNode(ReactorDataObject):
    def __init__(self, nodes=None):
        """
        :type nodes: dict[str: InputsValueElement]
        """
        self.nodes = nodes if nodes is not None else {}


class InputsValue(ReactorDataObject):
    def __init__(self, root_node):
        """
        :type root_node: InputsValueNode
        """
        self.root_node = root_node


class OutputsValueRef(ReactorDataObject):
    def __init__(self, artifact_reference=None, expression=None, expression_var=None):
        """
        :type artifact_reference: ArtifactReference
        :type expression: Expression
        :type expression_var: ExpressionVariable
        """
        self.artifact_reference = artifact_reference
        self.expression = expression
        self.expression_var = expression_var


class OutputsValueElement(ReactorDataObject):
    def __init__(self, value=None, node=None, list_=None):
        """
        :type value: OutputsValueRef
        :type node: OutputsValueNode
        :type list_: OutputsValueList
        """
        self.value = value
        self.node = node
        self.list_ = list_


class OutputsValueList(ReactorDataObject):
    def __init__(self, elements=None):
        """
        :type elements: list[OutputsValueElement]
        """
        self.elements = elements if elements is not None else []


class OutputsValueNode(ReactorDataObject):
    def __init__(self, nodes=None):
        """
        :type nodes: dict[str, OutputsValueElement] | None
        """
        self.nodes = nodes if nodes is not None else {}


class OutputsValue(ReactorDataObject):
    def __init__(self, root_node, expression_after_success=None, expression_after_failure=None):
        """
        :type root_node: OutputsValueNode
        :type expression_after_success: Expression
        :type expression_after_failure: Expression
        """
        self.root_node = root_node
        self.expression_after_success = expression_after_success
        self.expression_after_failure = expression_after_failure


class ProjectIdentifier(ReactorDataObject):
    def __init__(self, project_id=None, namespace_identifier=None, project_key=None):
        """
        :type project_id: int | None
        :type namespace_identifier: NamespaceIdentifier | None
        :type project_key: str | None
        """
        self.project_id = project_id
        self.namespace_identifier = namespace_identifier
        self.project_key = project_key


class TtlCleanupStrategy(ReactorDataObject):
    def __init__(self, ttl_days):
        """
        :type ttl_days: int
        """
        self.ttl_days = ttl_days


class CleanupStrategy(ReactorDataObject):
    def __init__(self, ttl_cleanup_strategy):
        """
        :type ttl_cleanup_strategy: TtlCleanupStrategy
        """
        self.ttl_cleanup_strategy = ttl_cleanup_strategy


class CleanupStrategyDescriptor(ReactorDataObject):
    def __init__(self, cleanup_strategies):
        """
        :type cleanup_strategies: List[CleanupStrategy]
        """
        self.cleanup_strategies = cleanup_strategies


class OperationDescriptor(ReactorDataObject):
    def __init__(self, namespace_desc=None, operation_type_identifier=None, start_conf=None,
                 parameters=None, inputs=None, outputs=None, version=None, dynamic_trigger_list=None,
                 project_identifier=None, cleanup_strategy=None):
        """
        :type namespace_desc: NamespaceDescriptor
        :type operation_type_identifier: OperationTypeIdentifier
        :type start_conf: StartConfiguration
        :type parameters: ParametersValue
        :type inputs: InputsValue
        :type outputs: OutputsValue
        :type version: int
        :type dynamic_trigger_list: DynamicTriggerList
        :type project_identifier: ProjectIdentifier
        :type cleanup_strategy: CleanupStrategyDescriptor
        """
        self.namespace_desc = namespace_desc
        self.operation_type_identifier = operation_type_identifier
        self.start_conf = start_conf
        self.parameters = parameters
        self.inputs = inputs
        self.outputs = outputs
        self.version = version
        self.dynamic_trigger_list = dynamic_trigger_list
        self.project_identifier = project_identifier
        self.cleanup_strategy = cleanup_strategy


class ReactionReference(ReactorDataObject):
    def __init__(self, reaction_id=None, namespace_id=None):
        """
        :type reaction_id: int
        :type namespace_id: int
        """
        self.reaction_id = reaction_id
        self.namespace_id = namespace_id


class NirvanaUpdatePolicy(Enum):
    IGNORE = "Ignore"
    MINIMUM_NOT_DEPRECATED = 'Minimum not deprecated'
    MINIMUM_NOT_MANDATORY_DEPRECATED = 'Minimum not mandatory deprecated'


class NirvanaResultCloningPolicy(Enum):
    DO_NOT_CLONE = None
    TOP_LEVEL = "simple"
    RECURSIVE = "deep"


class GreetResponse(ReactorDataObject):
    def __init__(self, id_, login, message):
        """
        :type id_: int
        :param type: str
        :param type: str
        """
        self.id_ = id_
        self.login = login
        self.message = message


class NamespaceType(Enum):
    ROOT = 0
    NODE = 1
    LEAF_ARTIFACT = 2
    LEAF_OPERATION = 3
    LEAF_QUEUE = 4
    LEAF_SLICE = 5


class Namespace(ReactorDataObject):
    def __init__(self, id_, parent_id, type_, name, description=None):
        """
        :type id_: int
        :type parent_id: int
        :type type_: NamespaceType | UnknownEnumValue
        :type name: str
        :type description: str
        """

        self.id_ = id_
        self.parent_id = parent_id
        self.type_ = type_
        self.name = name
        self.description = description


class NamespaceCreateRequest(ReactorDataObject):
    def __init__(self, namespace_identifier, description=None, permissions=None, create_parents=False, create_if_not_exist=True):
        """
        :type namespace_identifier: NamespaceIdentifier
        :type description: str
        :type permissions: NamespacePermissions
        :type create_parents: bool
        :type create_if_not_exist: bool
        """
        self.namespace_identifier = namespace_identifier
        self.description = description
        self.permissions = permissions
        self.create_parents = create_parents
        self.create_if_not_exist = create_if_not_exist


class NamespaceCreateResponse(ReactorDataObject):
    def __init__(self, namespace_id):
        """
        :type namespace_id: int
        """
        self.namespace_id = namespace_id


class OperationIdentifier(ReactorDataObject):
    def __init__(self, operation_id=None, namespace_identifier=None):
        """
        :type operation_id: int
        :type namespace_identifier: NamespaceIdentifier
        """
        self.operation_id = operation_id
        self.namespace_identifier = namespace_identifier

    def __hash__(self):
        return hash((self.operation_id, self.namespace_identifier))


class OperationStatus(Enum):
    INITIALIZING = 0
    INACTIVE = 1
    ACTIVE = 2
    DEPRECATED = 3
    DELETED = 4
    ACTIVATING = 5,
    DEACTIVATING = 6


class Operation(ReactorDataObject):
    def __init__(self, id_, operation_type_id, namespace_id, status,
                 start_conf, parameters, inputs, outputs,
                 project_id=None, ttl=None, dynamic_trigger_list=None):
        """
        :type id_: int
        :type operation_type_id: int
        :type namespace_id: int
        :type status: OperationStatus | UnknownEnumValue
        :type start_conf: StartConfiguration
        :type parameters: ParametersValue
        :type inputs: InputsValue
        :type outputs: OutputsValue
        :type project_id: int
        :type ttl: int
        :type dynamic_trigger_list: DynamicTriggerList
        """
        self.id_ = id_
        self.operation_type_id = operation_type_id
        self.namespace_id = namespace_id
        self.status = status
        self.start_conf = start_conf
        self.parameters = parameters
        self.inputs = inputs
        self.outputs = outputs
        self.project_id = project_id
        self.ttl = ttl
        self.dynamic_trigger_list = dynamic_trigger_list


class ArtifactTypes(Enum):
    """
    Warning! Integer ids may be different on every reactor instance.
    """
    YT_PATH = 0
    YT_LIST_PATH = 1
    EVENT = 2
    PRIMITIVE_STRING = 3


class StatusUpdate(Enum):
    UNDEFINED_UPDATE = 0
    ACTIVATE = 1
    DEACTIVATE = 2
    DEPRECATE = 3


class ReactionStatusUpdate(ReactorDataObject):
    def __init__(self, reaction, status_update):
        """
        :type reaction: OperationIdentifier
        :type status_update: StatusUpdate | UnknownEnumValue
        """
        self.reaction = reaction
        self.status_update = status_update


class PerArtifactCountStrategy(ReactorDataObject):
    def __init__(self, limit):
        """
        :param limit: int
        """
        self.limit = limit


class WaterlineStrategies(ReactorDataObject):
    def __init__(self, per_artifact_count_strategy):
        """
        :param per_artifact_count_strategy: PerArtifactCountStrategy
        """
        self.per_artifact_count_strategy = per_artifact_count_strategy


class ReactionStartConfigurationUpdate(ReactorDataObject):
    def __init__(self, reaction_identifier, waterline_strategies):
        """
        :type reaction_identifier: OperationIdentifier
        :type waterline_strategies: WaterlineStrategies
        """
        self.reaction_identifier = reaction_identifier
        self.waterline_strategies = waterline_strategies


class OperationTypeStatus(Enum):
    ACTIVE = 0
    DEPRECATED = 1
    READONLY = 2


class ReactionType(ReactorDataObject):
    """
    Warning! Ignores parameter descriptors
    """
    def __init__(self, id_, operation_set_key, operation_key, version,
                 operation_set_name, operation_name, description, status):
        """
        :type id_: int
        :type operation_set_key: str
        :type operation_key: str
        :type version: str
        :type operation_set_name: str
        :type operation_name: str
        :type description: str
        :type status: OperationTypeStatus | UnknownEnumValue
        """
        self.id_ = id_
        self.operation_set_key = operation_set_key
        self.operation_key = operation_key
        self.version = version
        self.operation_set_name = operation_set_name
        self.operation_name = operation_name
        self.description = description
        self.status = status


class RetryPolicyDescriptor(Enum):
    UNIFORM = {"key": "uniformRetry", "time_param_key": "delay"}
    EXPONENTIAL = {"key": "exponentialRetry", "time_param_key": "totalTime"}


class RetryPolicyDescriptorMultipleParams(Enum):
    UNIFORM = {
        'key': 'uniformRetry',
        'time_params': ['delay']
    }
    EXPONENTIAL = {
        'key': 'exponentialRetry',
        'time_params': ['totalTime']
    }
    RANDOM = {
        'key': 'randomRetry',
        'time_params': ['delayVariance', 'minDelay']
    }


class NotificationEventType(Enum):
    ARTIFACT_ACTIVATED = 0
    ARTIFACT_DEPRECATED = 1
    ARTIFACT_DELETED = 2
    REACTION_STARTED = 3
    REACTION_FAILED = 4
    REACTION_FINISHED = 5
    REACTION_CANCELED = 6
    REACTION_TIMEOUT_ON_FAILURE = 7
    NAMESPACE_MOVED = 8
    ARTIFACT_INSTANCE_APPEARANCE_DELAY = 9
    ARTIFACT_INSTANCE_DEPRECATION_PROBLEM = 10
    REACTION_LONG_RUNNING_WARN = 11
    REACTION_LONG_RUNNING_CRIT = 12
    REACTION_RECALCULATION_PROBLEM = 13


class NotificationTransportType(Enum):
    SMS = 0
    TELEGRAM = 1
    EMAIL = 2


class Notification(ReactorDataObject):
    def __init__(self, id_, namespace_id, event_type, transport, recipient):
        """
        :type id_: int
        :type namespace_id: int
        :type event_type: NotificationEventType
        :type transport: NotificationTransportType
        :type recipient: str
        """
        self.id_ = id_
        self.namespace_id = namespace_id
        self.event_type = event_type
        self.transport = transport
        self.recipient = recipient


class NotificationDescriptor(ReactorDataObject):
    """
    :type namespace_identifier: NamespaceIdentifier
    :type event_type: NotificationEventType
    :type transport: NotificationTransportType
    :type recipient: str
    """
    def __init__(self, namespace_identifier, event_type, transport, recipient):
        self.namespace_identifier = namespace_identifier
        self.event_type = event_type
        self.transport = transport
        self.recipient = recipient


class InputsInstanceConstHolder(ReactorDataObject):
    def __init__(self):
        pass  # ToDo this is reactor bug, fix later


class InputsInstanceExpression(ReactorDataObject):
    def __init__(self, input_refs, output_ref):
        """
        :type input_refs: list[ArtifactInstanceReference]
        :type output_ref: ArtifactInstanceReference
        """

        self.input_refs = input_refs
        self.output_ref = output_ref


class InputsInstanceValue(ReactorDataObject):
    def __init__(self, artifact_instance_reference=None, const_value=None, expression=None):
        """
        :type artifact_instance_reference: ArtifactInstanceReference
        :type const_value: InputsInstanceConstHolder
        :type expression:  InputsInstanceExpression
        """
        self.artifact_instance_reference = artifact_instance_reference
        self.const_value = const_value
        self.expression = expression


class InputsInstanceElement(ReactorDataObject):
    def __init__(self, value=None, node=None, list_=None):
        """
        :type value: InputsInstanceValue
        :type node: InputsInstanceNode
        :type list_: InputsInstanceList
        """
        self.value = value
        self.node = node
        self.list_ = list_


class InputsInstanceList(ReactorDataObject):
    def __init__(self, elements=None):
        """
        :type elements: list[InputsInstanceElement]
        """
        self.elements = elements if elements is not None else []


class InputsInstanceNode(ReactorDataObject):
    def __init__(self, nodes=None):
        """
        :type nodes: dict[str: InputsInstanceElement]
        """
        self.nodes = nodes if nodes is not None else {}


class InputsInstance(ReactorDataObject):
    def __init__(self, root_node=None, version=None):
        """
        :type root_node: InputsInstanceNode | Nona
        :type version: int | None
        """
        self.root_node = root_node
        self.version = version


class OutputsInstanceExpression(ReactorDataObject):
    def __init__(self, input_ref, output_ref):
        """
        :type input_ref: ArtifactInstanceReference
        :type output_ref: ArtifactInstanceReference
        """
        self.input_ref = input_ref
        self.output_ref = output_ref


class OutputsInstanceValueManualAction(ReactorDataObject):
    def __init__(self, deprectaion_target):
        """
        :type deprectaion_target: ArtifactInstanceReference
        """
        self.deprecation_target = deprectaion_target


class OutputsInstanceValue(ReactorDataObject):
    def __init__(self, artifact_instance_reference=None, expression=None, on_success_action=None):
        """
        :type artifact_instance_reference: ArtifactInstanceReference
        :type expression:  OutputsInstanceExpression
        """
        self.on_success_action = on_success_action
        self.artifact_instance_reference = artifact_instance_reference
        self.expression = expression


class OutputsInstanceElement(ReactorDataObject):
    def __init__(self, value=None, node=None, list_=None):
        """
        :type value: OutputsInstanceValue
        :type node: OutputsInstanceNode
        :type list_: OutputsInstanceList
        """
        self.value = value
        self.node = node
        self.list_ = list_


class OutputsInstanceList(ReactorDataObject):
    def __init__(self, elements=None):
        """
        :type elements: list[OutputsInstanceElement]
        """
        self.elements = elements if elements is not None else []


class OutputsInstanceNode(ReactorDataObject):
    def __init__(self, nodes=None):
        """
        :type nodes: dict[str: OutputsInstanceElement]
        """
        self.nodes = nodes if nodes is not None else {}


class OutputsInstance(ReactorDataObject):
    def __init__(self, root_node=None, version=None):
        """
        :type root_node: OutputsInstanceNode | None
        :type version: int | None
        """
        self.root_node = root_node
        self.version = version


class FilterTypes(Enum):
    EQUAL = 1
    NOT_EQUAL = 2
    LESS_THAN = 3
    LESS_OR_EQUAL = 4
    GREATER_THAN = 5
    GREATER_OR_EQUAL = 6


class OrderTypes(Enum):
    ASCENDING = 1
    DESCENDING = 2


class ReactionInstanceListRequest(ReactorDataObject):
    def __init__(self, reaction, bound_instance_id=None, filter_type=FilterTypes.GREATER_THAN,
                 order=OrderTypes.DESCENDING, limit=100):
        """
        :type reaction: r_objs.OperationIdentifier
        :type bound_instance_id: int | None
        :type filter_type: r_objs.FilterTypes
        :type order: r_objs.OrderTypes
        :param limit: limit lies between 1 and 100
        :type limit: int
        :rtype: r_objs.OperationInstance
        """
        self.reaction = reaction
        self.bound_instance_id = bound_instance_id
        self.filter_type = filter_type
        self.order = order
        self.limit = limit


class ConditionalArtifactInstanceDelta(ReactorDataObject):
    def __init__(self, artifact_instance_ref, delta_ms):
        """
        :type artifact_instance_ref: ArtifactInstanceReference
        :type delta_ms: int
        """
        self.artifact_instance_ref = artifact_instance_ref
        self.delta_ms = delta_ms


class ConditionalArtifactDeltaGroup(ReactorDataObject):
    def __init__(self, artifact_ref, alias_to_delta):
        """
        :type artifact_ref: ArtifactReference
        :type alias_to_delta: dict[str, ConditionalArtifactInstanceDelta]
        """
        self.artifact_ref = artifact_ref
        self.alias_to_delta = alias_to_delta


class ConditionalArtifacts(ReactorDataObject):
    def __init__(self, conditional_deltas_list):
        """
        :type conditional_deltas_list: list[ConditionalArtifactDeltaGroup]
        """
        self.conditional_deltas_list = conditional_deltas_list


class ReactorEntity(ReactorDataObject):
    def __init__(self, entity_type, entity_id, value):
        """
        :type entity_type: int
        :type entity_id: int
        :type value: str
        """
        self.entity_type = entity_type
        self.entity_id = entity_id
        self.value = value


class ReactorEntityType(ReactorDataObject):
    def __init__(self, entity_type_id, entity_id):
        """
        :type entity_type: int
        :type entity_id: int
        """
        self.entity_type_id = entity_type_id
        self.entity_id = entity_id


class InstantiationContext(ReactorDataObject):
    def __init__(self, triggered_by_refs=None, globals=None, conditional_artifacts=None, cron_schedule_time="",
                 user_time=None):
        """
        :param triggered_by_refs: list[ArtifactInstanceReference]
        :param globals: dict[str, ReactorEntity]
        :param conditional_artifacts: ConditionalArtifacts
        :type cron_schedule_time: str
        """
        self.triggered_by_refs = triggered_by_refs
        self.globals = globals
        self.conditional_artifacts = conditional_artifacts
        self.cron_schedule_time = cron_schedule_time
        self.user_time = user_time


class ReactionReplacementKind(Enum):
    UNDEFINED = 0
    REPLACE_AND_PRESERVE_OLD_VERSION = 1
    REPLACE_AND_DELETE_OLD_VERSION = 2


class ReactionInstanceStatus(Enum):
    CREATED = 0
    INITIALIZED = 1
    RUNNING = 2
    CANCELED = 3
    FAILED = 4
    COMPLETED = 5
    CANCELING = 6
    ASSIGNED = 7
    QUEUED = 8
    TIMEOUT = 9


class ReactionTriggerType(Enum):
    CRON = 0
    MANUAL = 1
    ARTIFACT_TRIGGER = 2
    ANY_OF = 3
    RESTART = 4


class OperationInstance(ReactorDataObject):
    def __init__(self, id_, operation_id, creator_id, description, creation_time, state, status, progress_msg,
                 progress_log, progress_rate, source, inputs, outputs, instantiation_context, completion_time=None):
        """
        :type id_: int
        :type operation_id: int
        :type creator_id: int
        :type description: str
        :type creation_time: datetime
        :type state: str
        :type status: ReactionInstanceStatus
        :type progress_msg: str
        :type progress_log: str
        :type progress_rate: float
        :type source: ReactionTriggerType
        :type inputs: InputsInstance
        :type outputs: OutputsInstance
        :type instantiation_context: InstantiationContext
        :type completion_time: datetime
        """
        self.id_ = id_
        self.operation_id = operation_id
        self.creator_id = creator_id
        self.description = description
        self.creation_time = creation_time
        self.state = state
        self.status = status
        self.progress_msg = progress_msg
        self.progress_log = progress_log
        self.progress_rate = progress_rate
        self.source = source
        self.inputs = inputs
        self.outputs = outputs
        self.instantiation_context = instantiation_context
        self.completion_time = completion_time


class OperationInstanceStatusView(ReactorDataObject):
    def __init__(self, id_, operation_id, status):
        """
        :type id_: int
        :type operation_id: int
        :type status: ReactionInstanceStatus
        """
        self.id_ = id_
        self.operation_id = operation_id
        self.status = status


class OperationInstanceStatus(ReactorDataObject):
    def __init__(self, host, status, operation_instance_id, update_timestamp):
        """
        :type host: str
        :type status: ReactionInstanceStatus
        :type operation_instance_id: int
        :type update_timestamp: datetime
        """
        self.host = host
        self.status = status
        self.operation_instance_id = operation_instance_id
        self.update_timestamp = update_timestamp


class TimestampRange(ReactorDataObject):
    def __init__(self, dt_from, dt_to):
        """
        :type dt_from: datetime
        :type dt_to: datetime
        """
        self.dt_from = dt_from
        self.dt_to = dt_to


class TimestampFilter(ReactorDataObject):
    def __init__(self, exact_time=None, time_range=None):
        """
        :type exact_time: datetime
        :type time_range: TimestampRange
        """

        self.exact_time = exact_time
        self.time_range = time_range


class ArtifactInstanceOrderBy(Enum):
    ID = 0
    CREATION_TIMESTAMP = 1
    USER_TIMESTAMP = 2


class ArtifactInstanceFilterDescriptor(ReactorDataObject):
    def __init__(self, artifact_identifier, user_timestamp_filter=None, limit=None, offset=None, order_by=None, statuses=None):
        """
        :type artifact_identifier: ArtifactIdentifier
        :type user_timestamp_filter: TimestampFilter
        :type limit: int
        :type offset: int
        :type order_by: ArtifactInstanceOrderBy
        :type statuses: List[ArtifactInstanceStatus]
        """
        self.artifact_identifier = artifact_identifier
        self.user_timestamp_filter = user_timestamp_filter
        self.limit = limit
        self.offset = offset
        self.order_by = order_by
        self.statuses = statuses


class QueuePriorityFunction(Enum):
    TIME_NEWEST_FIRST = 0
    TIME_ELDEST_FIRST = 1
    USER_TIME_NEWEST_FIRST = 2
    USER_TIME_ELDEST_FIRST = 3


class QueueMaxQueuedInstances(ReactorDataObject):
    def __init__(self, value):
        """
        :type value: int
        """
        self.value = value


class QueueMaxRunningInstancesPerReaction(ReactorDataObject):
    def __init__(self, value):
        """
        :type value: int
        """
        self.value = value


class QueueMaxQueuedInstancesPerReaction(ReactorDataObject):
    def __init__(self, value):
        """
        :type value: int
        """
        self.value = value


class CancelConstraintViolationPolicy(ReactorDataObject):
    pass


class TimeoutConstraintViolationPolicy(ReactorDataObject):
    pass


class Constraints(ReactorDataObject):
    def __init__(self,
                 unique_priorities=False,
                 unique_running_priorities=False,
                 unique_queued_priorities=False,
                 unique_per_reaction_running_priorities=False,
                 unique_per_reaction_queued_priorities=False,
                 cancel_constraint_violation_policy=None,
                 timeout_constraint_violation_policy=None):
        """
        :type unique_priorities: bool
        :type unique_running_priorities: bool
        :type unique_queued_priorities: bool
        :type unique_per_reaction_running_priorities: bool
        :type unique_per_reaction_queued_priorities: bool
        :type cancel_constraint_violation_policy: CancelConstraintViolationPolicy
        :type timeout_constraint_violation_policy: TimeoutConstraintViolationPolicy
        """
        self.unique_priorities = unique_priorities
        self.unique_running_priorities = unique_running_priorities
        self.unique_queued_priorities = unique_queued_priorities
        self.unique_per_reaction_running_priorities = unique_per_reaction_running_priorities
        self.unique_per_reaction_queued_priorities = unique_per_reaction_queued_priorities
        self.cancel_constraint_violation_policy = cancel_constraint_violation_policy
        self.timeout_constraint_violation_policy = timeout_constraint_violation_policy


class QueueConfiguration(ReactorDataObject):
    def __init__(self,
                 parallelism,
                 priority_function,
                 max_queued_instances=None,
                 max_running_instances_per_reaction=None,
                 max_queued_instances_per_reaction=None,
                 constraints=None):
        """
        :type parallelism: int
        :type priority_function: QueuePriorityFunction
        :type max_queued_instances: QueueMaxQueuedInstances
        :type max_running_instances_per_reaction: QueueMaxRunningInstancesPerReaction
        :type max_queued_instances_per_reaction: QueueMaxQueuedInstancesPerReaction
        :type constraints: Constraints
        """
        self.parallelism = parallelism
        self.priority_function = priority_function
        self.max_queued_instances = max_queued_instances
        self.max_running_instances_per_reaction = max_running_instances_per_reaction
        self.max_queued_instances_per_reaction = max_queued_instances_per_reaction
        self.constraints = constraints


class QueueDescriptor(ReactorDataObject):
    def __init__(self, namespace_descriptor, configuration,
                 project_identifier=None):
        """
        :type namespace_descriptor: NamespaceDescriptor
        :type configuration: QueueConfiguration
        :type project_identifier: ProjectIdentifier | None
        """
        self.namespace_descriptor = namespace_descriptor
        self.configuration = configuration
        self.project_identifier = project_identifier


class QueueIdentifier(ReactorDataObject):
    def __init__(self, queue_id=None, namespace_identifier=None):
        """
        :type queue_id: int
        :type namespace_identifier: NamespaceIdentifier
        """
        self.queue_id = queue_id
        self.namespace_identifier = namespace_identifier


class QueueOperationConfiguration(ReactorDataObject):
    def __init__(self):
        pass


class QueueOperation(ReactorDataObject):
    def __init__(self, reaction_id, configuration=None):
        """
        :type reaction_id: int
        :type configuration: QueueOperationConfiguration
        """
        self.reaction_id = reaction_id
        self.configuration = QueueOperationConfiguration()


class Queue(ReactorDataObject):
    def __init__(self, id_, namespace_id, configuration, project_id=None):
        """
        :type id_: int
        :type namespace_id: int
        :type configuration: QueueConfiguration
        :type project_id: int | None
        """
        self.id_ = id_
        self.namespace_id = namespace_id
        self.configuration = configuration
        self.project_id = project_id


class QueueReference(ReactorDataObject):
    def __init__(self, queue_id=None, namespace_id=None):
        """
        :type queue_id: int
        :type namespace_id: int
        """
        self.queue_id = queue_id
        self.namespace_id = namespace_id


class QueueReactions(ReactorDataObject):
    def __init__(self, queue=None, reactions=None):
        """
        :type queue: Queue
        :type reactions: list[QueueOperation]
        """
        self.queue = queue
        self.reactions = reactions


class QueueUpdateRequest(ReactorDataObject):
    def __init__(self,
                 queue_identifier,
                 new_queue_capacity=None,
                 remove_reactions=None,
                 add_reactions=None,
                 new_max_instances_in_queue=None,
                 new_max_instances_per_reaction_in_queue=None,
                 new_max_running_instances_per_reaction=None):
        """
        :type queue_identifier: QueueIdentifier
        :type new_queue_capacity: int | None
        :type remove_reactions: list[OperationIdentifier] | None
        :type add_reactions: list[OperationIdentifier] | None
        :type new_max_instances_in_queue: int | None
        :type new_max_instances_per_reaction_in_queue: int | None
        :type new_max_running_instances_per_reaction: int | None
        """
        self.queue_identifier = queue_identifier
        self.new_queue_capacity = new_queue_capacity
        self.remove_reactions = remove_reactions
        self.add_reactions = add_reactions
        self.new_max_instances_in_queue = new_max_instances_in_queue
        self.new_max_instances_per_reaction_in_queue = new_max_instances_per_reaction_in_queue
        self.new_max_running_instances_per_reaction = new_max_running_instances_per_reaction


class MetricType(Enum):
    REACTION_USERTIME_PROCESSING_DURATION = 1
    REACTION_USERTIME_PROCESSING_COUNT = 2
    REACTION_USERTIME_ATTEMPTS = 3
    REACTION_USERTIME_FAILURES = 4
    REACTION_AGGREGATED_PROCESSING_DURATION = 5
    REACTION_AGGREGATED_PROCESSING_COUNT = 6
    REACTION_AGGREGATED_RETRIES = 7
    REACTION_AGGREGATED_FAILURES = 8
    QUEUE_USERTIME_STATUS_COUNT = 9
    QUEUE_AGGREGATED_STATUS_COUNT = 10
    QUEUE_ELDEST = 11
    ARTIFACT_USERTIME_FIRST_DELAY = 12
    ARTIFACT_USERTIME_LAST_DELAY = 13
    ARTIFACT_USERTIME_COUNT = 14
    ARTIFACT_AGGREGATED_DELAY = 15


class MetricCreateRequest(ReactorDataObject):
    def __init__(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None, custom_tags=None):
        """
        :type metric_type: MetricType | UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :type custom_tags: Attributes | None
        """
        self.artifact_id = artifact_id
        self.queue_id = queue_id
        self.reaction_id = reaction_id
        self.metric_type = metric_type
        self.custom_tags = custom_tags


class MetricListRequest(ReactorDataObject):
    def __init__(self, artifact_id=None, queue_id=None, reaction_id=None):
        """
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        """
        self.artifact_id = artifact_id
        self.queue_id = queue_id
        self.reaction_id = reaction_id


class MetricUpdateRequest(ReactorDataObject):
    def __init__(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None, new_tags=None):
        """
        :type metric_type: MetricType | UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        :type new_tags: Attributes | None
        """
        self.artifact_id = artifact_id
        self.queue_id = queue_id
        self.reaction_id = reaction_id
        self.metric_type = metric_type
        self.new_tags = new_tags


class MetricDeleteRequest(ReactorDataObject):
    def __init__(self, metric_type, artifact_id=None, queue_id=None, reaction_id=None):
        """
        :type metric_type: MetricType | UnknownEnumValue
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        """
        self.artifact_id = artifact_id
        self.queue_id = queue_id
        self.reaction_id = reaction_id
        self.metric_type = metric_type


class MetricReference(ReactorDataObject):
    def __init__(self, metric_type, tags_list, artifact_id=None, queue_id=None, reaction_id=None):
        """
        :type metric_type: MetricType | UnknownEnumValue
        :type tags_list: list[Attributes]
        :type artifact_id: int
        :type queue_id: int
        :type reaction_id: int
        """
        self.artifact_id = artifact_id
        self.queue_id = queue_id
        self.reaction_id = reaction_id
        self.metric_type = metric_type
        self.tags_list = tags_list


class LongRunningOperationInstanceNotificationOptions(ReactorDataObject):
    def __init__(self, warn_percentile, warn_runs_count, warn_scale, crit_percentile, crit_runs_count, crit_scale):
        """
        :type warn_percentile: int
        :param warn_percentile: percentile of last instances that should be taken into account
        :type warn_runs_count: int
        :param warn_runs_count: number of last completed instances that should be taken into account
        :type warn_scale: float
        :param warn_scale: scale that running instance duration should be greater than durations of last complete instances
        :type crit_percentile: int
        :param crit_percentile: percentile of last instances that should be taken into account
        :type crit_runs_count: int
        :param crit_runs_count number of last completed instances that should be taken into account
        :type crit_scale: float
        :param crit_scale: scale that running instance duration should be greater than durations of last complete instances
        """

        self.warn_percentile = warn_percentile
        self.warn_runs_count = warn_runs_count
        self.warn_scale = warn_scale
        self.crit_percentile = crit_percentile
        self.crit_runs_count = crit_runs_count
        self.crit_scale = crit_scale


class ArtifactInstanceStatusRecord(ReactorDataObject):
    def __init__(self, status, update_timestamp):
        """
        :type status: ArtifactInstanceStatus
        :type update_timestamp: datetime
        """
        self.status = status
        self.update_timestamp = update_timestamp


class ExpressionGlobalVariableOrigin(Enum):
    UNDEFINED_ORIGIN = 0
    TRIGGER = 1
    BEFORE = 2


class ExpressionGlobalVariableUsage(Enum):
    UNDEFINED_USAGE = 0
    BEFORE_EXPRESSION = 1
    INPUT_VARIABLE = 2
    AFTER_SUCCESS_EXPRESSION = 3
    AFTER_FAIL_EXPRESSION = 4


class ExpressionGlobalVariable(ReactorDataObject):
    def __init__(self, origin=None, identifier=None, type=None, usage=None):
        """
        :type origin: ExpressionGlobalVariableOrigin
        :type identifier: string
        :type type: ReactorEntityType
        :type usage: list[ExpressionGlobalVariableUsage] | None
        """
        self.origin = origin
        self.identifier = identifier
        self.type = type
        self.usage = usage


class DynamicTriggerReplacementRequest(ReactorDataObject):
    def __init__(self, source_trigger_id):
        """
        :type source_trigger_id: int
        """
        self.source_trigger_id = source_trigger_id


class DynamicArtifactTrigger(ReactorDataObject):
    def __init__(self, triggers):
        """
        :type triggers: List[ArtifactReference]
        """
        self.triggers = triggers


class DynamicTrigger(ReactorDataObject):
    def __init__(self, trigger_name=None, expression=None, replacement=None,
                 cron_trigger=None, artifact_trigger=None):
        """
        :type trigger_name: string
        :type expression: Expression
        :type replacement: DynamicTriggerReplacementRequest
        :type cron_trigger: CronTrigger
        :type artifact_trigger: DynamicArtifactTrigger
        """
        self.trigger_name = trigger_name
        self.expression = expression
        self.replacement = replacement
        self.cron_trigger = cron_trigger
        self.artifact_trigger = artifact_trigger


class DynamicTriggerList(ReactorDataObject):
    def __init__(self, triggers=None):
        """
        :type triggers: list[DynamicTrigger] | None
        """
        self.triggers = triggers


class TriggerType(Enum):
    UNDEFINED_TYPE = 0
    CRON = 1
    ARTIFACT = 2


class TriggerStatus(Enum):
    UNDEFINED_STATUS = 0
    INACTIVE = 1
    ACTIVATING = 2
    ACTIVE = 3
    DEACTIVATING = 4
    DELETED = 5


class DynamicTriggerDescriptor(ReactorDataObject):
    def __init__(self, id_, reaction_id, type_, status,
                 name, creation_time, data):
        """
        :type id_: int
        :type reaction_id: int
        :type type_: TriggerType
        :type status: TriggerStatus
        :type name: str
        :type creation_time: datetime
        :type data: DynamicTrigger
        """
        self.id_ = id_
        self.reaction_id = reaction_id
        self.type_ = type_
        self.status = status
        self.name = name
        self.creation_time = creation_time
        self.data = data


class DynamicTriggerAction(Enum):
    UNDEFINED_ACTION = 0
    ACTIVATE = 1
    DEACTIVATE = 2


class DynamicTriggerIdentifier(ReactorDataObject):
    def __init__(self, id_=None, name=None):
        """
        :type id_: int | None
        :type name: str | None
        """
        self.id_ = id_
        self.name = name


class DynamicTriggerStatusUpdate(ReactorDataObject):
    def __init__(self, reaction, triggers, action):
        """
        :type reaction: OperationIdentifier
        :type triggers: list[DynamicTriggerIdentifier]
        :type action: DynamicTriggerAction
        """
        self.reaction = reaction
        self.triggers = triggers
        self.action = action


class CalculationDescriptor(ReactorDataObject):
    def __init__(self, folder, runner, dependency_resolver, artifacts=None, tags=None):
        """
        :type folder: NamespaceDescriptor
        :type runner: OperationDescriptor
        :type dependency_resolver: OperationDescriptor
        :type runner: list[ArtifactInstanceCreateRequest] | None
        :type tags: dict[str, str] | None
        """
        self.folder = folder
        self.runner = runner
        self.dependency_resolver = dependency_resolver
        self.artifacts = artifacts
        self.tags = tags


class CalculationReference(ReactorDataObject):
    def __init__(self, folder_id, runner_id, dependency_resolver_id):
        """
        :type folder_id: int
        :type runner_id: int
        :type dependency_resolver_id: int
        """
        self.folder_id = folder_id
        self.runner_id = runner_id
        self.dependency_resolver_id = dependency_resolver_id


class CalculationStatus(Enum):
    UNDEFINED_STATUS = 0
    ACTIVE = 1
    UPDATING = 2


class CalculationRuntimeMetadata(ReactorDataObject):
    def __init__(self, calculation_id, version, status):
        """
        :type calculation_id: int
        :type version: int
        :type status: CalculationStatus
        """
        self.calculation_id = calculation_id
        self.version = version
        self.status = status


class Calculation(ReactorDataObject):
    def __init__(self, folder_id, runner_id, dependency_resolver_id, metadata, tags=None):
        """
        :type folder_id: int
        :type runner_id: int
        :type dependency_resolver_id: int
        :type metadata: CalculationRuntimeMetadata
        :type tags: dict[str, str] | None
        """
        self.folder_id = folder_id
        self.runner_id = runner_id
        self.dependency_resolver_id = dependency_resolver_id
        self.metadata = metadata
        self.tags = tags


class YtPathMappingRule(ReactorDataObject):
    def __init__(self, yt_path_pattern, artifact_pattern, artifact=None, timestamp_pattern=None):
        """
        :type yt_path_pattern: str
        :type artifact_pattern: str
        :type artifact: ArtifactIdentifier | None
        :type timestamp_pattern: str | None
        """
        self.yt_path_pattern = yt_path_pattern
        self.artifact_pattern = artifact_pattern
        self.artifact = artifact
        self.timestamp_pattern = timestamp_pattern


class YtPathMappedArtifact(ReactorDataObject):
    def __init__(self, artifact, timestamp=None):
        """
        :type artifact: ArtifactReference
        :type timestamp: str | None
        """
        self.artifact = artifact
        self.timestamp = timestamp


class YtPathMappingCandidates(ReactorDataObject):
    def __init__(self, yt_path, candidates):
        """
        :type yt_path: str
        :type candidates: list[YtPathMappedArtifact]
        """
        self.yt_path = yt_path
        self.candidates = candidates


ARTIFACT_TYPE_PREFIX = '/yandex.reactor.artifact'


class ArtifactValueProto(Enum):
    INT = '{}.IntArtifactValueProto'.format(ARTIFACT_TYPE_PREFIX)
    STR = '{}.StringArtifactValueProto'.format(ARTIFACT_TYPE_PREFIX)
    BOOL = '{}.BoolArtifactValueProto'.format(ARTIFACT_TYPE_PREFIX)
    FLOAT = '{}.FloatArtifactValueProto'.format(ARTIFACT_TYPE_PREFIX)
    LIST_INT = '{}.IntListArtifactValueProto'.format(ARTIFACT_TYPE_PREFIX)
    LIST_STR = '{}.StringListArtifactValueProto'.format(ARTIFACT_TYPE_PREFIX)
    LIST_FLOAT = '{}.FloatListArtifactValueProto'.format(ARTIFACT_TYPE_PREFIX)


class Variable:
    def __init__(self, key, cast, proto):
        """
        :type key: str
        :type cast: type = str
        :type proto: ArtifactValueProto = ArtifactValueProto.STR
        """
        self.key = key
        self.cast = cast
        self.proto = proto


class VariableTypes(Enum):
    BOOL = Variable(
        key='bool', cast=bool, proto=ArtifactValueProto.BOOL.value
    )
    STR = Variable(
        key='str', cast=str, proto=ArtifactValueProto.STR.value
    )
    INT = Variable(
        key='int', cast=int, proto=ArtifactValueProto.INT.value
    )
    FLOAT = Variable(
        key='float', cast=float, proto=ArtifactValueProto.FLOAT.value
    )
    LIST_INT = Variable(
        key='list_int', cast=int, proto=ArtifactValueProto.LIST_INT.value
    )
    LIST_FLOAT = Variable(
        key='list_float', cast=float, proto=ArtifactValueProto.LIST_FLOAT.value
    )
    LIST_STR = Variable(
        key='list_str', cast=str, proto=ArtifactValueProto.LIST_STR.value
    )


class ArtifactInstancesDeprecationRequest(ReactorDataObject):
    def __init__(self, instances_to_deprecate, description):
        """
        :type instances_to_deprecate: List[int]
        :type description: str
        """
        self.instances_to_deprecate = instances_to_deprecate
        self.description = description
