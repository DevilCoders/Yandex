import sys
from collections import defaultdict
import six

from . import reactor_objects as r_objs

import logging
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
handler = logging.StreamHandler(sys.stderr)
handler.setLevel(logging.DEBUG)
logger.addHandler(handler)


def make_metadata_from_val(val):
    """
    :param val:
    :rtype: r_objs.Metadata
    """
    if isinstance(val, bool):
        proto = "/yandex.reactor.artifact.BoolArtifactValueProto"
        final_val = val
    elif isinstance(val, six.integer_types):
        proto = "/yandex.reactor.artifact.IntArtifactValueProto"
        final_val = str(val)
    elif isinstance(val, six.string_types):
        proto = "/yandex.reactor.artifact.StringArtifactValueProto"
        final_val = str(val)
    else:
        raise RuntimeError("Unknown value type {}".format(type(val)))

    return r_objs.Metadata(
        type_=proto,
        dict_obj={"value": final_val}
    )


def make_metadata_from_val_with_hints(val, val_type):
    """
    :param val: Union[str, int, bool, float, list] | None
    :param val_type: r_objs.VariableTypes
    :rtype: r_objs.Metadata
    """
    key = 'value'
    if 'list' in val_type.value.key:
        key = 'values'
        val = [val_type.value.cast(element) for element in val]
    else:
        val = val_type.value.cast(val)

    return r_objs.Metadata(
        type_=val_type.value.proto,
        dict_obj={key: val}
    )


def make_input_node_from_dict(dict_):
    """
    Converts one-level dict to InputValues
    If you want expression in leaf - wrap value with `Expresseion`
    If you want artifact - pass `ArtifactReference` as value
    :type dict_: dict[str, val | r_objs.Expression | r_objs.ArtifactReference]
    :rtype: r_objs.InputsValueNode
    """

    node = r_objs.InputsValueNode(nodes={})

    for key, val in six.iteritems(dict_):
        if isinstance(val, r_objs.Expression):
            converted_val = r_objs.InputsValueElement(r_objs.InputsValueRef(expression=val))
        elif isinstance(val, r_objs.ExpressionVariable):
            converted_val = r_objs.InputsValueElement(r_objs.InputsValueRef(expression_var=val))
        elif isinstance(val, r_objs.ArtifactReference):
            converted_val = r_objs.InputsValueElement(r_objs.InputsValueRef(artifact_reference=val))
        elif isinstance(val, list):
            converted_val = r_objs.InputsValueElement(list_=r_objs.InputsValueList([]))
        else:
            converted_val = r_objs.InputsValueElement(
                r_objs.InputsValueRef(const_value=r_objs.InputsValueConst(generic_value=make_metadata_from_val(val)))
            )

        node.nodes[key] = converted_val
    return node


def make_output_node_from_dict(dict_):
    """
    :type dict_: dict[str, r_objs.Expression | r_objs.ArtifactReference]
    :rtype: r_objs.OutputsValueNode
    """
    node = r_objs.OutputsValueNode(nodes={})

    for key, val in six.iteritems(dict_):
        if isinstance(val, r_objs.Expression):
            converted_val = r_objs.OutputsValueRef(expression=val)
        elif isinstance(val, r_objs.ArtifactReference):
            converted_val = r_objs.OutputsValueRef(artifact_reference=val)
        elif isinstance(val, r_objs.ExpressionVariable):
            converted_val = r_objs.OutputsValueRef(expression_var=val)
        else:
            raise ValueError("Output can be either Expression or Artifact")

        node.nodes[key] = r_objs.OutputsValueElement(value=converted_val)
    return node


class NirvanaReactionBuilder(object):
    def __init__(self):
        self._reaction_path = None
        self._description = None
        self._permissions = None

        self._triggers = None
        self._dynamic_triggers = None
        self._owner = None
        self._quota = None

        self._retries = None
        self._retry_time_param = None  # see set_retry_policy for details on _retry_time_param
        self._retry_policy_descriptor = r_objs.RetryPolicyDescriptor.UNIFORM.value
        self._result_cloning_policy = None
        self._retry_params = None   # for multiple params (like in Random policy)

        self._globals = None
        self._deprecation_strategies = None

        self._update_policy = r_objs.NirvanaUpdatePolicy.IGNORE.value
        self._result_ttl = None
        self._instance_ttl = None
        self._block_results_ttl = None

        self._source_instance = None
        self._source_flow = None

        self._target_flow = None

        self._block_params = defaultdict(dict)
        self._free_block_inputs = defaultdict(dict)

        self._data_blocks = {}
        self._global_params = {}

        self._block_outputs = defaultdict(dict)

        self._version = None
        self._expression_after_success = None
        self._expression_after_failure = None

        self._project = None
        self._cleanup_strategy = None

    def trigger_by_cron(self, cron_expr, misfire_policy=None):
        """
        :param cron_expr: expression like in crontab file
        :type cron_expr: str
        :type misfire_policy: r_objs.MisfirPolicy
        :return: NirvanaReactionBuilder
        """

        self._triggers = r_objs.Triggers(cron_trigger=r_objs.CronTrigger(
            cron_expression=cron_expr,
            misfire_policy=misfire_policy
        ))
        return self

    def trigger_by_artifacts(self, artifact_refs, relationship):
        """
        :type artifact_refs: list[r_objs.ArtifactReference]
        :type relationship: r_objs.Relationship
        :return: NirvanaReactionBuilder
        """

        self._triggers = r_objs.Triggers(selected_artifacts_trigger=r_objs.SelectedArtifactsTrigger(
            artifact_refs=artifact_refs, relationship=relationship
        ))

        return self

    def trigger_by_delta_patterns(self, delta_patterns):
        """
        :type delta_patterns: list[r_objs.DeltaPattern]
        :return: NirvanaReactionBuilder
        """

        self._triggers = r_objs.Triggers(delta_pattern_trigger=r_objs.DeltaPatternTrigger(delta_patterns))
        return self

    def trigger_by_inputs(self, artifact_refs=None, any_input=None, all_inputs=None):
        """
        :type artifact_refs: list[r_objs.ArtifactReference]
        :type any_input: bool
        :type all_inputs: bool
        :return: NirvanaReactionBuilder
        """
        self._triggers = r_objs.Triggers(
            inputs_trigger=r_objs.InputsTrigger(
                artifact_refs, any_input, all_inputs
            )
        )

    def trigger_by_expression(self, expression):
        """
        :type expression: Expression
        :return: NirvanaReactionBuilder
        """
        self._triggers = r_objs.Triggers(
            selected_artifacts_trigger=r_objs.SelectedArtifactsTrigger(
                artifact_refs=[],
                relationship=r_objs.Relationship.CONDITION,
                expression=expression
            ))

    def set_dynamic_triggers(self, triggers):
        """
        :type triggers: list[r_objs.DynamicTrigger]
        :return: NirvanaReactionBuilder
        """
        self._triggers = r_objs.Triggers(
            dynamic_trigger=r_objs.DynamicTriggerPlaceholder()
        )
        self._dynamic_triggers = r_objs.DynamicTriggerList(triggers)

    def set_owner(self, owner):
        """
        :type owner: str
        :rtype: NirvanaReactionBuilder
        """
        self._owner = owner
        return self

    def set_quota(self, quota):
        """
        :type quota: str
        :rtype: NirvanaReactionBuilder
        """
        self._quota = quota
        return self

    def set_retry_policy(self, retries, time_param=None, retry_policy_descriptor=None, result_cloning_policy=None):
        """
        :type retries: int
        :type time_param: int
        :param time_param: meaning depends on retry_policy_descriptor
               if retry policy is UNIFORM, time_param denotes delay between uniform retries
               if retry policy is EXPONENTIAL, time_param denotes total time for all retries
        :type retry_policy_descriptor: r_objs.RetryPolicyDescriptor
        :param result_cloning_policy: Result Clone Policy from Nirvana. DO_NOT_CLONE by default
        :type result_cloning_policy: r_objs.NirvanaResultCloningPolicy
        :rtype: NirvanaReactionBuilder
        """

        self._retries = retries
        if time_param is not None:
            self._retry_time_param = time_param
        if retry_policy_descriptor is not None:
            self._retry_policy_descriptor = retry_policy_descriptor.value
        if result_cloning_policy is not None:
            self._result_cloning_policy = result_cloning_policy.value

        return self

    def set_retry_policy_with_multiple_params(
        self, retries, retry_params=None,
        retry_policy_descriptor=None, result_cloning_policy=None
    ):
        """
        :type retries: int
        :type retry_params: Mapping[str, int]] | None
        :type retry_policy_descriptor: r_objs.RetryPolicyDescriptor
        :type result_cloning_policy: r_objs.NirvanaResultCloningPolicy
        :param result_cloning_policy: Result Clone Policy from Nirvana. DO_NOT_CLONE by default
        :rtype: NirvanaReactionBuilder
        """
        self._retries = retries
        self._retry_params = retry_params
        if retry_policy_descriptor is not None:
            self._retry_policy_descriptor = retry_policy_descriptor.value
        if result_cloning_policy is not None:
            self._result_cloning_policy = result_cloning_policy.value

    def set_reaction_path(self, path, description=None):
        """
        :type path: str
        :type description: str | None
        :rtype: NirvanaReactionBuilder
        """
        self._reaction_path = path
        self._description = description
        return self

    def set_global_expression(self, globals):
        """
        :type globals: str
        :rtype: NirvanaReactionBuilder
        """
        self._globals = globals
        return NirvanaReactionBuilder

    def set_deprecation_strategies(self, deprecation_strategies):
        """
        :type deprecation_strategies: list[r_objs.DeprecationStrategy]
        :rtype: NirvanaReactionBuilder
        """
        self._deprecation_strategies = deprecation_strategies
        return NirvanaReactionBuilder

    def set_block_update_policy(self, update_policy):
        """
        :type update_policy: r_objs.NirvanaUpdatePolicy
        :rtype: NirvanaReactionBuilder
        """
        self._update_policy = update_policy.value
        return self

    def set_result_ttl(self, ttl):
        """
        :type ttl: int
        :rtype: NirvanaReactionBuilder
        """
        self._result_ttl = ttl
        return self

    def set_instance_ttl(self, ttl):
        """
        :type ttl: int
        :rtype: NirvanaReactionBuilder
        """
        self._instance_ttl = ttl
        return self

    def set_source_graph(self, instance_id=None, flow_id=None):
        """
        :type instance_id: str
        :type flow_id: str
        :rtype: NirvanaReactionBuilder
        """

        self._source_instance = instance_id
        self._source_flow = flow_id

        if self._source_instance is not None and self._source_flow is not None:
            raise ValueError("Specify either instance id or flow id")

        return self

    def set_target_graph(self, flow_id):
        """
        :type flow_id: str
        :rtype: NirvanaReactionBuilder
        """
        self._target_flow = flow_id
        return self

    def set_block_param_to_artifact(self, operation_id, param_name, artifact_reference):
        """
        :type operation_id: str
        :type  param_name: str
        :type artifact_reference: r_objs.ArtifactReference
        :rtype: NirvanaReactionBuilder
        """
        self._block_params[operation_id][param_name] = artifact_reference
        return self

    def set_block_param_to_expression(self, operation_id, param_name, expression):
        """
        :type operation_id: str
        :type param_name: str
        :type expression: str
        :rtype: NirvanaReactionBuilder
        """
        self._block_params[operation_id][param_name] = r_objs.Expression(expression)
        return self

    def set_block_param_to_expression_var(self, operation_id, param_name, expression_var):
        """
        :type operation_id: str
        :type param_name: str
        :type expression_var: r_objs.ExpressionVariable
        :rtype: NirvanaReactionBuilder
        """
        self._block_params[operation_id][param_name] = expression_var
        return self

    def set_block_param_to_value(self, operation_id, param_name, value):
        """
        :type operation_id: str
        :type param_name: str
        :type value: Any
        :rtype: NirvanaReactionBuilder
        """
        self._block_params[operation_id][param_name] = make_metadata_from_val(value)
        return self

    def set_block_param_to_value_with_hints(
        self, operation_id, param_name, value, value_type
    ):
        """
        :type operation_id: str
        :type param_name: str
        :type value: Union[str, int, bool, float, list]
        :type value_type: r_objs.VariableTypes
        :rtype: NirvanaReactionBuilder
        """
        self._block_params[operation_id][param_name] = (
            make_metadata_from_val_with_hints(value, value_type)
        )
        return self

    def set_free_block_input_to_artifact(self, operation_id, param_name, artifact_reference):
        """
        :type operation_id: str
        :type  param_name: str
        :type artifact_reference: r_objs.ArtifactReference
        :rtype: NirvanaReactionBuilder
        """
        self._free_block_inputs[operation_id][param_name] = artifact_reference
        return self

    def set_free_block_input_to_expression(self, operation_id, param_name, expression):
        """
        :type operation_id: str
        :type param_name: str
        :type expression: str
        :rtype: NirvanaReactionBuilder
        """
        self._free_block_inputs[operation_id][param_name] = r_objs.Expression(expression)
        return self

    def set_free_block_input_to_value(self, operation_id, param_name, value):
        """
        :type operation_id: str
        :type param_name: str
        :type value: str
        :rtype: NirvanaReactionBuilder
        """
        self._free_block_inputs[operation_id][param_name] = make_metadata_from_val(value)
        return self

    def set_free_block_input_to_value_with_hints(
        self, operation_id, param_name, value, value_type
    ):
        """
        :type operation_id: str
        :type param_name: str
        :type value: Union[str, int, bool, float, list]
        :type value_type: r_objs.VariableTypes
        :rtype: NirvanaReactionBuilder
        """
        self._free_block_inputs[operation_id][param_name] = (
            make_metadata_from_val_with_hints(value, value_type)
        )
        return self

    def set_free_block_input_to_expression_var(self, operation_id, param_name, expression_var):
        """
        :type operation_id: str
        :type param_name: str
        :type expression_var: r_objs.ExpressionVariable
        :rtype: NirvanaReactionBuilder
        """
        self._free_block_inputs[operation_id][param_name] = expression_var
        return self

    def set_data_block_to_artifact(self, block_name, artifact_reference):
        """
        :type block_name: str
        :type artifact_reference: r_objs.ArtifactReference
        :rtype: NirvanaReactionBuilder
        """
        self._data_blocks[block_name] = artifact_reference
        return self

    def set_data_block_to_expression(self, block_name, expression):
        """
        :type block_name: str
        :type expression: str
        :rtype: NirvanaReactionBuilder
        """
        self._data_blocks[block_name] = r_objs.Expression(expression)
        return self

    def set_data_block_to_expression_var(self, block_name, expression_var):
        """
        :type block_name: str
        :type expression: r_objs.ExpressionVariable
        :rtype: NirvanaReactionBuilder
        """
        self._data_blocks[block_name] = expression_var
        return self

    def set_data_block_to_value(self, block_name, value):
        """
        :type block_name: str
        :type value: str
        :rtype: NirvanaReactionBuilder
        """
        self._data_blocks[block_name] = make_metadata_from_val(value)
        return self

    def set_data_block_to_value_with_hints(self, block_name, value, value_type):
        """
        :type block_name: str
        :type value: Union[str, int, bool, float, list]
        :type value_type: r_objs.VariableTypes
        :rtype: NirvanaReactionBuilder
        """
        self._data_blocks[block_name] = (
            make_metadata_from_val_with_hints(value, value_type)
        )
        return self

    def set_permissions(self, namespace_permissions):
        """
        :type namespace_permissions: r_objs.NamespacePermissions
        :rtype: NirvanaReactionBuilder
        """
        self._permissions = namespace_permissions
        return self

    def set_global_param_to_artifact(self, param, artifact_reference):
        """
        :type param: str
        :type artifact_reference: r_objs.ArtifactReference
        :rtype: NirvanaReactionBuilder
        """
        self._global_params[param] = artifact_reference
        return self

    def set_global_param_to_expression(self, param, expression):
        """
        :type param: str
        :type expression: str
        :rtype: NirvanaReactionBuilder
        """
        self._global_params[param] = r_objs.Expression(expression)
        return self

    def set_global_param_to_expression_var(self, param, expression_var):
        """
        :type param: str
        :type expression: r_objs.ExpressionVariable
        :rtype: NirvanaReactionBuilder
        """
        self._global_params[param] = expression_var
        return self

    def set_global_param_to_value(self, param, value):
        """
        :type param: str
        :type value: str
        :rtype: NirvanaReactionBuilder
        """
        self._global_params[param] = make_metadata_from_val(value)
        return self

    def set_global_param_to_value_with_hints(self, param, value, value_type):
        """
        :type param: str
        :type value: Union[str, int, bool, float, list]
        :type value_type: r_objs.VariableTypes
        :rtype: NirvanaReactionBuilder
        """
        self._global_params[param] = make_metadata_from_val_with_hints(value, value_type)
        return self

    def set_block_output_to_artifact(self, operation, output_name, artifact_reference):
        """
        :type operation: str
        :type output_name: str
        :type artifact_reference: r_objs.ArtifactReference
        :rtype: NirvanaReactionBuilder
        """
        self._block_outputs[operation][output_name] = artifact_reference
        return self

    def set_block_output_to_expression(self, operation, output_name, expression):
        """
        :type operation: str
        :type output_name: str
        :type expression: str
        :rtype: NirvanaReactionBuilder
        """
        self._block_outputs[operation][output_name] = r_objs.Expression(expression)
        return self

    def set_block_output_to_expression_var(self, operation, output_name, expression_var):
        """
        :type operation: str
        :type output_name: str
        :type expression: r_objs.ExpressionVariable
        :rtype: NirvanaReactionBuilder
        """
        self._block_outputs[operation][output_name] = expression_var
        return self

    def set_version(self, version):
        # type: (int) -> NirvanaReactionBuilder
        self._version = version
        return self

    def set_expression_after_success(self, expression):
        # type: (str) -> NirvanaReactionBuilder
        self._expression_after_success = r_objs.Expression(expression)
        return self

    def set_expression_after_failure(self, expression):
        # type: (str) -> NirvanaReactionBuilder
        self._expression_after_failure = r_objs.Expression(expression)
        return self

    def set_project(self, project_identifier):
        """
        :type project_identifier: r_objs.ProjectIdentifier
        :rtype: NirvanaReactionBuilder
        """
        self._project = project_identifier

    def set_cleanup_strategy(self, cleanup_strategy):
        """
        :type cleanup_strategy: r_objs.CleanupStrategyDescriptor
        :rtype: NirvanaReactionBuilder
        """
        self._cleanup_strategy = cleanup_strategy

    def set_block_results_ttl(self, ttl):
        """
        :type ttl: int
        :rtype: NirvanaReactionBuilder
        """
        self._block_results_ttl = ttl

    @staticmethod
    def _dict_to_input_node(dict_):
        """
        :param dict_:
        :rtype: InputsValueNode
        """
        node = r_objs.InputsValueNode(nodes={})
        for k, val in six.iteritems(dict_):
            if isinstance(val, r_objs.Expression):
                leaf = r_objs.InputsValueRef(expression=val)
            elif isinstance(val, r_objs.ExpressionVariable):
                leaf = r_objs.InputsValueRef(expression_var=val)
            elif isinstance(val, r_objs.Metadata):
                leaf = r_objs.InputsValueRef(const_value=r_objs.InputsValueConst(generic_value=val))
            elif isinstance(val, r_objs.ArtifactReference):
                leaf = r_objs.InputsValueRef(artifact_reference=val)
            else:
                raise RuntimeError()
            node.nodes[k] = r_objs.InputsValueElement(value=leaf)
        return r_objs.InputsValueElement(node=node)

    @staticmethod
    def _dict_to_out_node(dict_):
        """
        :param dict_:
        :rtype: InputsValueNode
        """
        node = r_objs.OutputsValueNode(nodes={})
        for k, val in six.iteritems(dict_):
            if isinstance(val, r_objs.Expression):
                leaf = r_objs.OutputsValueRef(expression=val)
            elif isinstance(val, r_objs.ExpressionVariable):
                leaf = r_objs.OutputsValueRef(expression_var=val)
            elif isinstance(val, r_objs.ArtifactReference):
                leaf = r_objs.OutputsValueRef(artifact_reference=val)
            else:
                raise RuntimeError()
            node.nodes[k] = r_objs.OutputsValueElement(value=leaf)
        return r_objs.OutputsValueElement(node=node)

    @property
    def operation_descriptor(self):
        if self._reaction_path is None:
            raise RuntimeError("Reaction path is not specified!")

        namespace = r_objs.NamespaceDescriptor(
            namespace_identifier=r_objs.NamespaceIdentifier(
                namespace_path=self._reaction_path),
            description=self._description,
            create_parent_namespaces=True,
            permissions=self._permissions
        )

        operation = r_objs.OperationTypeIdentifier(
            operation_type_key=r_objs.OperationTypeKey(
                set_key="nirvana_operations",
                key="launch_graph",
                version="2"
            ),
        )

        if self._triggers is None:
            raise RuntimeError("You must set reaction triggers!")

        start_conf = r_objs.StartConfiguration(
            triggers=self._triggers,
            operation_globals=r_objs.OperationGlobals(expression=r_objs.Expression(self._globals)) if self._globals is not None else None,
            deprecation_strategies=self._deprecation_strategies if self._deprecation_strategies else None
        )

        dynamic_trigger_list = self._dynamic_triggers

        if self._owner is None:
            raise RuntimeError("You must specify owner!")

        owner = r_objs.ParametersValueElement(
            value=r_objs.ParametersValueLeaf(
                generic_value=make_metadata_from_val(self._owner)
            )
        )

        if self._retries is not None:
            retry_policy_dict = {
                "retryNumber": r_objs.ParametersValueElement(
                    value=r_objs.ParametersValueLeaf(
                        generic_value=make_metadata_from_val(self._retries)
                    )
                ),
            }
            retry_type = self._retry_policy_descriptor["key"]
            if self._retry_params:
                retry_params = {
                    param: r_objs.ParametersValueElement(
                        value=r_objs.ParametersValueLeaf(
                            generic_value=make_metadata_from_val(
                                self._retry_params.get(param)
                            )
                        )
                    )
                    for param in self._retry_policy_descriptor['time_params']
                }
            else:
                retry_params = {
                    self._retry_policy_descriptor["time_param_key"]: (
                        r_objs.ParametersValueElement(
                            value=r_objs.ParametersValueLeaf(
                                generic_value=make_metadata_from_val(self._retry_time_param)
                            )
                        )
                    )
                }

            retry_policy_dict[retry_type] = r_objs.ParametersValueElement(
                node=r_objs.ParametersValueNode(retry_params)
            )
            if self._result_cloning_policy is not None:
                retry_policy_dict["resultCloningPolicy"] = r_objs.ParametersValueElement(
                    value=r_objs.ParametersValueLeaf(
                        generic_value=make_metadata_from_val(self._result_cloning_policy)
                    )
                )
            retry_policy = r_objs.ParametersValueElement(node=r_objs.ParametersValueNode(retry_policy_dict))

        upgrade_strategy = r_objs.ParametersValueElement(
            value=r_objs.ParametersValueLeaf(
                generic_value=make_metadata_from_val(self._update_policy)
            )
        )

        target_flow = None
        if self._target_flow is not None:
            target_flow = r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=make_metadata_from_val(self._target_flow)
                )
            )

        if self._source_flow is None and self._source_instance is None:
            raise RuntimeError("Specify either source flow or source instance!")

        if self._source_flow is not None:
            source_flow = r_objs.ParametersValueElement(node=r_objs.ParametersValueNode(
                {
                    "workflowId": r_objs.ParametersValueElement(
                        value=r_objs.ParametersValueLeaf(
                            generic_value=make_metadata_from_val(self._source_flow)
                        )
                    )
                }
            ))

        else:
            source_flow = r_objs.ParametersValueElement(node=r_objs.ParametersValueNode(
                {
                    "workflowInstanceId": r_objs.ParametersValueElement(
                        value=r_objs.ParametersValueLeaf(
                            generic_value=make_metadata_from_val(self._source_instance)
                        )
                    )
                }
            ))

        parameters = r_objs.ParametersValue(
            root_node=r_objs.ParametersValueNode(
                {
                    "sourceFlowchartId": source_flow,
                    "owner": owner,
                    "upgradeStrategy": upgrade_strategy,
                }
            )
        )

        if self._quota is not None:
            quota = r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=make_metadata_from_val(self._quota)
                )
            )
            parameters.root_node.nodes["quotaProject"] = quota

        if self._retries is not None:
            parameters.root_node.nodes["retryPolicy"] = retry_policy

        if self._result_ttl is not None:
            result_ttl = r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=make_metadata_from_val(self._result_ttl)
                )
            )
            parameters.root_node.nodes["resultTtl"] = result_ttl

        if self._instance_ttl is not None:
            instance_ttl = r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=make_metadata_from_val(self._instance_ttl)
                )
            )
            parameters.root_node.nodes["instanceTtl"] = instance_ttl

        if self._target_flow is not None:
            parameters.root_node.nodes["targetFlowchartId"] = target_flow

        inputs = r_objs.InputsValue(
            root_node=r_objs.InputsValueNode(
                {
                    "blockParameters": r_objs.InputsValueElement(
                        node=r_objs.InputsValueNode({
                            k: self._dict_to_input_node(d) for k, d in six.iteritems(self._block_params)
                        })),
                    "dataBlocks": self._dict_to_input_node(self._data_blocks),
                }
            )
        )

        if self._free_block_inputs:
            inputs.root_node.nodes["freeBlockInputs"] = r_objs.InputsValueElement(
                node=r_objs.InputsValueNode({
                    k: self._dict_to_input_node(d) for k, d in six.iteritems(self._free_block_inputs)
                }))

        if self._global_params:
            inputs.root_node.nodes["globalParameters"] = self._dict_to_input_node(self._global_params)

        outputs = r_objs.OutputsValue(root_node=r_objs.OutputsValueNode({}),
                                      expression_after_success=self._expression_after_success,
                                      expression_after_failure=self._expression_after_failure)

        if self._block_outputs:
            outputs.root_node.nodes["blockOutputs"] = r_objs.OutputsValueElement(node=r_objs.OutputsValueNode({
                k: self._dict_to_out_node(d) for k, d in six.iteritems(self._block_outputs)
            }))

        project_identifier = self._project
        cleanup_strategy = self._cleanup_strategy

        if self._block_results_ttl is not None:
            block_results_ttl = r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=make_metadata_from_val(self._block_results_ttl)
                )
            )
            parameters.root_node.nodes['blockResultsTtl'] = block_results_ttl

        if self._version != 2:
            logging.warning("Version 1 expressions are deprecated. Please use v2 expression and set version for builder to 2\n")

        return r_objs.OperationDescriptor(
            namespace_desc=namespace,
            operation_type_identifier=operation,
            start_conf=start_conf,
            dynamic_trigger_list=dynamic_trigger_list,
            parameters=parameters,
            inputs=inputs,
            outputs=outputs,
            project_identifier=project_identifier,
            cleanup_strategy=cleanup_strategy,
            version=self._version
        )
