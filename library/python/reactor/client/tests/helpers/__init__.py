import reactor_client.reactor_objects as r_objs
from reactor_client.reaction_builders import NirvanaReactionBuilder


def make_test_cron_reaction():
    namespace = r_objs.NamespaceDescriptor(
        namespace_identifier=r_objs.NamespaceIdentifier(namespace_path="/home/test"),
        description="test reaction",
        create_parent_namespaces=True
    )

    operation = r_objs.OperationTypeIdentifier(
        operation_type_key=r_objs.OperationTypeKey(set_key="conversion", key="int_to_string", version="1"),
    )

    start_conf = r_objs.StartConfiguration(
        triggers=r_objs.Triggers(r_objs.CronTrigger(cron_expression="0 0 0 * * ?")),
        operation_globals=r_objs.OperationGlobals(expression=r_objs.Expression("global pi = 3.1415;")),
        deprecation_strategies=[
            r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(artifact_id=42), sensitivity=r_objs.DeprecationSensitivity.STOP),
            r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_id=42)),
                                       sensitivity=r_objs.DeprecationSensitivity.STOP_AND_RECALCULATE)
        ]
    )

    parameters = r_objs.ParametersValue()

    inputs = r_objs.InputsValue(r_objs.InputsValueNode(
        {
            "0": r_objs.InputsValueElement(
                value=r_objs.InputsValueRef(
                    const_value=r_objs.InputsValueConst(
                        generic_value=r_objs.Metadata(type_="/yandex.reactor.artifact.IntArtifactValueProto",
                                                      dict_obj={"value": "1"})
                    )
                )
            )
        }
    ))

    outputs = r_objs.OutputsValue(r_objs.OutputsValueNode(
        {
            "0": r_objs.OutputsValueElement(
                value=r_objs.OutputsValueRef(
                    expression=r_objs.Expression("return a'/user_sessions/routers/daily/artefacts/v1'"
                                                 ".instantiate(Datum.event(), dateNow.plusDays(-1).withNano(0)"
                                                 ".withSecond(0).withMinute(0).withHour(0));")
                )
            )
        }
    ))

    return r_objs.OperationDescriptor(
        namespace_desc=namespace,
        operation_type_identifier=operation,
        start_conf=start_conf,
        parameters=parameters,
        inputs=inputs,
        outputs=outputs
    )


def make_test_nirvana_reaction():
    namespace = r_objs.NamespaceDescriptor(
        namespace_identifier=r_objs.NamespaceIdentifier(namespace_path="/home/solozobov/tests/my_first_api_reaction"),
        description="my first reaction created with API",
        create_parent_namespaces=True
    )
    operation = r_objs.OperationTypeIdentifier(
        operation_type_key=r_objs.OperationTypeKey(
            set_key="nirvana_operations",
            key="launch_graph",
            version="2"
        ),
    )

    start_conf = r_objs.StartConfiguration(
        triggers=r_objs.Triggers(selected_artifacts_trigger=r_objs.SelectedArtifactsTrigger(
            relationship=r_objs.Relationship.OR,
            artifact_refs=[r_objs.ArtifactReference(namespace_id=6903)])),
        operation_globals=r_objs.OperationGlobals(expression=r_objs.Expression("global pi = 3.1415;")),
        deprecation_strategies=[
            r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(artifact_id=42), sensitivity=r_objs.DeprecationSensitivity.STOP),
            r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_id=42)),
                                       sensitivity=r_objs.DeprecationSensitivity.STOP_AND_RECALCULATE)
        ]

    )

    source_flowchart_id_param = r_objs.ParametersValueElement(node=r_objs.ParametersValueNode(
        {
            "workflowId": r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=r_objs.Metadata(
                        type_="/yandex.reactor.artifact.StringArtifactValueProto",
                        dict_obj={"value": "6f65be8d-7421-4ba5-9b72-a26a2ec6f243"}

                    )
                )
            )
        }
    ))

    owner = r_objs.ParametersValueElement(
        value=r_objs.ParametersValueLeaf(
            generic_value=r_objs.Metadata(
                type_="/yandex.reactor.artifact.StringArtifactValueProto",
                dict_obj={"value": "solozobov"}

            )
        )
    )

    quota = r_objs.ParametersValueElement(
        value=r_objs.ParametersValueLeaf(
            generic_value=r_objs.Metadata(
                type_="/yandex.reactor.artifact.StringArtifactValueProto",
                dict_obj={"value": "reactor"}

            )
        )
    )

    retry_policy = r_objs.ParametersValueElement(node=r_objs.ParametersValueNode(
        {
            "retryNumber": r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=r_objs.Metadata(
                        type_="/yandex.reactor.artifact.IntArtifactValueProto",
                        dict_obj={"value": "3"}

                    )
                )
            ),
            "uniformRetry": r_objs.ParametersValueElement(
                node=r_objs.ParametersValueNode(
                    {
                        "delay": r_objs.ParametersValueElement(
                            value=r_objs.ParametersValueLeaf(
                                generic_value=r_objs.Metadata(
                                    type_="/yandex.reactor.artifact.IntArtifactValueProto",
                                    dict_obj={"value": "100"}
                                )
                            )
                        )

                    }
                )
            ),
            "resultCloningPolicy": r_objs.ParametersValueElement(
                value=r_objs.ParametersValueLeaf(
                    generic_value=r_objs.Metadata(
                        type_="/yandex.reactor.artifact.StringArtifactValueProto",
                        dict_obj={"value": "simple"}

                    )
                )
            ),
        }
    ))

    upgrade_strategy = r_objs.ParametersValueElement(
        value=r_objs.ParametersValueLeaf(
            generic_value=r_objs.Metadata(
                type_="/yandex.reactor.artifact.StringArtifactValueProto",
                dict_obj={"value": "Ignore"}

            )
        )
    )

    target_flow = r_objs.ParametersValueElement(
        value=r_objs.ParametersValueLeaf(
            generic_value=r_objs.Metadata(
                type_="/yandex.reactor.artifact.StringArtifactValueProto",
                dict_obj={"value": "1a85b909-8abc-44fe-8d9e-1652a6e1fc65"}

            )
        )
    )

    result_ttl = r_objs.ParametersValueElement(
        value=r_objs.ParametersValueLeaf(
            generic_value=r_objs.Metadata(
                type_="/yandex.reactor.artifact.IntArtifactValueProto",
                dict_obj={"value": "3"}

            )
        )
    )

    instance_ttl = r_objs.ParametersValueElement(
        value=r_objs.ParametersValueLeaf(
            generic_value=r_objs.Metadata(
                type_="/yandex.reactor.artifact.IntArtifactValueProto",
                dict_obj={"value": "1"}

            )
        )
    )
    block_results_ttl = r_objs.ParametersValueElement(
        value=r_objs.ParametersValueLeaf(
            generic_value=r_objs.Metadata(
                type_="/yandex.reactor.artifact.IntArtifactValueProto",
                dict_obj={"value": "14"}
            )
        )
    )
    parameters = r_objs.ParametersValue(
        root_node=r_objs.ParametersValueNode(
            {
                "sourceFlowchartId": source_flowchart_id_param,
                "owner": owner,
                "quotaProject": quota,
                "retryPolicy": retry_policy,
                "upgradeStrategy": upgrade_strategy,
                "targetFlowchartId": target_flow,
                "resultTtl": result_ttl,
                "instanceTtl": instance_ttl,
                'blockResultsTtl': block_results_ttl
            }
        )
    )

    block_parameters = r_objs.InputsValueElement(node=r_objs.InputsValueNode(
        {
            "operation-1497596731624-50$181": r_objs.InputsValueElement(node=r_objs.InputsValueNode(
                {
                    "comment": r_objs.InputsValueElement(value=r_objs.InputsValueRef(
                        const_value=r_objs.InputsValueConst(generic_value=r_objs.Metadata(
                            type_="/yandex.reactor.artifact.StringArtifactValueProto",
                            dict_obj={"value": "description"}

                        ))
                    ))
                }
            ))
        }
    ))

    global_parameters = r_objs.InputsValueElement(node=r_objs.InputsValueNode(
        {
            "dump-date": r_objs.InputsValueElement(value=r_objs.InputsValueRef(
                artifact_reference=r_objs.ArtifactReference(namespace_id=6903)
            )),
            "mr-account": r_objs.InputsValueElement(value=r_objs.InputsValueRef(
                const_value=r_objs.InputsValueConst(generic_value=r_objs.Metadata(
                    type_="/yandex.reactor.artifact.StringArtifactValueProto",
                    dict_obj={"value": "test-account"}
                ))
            )),
            "timestamp": r_objs.InputsValueElement(value=r_objs.InputsValueRef(
                expression=r_objs.Expression("return 1234567890;")
            )),
            "list": r_objs.InputsValueElement(value=r_objs.InputsValueRef(
                const_value=r_objs.InputsValueConst(generic_value=r_objs.Metadata(
                    type_="/yandex.reactor.artifact.FloatListArtifactValueProto",
                    dict_obj={"values": [1.0, 4.2]}
                ))
            )),
        }
    ))

    data_blocks = r_objs.InputsValueElement(node=r_objs.InputsValueNode(
        {
            "aggregated-table-example": r_objs.InputsValueElement(value=r_objs.InputsValueRef(
                artifact_reference=r_objs.ArtifactReference(namespace_id=6902)
            ))
        }
    ))

    inputs = r_objs.InputsValue(
        root_node=r_objs.InputsValueNode(
            {
                "globalParameters": global_parameters,
                "blockParameters": block_parameters,
                "dataBlocks": data_blocks,
            }
        )
    )

    op_outs = r_objs.OutputsValueElement(node=r_objs.OutputsValueNode(
        {
            "outputFormula": r_objs.OutputsValueElement(
                value=r_objs.OutputsValueRef(
                    expression=r_objs.Expression(
                        "return a'/web-conveyor/formulas/rated/rated-kubr-l3'.instantiate(output);")
                )
            )
        }
    ))

    sum_formulas = r_objs.OutputsValueElement(node=r_objs.OutputsValueNode(
        {
            "combinedFormula": r_objs.OutputsValueElement(
                value=r_objs.OutputsValueRef(
                    artifact_reference=r_objs.ArtifactReference(namespace_id=329)
                )
            )
        }
    ))

    outputs = r_objs.OutputsValue(
        root_node=r_objs.OutputsValueNode(
            {
                "blockOutputs": r_objs.OutputsValueElement(node=r_objs.OutputsValueNode(
                    {
                        "operation-1497596731624-50$217": op_outs,
                        "sum_formulas_desktop": sum_formulas

                    }
                ))
            }
        )
    )
    project_identifier = r_objs.ProjectIdentifier(project_id=42)
    cleanup_strategy = r_objs.CleanupStrategyDescriptor(
        cleanup_strategies=[r_objs.CleanupStrategy(
            ttl_cleanup_strategy=r_objs.TtlCleanupStrategy(42)
        )]
    )

    return r_objs.OperationDescriptor(
        namespace_desc=namespace,
        operation_type_identifier=operation,
        start_conf=start_conf,
        parameters=parameters,
        inputs=inputs,
        outputs=outputs,
        project_identifier=project_identifier,
        cleanup_strategy=cleanup_strategy
    )


def make_nirvana_reaction_with_builder():
    builder = NirvanaReactionBuilder()
    builder.set_reaction_path(path="/home/solozobov/tests/my_first_api_reaction",
                              description="my first reaction created with API")

    builder.trigger_by_artifacts(artifact_refs=[r_objs.ArtifactReference(namespace_id=6903)],
                                 relationship=r_objs.Relationship.OR)

    builder.set_global_expression("global pi = 3.1415;")
    builder.set_deprecation_strategies(
        [r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(artifact_id=42), sensitivity=r_objs.DeprecationSensitivity.STOP),
         r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_id=42)),
                                    sensitivity=r_objs.DeprecationSensitivity.STOP_AND_RECALCULATE)
         ]
    )
    builder.set_owner("solozobov")
    builder.set_quota("reactor")
    builder.set_source_graph(flow_id="6f65be8d-7421-4ba5-9b72-a26a2ec6f243")
    builder.set_result_ttl(3)
    builder.set_instance_ttl(1)
    builder.set_block_results_ttl(14)
    builder.set_retry_policy(3, 100, result_cloning_policy=r_objs.NirvanaResultCloningPolicy.TOP_LEVEL)
    builder.set_target_graph("1a85b909-8abc-44fe-8d9e-1652a6e1fc65")

    builder.set_global_param_to_artifact("dump-date", r_objs.ArtifactReference(namespace_id=6903))
    builder.set_global_param_to_value("mr-account", "test-account")
    builder.set_global_param_to_expression("timestamp", "return 1234567890;")
    builder.set_global_param_to_value_with_hints("list", [1, 4.2], r_objs.VariableTypes.LIST_FLOAT)

    builder.set_block_param_to_value("operation-1497596731624-50$181", "comment", "description")

    builder.set_data_block_to_artifact("aggregated-table-example", r_objs.ArtifactReference(namespace_id=6902))
    builder.set_block_output_to_expression(
        "operation-1497596731624-50$217",
        "outputFormula",
        expression="return a'/web-conveyor/formulas/rated/rated-kubr-l3'.instantiate(output);"
    )

    builder.set_block_output_to_artifact(
        "sum_formulas_desktop",
        "combinedFormula",
        r_objs.ArtifactReference(namespace_id=329)
    )
    builder.set_project(r_objs.ProjectIdentifier(project_id=42))
    builder.set_cleanup_strategy(r_objs.CleanupStrategyDescriptor(
        cleanup_strategies=[r_objs.CleanupStrategy(
            ttl_cleanup_strategy=r_objs.TtlCleanupStrategy(42)
        )]
    ))
    return builder.operation_descriptor


def make_nirvana_reaction_with_builder_block_params():
    builder = NirvanaReactionBuilder()
    builder.set_reaction_path(path="/home/test", description="test reaction")
    builder.trigger_by_artifacts(artifact_refs=[r_objs.ArtifactReference(namespace_id=2749, artifact_id=680)],
                                 relationship=r_objs.Relationship.OR)

    builder.set_owner("timmyb32r")
    builder.set_source_graph(instance_id="c3694475-c91d-4009-b130-fc85b5859f2b")
    builder.set_target_graph("8e219d1b-975c-4466-8375-93f86b594da8")
    builder.set_block_update_policy(r_objs.NirvanaUpdatePolicy.MINIMUM_NOT_DEPRECATED)

    builder.set_data_block_to_artifact(block_name="params_json",
                                       artifact_reference=r_objs.ArtifactReference(namespace_id=2748,
                                                                                   artifact_id=679))

    builder.set_data_block_to_artifact(block_name="logfeller_indexer_executable",
                                       artifact_reference=r_objs.ArtifactReference(namespace_id=2154,
                                                                                   artifact_id=477))

    builder.set_block_param_to_value("logfeller_util_env_settings", "logfeller_operation", "build_fast_logs")
    builder.set_free_block_input_to_artifact("logfeller_util_env_settings", "settings_json_2",
                                             r_objs.ArtifactReference(namespace_id=2746, artifact_id=677))
    builder.set_free_block_input_to_artifact("logfeller_util_env_settings", "settings_json_1",
                                             r_objs.ArtifactReference(namespace_id=2745, artifact_id=676))
    builder.set_free_block_input_to_artifact("logfeller_util_env_settings", "settings_json_0",
                                             r_objs.ArtifactReference(namespace_id=2578, artifact_id=609))

    return builder.operation_descriptor


def make_nirvana_reaction_with_builder_exp_retries():
    builder = NirvanaReactionBuilder()
    builder.set_reaction_path(path="/home/test", description="test reaction")
    builder.trigger_by_artifacts(artifact_refs=[r_objs.ArtifactReference(namespace_id=0, artifact_id=0)],
                                 relationship=r_objs.Relationship.OR)

    builder.set_owner("owner")
    builder.set_source_graph(instance_id="source_graph")
    builder.set_target_graph("target_graph")

    builder.set_retry_policy(3, 100, r_objs.RetryPolicyDescriptor.EXPONENTIAL)

    return builder.operation_descriptor


def make_nirvana_reaction_with_builder_random_retries():
    builder = NirvanaReactionBuilder()
    builder.set_reaction_path(path="/home/test", description="test reaction")
    builder.trigger_by_artifacts(artifact_refs=[r_objs.ArtifactReference(namespace_id=0, artifact_id=0)],
                                 relationship=r_objs.Relationship.OR)

    builder.set_owner("owner")
    builder.set_source_graph(instance_id="source_graph")
    builder.set_target_graph("target_graph")

    builder.set_retry_policy_with_multiple_params(
        retries=3,
        retry_params={'delayVariance': 10000, 'minDelay': 300000},
        retry_policy_descriptor=r_objs.RetryPolicyDescriptorMultipleParams.RANDOM,
        result_cloning_policy=r_objs.NirvanaResultCloningPolicy.RECURSIVE
    )
    return builder.operation_descriptor


def make_nirvana_reaction_with_builder_dynamic_triggers():
    builder = NirvanaReactionBuilder()
    builder.set_reaction_path(path="/home/test", description="test reaction")
    builder.set_dynamic_triggers(
        triggers=[
            r_objs.DynamicTrigger(
                trigger_name='cron_trigger',
                expression=r_objs.Expression('global a = 42;'),
                cron_trigger=r_objs.CronTrigger(
                    cron_expression='0 0 10 ? * * *'
                )
            ),
            r_objs.DynamicTrigger(
                trigger_name='artifact_trigger',
                artifact_trigger=r_objs.DynamicArtifactTrigger(
                    triggers=[r_objs.ArtifactReference(
                        namespace_identifier=r_objs.NamespaceIdentifier(
                            namespace_path=path
                        )
                    ) for path in ['a', 'b']]
                )
            )
        ]
    )
    builder.set_owner("owner")
    builder.set_source_graph(instance_id="source_graph")
    return builder.operation_descriptor


def make_test_operation():
    return r_objs.Operation(
        id_=42,
        operation_type_id=42,
        namespace_id=42,
        status=r_objs.OperationStatus.DELETED,
        start_conf=r_objs.StartConfiguration(
            inputs_resolver=r_objs.InputResolver(equal_attr_resolver=r_objs.EqualAttributesResolver(
                artifact_to_attr={
                    42: "attribute_2",
                    43: "attribute_1"
                }
            )),
            operation_globals=r_objs.OperationGlobals(r_objs.Expression('global a = 3.14')),
            triggers=r_objs.Triggers(
                selected_artifacts_trigger=r_objs.SelectedArtifactsTrigger(
                    artifact_refs=[
                        r_objs.ArtifactReference(
                            artifact_id=42,
                            namespace_id=42,
                            namespace_identifier=r_objs.NamespaceIdentifier(namespace_path="test")
                        )
                    ],
                    relationship=r_objs.Relationship.USER_TIMESTAMP_EQUALITY
                )
            ),
            deprecation_strategies=[
                r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(artifact_id=42), sensitivity=r_objs.DeprecationSensitivity.STOP),
                r_objs.DeprecationStrategy(artifact=r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_id=42)),
                                           sensitivity=r_objs.DeprecationSensitivity.STOP_AND_RECALCULATE)
            ]
        ),
        parameters=r_objs.ParametersValue(root_node=r_objs.ParametersValueNode(
            {
                "parameter_key_1": r_objs.ParametersValueElement(),
                "parameter_key_2": r_objs.ParametersValueElement(list_=r_objs.ParametersValueList(elements=[]))
            }
        )),
        inputs=r_objs.InputsValue(root_node=r_objs.InputsValueNode(
            {
                "input_key_1": r_objs.InputsValueElement(),
                "input_key_2": r_objs.InputsValueElement(list_=r_objs.InputsValueList(elements=[]))
            }
        )),
        outputs=r_objs.OutputsValue(root_node=r_objs.OutputsValueNode(
            {
                "output_key_1": r_objs.OutputsValueElement(),
                "output_key_2": r_objs.OutputsValueElement(list_=r_objs.OutputsValueList(elements=[]))
            }
        ))
    )


def make_test_v2_nirvana_reaction():
    builder = NirvanaReactionBuilder()
    builder.set_reaction_path(path="/home/test", description="test reaction")
    builder.trigger_by_artifacts(artifact_refs=[],
                                 relationship=r_objs.Relationship.AND)

    builder.set_owner("npytincev")
    builder.set_source_graph(instance_id="1c65af72-bd38-4cf7-9dfe-c282e6e6bfba")
    builder.set_target_graph("b7df825c-8a9f-4faf-a78e-8ad2ad474752")

    builder.set_block_output_to_expression_var(
        operation="timmyb32r_expression_example",
        output_name="timmyb32r_output",
        expression_var=r_objs.ExpressionVariable(var_name="my_output")
    )

    builder.set_block_param_to_value(
        operation_id="timmyb32r_expression_example",
        param_name="exit_code",
        value=0
    )
    builder.set_version(2)
    builder.set_expression_after_failure(
        "a'/home/timmyb32r/expression_example/artifact_on_fail'.instantiate(my_output)")
    builder.set_expression_after_success(
        "a'/home/timmyb32r/expression_example/artifact_on_success'.instantiate(my_output)")

    return builder.operation_descriptor


def make_test_namespace(test_user, path):
    return r_objs.NamespaceDescriptor(
        r_objs.NamespaceIdentifier(namespace_path=path),
        '',
        r_objs.NamespacePermissions({test_user: r_objs.NamespaceRole.RESPONSIBLE}),
        create_parent_namespaces=True
    )


def make_test_cron_blank_reaction(test_user, path):
    return r_objs.OperationDescriptor(
        make_test_namespace(test_user, path),
        r_objs.OperationTypeIdentifier(operation_type_key=r_objs.OperationTypeKey('miscellaneous', 'blank_operation', '1')),
        r_objs.StartConfiguration(triggers=r_objs.Triggers(cron_trigger=r_objs.CronTrigger('0 0 9 * * ?'))),
        r_objs.ParametersValue(),
        r_objs.InputsValue(r_objs.InputsValueNode()),
        r_objs.OutputsValue(r_objs.OutputsValueNode()),
        version=2,
    )


def make_test_blank_reaction_triggered_by_artifacts(test_user, test_project, path, artifacts, relationship):
    return r_objs.OperationDescriptor(
        make_test_namespace(test_user, path),
        r_objs.OperationTypeIdentifier(operation_type_key=r_objs.OperationTypeKey('miscellaneous', 'blank_operation', '1')),
        r_objs.StartConfiguration(triggers=r_objs.Triggers(selected_artifacts_trigger=r_objs.SelectedArtifactsTrigger(
            [r_objs.ArtifactReference(artifact_id=artifact.artifact_id, namespace_id=artifact.namespace_id) for artifact in artifacts],
            relationship
        ))),
        r_objs.ParametersValue(),
        r_objs.InputsValue(r_objs.InputsValueNode()),
        r_objs.OutputsValue(r_objs.OutputsValueNode()),
        version=2,
        project_identifier=r_objs.ProjectIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=test_project)),
        cleanup_strategy=r_objs.CleanupStrategyDescriptor([r_objs.CleanupStrategy(r_objs.TtlCleanupStrategy(1))]),
    )


def make_test_queue(test_user, test_project, path):
    return r_objs.QueueDescriptor(
        make_test_namespace(test_user, path),
        r_objs.QueueConfiguration(0,
                                  r_objs.QueuePriorityFunction.USER_TIME_NEWEST_FIRST,
                                  r_objs.QueueMaxQueuedInstances(1000),
                                  r_objs.QueueMaxRunningInstancesPerReaction(1000),
                                  r_objs.QueueMaxQueuedInstancesPerReaction(1000)),
        project_identifier=r_objs.ProjectIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=test_project)),
    )
