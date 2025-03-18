from copy import deepcopy
import six
import marshmallow
from . import reactor_objects as r_objs
from . import marshmallow_custom as m_custom


class ReactorObjectValidationError(Exception):
    def __init__(self, messages, raw_data=None):
        self.messages = messages
        self.raw_data = raw_data

    def __repr__(self):
        if self.raw_data is not None:
            return self.__class__.__name__ + "({}), raw_data: {}".format(self.messages, self.raw_data)
        else:
            return self.__class__.__name__ + "({})".format(self.messages)

    def __str__(self):
        return self.__repr__()

    def __reduce__(self):
        return (type(self), (self.messages, self.raw_data))


class OneOfFieldsOpts(marshmallow.SchemaOpts):
    def __init__(self, meta, **kwargs):
        super(OneOfFieldsOpts, self).__init__(meta, **kwargs)
        self.one_of = getattr(meta, 'one_of', None)
        self.unknown = marshmallow.EXCLUDE if six.PY3 else None


class OneOfSchema(marshmallow.Schema):
    OPTIONS_CLASS = OneOfFieldsOpts

    @marshmallow.validates_schema
    def validate_one_of(self, data, **kwargs):
        if self.opts.one_of is not None:
            fields_in_schema = sum(f in data for
                                   f in self.opts.one_of)
            if fields_in_schema > 1:
                raise marshmallow.ValidationError('Only one of fields {} must be initialized'.format(self.opts.one_of))

    @marshmallow.post_dump
    def remove_nones(self, data, **kwargs):
        data = {k: v for k, v in six.iteritems(data) if v is not None}
        return data


class EntityReferenceSchema(marshmallow.Schema):
    type_ = marshmallow.fields.String(load_from="type", dump_to="type", data_key="type")
    entity_id = marshmallow.fields.String(load_from="entityId", dump_to="entityId", data_key="entityId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.EntityReference(**data)


class NamespaceIdentifierSchema(OneOfSchema):
    namespace_path = marshmallow.fields.String(load_from="namespacePath", dump_to="namespacePath", data_key="namespacePath")
    namespace_id = marshmallow.fields.Integer(load_from="namespaceId", dump_to="namespaceId", as_string=True, data_key="namespaceId")

    class Meta:
        one_of = ["namespace_id", "namespace_path"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.NamespaceIdentifier(**data)


class ArtifactIdentifierSchema(OneOfSchema):
    class Meta:
        one_of = ["artifact_id", "namespace_identifier"]

    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, data_key="artifactId")

    namespace_identifier = marshmallow.fields.Nested(
        NamespaceIdentifierSchema,
        load_from="namespaceIdentifier",
        dump_to="namespaceIdentifier",
        data_key="namespaceIdentifier"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactIdentifier(**data)


class ArtifactReferenceSchema(OneOfSchema):
    artifact_id = marshmallow.fields.Integer(dump_to="artifactId", load_from="artifactId", as_string=True, data_key="artifactId")
    namespace_id = marshmallow.fields.Integer(dump_to="namespaceId", load_from="namespaceId", as_string=True, data_key="namespaceId")
    namespace_identifier = marshmallow.fields.Nested(NamespaceIdentifierSchema, dump_to="namespace", load_from="namespace",
                                                     data_key="namespace")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactReference(**data)


class ArtifactSchema(OneOfSchema):
    artifact_id = marshmallow.fields.Integer(load_from="id", dump_to="id", as_string=True, data_key="id")
    artifact_type_id = marshmallow.fields.Integer(load_from="artifactTypeId", dump_to="artifactTypeId", as_string=True,
                                                  data_key="artifactTypeId")
    namespace_id = marshmallow.fields.Integer(load_from="namespaceId", dump_to="namespaceId", as_string=True, data_key="namespaceId")
    project_id = marshmallow.fields.Integer(
        load_from="projectId", dump_to="projectId", as_string=True, data_key="projectId"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Artifact(**data)


class MetadataSchema(OneOfSchema):
    type_ = marshmallow.fields.String(load_from="@type", dump_to="@type", required=False, allow_none=True, data_key="@type")
    dict_obj = m_custom.StructuredDict(required=True, keys=marshmallow.fields.String())

    @marshmallow.post_dump
    def make_dict(self, data, **kwargs):
        data = deepcopy(data)
        data = {k: v for k, v in six.iteritems(data) if v is not None}
        dict_obj = data.pop('dict_obj')
        data.update(dict_obj)
        return data

    @marshmallow.pre_load
    def gather_dict(self, data, **kwargs):
        data = deepcopy(data)
        dict_obj = {k: v for k, v in six.iteritems(data) if k != "@type"}
        return {"@type": data["@type"], "dict_obj": dict_obj}

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Metadata(**data)


class ArtifactTypeIdentifierSchema(OneOfSchema):
    artifact_type_id = marshmallow.fields.Integer(load_from="artifactTypeId", dump_to="artifactTypeId", as_string=True,
                                                  data_key="artifactTypeId")
    artifact_type_key = marshmallow.fields.String(load_from="artifactTypeKey", dump_to="artifactTypeKey", data_key="artifactTypeKey")

    class Meta:
        one_of = ["artifact_type_id", "artifact_type_key"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactTypeIdentifier(**data)


class ArtifactTypeSchema(OneOfSchema):
    artifact_type_id = marshmallow.fields.Integer(load_from="id", dump_to="id", data_key="id", as_string=True)
    key = marshmallow.fields.String()
    name = marshmallow.fields.String()
    description = marshmallow.fields.String()
    status = m_custom.EnumField(r_objs.ArtifactTypeStatus, r_objs.UnknownEnumValue)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactType(**data)


class NamespacePermissionsSchema(OneOfSchema):
    roles = m_custom.StructuredDict(keys=marshmallow.fields.String(),
                                    values=m_custom.EnumField(r_objs.NamespaceRole, r_objs.UnknownEnumValue))
    version = marshmallow.fields.Integer(load_from="version", dump_to="version", data_key="version")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.NamespacePermissions(**data)


class ArtifactCreateRequestSchema(OneOfSchema):
    artifact_type_identifier = marshmallow.fields.Nested(
        ArtifactTypeIdentifierSchema,
        load_from="artifactTypeIdentifier",
        dump_to="artifactTypeIdentifier",
        required=True, data_key="artifactTypeIdentifier"
    )

    artifact_identifier = marshmallow.fields.Nested(
        ArtifactIdentifierSchema,
        load_from="artifactIdentifier",
        dump_to="artifactIdentifier",
        required=True, data_key="artifactIdentifier"
    )

    description = marshmallow.fields.String()
    permissions = marshmallow.fields.Nested(NamespacePermissionsSchema)

    project_identifier = marshmallow.fields.Nested(
        "ProjectSchema",
        dump_to="projectIdentifier",
        load_from="projectIdentifier",
        data_key="projectIdentifier"
    )
    cleanup_strategy = marshmallow.fields.Nested(
        "CleanupStrategyDescriptorSchema",
        dump_to="cleanupStrategy",
        load_from="cleanupStrategy",
        data_key="cleanupStrategy"
    )

    create_parent_namespaces = marshmallow.fields.Bool(load_from="createParentNamespaces", dump_to="createParentNamespaces",
                                                       data_key="createParentNamespaces")
    create_if_not_exist = marshmallow.fields.Bool(load_from="createIfNotExist", dump_to="createIfNotExist", data_key="createIfNotExist")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactCreateRequest(**data)


class ArtifactCreateResponseSchema(OneOfSchema):
    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, data_key="artifactId")
    namespace_id = marshmallow.fields.Integer(load_from="namespaceId", dump_to="namespaceId", as_string=True, data_key="namespaceId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactCreateResponse(**data)


class AttributesSchema(OneOfSchema):
    key_value = m_custom.StructuredDict(
        keys=marshmallow.fields.String(),
        values=marshmallow.fields.String(),
        load_from="keyValue",
        dump_to="keyValue",
        data_key="keyValue"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Attributes(**data)


class ArtifactInstanceInstantiateRequestSchema(OneOfSchema):
    artifact_identifier = marshmallow.fields.Nested(
        ArtifactIdentifierSchema,
        load_from="artifactIdentifier",
        dump_to="artifactIdentifier",
        required=True,
        data_key="artifactIdentifier"
    )

    metadata = marshmallow.fields.Nested(
        MetadataSchema
    )
    attributes = marshmallow.fields.Nested(
        AttributesSchema
    )

    user_time = marshmallow.fields.DateTime(
        load_from="userTimestamp",
        dump_to="userTimestamp",
        format="iso",
        data_key="userTimestamp"
    )

    create_if_not_exist = marshmallow.fields.Bool(load_from="createIfNotExist", dump_to="createIfNotExist", data_key="createIfNotExist")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstanceInstantiateRequest(**data)


class ArtifactInstanceInstantiateResponseSchema(OneOfSchema):
    artifact_instance_id = marshmallow.fields.Integer(load_from="artifactInstanceId", dump_to="artifactInstanceId", as_string=True,
                                                      data_key="artifactInstanceId")
    creation_time = marshmallow.fields.DateTime(load_from="creationTimestamp", dump_to="creationTimestamp", format="iso",
                                                data_key="creationTimestamp")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstanceInstantiateResponse(**data)


class ArtifactInstanceCreateRequestSchema(OneOfSchema):
    artifact_create_request = marshmallow.fields.Nested(
        ArtifactCreateRequestSchema,
        load_from="artifactCreateRequest",
        dump_to="artifactCreateRequest",
        required=True,
        data_key="artifactCreateRequest"
    )

    metadata = marshmallow.fields.Nested(
        MetadataSchema
    )

    user_time = marshmallow.fields.DateTime(
        load_from="userTimestamp",
        dump_to="userTimestamp",
        format="iso",
        data_key="userTimestamp"
    )

    attributes = marshmallow.fields.Nested(
        AttributesSchema
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstanceCreateRequest(**data)


class ArtifactInstanceSchema(OneOfSchema):
    instance_id = marshmallow.fields.Integer(load_from="id", dump_to="id", as_string=True, required=True, data_key="id")
    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, required=True,
                                             data_key="artifactId")
    creator = marshmallow.fields.String(load_from="creatorLogin", dump_to="creatorLogin", required=True, data_key="creatorLogin")
    metadata = marshmallow.fields.Nested(MetadataSchema, required=True)
    attributes = marshmallow.fields.Nested(AttributesSchema, required=True)
    user_time = marshmallow.fields.DateTime(load_from="userTimestamp", dump_to="userTimestamp", format="iso", required=True,
                                            data_key="userTimestamp")
    creation_time = marshmallow.fields.DateTime(load_from="creationTimestamp", dump_to="creationTimestamp", format="iso", required=True,
                                                data_key="creationTimestamp")
    status = m_custom.EnumField(r_objs.ArtifactInstanceStatus, r_objs.UnknownEnumValue, required=True)
    source = m_custom.EnumField(r_objs.ArtifactInstanceSource, r_objs.UnknownEnumValue, required=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstance(**data)


class NamespaceDescriptorSchema(OneOfSchema):
    namespace_identifier = marshmallow.fields.Nested(
        NamespaceIdentifierSchema,
        load_from="namespaceIdentifier",
        dump_to="namespaceIdentifier",
        data_key="namespaceIdentifier"
    )

    description = marshmallow.fields.String()
    permissions = marshmallow.fields.Nested(
        NamespacePermissionsSchema
    )
    create_parent_namespaces = marshmallow.fields.Bool(
        load_from="createParentNamespaces",
        dump_to="createParentNamespaces",
        data_key="createParentNamespaces"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.NamespaceDescriptor(**data)


class OperationTypeKeySchema(OneOfSchema):
    set_key = marshmallow.fields.String(load_from="operationSetKey", dump_to="operationSetKey", data_key="operationSetKey")
    key = marshmallow.fields.String(load_from="operationKey", dump_to="operationKey", data_key="operationKey")
    version = marshmallow.fields.String(load_from="operationVersion", dump_to="operationVersion", data_key="operationVersion")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationTypeKey(**data)


class OperationTypeIdentifierSchema(OneOfSchema):
    operation_type_id = marshmallow.fields.Integer(load_from="operationTypeId", dump_to="operationTypeId", as_string=True,
                                                   data_key="operationTypeId")
    operation_type_key = marshmallow.fields.Nested(OperationTypeKeySchema, load_from="operationTypeKey",
                                                   dump_to="operationTypeKey", data_key="operationTypeKey")

    class Meta:
        one_of = ["operation_type_id", "operation_type_key"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationTypeIdentifier(**data)


class CronTriggerSchema(OneOfSchema):
    cron_expression = marshmallow.fields.String(load_from="cronExpression", dump_to="cronExpression", data_key="cronExpression")
    misfire_policy = m_custom.EnumField(r_objs.MisfirePolicy, r_objs.UnknownEnumValue, load_from="misfirePolicy",
                                        dump_to="misfirePolicy", data_key="misfirePolicy")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.CronTrigger(**data)


class SelectedArtifactsTriggerSchema(OneOfSchema):
    artifact_refs = marshmallow.fields.Nested(ArtifactReferenceSchema,
                                              many=True,
                                              load_from="artifactRefs",
                                              dump_to="artifactRefs", data_key="artifactRefs")
    relationship = m_custom.EnumField(r_objs.Relationship, r_objs.UnknownEnumValue)
    expression = marshmallow.fields.Nested("ExpressionSchema")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.SelectedArtifactsTrigger(**data)


class LatestUpdateResolverSchema(OneOfSchema):
    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.LatestUpdateResolver()


class EqualAttributesResolverSchema(OneOfSchema):
    artifact_to_attr = m_custom.StructuredDict(
        keys=marshmallow.fields.Integer(as_string=True),
        values=marshmallow.fields.String(),
        load_from="artifactId2attributeKey",
        dump_to="artifactId2attributeKey", data_key="artifactId2attributeKey"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.EqualAttributesResolver(**data)


class InputResolverSchema(OneOfSchema):
    latest_update_resolver = marshmallow.fields.Nested(
        LatestUpdateResolverSchema,
        load_from="latestUpdateResolverProto",
        dump_to="latestUpdateResolverProto", data_key="latestUpdateResolverProto"
    )
    equal_attr_resolver = marshmallow.fields.Nested(
        EqualAttributesResolverSchema,
        load_from="equalAttributesResolverProto",
        dump_to="equalAttributesResolverProto", data_key="equalAttributesResolverProto"
    )

    class Meta:
        one_of = ["latest_update_resolver", "equal_attr_resolver"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputResolver(**data)


class InputsTriggerSchema(OneOfSchema):
    artifact_refs = marshmallow.fields.Nested(
        ArtifactReferenceSchema,
        many=True,
        load_from="artifactRefs",
        dump_to="artifactRefs", data_key="artifactRefs"
    )
    all_inputs = marshmallow.fields.Boolean(load_from="allInputs", dump_to="allInputs", data_key="allInputs")
    any_input = marshmallow.fields.Boolean(load_from="anyInput", dump_to="anyInput", data_key="anyInput")

    class Meta:
        one_of = ["all_inputs", "any_input"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsTrigger(**data)


class DeltaPatternSchema(OneOfSchema):
    artifact_reference = marshmallow.fields.Nested(ArtifactReferenceSchema, load_from="artifactRef", dump_to="artifactRef",
                                                   data_key="artifactRef")
    deltas_dict = m_custom.StructuredDict(keys=marshmallow.fields.String(),
                                          values=marshmallow.fields.Integer(as_string=True),
                                          dump_to="alias2deltasMs",
                                          load_from="alias2deltasMs", data_key="alias2deltasMs")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DeltaPattern(**data)


class DeltaPatternTriggerSchema(OneOfSchema):
    delta_pattern_list = marshmallow.fields.Nested(DeltaPatternSchema, many=True,
                                                   load_from="userTimestampDeltaPattern",
                                                   dump_to="userTimestampDeltaPattern", data_key="userTimestampDeltaPattern")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DeltaPatternTrigger(**data)


class DynamicTriggerPlaceholderSchema(OneOfSchema):
    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicTriggerPlaceholder(**data)


class DemandTriggerPlaceholderSchema(OneOfSchema):
    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DemandTriggerPlaceholder(**data)


class TriggersSchema(OneOfSchema):
    cron_trigger = marshmallow.fields.Nested(
        CronTriggerSchema,
        load_from="cronTrigger",
        dump_to="cronTrigger", data_key="cronTrigger"
    )
    inputs_trigger = marshmallow.fields.Nested(
        InputsTriggerSchema,
        load_from="inputsTrigger",
        dump_to="inputsTrigger", data_key="inputsTrigger"
    )
    selected_artifacts_trigger = marshmallow.fields.Nested(
        SelectedArtifactsTriggerSchema,
        load_from="selectedArtifactsTrigger",
        dump_to="selectedArtifactsTrigger", data_key="selectedArtifactsTrigger"
    )

    delta_pattern_trigger = marshmallow.fields.Nested(
        DeltaPatternTriggerSchema,
        load_from="deltaPatternTrigger",
        dump_to="deltaPatternTrigger", data_key="deltaPatternTrigger"
    )

    dynamic_trigger = marshmallow.fields.Nested(
        DynamicTriggerPlaceholderSchema,
        load_from="dynamicTrigger",
        dump_to="dynamicTrigger", data_key="dynamicTrigger"
    )

    demand_trigger = marshmallow.fields.Nested(
        DemandTriggerPlaceholderSchema,
        load_from="demandTrigger",
        dump_to="demandTrigger", data_key="demandTrigger"
    )

    class Meta:
        one_of = ["cron_trigger", "inputs_trigger", "selected_artifacts_trigger", "delta_pattern_trigger", "dynamic_trigger",
                  "demand_trigger"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Triggers(**data)


class ExpressionSchema(OneOfSchema):
    expression = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Expression(**data)


class ExpressionVariableSchema(OneOfSchema):
    var_name = marshmallow.fields.String(load_from="variableName", dump_to="variableName", data_key="variableName")
    artifact_type_id = marshmallow.fields.Integer(load_from="artifactTypeId", dump_to="artifactTypeId", as_string=True,
                                                  data_key="artifactTypeId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ExpressionVariable(**data)


class ReactorEntityTypeSchema(OneOfSchema):
    entity_type_id = marshmallow.fields.Integer(dump_to="entityTypeId", load_from="entityTypeId", as_string=True,
                                                data_key="entityTypeId")
    entity_id = marshmallow.fields.Integer(dump_to="entityId", load_from="entityId", as_string=True, data_key="entityId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ReactorEntityType(**data)


class ExpressionGlobalVariableSchema(OneOfSchema):
    origin = m_custom.EnumField(r_objs.ExpressionGlobalVariableOrigin, r_objs.UnknownEnumValue)
    identifier = marshmallow.fields.String()
    type = marshmallow.fields.Nested(
        ReactorEntityTypeSchema
    )
    usage = marshmallow.fields.List(m_custom.EnumField(r_objs.ExpressionGlobalVariableUsage, r_objs.UnknownEnumValue))

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ExpressionGlobalVariable(**data)


class OperationGlobalsSchema(OneOfSchema):
    expression = marshmallow.fields.Nested(
        ExpressionSchema
    )

    global_variables = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested(
            ExpressionGlobalVariableSchema
        ),
        load_from="globalVariables",
        dump_to="globalVariables", data_key="globalVariables"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationGlobals(**data)


class DeprecationStrategySchema(OneOfSchema):
    artifact = marshmallow.fields.Nested(ArtifactIdentifierSchema, load_from="artifact", dump_to="artifact", data_key="artifact")
    sensitivity = m_custom.EnumField(r_objs.DeprecationSensitivity, r_objs.UnknownEnumValue, load_from="sensitivity",
                                     dump_to="sensitivity", data_key="sensitivity")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DeprecationStrategy(**data)


class StartConfigurationSchema(OneOfSchema):
    triggers = marshmallow.fields.Nested(
        TriggersSchema,
        load_from="triggersProto",
        dump_to="triggersProto", data_key="triggersProto"
    )
    inputs_resolver = marshmallow.fields.Nested(
        InputResolverSchema,
        load_from="inputResolverProto",
        dump_to="inputResolverProto", data_key="inputResolverProto"
    )
    operation_globals = marshmallow.fields.Nested(
        OperationGlobalsSchema,
        load_from="operationGlobals",
        dump_to="operationGlobals", data_key="operationGlobals"
    )
    deprecation_strategies = marshmallow.fields.List(
        marshmallow.fields.Nested(DeprecationStrategySchema),
        load_from="deprecationStrategies",
        dump_to="deprecationStrategies", data_key="deprecationStrategies"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.StartConfiguration(**data)


class ParametersValueLeafSchema(OneOfSchema):
    value = marshmallow.fields.String()
    generic_value = marshmallow.fields.Nested(MetadataSchema, load_from="genericValue", dump_to="genericValue", data_key="genericValue")

    class Meta:
        one_of = ["value", "generic_value"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ParametersValueLeaf(**data)


class ParametersValueListSchema(OneOfSchema):
    elements = marshmallow.fields.Nested(
        "ParametersValueElementSchema",  # refer as string because of circular dependency
        many=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ParametersValueList(**data)


class ParametersValueNodeSchema(OneOfSchema):
    nodes = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested("ParametersValueElementSchema")
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ParametersValueNode(**data)


class ParametersValueElementSchema(OneOfSchema):
    value = marshmallow.fields.Nested(ParametersValueLeafSchema)
    list_ = marshmallow.fields.Nested(ParametersValueListSchema, dump_to="list", load_from="list", data_key="list")
    node = marshmallow.fields.Nested(ParametersValueNodeSchema)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ParametersValueElement(**data)


class ParametersValueSchema(OneOfSchema):
    root_node = marshmallow.fields.Nested(ParametersValueNodeSchema, dump_to="rootNode", load_from="rootNode", data_key="rootNode")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ParametersValue(**data)


class InputsValueConst(OneOfSchema):
    const_value = marshmallow.fields.String(load_from="constValue", dump_to="constValue", data_key="constValue")
    generic_value = marshmallow.fields.Nested(MetadataSchema, load_from="genericConstValue", dump_to="genericConstValue",
                                              data_key="genericConstValue")

    class Meta:
        one_of = ["const_value", "generic_value"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsValueConst(**data)


class InputsValueRefSchema(OneOfSchema):
    artifact_reference = marshmallow.fields.Nested(ArtifactReferenceSchema, load_from="artifactRef", dump_to="artifactRef",
                                                   data_key="artifactRef")
    const_value = marshmallow.fields.Nested(InputsValueConst, load_from="constRef", dump_to="constRef", data_key="constRef")
    expression = marshmallow.fields.Nested(ExpressionSchema, load_from="expression", dump_to="expression", data_key="expression")
    expression_var = marshmallow.fields.Nested(ExpressionVariableSchema, load_from="expressionVariable", dump_to="expressionVariable",
                                               data_key="expressionVariable")

    class Meta:
        one_of = ["artifact_reference", "const_value", "expression", "expression_var"]

    @marshmallow.post_dump
    def nest_expression(self, data, **kwargs):
        if "expression" in data and data["expression"] is not None:
            data = deepcopy(data)
            data["expression"] = {"expression": data["expression"]}
        return data

    @marshmallow.pre_load
    def unnest_expression(self, data, **kwargs):
        if "expression" in data:
            data = deepcopy(data)
            data["expression"] = data["expression"]["expression"]
        return data

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsValueRef(**data)


class InputsValueListSchema(OneOfSchema):
    elements = marshmallow.fields.Nested(
        "InputsValueElementSchema",
        many=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsValueList(**data)


class InputsValueNodeSchema(OneOfSchema):
    nodes = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested("InputsValueElementSchema")
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsValueNode(**data)


class InputsValueElementSchema(OneOfSchema):
    value = marshmallow.fields.Nested(InputsValueRefSchema)
    list_ = marshmallow.fields.Nested(InputsValueListSchema, dump_to="list", load_from="list", data_key="list")
    node = marshmallow.fields.Nested(InputsValueNodeSchema)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsValueElement(**data)


class InputsValueSchema(OneOfSchema):
    root_node = marshmallow.fields.Nested(InputsValueNodeSchema, dump_to="rootNode", load_from="rootNode", data_key="rootNode")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsValue(**data)


class OutputsValueRefSchema(OneOfSchema):
    artifact_reference = marshmallow.fields.Nested(ArtifactReferenceSchema, load_from="artifactRef", dump_to="artifactRef",
                                                   data_key="artifactRef")
    expression = marshmallow.fields.Nested(ExpressionSchema, load_from="expression", dump_to="expression", data_key="expression")
    expression_var = marshmallow.fields.Nested(ExpressionVariableSchema, load_from="expressionVariable", dump_to="expressionVariable",
                                               data_key="expressionVariable")

    class Meta:
        one_of = ["artifact_reference", "expression", "expression_var"]

    @marshmallow.post_dump
    def nest_expression(self, data, **kwargs):
        if "expression" in data and data["expression"] is not None:
            data = deepcopy(data)
            data["expression"] = {"expression": data["expression"]}
        return data

    @marshmallow.pre_load
    def unnest_expression(self, data, **kwargs):
        if "expression" in data:
            data = deepcopy(data)
            data["expression"] = data["expression"]["expression"]
        return data

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsValueRef(**data)


class OutputsValueListSchema(OneOfSchema):
    elements = marshmallow.fields.Nested(
        "OutputsValueElementSchema",
        many=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsValueList(**data)


class OutputsValueNodeSchema(OneOfSchema):
    nodes = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested("OutputsValueElementSchema")
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsValueNode(**data)


class OutputsValueElementSchema(OneOfSchema):
    value = marshmallow.fields.Nested(OutputsValueRefSchema)
    list_ = marshmallow.fields.Nested(OutputsValueListSchema, dump_to="list", load_from="list", data_key="list")
    node = marshmallow.fields.Nested(OutputsValueNodeSchema)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsValueElement(**data)


class OutputsValueSchema(OneOfSchema):
    root_node = marshmallow.fields.Nested(OutputsValueNodeSchema, dump_to="rootNode", load_from="rootNode", data_key="rootNode")
    expression_after_success = marshmallow.fields.Nested(ExpressionSchema, dump_to="expressionAfterSuccess",
                                                         load_from="expressionAfterSuccess", data_key="expressionAfterSuccess")
    expression_after_failure = marshmallow.fields.Nested(ExpressionSchema, dump_to="expressionAfterFail",
                                                         load_from="expressionAfterFail", data_key="expressionAfterFail")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsValue(**data)


class DynamicTriggerReplacementRequestSchema(OneOfSchema):
    source_trigger_id = marshmallow.fields.Integer(
        load_from="sourceTriggerId", dump_to="sourceTriggerId",
        as_string=True, data_key="sourceTriggerId"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicTriggerReplacementRequest(**data)


class DynamicArtifactTriggerSchema(OneOfSchema):
    triggers = marshmallow.fields.Nested(
        ArtifactReferenceSchema, many=True,
        load_from="triggerArtifactReferences",
        dump_to="triggerArtifactReferences", data_key="triggerArtifactReferences"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicArtifactTrigger(**data)


class DynamicTriggerSchema(OneOfSchema):
    trigger_name = marshmallow.fields.String(load_from="triggerName", dump_to="triggerName", data_key="triggerName")
    expression = marshmallow.fields.Nested(ExpressionSchema, load_from="expression", dump_to="expression", data_key="expression")
    replacement = marshmallow.fields.Nested(
        DynamicTriggerReplacementRequestSchema,
        load_from="replacementRequest",
        dump_to="replacementRequest", data_key="replacementRequest"
    )
    cron_trigger = marshmallow.fields.Nested(CronTriggerSchema, load_from="cronTrigger", dump_to="cronTrigger", data_key="cronTrigger")
    artifact_trigger = marshmallow.fields.Nested(
        DynamicArtifactTriggerSchema,
        load_from="artifactTrigger",
        dump_to="artifactTrigger", data_key="artifactTrigger"
    )

    class Meta:
        one_of = ["artifact_trigger", "cron_trigger"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicTrigger(**data)


class DynamicTriggerListSchema(OneOfSchema):
    triggers = marshmallow.fields.Nested(DynamicTriggerSchema, many=True, load_from="triggers", dump_to="triggers", data_key="triggers")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicTriggerList(**data)


class DynamicTriggerIdentifierSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(
        dump_to="id", load_from="id", as_string=True, data_key="id"
    )
    name = marshmallow.fields.String()

    class Meta:
        one_of = ["id_", "name"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicTriggerIdentifier(**data)


class DynamicTriggerDescriptorSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(
        dump_to="id", load_from="id", as_string=True, data_key="id"
    )
    reaction_id = marshmallow.fields.Integer(
        dump_to="reactionId",
        load_from="reactionId",
        as_string=True, data_key="reactionId"
    )
    type_ = m_custom.EnumField(
        r_objs.TriggerType, r_objs.UnknownEnumValue,
        load_from="type",
        dump_to="type",
        data_key="type"
    )
    status = m_custom.EnumField(r_objs.TriggerStatus, r_objs.UnknownEnumValue)
    name = marshmallow.fields.String(dump_to="name", load_from="name")
    creation_time = marshmallow.fields.DateTime(
        load_from="creationTimestamp",
        dump_to="creationTimestamp",
        format="iso", data_key="creationTimestamp"
    )
    data = marshmallow.fields.Nested(
        DynamicTriggerSchema,
        load_from="data",
        dump_to="data",
        data_key="data"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicTriggerDescriptor(**data)


class DynamicTriggerStatusUpdateSchema(OneOfSchema):
    reaction = marshmallow.fields.Nested(
        "OperationIdentifierSchema",
        dump_to="reactionIdentifier", load_from="reactionIdentifier", data_key="reactionIdentifier"
    )
    triggers = marshmallow.fields.Nested(
        DynamicTriggerIdentifierSchema, many=True,
        dump_to="triggerIdentifier", load_from="triggerIdentifier", data_key="triggerIdentifier"
    )
    action = m_custom.EnumField(
        r_objs.DynamicTriggerAction, r_objs.UnknownEnumValue,
        dump_to="action", load_from="action", data_key="action"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.DynamicTriggerStatusUpdate(**data)


class OperationDescriptorSchema(OneOfSchema):
    namespace_desc = marshmallow.fields.Nested(
        NamespaceDescriptorSchema,
        dump_to="namespaceDescriptor",
        load_from="namespaceDescriptor", data_key="namespaceDescriptor"
    )

    operation_type_identifier = marshmallow.fields.Nested(
        OperationTypeIdentifierSchema,
        dump_to="operationTypeIdentifier",
        load_from="operationTypeIdentifier", data_key="operationTypeIdentifier"
    )

    start_conf = marshmallow.fields.Nested(
        StartConfigurationSchema,
        dump_to="startConfiguration",
        load_from="startConfiguration", data_key="startConfiguration"
    )

    parameters = marshmallow.fields.Nested(
        ParametersValueSchema,
        dump_to="parametersValue",
        load_from="parametersValue", data_key="parametersValue"
    )

    inputs = marshmallow.fields.Nested(
        InputsValueSchema,
        dump_to="inputsValue",
        load_from="inputsValue", data_key="inputsValue"
    )

    outputs = marshmallow.fields.Nested(
        OutputsValueSchema,
        dump_to="outputsValue",
        load_from="outputsValue", data_key="outputsValue"
    )

    version = marshmallow.fields.Integer()

    dynamic_trigger_list = marshmallow.fields.Nested(
        DynamicTriggerListSchema,
        dump_to="dynamicTriggerList",
        load_from="dynamicTriggerList", data_key="dynamicTriggerList"
    )

    project_identifier = marshmallow.fields.Nested(
        "ProjectSchema",
        dump_to="projectIdentifier",
        load_from="projectIdentifier", data_key="projectIdentifier"
    )

    cleanup_strategy = marshmallow.fields.Nested(
        "CleanupStrategyDescriptorSchema",
        dump_to="cleanupStrategy",
        load_from="cleanupStrategy", data_key="cleanupStrategy"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationDescriptor(**data)


class ReactionReferenceSchema(OneOfSchema):
    reaction_id = marshmallow.fields.Integer(dump_to="reactionId", load_from="reactionId", as_string=True, data_key="reactionId")
    namespace_id = marshmallow.fields.Integer(dump_to="namespaceId", load_from="namespaceId", as_string=True, data_key="namespaceId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ReactionReference(**data)


class GreetResponseSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(dump_to="id", load_from="id", as_string=True, data_key="id")
    login = marshmallow.fields.String()
    message = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.GreetResponse(**data)


class NamespaceSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(dump_to="id", load_from="id", as_string=True, data_key="id")
    parent_id = marshmallow.fields.Integer(dump_to="parentId", load_from="parentId", as_string=True, data_key="parentId")
    type_ = m_custom.EnumField(r_objs.NamespaceType, r_objs.UnknownEnumValue, dump_to="type", load_from="type", data_key="type")
    name = marshmallow.fields.String()
    description = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Namespace(**data)


class NamespaceCreateRequestSchema(OneOfSchema):
    namespace_identifier = marshmallow.fields.Nested(
        NamespaceIdentifierSchema,
        load_from="namespaceIdentifier",
        dump_to="namespaceIdentifier", data_key="namespaceIdentifier"
    )

    description = marshmallow.fields.String()
    permissions = marshmallow.fields.Nested(
        NamespacePermissionsSchema
    )
    create_parents = marshmallow.fields.Bool(load_from="createParents", dump_to="createParents", data_key="createParents")
    create_if_not_exist = marshmallow.fields.Bool(load_from="createIfNotExist", dump_to="createIfNotExist", data_key="createIfNotExist")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.NamespaceCreateRequest(**data)


class NamespaceCreateResponseSchema(OneOfSchema):
    namespace_id = marshmallow.fields.Integer(dump_to="namespaceId", load_from="namespaceId", as_string=True, data_key="namespaceId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.NamespaceCreateResponse(**data)


class OperationIdentifierSchema(OneOfSchema):
    operation_id = marshmallow.fields.Integer(dump_to="operationId", load_from="operationId", as_string=True, data_key="operationId")
    namespace_identifier = marshmallow.fields.Nested(
        NamespaceIdentifierSchema,
        load_from="namespaceIdentifier",
        dump_to="namespaceIdentifier", data_key="namespaceIdentifier"
    )

    class Meta:
        one_of = ["operation_id", "namespace_identifier"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationIdentifier(**data)


class OperationSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(dump_to="id", load_from="id", as_string=True, data_key="id")

    operation_type_id = marshmallow.fields.Integer(dump_to="operationTypeId", load_from="operationTypeId", as_string=True,
                                                   data_key="operationTypeId")

    namespace_id = marshmallow.fields.Integer(dump_to="namespaceId", load_from="namespaceId", as_string=True, data_key="namespaceId")

    status = m_custom.EnumField(r_objs.OperationStatus, r_objs.UnknownEnumValue)

    start_conf = marshmallow.fields.Nested(
        StartConfigurationSchema,
        dump_to="startConfiguration",
        load_from="startConfiguration", data_key="startConfiguration"
    )

    parameters = marshmallow.fields.Nested(
        ParametersValueSchema,
        dump_to="parametersValue",
        load_from="parametersValue", data_key="parametersValue"
    )

    inputs = marshmallow.fields.Nested(
        InputsValueSchema,
        dump_to="inputsValue",
        load_from="inputsValue", data_key="inputsValue"
    )

    outputs = marshmallow.fields.Nested(
        OutputsValueSchema,
        dump_to="outputsValue",
        load_from="outputsValue", data_key="outputsValue"
    )
    dynamic_trigger_list = marshmallow.fields.Nested(
        DynamicTriggerListSchema, as_string=True,
        dump_to="dynamicTriggerList",
        load_from="dynamicTriggerList", data_key="dynamicTriggerList"
    )

    project_id = marshmallow.fields.Integer(
        dump_to="projectId", load_from="projectId", as_string=True, data_key="projectId"
    )

    cleanup_strategy = marshmallow.fields.Nested(
        "CleanupStrategyDescriptorSchema",
        dump_to="cleanupStrategy",
        load_from="cleanupStrategy", data_key="cleanupStrategy"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Operation(**data)


class ReactionStatusUpdateSchema(OneOfSchema):
    reaction = marshmallow.fields.Nested(OperationIdentifierSchema)
    status_update = m_custom.EnumField(r_objs.StatusUpdate, r_objs.UnknownEnumValue, dump_to="statusUpdate", load_from="statusUpdate",
                                       data_key="statusUpdate")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ReactionStatusUpdate(**data)


class PerArtifactCountStrategySchema(OneOfSchema):
    limit = marshmallow.fields.Integer(as_string=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.PerArtifactCountStrategy(**data)


class WaterlineStrategiesSchema(OneOfSchema):
    per_artifact_count_strategy = marshmallow.fields.Nested(PerArtifactCountStrategySchema, load_from="perArtifactCountStrategy",
                                                            dump_to="perArtifactCountStrategy", data_key="perArtifactCountStrategy")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.WaterlineStrategies(**data)


class ReactionStartConfigurationUpdateSchema(OneOfSchema):
    reaction_identifier = marshmallow.fields.Nested(OperationIdentifierSchema, load_from="reactionIdentifier",
                                                    dump_to="reactionIdentifier", data_key="reactionIdentifier")
    waterline_strategies = marshmallow.fields.Nested(WaterlineStrategiesSchema, load_from="waterlineStrategies",
                                                     dump_to="waterlineStrategies", data_key="waterlineStrategies")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ReactionStartConfigurationUpdate(**data)


class ReactionTypeSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(dump_to="id", load_from="id", as_string=True, data_key="id")
    operation_set_key = marshmallow.fields.String(dump_to="operationSetKey", load_from="operationSetKey", as_string=True,
                                                  data_key="operationSetKey")
    operation_key = marshmallow.fields.String(dump_to="operationKey", load_from="operationKey", as_string=True, data_key="operationKey")
    version = marshmallow.fields.String()
    operation_set_name = marshmallow.fields.String(dump_to="operationSetName", load_from="operationSetName", as_string=True,
                                                   data_key="operationSetName")
    operation_name = marshmallow.fields.String(dump_to="operationName", load_from="operationName", as_string=True,
                                               data_key="operationName")
    description = marshmallow.fields.String()
    status = m_custom.EnumField(r_objs.OperationTypeStatus, r_objs.UnknownEnumValue)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ReactionType(**data)


class NotificationSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(dump_to="notificationId", load_from="notificationId", as_string=True, data_key="notificationId")
    namespace_id = marshmallow.fields.Integer(dump_to="namespaceId", load_from="namespaceId", as_string=True, data_key="namespaceId")
    event_type = m_custom.EnumField(r_objs.NotificationEventType, r_objs.UnknownEnumValue, dump_to="eventType", load_from="eventType",
                                    data_key="eventType")
    transport = m_custom.EnumField(r_objs.NotificationTransportType, r_objs.UnknownEnumValue)
    recipient = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Notification(**data)


class NotificationDescriptorSchema(OneOfSchema):
    namespace_identifier = marshmallow.fields.Nested(NamespaceIdentifierSchema, dump_to="namespace", load_from="namespace",
                                                     data_key="namespace")
    event_type = m_custom.EnumField(r_objs.NotificationEventType, r_objs.UnknownEnumValue, dump_to="eventType", load_from="eventType",
                                    data_key="eventType")
    transport = m_custom.EnumField(r_objs.NotificationTransportType, r_objs.UnknownEnumValue)
    recipient = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.NotificationDescriptor(**data)


class ArtifactInstanceReferenceSchema(OneOfSchema):
    artifact_type_id = marshmallow.fields.Integer(dump_to="artifactTypeId", load_from="artifactTypeId", as_string=True,
                                                  data_key="artifactTypeId")
    artifact_id = marshmallow.fields.Integer(dump_to="artifactId", load_from="artifactId", as_string=True, data_key="artifactId")
    artifact_instance_id = marshmallow.fields.Integer(dump_to="artifactInstanceId", load_from="artifactInstanceId", as_string=True,
                                                      data_key="artifactInstanceId")
    namespace_id = marshmallow.fields.Integer(dump_to="namespaceId", load_from="namespaceId", as_string=True, data_key="namespaceId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstanceReference(**data)


class InputsInstanceConstHolderSchema(OneOfSchema):
    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsInstanceConstHolder(**data)


class InputsInstanceExpressionSchema(OneOfSchema):
    input_refs = marshmallow.fields.Nested(ArtifactInstanceReferenceSchema, dump_to="inputRefs", load_from="inputRefs", many=True,
                                           data_key="inputRefs")
    output_ref = marshmallow.fields.Nested(ArtifactInstanceReferenceSchema, dump_to="outputRef", load_from="outputRef",
                                           data_key="outputRef")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsInstanceExpression(**data)


class InputsInstanceValueSchema(OneOfSchema):
    artifact_instance_reference = marshmallow.fields.Nested(
        ArtifactInstanceReferenceSchema,
        load_from="artifactInstanceRef",
        dump_to="artifactInstanceRef", data_key="artifactInstanceRef"
    )
    const_value = marshmallow.fields.Nested(InputsInstanceConstHolderSchema, load_from="constHolder", dump_to="constHolder",
                                            data_key="constHolder")
    expression = marshmallow.fields.Nested(InputsInstanceExpressionSchema, load_from="expression", dump_to="expression",
                                           data_key="expression")

    class Meta:
        one_of = ["artifact_reference", "const_value", "expression"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsInstanceValue(**data)


class InputsInstanceListSchema(OneOfSchema):
    elements = marshmallow.fields.Nested(
        "InputsInstanceElementSchema",
        many=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsInstanceList(**data)


class InputsInstanceNodeSchema(OneOfSchema):
    nodes = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested("InputsInstanceElementSchema")
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsInstanceNode(**data)


class InputsInstanceElementSchema(OneOfSchema):
    value = marshmallow.fields.Nested(InputsInstanceValueSchema)
    list_ = marshmallow.fields.Nested(InputsInstanceListSchema, dump_to="list", load_from="list", data_key="list")
    node = marshmallow.fields.Nested(InputsInstanceNodeSchema)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsInstanceElement(**data)


class InputsInstanceSchema(OneOfSchema):
    root_node = marshmallow.fields.Nested(InputsInstanceNodeSchema, dump_to="rootNode", load_from="rootNode", data_key="rootNode")
    version = marshmallow.fields.Integer()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InputsInstance(**data)


class OutputsInstanceExpressionSchema(OneOfSchema):
    input_ref = marshmallow.fields.Nested(ArtifactInstanceReferenceSchema, load_from="inputRef", dump_to="inputRef",
                                          data_key="inputRef")
    output_ref = marshmallow.fields.Nested(ArtifactInstanceReferenceSchema, load_from="outputRef", dump_to="outputRef",
                                           data_key="outputRef")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsInstanceExpression(**data)


class OutputsInstanceValueManualActionSchema(OneOfSchema):
    deprecation_target = marshmallow.fields.Nested(ArtifactInstanceReferenceSchema, load_from="deprecationTarget",
                                                   dump_to="deprecationTarget", data_key="deprecationTarget")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsInstanceValueManualAction(**data)


class OutputsInstanceValueSchema(OneOfSchema):
    on_success_action = marshmallow.fields.Nested(OutputsInstanceValueManualActionSchema, load_from="onSuccessAction",
                                                  dump_to="onSuccessAction", data_key="onSuccessAction")

    artifact_instance_reference = marshmallow.fields.Nested(
        ArtifactInstanceReferenceSchema,
        load_from="artifactInstanceRef",
        dump_to="artifactInstanceRef", data_key="artifactInstanceRef"
    )

    expression = marshmallow.fields.Nested(OutputsInstanceExpressionSchema, load_from="expression", dump_to="expression",
                                           data_key="expression")

    class Meta:
        one_of = ["artifact_reference", "expression"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsInstanceValue(**data)


class OutputsInstanceListSchema(OneOfSchema):
    elements = marshmallow.fields.Nested(
        "OutputsInstanceElementSchema",
        many=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsInstanceList(**data)


class OutputsInstanceNodeSchema(OneOfSchema):
    nodes = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested("OutputsInstanceElementSchema")
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsInstanceNode(**data)


class OutputsInstanceElementSchema(OneOfSchema):
    value = marshmallow.fields.Nested(OutputsInstanceValueSchema)
    list_ = marshmallow.fields.Nested(OutputsInstanceListSchema, dump_to="list", load_from="list", data_key="list")
    node = marshmallow.fields.Nested(OutputsInstanceNodeSchema)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsInstanceElement(**data)


class OutputsInstanceSchema(OneOfSchema):
    root_node = marshmallow.fields.Nested(OutputsInstanceNodeSchema, dump_to="rootNode", load_from="rootNode", data_key="rootNode")
    version = marshmallow.fields.Integer()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OutputsInstance(**data)


class ReactionInstanceListRequestSchema(OneOfSchema):
    reaction = marshmallow.fields.Nested(OperationIdentifierSchema)
    bound_instance_id = marshmallow.fields.Integer(as_string=True,
                                                   load_from="boundingReactionInstanceId",
                                                   dump_to="boundingReactionInstanceId", data_key="boundingReactionInstanceId",
                                                   allow_none=True)
    filter_type = m_custom.EnumField(r_objs.FilterTypes, r_objs.UnknownEnumValue, dump_to="filteringFunction",
                                     load_from="filteringFunction", data_key="filteringFunction")
    order = m_custom.EnumField(r_objs.OrderTypes, r_objs.UnknownEnumValue)
    limit = marshmallow.fields.Integer(as_string=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ReactionInstanceListRequest(**data)


class ConditionalArtifactInstanceDeltaSchema(OneOfSchema):
    artifact_instance_ref = marshmallow.fields.Nested(ArtifactInstanceReferenceSchema, dump_to="artifactInstanceRef",
                                                      load_from="artifactInstanceRef", data_key="artifactInstanceRef")
    delta_ms = marshmallow.fields.Integer(as_string=True, dump_to="deltaMs", load_from="deltaMs", data_key="deltaMs")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ConditionalArtifactInstanceDelta(**data)


class ConditionalArtifactDeltaGroupSchema(OneOfSchema):
    artifact_ref = marshmallow.fields.Nested(ArtifactReferenceSchema, dump_to="artifactRef", load_from="artifactRef",
                                             data_key="artifactRef")
    alias_to_delta = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested(ConditionalArtifactInstanceDeltaSchema),
        load_from="alias2deltaInstance",
        dump_to="alias2deltaInstance", data_key="alias2deltaInstance"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ConditionalArtifactDeltaGroup(**data)


class ConditionalArtifactsSchema(OneOfSchema):
    conditional_deltas_list = marshmallow.fields.Nested(ConditionalArtifactDeltaGroupSchema,
                                                        dump_to="conditionalDeltas",
                                                        load_from="conditionalDeltas", data_key="conditionalDeltas",
                                                        many=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ConditionalArtifacts(**data)


class ReactorEntitySchema(OneOfSchema):
    entity_type = marshmallow.fields.Integer(dump_to="entityTypeId", load_from="entityTypeId", as_string=True, data_key="entityTypeId")
    entity_id = marshmallow.fields.Integer(dump_to="entityId", load_from="entityId", as_string=True, data_key="entityId")
    value = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ReactorEntity(**data)


class InstantiationContextSchema(OneOfSchema):
    triggered_by_refs = marshmallow.fields.Nested(ArtifactInstanceReferenceSchema,
                                                  dump_to="triggeredByRefs",
                                                  load_from="triggeredByRefs",
                                                  many=True, data_key="triggeredByRefs")
    globals = m_custom.StructuredDict(
        keys=marshmallow.fields.String,
        values=marshmallow.fields.Nested(ReactorEntitySchema),
    )
    conditional_artifacts = marshmallow.fields.Nested(ConditionalArtifactsSchema, dump_to="conditionalArtifacts",
                                                      load_from="conditionalArtifacts", data_key="conditionalArtifacts")
    cron_schedule_time = marshmallow.fields.String(dump_to="cronScheduledTime", load_from="cronScheduledTime",
                                                   data_key="cronScheduledTime")
    user_time = marshmallow.fields.DateTime(load_from="userTime", dump_to="userTime", format="iso", data_key="userTime")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.InstantiationContext(**data)

    @marshmallow.pre_load
    def allow_empty_date(self, data, **kwargs):
        if "userTime" in data and not data["userTime"]:
            del data["userTime"]
        return data


class OperationInstanceSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(dump_to="id", load_from="id", as_string=True, data_key="id")
    operation_id = marshmallow.fields.Integer(dump_to="operationId", load_from="operationId", as_string=True, data_key="operationId")
    creator_id = marshmallow.fields.Integer(dump_to="creatorId", load_from="creatorId", as_string=True, data_key="creatorId")
    description = marshmallow.fields.String()
    creation_time = marshmallow.fields.DateTime(
        load_from="creationTimestamp",
        dump_to="creationTimestamp",
        format="iso", data_key="creationTimestamp"
    )
    state = marshmallow.fields.String()
    status = m_custom.EnumField(r_objs.ReactionInstanceStatus, r_objs.UnknownEnumValue)
    progress_msg = marshmallow.fields.String(dump_to="progressMessage", load_from="progressMessage", data_key="progressMessage")
    progress_log = marshmallow.fields.String(dump_to="progressLog", load_from="progressLog", data_key="progressLog")
    progress_rate = marshmallow.fields.Float(dump_to="progressRate", load_from="progressRate", data_key="progressRate")
    source = m_custom.EnumField(r_objs.ReactionTriggerType, r_objs.UnknownEnumValue)
    inputs = marshmallow.fields.Nested(InputsInstanceSchema, dump_to="inputsInstances", load_from="inputsInstances",
                                       data_key="inputsInstances")
    outputs = marshmallow.fields.Nested(OutputsInstanceSchema, dump_to="outputsInstances", load_from="outputsInstances",
                                        data_key="outputsInstances")
    instantiation_context = marshmallow.fields.Nested(InstantiationContextSchema, dump_to="instantiationContext",
                                                      load_from="instantiationContext", data_key="instantiationContext")
    completion_time = marshmallow.fields.DateTime(
        load_from="completionTimestamp",
        dump_to="completionTimestamp",
        format="iso", data_key="completionTimestamp"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationInstance(**data)

    @marshmallow.pre_load
    def allow_empty_completion_timestamp(self, data, **kwargs):
        if ("completionTimestamp" in data) and (not data["completionTimestamp"]):
            del data["completionTimestamp"]
        return data


class OperationInstanceStatusViewSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(dump_to="id", load_from="id", as_string=True, data_key="id")
    operation_id = marshmallow.fields.Integer(dump_to="operationId", load_from="operationId", as_string=True, data_key="operationId")
    status = m_custom.EnumField(r_objs.ReactionInstanceStatus, r_objs.UnknownEnumValue)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationInstanceStatusView(**data)


class OperationInstanceStatusSchema(OneOfSchema):
    host = marshmallow.fields.String()
    status = m_custom.EnumField(r_objs.ReactionInstanceStatus, r_objs.UnknownEnumValue)
    operation_instance_id = marshmallow.fields.Integer(dump_to="operationInstanceId", load_from="operationInstanceId", as_string=True,
                                                       data_key="operationInstanceId")
    update_timestamp = marshmallow.fields.DateTime(load_from="updateTimestamp", dump_to="updateTimestamp", format="iso",
                                                   data_key="updateTimestamp")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.OperationInstanceStatus(**data)


class TimestampRangeSchema(OneOfSchema):
    dt_from = marshmallow.fields.DateTime(load_from="from", dump_to="from", format="iso", data_key="from")
    dt_to = marshmallow.fields.DateTime(load_from="to", dump_to="to", format="iso", data_key="to")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.TimestampRange(**data)


class TimestampFilterSchema(OneOfSchema):
    exact_time = marshmallow.fields.DateTime(
        load_from="exactTimestamp",
        dump_to="exactTimestamp",
        format="iso", data_key="exactTimestamp"
    )

    time_range = marshmallow.fields.Nested(TimestampRangeSchema, load_from="timestampRange", dump_to="timestampRange",
                                           data_key="timestampRange")

    class Meta:
        one_of = ["exact_time", "time_range"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.TimestampFilter(**data)


class ArtifactInstanceFilterDescriptorSchema(OneOfSchema):
    artifact_identifier = marshmallow.fields.Nested(ArtifactIdentifierSchema,
                                                    load_from="artifactIdentifier",
                                                    dump_to="artifactIdentifier", data_key="artifactIdentifier")
    user_timestamp_filter = marshmallow.fields.Nested(TimestampFilterSchema,
                                                      load_from="userTimestampFilter",
                                                      dump_to="userTimestampFilter", data_key="userTimestampFilter")

    limit = marshmallow.fields.Integer()
    offset = marshmallow.fields.Integer()
    order_by = m_custom.EnumField(r_objs.ArtifactInstanceOrderBy, load_from="orderBy", dump_to="orderBy",
                                  unknown_enum_type=r_objs.UnknownEnumValue, data_key="orderBy")

    statuses = marshmallow.fields.List(m_custom.EnumField(r_objs.ArtifactInstanceStatus, r_objs.UnknownEnumValue))

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstanceFilterDescriptor(**data)


class QueueMaxQueuedInstancesSchema(OneOfSchema):
    value = marshmallow.fields.Integer(as_string=True, load_from="value", dump_to="value", data_key="value")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueMaxQueuedInstances(**data)


class QueueMaxRunningInstancesPerReactionSchema(OneOfSchema):
    value = marshmallow.fields.Integer(as_string=True, load_from="value", dump_to="value", data_key="value")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueMaxRunningInstancesPerReaction(**data)


class QueueMaxQueuedInstancesPerReactionSchema(OneOfSchema):
    value = marshmallow.fields.Integer(as_string=True, load_from="value", dump_to="value", data_key="value")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueMaxQueuedInstancesPerReaction(**data)


class CancelConstraintViolationPolicySchema(OneOfSchema):
    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.CancelConstraintViolationPolicy(**data)


class TimeoutConstraintViolationPolicySchema(OneOfSchema):
    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.TimeoutConstraintViolationPolicy(**data)


class ConstraintsSchema(OneOfSchema):
    unique_priorities = marshmallow.fields.Bool(load_from="uniquePriorities",
                                                dump_to="uniquePriorities",
                                                data_key="uniquePriorities")
    unique_running_priorities = marshmallow.fields.Bool(load_from="uniqueRunningPriorities",
                                                        dump_to="uniqueRunningPriorities",
                                                        data_key="uniqueRunningPriorities")
    unique_queued_priorities = marshmallow.fields.Bool(load_from="uniqueQueuedPriorities",
                                                       dump_to="uniqueQueuedPriorities",
                                                       data_key="uniqueQueuedPriorities")
    unique_per_reaction_running_priorities = marshmallow.fields.Bool(load_from="uniquePerReactionRunningPriorities",
                                                                     dump_to="uniquePerReactionRunningPriorities",
                                                                     data_key="uniquePerReactionRunningPriorities")
    unique_per_reaction_queued_priorities = marshmallow.fields.Bool(load_from="uniquePerReactionQueuedPriorities",
                                                                    dump_to="uniquePerReactionQueuedPriorities",
                                                                    data_key="uniquePerReactionQueuedPriorities")
    cancel_constraint_violation_policy = marshmallow.fields.Nested(CancelConstraintViolationPolicySchema,
                                                                   load_from="cancelConstraintViolationPolicySchema",
                                                                   dump_to="cancelConstraintViolationPolicySchema",
                                                                   data_key="cancelConstraintViolationPolicySchema")
    timeout_constraint_violation_policy = marshmallow.fields.Nested(TimeoutConstraintViolationPolicySchema,
                                                                    load_from="timeoutConstraintViolationPolicy",
                                                                    dump_to="timeoutConstraintViolationPolicy",
                                                                    data_key="timeoutConstraintViolationPolicy")

    class Meta:
        one_of = ["cancel_constraint_violation_policy", "timeout_constraint_violation_policy"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Constraints(**data)


class QueueConfigurationSchema(OneOfSchema):
    parallelism = marshmallow.fields.Integer(as_string=True, load_from="maxRunningInstances", dump_to="maxRunningInstances",
                                             data_key="maxRunningInstances")
    priority_function = m_custom.EnumField(r_objs.QueuePriorityFunction, r_objs.UnknownEnumValue, load_from="priorityFunction",
                                           dump_to="priorityFunction", data_key="priorityFunction")
    max_queued_instances = marshmallow.fields.Nested(QueueMaxQueuedInstancesSchema,
                                                     load_from="maxQueuedInstances",
                                                     dump_to="maxQueuedInstances", data_key="maxQueuedInstances")
    max_running_instances_per_reaction = marshmallow.fields.Nested(QueueMaxRunningInstancesPerReactionSchema,
                                                                   load_from="maxRunningInstancesPerReaction",
                                                                   dump_to="maxRunningInstancesPerReaction",
                                                                   data_key="maxRunningInstancesPerReaction")
    max_queued_instances_per_reaction = marshmallow.fields.Nested(QueueMaxQueuedInstancesPerReactionSchema,
                                                                  load_from="maxQueuedInstancesPerReaction",
                                                                  dump_to="maxQueuedInstancesPerReaction",
                                                                  data_key="maxQueuedInstancesPerReaction")
    constraints = marshmallow.fields.Nested(ConstraintsSchema, load_from="constraints", dump_to="constraints", data_key="constraints")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueConfiguration(**data)


class QueueDescriptorSchema(OneOfSchema):
    namespace_descriptor = marshmallow.fields.Nested(NamespaceDescriptorSchema, load_from="namespaceDescriptor",
                                                     dump_to="namespaceDescriptor", data_key="namespaceDescriptor")
    configuration = marshmallow.fields.Nested(QueueConfigurationSchema, load_from="queueConfig", dump_to="queueConfig",
                                              data_key="queueConfig")
    project_identifier = marshmallow.fields.Nested(
        "ProjectSchema",
        dump_to="projectIdentifier",
        load_from="projectIdentifier", data_key="projectIdentifier"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueDescriptor(**data)


class QueueIdentifierSchema(OneOfSchema):
    queue_id = marshmallow.fields.Integer(as_string=True, load_from="queueId", dump_to="queueId", data_key="queueId")
    namespace_identifier = marshmallow.fields.Nested(NamespaceIdentifierSchema, load_from="namespaceIdentifier",
                                                     dump_to="namespaceIdentifier", data_key="namespaceIdentifier")

    class Meta:
        one_of = ["queue_id", "namespace_identifier"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueIdentifier(**data)


class QueueOperationConfigurationSchema(OneOfSchema):
    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueOperationConfiguration()


class QueueOperationSchema(OneOfSchema):
    reaction_id = marshmallow.fields.Integer(as_string=True, load_from="reactionId", dump_to="reactionId", data_key="reactionId")
    configuration = marshmallow.fields.Nested(QueueOperationConfigurationSchema)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueOperation(**data)


class QueueSchema(OneOfSchema):
    id_ = marshmallow.fields.Integer(as_string=True, load_from="id", dump_to="id", data_key="id")
    namespace_id = marshmallow.fields.Integer(as_string=True, load_from="namespaceId", dump_to="namespaceId", data_key="namespaceId")
    configuration = marshmallow.fields.Nested(QueueConfigurationSchema)
    project_id = marshmallow.fields.Integer(
        load_from="projectId", dump_to="projectId", as_string=True, data_key="projectId"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Queue(**data)


class QueueReferenceSchema(OneOfSchema):
    queue_id = marshmallow.fields.Integer(as_string=True, load_from="queueId", dump_to="queueId", data_key="queueId")
    namespace_id = marshmallow.fields.Integer(as_string=True, load_from="namespaceId", dump_to="namespaceId", data_key="namespaceId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueReference(**data)


class QueueReactionsSchema(OneOfSchema):
    queue = marshmallow.fields.Nested(QueueSchema)
    reactions = marshmallow.fields.Nested(QueueOperationSchema, many=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueReactions(**data)


class QueueUpdateRequestSchema(OneOfSchema):
    queue_identifier = marshmallow.fields.Nested(QueueIdentifierSchema, load_from="queueIdentifier", dump_to="queueIdentifier",
                                                 required=True, data_key="queueIdentifier")
    new_queue_capacity = marshmallow.fields.Integer(as_string=True, load_from="newQueueCapacity", dump_to="newQueueCapacity",
                                                    data_key="newQueueCapacity")
    remove_reactions = marshmallow.fields.Nested(OperationIdentifierSchema, many=True, load_from="removeReactions",
                                                 dump_to="removeReactions", data_key="removeReactions")
    add_reactions = marshmallow.fields.Nested(OperationIdentifierSchema, many=True, load_from="addReactions", dump_to="addReactions",
                                              data_key="addReactions")
    new_max_instances_in_queue = marshmallow.fields.Integer(as_string=True, load_from="newMaxInstancesInQueue",
                                                            dump_to="newMaxInstancesInQueue", data_key="newMaxInstancesInQueue")
    new_max_instances_per_reaction_in_queue = marshmallow.fields.Integer(as_string=True, load_from="newMaxInstancesPerReactionInQueue",
                                                                         dump_to="newMaxInstancesPerReactionInQueue",
                                                                         data_key="newMaxInstancesPerReactionInQueue")
    new_max_running_instances_per_reaction = marshmallow.fields.Integer(as_string=True, load_from="newMaxRunningInstancesPerReaction",
                                                                        dump_to="newMaxRunningInstancesPerReaction",
                                                                        data_key="newMaxRunningInstancesPerReaction")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.QueueUpdateRequest(**data)


class MetricCreateRequestSchema(OneOfSchema):
    metric_type = m_custom.EnumField(r_objs.MetricType, r_objs.UnknownEnumValue, dump_to="metricType", load_from="metricType",
                                     required=True, data_key="metricType")
    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, data_key="artifactId")
    queue_id = marshmallow.fields.Integer(load_from="queueId", dump_to="queueId", as_string=True, data_key="queueId")
    reaction_id = marshmallow.fields.Integer(load_from="reactionId", dump_to="reactionId", as_string=True, data_key="reactionId")
    custom_tags = marshmallow.fields.Nested(
        AttributesSchema,
        load_from="customTags",
        dump_to="customTags", data_key="customTags"
    )

    class Meta:
        one_of = ["artifact_id", "queue_id", "reaction_id"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.MetricCreateRequest(**data)


class MetricListRequestSchema(OneOfSchema):
    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, data_key="artifactId")
    queue_id = marshmallow.fields.Integer(load_from="queueId", dump_to="queueId", as_string=True, data_key="queueId")
    reaction_id = marshmallow.fields.Integer(load_from="reactionId", dump_to="reactionId", as_string=True, data_key="reactionId")

    class Meta:
        one_of = ["artifact_id", "queue_id", "reaction_id"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.MetricListRequest(**data)


class MetricUpdateRequestSchema(OneOfSchema):
    metric_type = m_custom.EnumField(r_objs.MetricType, r_objs.UnknownEnumValue, dump_to="metricType", load_from="metricType",
                                     required=True, data_key="metricType")
    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, data_key="artifactId")
    queue_id = marshmallow.fields.Integer(load_from="queueId", dump_to="queueId", as_string=True, data_key="queueId")
    reaction_id = marshmallow.fields.Integer(load_from="reactionId", dump_to="reactionId", as_string=True, data_key="reactionId")
    new_tags = marshmallow.fields.Nested(
        AttributesSchema,
        load_from="newTags",
        dump_to="newTags", data_key="newTags"
    )

    class Meta:
        one_of = ["artifact_id", "queue_id", "reaction_id"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.MetricUpdateRequest(**data)


class MetricDeleteRequestSchema(OneOfSchema):
    metric_type = m_custom.EnumField(r_objs.MetricType, r_objs.UnknownEnumValue, dump_to="metricType", load_from="metricType",
                                     required=True, data_key="metricType")
    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, data_key="artifactId")
    queue_id = marshmallow.fields.Integer(load_from="queueId", dump_to="queueId", as_string=True, data_key="queueId")
    reaction_id = marshmallow.fields.Integer(load_from="reactionId", dump_to="reactionId", as_string=True, data_key="reactionId")

    class Meta:
        one_of = ["artifact_id", "queue_id", "reaction_id"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.MetricDeleteRequest(**data)


class MetricReferenceSchema(OneOfSchema):
    metric_type = m_custom.EnumField(r_objs.MetricType, r_objs.UnknownEnumValue, dump_to="metricType", load_from="metricType",
                                     required=True, data_key="metricType")
    tags_list = marshmallow.fields.Nested(
        AttributesSchema,
        many=True,
        load_from="tags",
        dump_to="tags", data_key="tags"
    )
    artifact_id = marshmallow.fields.Integer(load_from="artifactId", dump_to="artifactId", as_string=True, data_key="artifactId")
    queue_id = marshmallow.fields.Integer(load_from="queueId", dump_to="queueId", as_string=True, data_key="queueId")
    reaction_id = marshmallow.fields.Integer(load_from="reactionId", dump_to="reactionId", as_string=True, data_key="reactionId")

    class Meta:
        one_of = ["artifact_id", "queue_id", "reaction_id"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.MetricReference(**data)


class LongRunningOperationInstanceNotificationOptionsSchema(OneOfSchema):
    warn_percentile = marshmallow.fields.Integer(load_from="warnPercentile", dump_to="warnPercentile", data_key="warnPercentile")
    warn_runs_count = marshmallow.fields.Integer(load_from="warnRunsCount", dump_to="warnRunsCount", data_key="warnRunsCount")
    warn_scale = marshmallow.fields.Float(load_from="warnScale", dump_to="warnScale", data_key="warnScale")
    crit_percentile = marshmallow.fields.Integer(load_from="critPercentile", dump_to="critPercentile", data_key="critPercentile")
    crit_runs_count = marshmallow.fields.Integer(load_from="critRunsCount", dump_to="critRunsCount", data_key="critRunsCount")
    crit_scale = marshmallow.fields.Float(load_from="critScale", dump_to="critScale", data_key="critScale")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.LongRunningOperationInstanceNotificationOptions(**data)


class ArtifactInstanceStatusRecordSchema(OneOfSchema):
    status = m_custom.EnumField(r_objs.ArtifactInstanceStatus, r_objs.UnknownEnumValue)
    update_timestamp = m_custom.RemoveUTCDateTimeField(
        load_from="updateTimestamp",
        dump_to="updateTimestamp",
        format="iso", data_key="updateTimestamp"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstanceStatusRecord(**data)


class ProjectSchema(OneOfSchema):
    namespace_identifier = marshmallow.fields.Nested(
        NamespaceIdentifierSchema,
        load_from="namespaceIdentifier",
        dump_to="namespaceIdentifier", data_key="namespaceIdentifier"
    )
    project_id = marshmallow.fields.Integer(
        dump_to="projectId", load_from="projectId", as_string=True, data_key="projectId"
    )
    project_key = marshmallow.fields.String(
        dump_to="projectKey", load_from="projectKey", data_key="projectKey"
    )

    class Meta:
        one_of = ["project_id", "namespace_identifier", "project_key"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ProjectIdentifier(**data)


class TtlCleanupStrategySchema(OneOfSchema):
    ttl_days = marshmallow.fields.Integer(
        dump_to="ttlDays", load_from="ttlDays", as_string=True, data_key="ttlDays"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.TtlCleanupStrategy(**data)


class CleanupStrategySchema(OneOfSchema):
    ttl_cleanup_strategy = marshmallow.fields.Nested(
        TtlCleanupStrategySchema,
        load_from="ttlCleanupStrategy",
        dump_to="ttlCleanupStrategy", data_key="ttlCleanupStrategy"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.CleanupStrategy(**data)


class CleanupStrategyDescriptorSchema(OneOfSchema):
    cleanup_strategies = marshmallow.fields.Nested(
        CleanupStrategySchema,
        many=True,
        load_from="cleanupStrategies",
        dump_to="cleanupStrategies", data_key="cleanupStrategies"
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.CleanupStrategyDescriptor(**data)


class CalculationDescriptorSchema(OneOfSchema):
    folder = marshmallow.fields.Nested(
        NamespaceDescriptorSchema,
        required=True
    )

    runner = marshmallow.fields.Nested(
        OperationDescriptorSchema,
        required=True
    )

    dependency_resolver = marshmallow.fields.Nested(
        OperationDescriptorSchema,
        load_from="dependencyResolver",
        dump_to="dependencyResolver",
        required=True, data_key="dependencyResolver"
    )

    artifacts = marshmallow.fields.Nested(
        ArtifactInstanceCreateRequestSchema,
        many=True
    )

    tags = m_custom.StructuredDict(
        keys=marshmallow.fields.String(),
        values=marshmallow.fields.String()
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.CalculationDescriptor(**data)


class CalculationReferenceSchema(OneOfSchema):
    folder_id = marshmallow.fields.Integer(dump_to="folderId", load_from="folderId", as_string=True, required=True, data_key="folderId")
    runner_id = marshmallow.fields.Integer(dump_to="runnerId", load_from="runnerId", as_string=True, required=True, data_key="runnerId")
    dependency_resolver_id = marshmallow.fields.Integer(dump_to="dependencyResolverId", load_from="dependencyResolverId",
                                                        as_string=True, required=True, data_key="dependencyResolverId")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.CalculationReference(**data)


class CalculationRuntimeMetadataSchema(OneOfSchema):
    calculation_id = marshmallow.fields.Integer(load_from="calculationId", dump_to="calculationId", as_string=True, required=True,
                                                data_key="calculationId")
    version = marshmallow.fields.Integer(as_string=True, required=True)
    status = m_custom.EnumField(r_objs.CalculationStatus, r_objs.UnknownEnumValue, required=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.CalculationRuntimeMetadata(**data)


class CalculationSchema(OneOfSchema):
    folder_id = marshmallow.fields.Integer(dump_to="folderId", load_from="folderId", as_string=True, required=True, data_key="folderId")
    runner_id = marshmallow.fields.Integer(dump_to="runnerId", load_from="runnerId", as_string=True, required=True, data_key="runnerId")
    dependency_resolver_id = marshmallow.fields.Integer(dump_to="dependencyResolverId", load_from="dependencyResolverId",
                                                        as_string=True, required=True, data_key="dependencyResolverId")

    metadata = marshmallow.fields.Nested(CalculationRuntimeMetadataSchema, required=True)

    tags = m_custom.StructuredDict(
        keys=marshmallow.fields.String(),
        values=marshmallow.fields.String()
    )

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.Calculation(**data)


class YtPathMappingRuleSchema(OneOfSchema):
    yt_path_pattern = marshmallow.fields.String(dump_to="ytPathPattern", load_from="ytPathPattern", required=True,
                                                data_key="ytPathPattern")
    artifact_pattern = marshmallow.fields.String(dump_to="artifactPattern", load_from="artifactPattern", required=True,
                                                 data_key="artifactPattern")
    artifact = marshmallow.fields.Nested(ArtifactIdentifierSchema)
    timestamp_pattern = marshmallow.fields.String(dump_to="timestampPattern", load_from="timestampPattern", data_key="timestampPattern")

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.YtPathMappingRule(**data)


class YtPathMappedArtifactSchema(OneOfSchema):
    artifact = marshmallow.fields.Nested(ArtifactReferenceSchema, required=True)
    timestamp = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.YtPathMappedArtifact(**data)


class YtPathMappingCandidatesSchema(OneOfSchema):
    yt_path = marshmallow.fields.String(dump_to="ytPath", load_from="ytPath", required=True, data_key="ytPath")
    candidates = marshmallow.fields.Nested(YtPathMappedArtifactSchema, many=True, required=True)

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.YtPathMappingCandidates(**data)


class ArtifactInstancesDeprecationRequestSchema(OneOfSchema):
    instances_to_deprecate = marshmallow.fields.List(
        marshmallow.fields.Integer(as_string=True),
        load_from="artifactInstanceIdsToDeprecate",
        dump_to="artifactInstanceIdsToDeprecate",
        data_key="artifactInstanceIdsToDeprecate",
    )
    description = marshmallow.fields.String()

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return r_objs.ArtifactInstancesDeprecationRequest(**data)


def to_json(reactor_obj, schema):
    if six.PY3:
        try:
            data = schema.dump(reactor_obj)
            schema.load(data)
        except ValueError as e:
            raise ReactorObjectValidationError(messages=[str(e)])
        except marshmallow.ValidationError as e:
            raise ReactorObjectValidationError(messages=e.messages)
    else:
        data = schema.dump(reactor_obj).data
        errors = schema.validate(data)
        if errors:
            raise ReactorObjectValidationError(errors)

    return data


def from_json(data, schema):
    if six.PY3:
        try:
            result = schema.load(data)
        except marshmallow.ValidationError as e:
            raise ReactorObjectValidationError(messages=e.messages, raw_data=data)
    else:
        unmarshaller_result = schema.load(data)
        if unmarshaller_result.errors:
            raise ReactorObjectValidationError(unmarshaller_result.errors, raw_data=data)
        result = unmarshaller_result.data

    return result
