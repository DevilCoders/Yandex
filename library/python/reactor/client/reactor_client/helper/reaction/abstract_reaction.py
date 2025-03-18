from abc import abstractmethod

from reactor_client import reactor_objects as r_objs


class AbstractReactionBuilder(object):
    def __init__(self):
        self._reaction_path = None
        self._description = None
        self._permissions = None

        self._project = None
        self._cleanup_strategy = None

        self._demand_trigger = False
        self._dynamic_triggers = []

        self._deprecation_strategies = []

        self._reaction_type = None

        self._version = 2
        self._before_expression = None
        self._after_success_expression = None
        self._after_fail_expression = None

    def set_reaction_path(self, path, description=None):
        """
        :type path: str
        :type description: str | None
        :rtype: AbstractReactionBuilder
        """
        self._reaction_path = path
        self._description = description
        return self

    def set_permissions(self, role_2_permission):
        """
        :type role_2_permission: dict[str, r_objs.NamespaceRole]
        :rtype: AbstractReactionBuilder
        """
        self._permissions = role_2_permission
        return self

    def set_project(self, project_path):
        """
        :type project_path: str
        :rtype: AbstractReactionBuilder
        """
        self._project = r_objs.ProjectIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=project_path))
        return self

    def set_reaction_instance_ttl_days(self, ttl_days):
        """
        :type ttl_days: int
        :rtype: AbstractReactionBuilder
        """
        self._cleanup_strategy = r_objs.CleanupStrategyDescriptor([r_objs.CleanupStrategy(r_objs.TtlCleanupStrategy(ttl_days))])
        return self

    # Triggers

    def add_trigger_by_cron(self, trigger_name, cron_expr, misfire_policy=None, expr=None):
        """
        :param trigger_name: str
        :param cron_expr: expression like in crontab file
        :type cron_expr: str
        :type misfire_policy: r_objs.MisfirePolicy
        :type expr: str | None
        :return: AbstractReactionBuilder
        """
        self._demand_trigger = False
        self._dynamic_triggers.append(
            r_objs.DynamicTrigger(
                trigger_name,
                None if expr is None else r_objs.Expression(expr),
                cron_trigger=r_objs.CronTrigger(cron_expr, r_objs.MisfirePolicy.FIRE_ALL if misfire_policy is None else misfire_policy)
            )
        )
        return self

    def add_trigger_by_artifacts(self, trigger_name, artifacts, expr=None):
        """
        :type trigger_name: str
        :type artifacts: List[r_objs.ArtifactReference]
        :type expr: str | None
        :rtype: AbstractReactionBuilder
        """
        self._demand_trigger = False
        self._dynamic_triggers.append(
            r_objs.DynamicTrigger(
                trigger_name,
                None if expr is None else r_objs.Expression(expr),
                artifact_trigger=r_objs.DynamicArtifactTrigger(artifacts)
            )
        )
        return self

    def set_trigger_by_demand(self):
        """
        :rtype: AbstractReactionBuilder
        """
        self._demand_trigger = True
        self._dynamic_triggers = []
        return self

    def add_deprecation_strategy(self, deprecation_strategy):
        """
        :type deprecation_strategy: r_objs.DeprecationStrategy
        :rtype: AbstractReactionBuilder
        """
        self._deprecation_strategies.append(deprecation_strategy)
        return self

    def set_reaction_type(self, reaction_set, reaction, version):
        """
        :type reaction_set: str
        :type reaction: str
        :type version: str
        :rtype: AbstractReactionBuilder
        """
        self._reaction_type = r_objs.OperationTypeIdentifier(
            operation_type_key=r_objs.OperationTypeKey(set_key=reaction_set, key=reaction, version=version),
        )
        return self

    # Expressions

    def set_before_expression(self, expression):
        """
        :type expression: str
        :rtype: AbstractReactionBuilder
        """
        self._before_expression = r_objs.Expression(expression)
        return self

    def set_after_success_expression(self, expression):
        """
        :type expression: str
        :rtype: AbstractReactionBuilder
        """
        self._after_success_expression = r_objs.Expression(expression)
        return self

    def set_after_fail_expression(self, expression):
        """
        :type expression: str
        :rtype: AbstractReactionBuilder
        """
        self._after_fail_expression = r_objs.Expression(expression)
        return self

    # Endpoints

    @property
    @abstractmethod
    def parameters_values(self):
        """
        :rtype: dict[str, r_objs.ParametersValueElement]
        """
        return {}

    @property
    def inputs_values(self):
        """
        :rtype: dict[str, r_objs.InputsValueElement]
        """
        return {}

    @property
    def outputs_value(self):
        """
        :rtype: dict[str, r_objs.OutputsValueElement]
        """
        return {}

    @property
    def operation_descriptor(self):
        if self._reaction_path is None:
            raise RuntimeError("Reaction path is not specified!")

        namespace = r_objs.NamespaceDescriptor(
            namespace_identifier=r_objs.NamespaceIdentifier(
                namespace_path=self._reaction_path),
            description=self._description,
            create_parent_namespaces=True,
            permissions=r_objs.NamespacePermissions(self._permissions, version=1) if self._permissions else None
        )

        if self._reaction_type is None:
            raise RuntimeError("Reaction type is not specified!")

        if self._demand_trigger:
            trigger_type = r_objs.Triggers(demand_trigger=r_objs.DemandTriggerPlaceholder())
            dynamic_trigger_list = r_objs.DynamicTriggerList([])
        elif self._dynamic_triggers is None:
            raise RuntimeError("You must set at least one trigger!")
        else:
            trigger_type = r_objs.Triggers(dynamic_trigger=r_objs.DynamicTriggerPlaceholder())
            dynamic_trigger_list = r_objs.DynamicTriggerList(self._dynamic_triggers)
        start_conf = r_objs.StartConfiguration(
            triggers=trigger_type,
            operation_globals=r_objs.OperationGlobals(expression=self._before_expression),
            deprecation_strategies=self._deprecation_strategies if self._deprecation_strategies else None
        )

        parameters = r_objs.ParametersValue(root_node=r_objs.ParametersValueNode(self.parameters_values))

        inputs = r_objs.InputsValue(root_node=r_objs.InputsValueNode(self.inputs_values))

        outputs = r_objs.OutputsValue(root_node=r_objs.OutputsValueNode(self.outputs_value),
                                      expression_after_success=self._after_success_expression,
                                      expression_after_failure=self._after_fail_expression)

        project_identifier = self._project
        cleanup_strategy = self._cleanup_strategy

        return r_objs.OperationDescriptor(
            namespace_desc=namespace,
            operation_type_identifier=self._reaction_type,
            start_conf=start_conf,
            dynamic_trigger_list=dynamic_trigger_list,
            parameters=parameters,
            inputs=inputs,
            outputs=outputs,
            project_identifier=project_identifier,
            cleanup_strategy=cleanup_strategy,
            version=self._version
        )
