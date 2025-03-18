import json
import datetime
import pytz
import six
import sys
import logging

import pytest
import marshmallow

import yatest

import reactor_client.reactor_objects as r_objs
import reactor_client.reactor_schemas as r_schemas

from library.python.reactor.client.tests.helpers import (
    make_test_cron_reaction,
    make_test_nirvana_reaction,
    make_nirvana_reaction_with_builder,
    make_nirvana_reaction_with_builder_block_params,
    make_nirvana_reaction_with_builder_dynamic_triggers,
    make_nirvana_reaction_with_builder_exp_retries,
    make_nirvana_reaction_with_builder_random_retries,
    make_test_v2_nirvana_reaction
)

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
handler = logging.StreamHandler(sys.stderr)
handler.setLevel(logging.DEBUG)
logger.addHandler(handler)


def get_mock_requests(path):
    with open(path, 'rb') as f:
        return json.load(f)


MOCK_REQUESTS = get_mock_requests(
    yatest.common.source_path('library/python/reactor/client/tests/resourses/test_requests.json'))


class MockObj(r_objs.ReactorDataObject):
    def __init__(self, field_a=None, field_b=None):
        self.field_a = field_a
        self.field_b = field_b


class MockSchema(r_schemas.OneOfSchema):
    field_a = marshmallow.fields.Integer()
    field_b = marshmallow.fields.Integer()

    class Meta:
        one_of = ["field_a", "field_b"]

    @marshmallow.post_load
    def make(self, data, **kwargs):
        return MockObj(**data)


def test_one_of_schema_raise_error_if_more_than_one_fields():
    schema = MockSchema()
    obj = MockObj(field_a=1, field_b=2)
    if six.PY2:
        errors = schema.validate(schema.dump(obj).data)
        assert errors
    else:
        with pytest.raises(marshmallow.ValidationError):
            schema.load(schema.dump(obj))


OBJS_DUMPS_AND_SCHEMAS = [
    (
        MockObj(field_a=1),
        {"field_a": 1},
        MockSchema()
    ),
    (
        r_objs.ParametersValueLeaf(),
        {},
        r_schemas.ParametersValueLeafSchema()
    ),
    (
        r_objs.ArtifactIdentifier(artifact_id=1231),
        {"artifactId": "1231"},
        r_schemas.ArtifactIdentifierSchema()
    ),
    (
        r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path="/a/b")),
        {"namespaceIdentifier": {"namespacePath": "/a/b"}},
        r_schemas.ArtifactIdentifierSchema()
    ),
    (
        r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_id=42)),
        {"namespaceIdentifier": {"namespaceId": "42"}},
        r_schemas.ArtifactIdentifierSchema()
    ),
    (
        r_objs.Artifact(artifact_id=42, artifact_type_id=42,
                        namespace_id=42, project_id=42),
        {
            "id": "42",
            "artifactTypeId": "42",
            "namespaceId": "42",
            "projectId": "42"

        },
        r_schemas.ArtifactSchema()
    ),
    (
        r_objs.ArtifactTypeIdentifier(artifact_type_key="YT_PATH"),
        {
            "artifactTypeKey": "YT_PATH"
        },
        r_schemas.ArtifactTypeIdentifierSchema()
    ),
    (
        r_objs.ArtifactTypeIdentifier(artifact_type_id=42),
        {
            "artifactTypeId": "42"
        },
        r_schemas.ArtifactTypeIdentifierSchema()
    ),
    (
        r_objs.ArtifactType(artifact_type_id=42, key="YT_PATH", name="Quick Silver",
                            description="Some description", status=r_objs.ArtifactTypeStatus.DEPRECATED),
        {
            "id": "42",
            "key": "YT_PATH",
            "name": "Quick Silver",
            "description": "Some description",
            "status": "DEPRECATED"
        },
        r_schemas.ArtifactTypeSchema()
    ),
    (
        r_objs.NamespacePermissions({
            "login_a": r_objs.NamespaceRole.READER,
            "login_b": r_objs.NamespaceRole.WRITER,
        }),
        {
            "roles": {
                "login_a": "READER",
                "login_b": "WRITER",
            },
            "version": 0
        },
        r_schemas.NamespacePermissionsSchema()
    ),
    (
        r_objs.Attributes({
            "login_a": "test",
            "login_b": "test",
        }),
        {
            "keyValue": {
                "login_a": "test",
                "login_b": "test",
            }
        },
        r_schemas.AttributesSchema()
    ),
    (
        r_objs.NamespaceDescriptor(
            namespace_identifier=r_objs.NamespaceIdentifier(namespace_path="test"),
            description="YT reduce operations queue",
            permissions=r_objs.NamespacePermissions(
                roles={
                    "login_1": r_objs.NamespaceRole.READER,
                    "login_2": r_objs.NamespaceRole.WRITER
                },
                version=1
            ),
            create_parent_namespaces=True
        ),
        {
            "namespaceIdentifier": {
                "namespacePath": "test"
            },
            "description": "YT reduce operations queue",
            "permissions": {
                "roles": {
                    "login_1": "READER",
                    "login_2": "WRITER"
                },
                "version": 1
            },
            "createParentNamespaces": True
        },
        r_schemas.NamespaceDescriptorSchema()
    ),
    (
        r_objs.OperationTypeIdentifier(
            operation_type_id=42
        ),
        {
            "operationTypeId": "42"
        },
        r_schemas.OperationTypeIdentifierSchema()
    ),
    (
        r_objs.OperationTypeIdentifier(
            operation_type_key=r_objs.OperationTypeKey(
                set_key="nirvana_operations",
                key="launch_graph",
                version="2"
            )
        ),
        {
            "operationTypeKey": {
                "operationSetKey": "nirvana_operations",
                "operationKey": "launch_graph",
                "operationVersion": "2"
            }
        },
        r_schemas.OperationTypeIdentifierSchema()
    ),
    (
        r_objs.ParametersValue(
            r_objs.ParametersValueNode(
                nodes={
                    "parameter_key_1": r_objs.ParametersValueElement(
                        value=r_objs.ParametersValueLeaf(
                            generic_value=r_objs.Metadata(type_="/yandex.reactor.artifact.IntArtifactValueProto",
                                                          dict_obj={"value": "42"})
                        )
                    ),
                    "parameter_key_2": r_objs.ParametersValueElement(
                        list_=r_objs.ParametersValueList(
                            elements=[r_objs.ParametersValueElement(
                                value=r_objs.ParametersValueLeaf(
                                    generic_value=r_objs.Metadata(
                                        type_="/yandex.reactor.artifact.IntArtifactValueProto", dict_obj={"value": "42"}
                                    )
                                )
                            )]
                        )
                    )

                }
            )
        ),
        {
            "rootNode": {
                "nodes": {
                    "parameter_key_1": {
                        "value": {
                            "genericValue": {
                                "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                                "value": "42"
                            }
                        }
                    },
                    "parameter_key_2": {
                        "list": {
                            "elements": [
                                {
                                    "value": {
                                        "genericValue": {
                                            "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                                            "value": "42"
                                        }
                                    }
                                }
                            ]
                        }
                    }
                }
            }
        },
        r_schemas.ParametersValueSchema()
    ),
    (
        r_objs.InputsValueRef(expression=r_objs.Expression("global a = 3.14")),
        {
            "expression": {
                "expression": {
                    "expression": "global a = 3.14"
                }
            }
        },
        r_schemas.InputsValueRefSchema()
    ),
    (
        r_objs.ExpressionVariable(var_name="test", artifact_type_id=1),
        {
            "variableName": "test",
            "artifactTypeId": "1"
        },
        r_schemas.ExpressionVariableSchema()
    ),
    (
        r_objs.OutputsValueRef(expression_var=r_objs.ExpressionVariable(var_name="test", artifact_type_id=1)),
        {
            "expressionVariable":
                {
                    "variableName": "test",
                    "artifactTypeId": "1"
                }
        },
        r_schemas.OutputsValueRefSchema()
    ),
    (
        r_objs.InputsValueRef(expression_var=r_objs.ExpressionVariable(var_name="test", artifact_type_id=1)),
        {
            "expressionVariable":
                {
                    "variableName": "test",
                    "artifactTypeId": "1"
                }
        },
        r_schemas.InputsValueRefSchema()
    ),
    (
        r_objs.OutputsValue(
            r_objs.OutputsValueNode({
                "0": r_objs.OutputsValueElement(
                    value=r_objs.OutputsValueRef(
                        expression=r_objs.Expression("return a'/user_sessions/routers/daily/artefacts/v1'"
                                                     ".instantiate(Datum.event(), dateNow.plusDays(-1).withNano(0)"
                                                     ".withSecond(0).withMinute(0).withHour(0));")
                    )
                )
            })
        ),
        {
            "rootNode": {
                "nodes": {
                    "0": {
                        "value": {
                            "expression": {
                                "expression": {
                                    "expression": "return a'/user_sessions/routers/daily/artefacts/v1'.instantiate(Datu"
                                                  "m.event(), dateNow.plusDays(-1).withNano(0).withSecond(0).withMinute"
                                                  "(0).withHour(0));"
                                }
                            }
                        }
                    }
                }
            }
        },
        r_schemas.OutputsValueSchema()
    ),
    (
        r_objs.OutputsValue(
            r_objs.OutputsValueNode(),
            expression_after_success=r_objs.Expression("global a = 1;"),
            expression_after_failure=r_objs.Expression("global a = 0;"),
        ),
        {
            "rootNode": {
                "nodes": {
                }
            },
            "expressionAfterSuccess": {
                "expression": "global a = 1;"
            },
            "expressionAfterFail": {
                "expression": "global a = 0;"
            }
        },
        r_schemas.OutputsValueSchema()
    ),
    (
        make_test_cron_reaction(),
        MOCK_REQUESTS["cron_reaction"]["reaction"],
        r_schemas.OperationDescriptorSchema()
    ),
    (
        make_test_nirvana_reaction(),
        MOCK_REQUESTS["nirvana_reaction"]["reaction"],
        r_schemas.OperationDescriptorSchema()
    ),
    (
        make_nirvana_reaction_with_builder(),
        MOCK_REQUESTS["nirvana_reaction"]["reaction"],
        r_schemas.OperationDescriptorSchema()
    ),
    (
        make_nirvana_reaction_with_builder_block_params(),
        MOCK_REQUESTS["reaction_with_free_block_params"]["reaction"],
        r_schemas.OperationDescriptorSchema()
    ),
    (
        make_test_v2_nirvana_reaction(),
        MOCK_REQUESTS["test_v2_operation_descriptor"],
        r_schemas.OperationDescriptorSchema()
    ),
    (
        make_nirvana_reaction_with_builder_dynamic_triggers().dynamic_trigger_list,
        {
            "triggers": [
                {
                    "triggerName": "cron_trigger",
                    "expression": {"expression": "global a = 42;"},
                    "cronTrigger": {"cronExpression": "0 0 10 ? * * *"},
                },
                {
                    "triggerName": "artifact_trigger",
                    "artifactTrigger": {
                        "triggerArtifactReferences": [
                            {"namespace": {"namespacePath": "a"}},
                            {"namespace": {"namespacePath": "b"}}
                        ]}
                }
            ]
        },
        r_schemas.DynamicTriggerListSchema()
    ),
    (
        make_nirvana_reaction_with_builder_exp_retries().parameters.root_node.nodes["retryPolicy"],
        {
            "node": {
                "nodes": {
                    "retryNumber": {
                        "value": {
                            "genericValue": {
                                "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                                "value": "3"
                            }
                        }
                    },
                    "exponentialRetry": {
                        "node": {
                            "nodes": {
                                "totalTime": {
                                    "value": {
                                        "genericValue": {
                                            "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                                            "value": "100"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        r_schemas.ParametersValueElementSchema()
    ),
    (
        make_nirvana_reaction_with_builder_random_retries().parameters.root_node.nodes["retryPolicy"],
        {
            "node": {
                "nodes": {
                    "retryNumber": {
                        "value": {
                            "genericValue": {
                                "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                                "value": "3"
                            }
                        }
                    },
                    "randomRetry": {
                        "node": {
                            "nodes": {
                                "delayVariance": {
                                    "value": {
                                        "genericValue": {
                                            "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                                            "value": "10000"
                                        }
                                    }
                                },
                                "minDelay": {
                                    "value": {
                                        "genericValue": {
                                            "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                                            "value": "300000"
                                        }
                                    }
                                }
                            }
                        }
                    },
                    "resultCloningPolicy": {
                        "value": {
                            "genericValue": {
                                "@type": "/yandex.reactor.artifact.StringArtifactValueProto",
                                "value": "deep"
                            }
                        }
                    }
                }
            }
        },
        r_schemas.ParametersValueElementSchema()
    ),
    (
        r_objs.DeltaPattern(
            artifact_reference=r_objs.ArtifactReference(
                namespace_identifier=r_objs.NamespaceIdentifier(namespace_path="test")),
            deltas_dict={
                "key_1": 43,
                "key_2": 42
            }
        ),
        {
            "artifactRef": {
                "namespace": {
                    "namespacePath": "test"
                }
            },
            "alias2deltasMs": {
                "key_1": "43",
                "key_2": "42"
            }
        },
        r_schemas.DeltaPatternSchema()
    ),
    (
        r_objs.Triggers(delta_pattern_trigger=r_objs.DeltaPatternTrigger([
            r_objs.DeltaPattern(
                artifact_reference=r_objs.ArtifactReference(
                    namespace_identifier=r_objs.NamespaceIdentifier(namespace_path="test")),
                deltas_dict={
                    "key_1": 43,
                    "key_2": 42
                }
            )])),
        {
            "deltaPatternTrigger": {
                "userTimestampDeltaPattern": [{
                    "artifactRef": {
                        "namespace": {
                            "namespacePath": "test"
                        }
                    },
                    "alias2deltasMs": {
                        "key_1": "43",
                        "key_2": "42"
                    }
                }]
            }
        },
        r_schemas.TriggersSchema()
    ),
    (
        r_objs.InputsInstance(r_objs.InputsInstanceNode({
            "blockParameters": r_objs.InputsInstanceElement(node=r_objs.InputsInstanceNode()),
            "globalParameters": r_objs.InputsInstanceElement(node=r_objs.InputsInstanceNode(
                {
                    "oauth_token": r_objs.InputsInstanceElement(value=r_objs.InputsInstanceValue(
                        const_value=r_objs.InputsInstanceConstHolder(
                        )
                    )),
                    "user_time": r_objs.InputsInstanceElement(value=r_objs.InputsInstanceValue(
                        expression=r_objs.InputsInstanceExpression(
                            input_refs=[
                                r_objs.ArtifactInstanceReference(
                                    artifact_id=23810,
                                    artifact_instance_id=46890644,
                                    artifact_type_id=138,
                                    namespace_id=218597
                                )
                            ],
                            output_ref=r_objs.ArtifactInstanceReference(
                                artifact_id=45149,
                                artifact_instance_id=46917640,
                                artifact_type_id=4,
                                namespace_id=250971)
                        )
                    )),
                }
            )
            )
        })
        ),
        {
            'rootNode': {
                'nodes': {
                    'blockParameters': {'node': {'nodes': {}}},
                    'globalParameters': {
                        'node': {
                            'nodes': {
                                'oauth_token': {
                                    'value': {
                                        'constHolder': {}
                                    }
                                },
                                'user_time': {
                                    'value': {'expression': {'inputRefs': [{'artifactId': '23810',
                                                                            'artifactInstanceId': '46890644',
                                                                            'artifactTypeId': '138',
                                                                            'namespaceId': '218597'}],
                                                             'outputRef': {'artifactId': '45149',
                                                                           'artifactInstanceId': '46917640',
                                                                           'artifactTypeId': '4',
                                                                           'namespaceId': '250971'}}}
                                }
                            }
                        }
                    }
                }
            }
        },
        r_schemas.InputsInstanceSchema()
    ),
    (
        r_objs.OutputsInstance(root_node=r_objs.OutputsInstanceNode(nodes={
            '0': r_objs.OutputsInstanceElement(
                value=r_objs.OutputsInstanceValue(
                    expression=r_objs.OutputsInstanceExpression(
                        input_ref=r_objs.ArtifactInstanceReference(
                            artifact_id=23850,
                            artifact_instance_id=46952328,
                            artifact_type_id=2,
                            namespace_id=218694
                        ),
                        output_ref=r_objs.ArtifactInstanceReference(
                            artifact_id=23840,
                            artifact_instance_id=46952329,
                            artifact_type_id=37,
                            namespace_id=218672
                        )
                    )
                )
            )
        }), version=1),
        {
            'rootNode': {
                'nodes': {
                    '0': {
                        'value': {
                            'expression': {
                                'inputRef': {
                                    'artifactId': '23850',
                                    'artifactInstanceId': '46952328',
                                    'artifactTypeId': '2',
                                    'namespaceId': '218694'
                                },
                                'outputRef': {
                                    'artifactId': '23840',
                                    'artifactInstanceId': '46952329',
                                    'artifactTypeId': '37',
                                    'namespaceId': '218672'}
                            }
                        }
                    }
                }
            },
            'version': 1
        },
        r_schemas.OutputsInstanceSchema()
    ),
    (
        r_objs.InstantiationContext(
            cron_schedule_time="",
            globals={
                'table': r_objs.ReactorEntity(entity_id=37, entity_type=0,
                                              value="CgRoYWhuEiwvL2xvZ3MvYnMtY2hldmVudC1sb2cvMWgvMjAxOC0xMS0xM1QxOTowMDowMA=="),
                'tableUserTime': r_objs.ReactorEntity(entity_id=2, entity_type=2,
                                                      value="ChMyMDE4LTExLTEzVDE5OjAwOjAw")
            },
            triggered_by_refs=[
                r_objs.ArtifactInstanceReference(
                    artifact_id=3581,
                    artifact_instance_id=46952300,
                    artifact_type_id=37,
                    namespace_id=8917
                )
            ]
        ),
        {
            'cronScheduledTime': '',
            'globals':
                {
                    'table': {
                        'entityId': '37',
                        'entityTypeId': '0',
                        'value': 'CgRoYWhuEiwvL2xvZ3MvYnMtY2hldmVudC1sb2cvMWgvMjAxOC0xMS0xM1QxOTowMDowMA=='
                    },
                    'tableUserTime': {
                        'entityId': '2',
                        'entityTypeId': '2',
                        'value': 'ChMyMDE4LTExLTEzVDE5OjAwOjAw'
                    }
                },
            'triggeredByRefs':
                [
                    {
                        'artifactId': '3581',
                        'artifactInstanceId': '46952300',
                        'artifactTypeId': '37',
                        'namespaceId': '8917'
                    }
                ]
        },

        r_schemas.InstantiationContextSchema()
    ),
    (
        r_objs.OperationInstance(
            creation_time=datetime.datetime(2018, 11, 14, 1, 59, 0, tzinfo=pytz.UTC),
            creator_id=2,
            description="",
            id_=1345871,
            inputs=r_objs.InputsInstance(root_node=r_objs.InputsInstanceNode(), version=1),
            instantiation_context=r_objs.InstantiationContext(),
            operation_id=6582,
            outputs=r_objs.OutputsInstance(root_node=r_objs.OutputsInstanceNode(), version=1),
            progress_log="",
            progress_msg="Completed",
            progress_rate=1.0,
            source=r_objs.ReactionTriggerType.ARTIFACT_TRIGGER,
            state="",
            status=r_objs.ReactionInstanceStatus.COMPLETED
        ),
        {
            'creationTimestamp': '2018-11-14T01:59:00+00:00',
            'creatorId': '2',
            'description': '',
            'id': '1345871',
            'inputsInstances': {'rootNode': {'nodes': {}}, 'version': 1},
            'instantiationContext': {'cronScheduledTime': ''},
            'operationId': '6582',
            'outputsInstances': {'rootNode': {'nodes': {}},
                                 'version': 1},
            'progressLog': u'',
            'progressMessage': 'Completed',
            'progressRate': 1.0,
            'source': 'ARTIFACT_TRIGGER',
            'state': '',
            'status': 'COMPLETED'
        },
        r_schemas.OperationInstanceSchema()
    ),
    (
        r_objs.ArtifactInstanceFilterDescriptor(
            artifact_identifier=r_objs.ArtifactIdentifier(
                namespace_identifier=r_objs.NamespaceIdentifier(
                    namespace_path="test"
                )
            ),
            user_timestamp_filter=r_objs.TimestampFilter(time_range=r_objs.TimestampRange(
                dt_from=datetime.datetime(2017, 1, 1, 1, 1, tzinfo=pytz.utc),
                dt_to=datetime.datetime(2017, 1, 1, 1, 2, tzinfo=pytz.utc)
            )),
            limit=30,
            offset=60,
            order_by=r_objs.ArtifactInstanceOrderBy.USER_TIMESTAMP
        ),
        {
            "artifactIdentifier": {
                "namespaceIdentifier": {
                    "namespacePath": "test"
                }
            },
            "userTimestampFilter": {
                "timestampRange": {
                    "from": "2017-01-01T01:01:00+00:00",
                    "to": "2017-01-01T01:02:00+00:00"
                }
            },
            "orderBy": "USER_TIMESTAMP",
            "limit": 30,
            "offset": 60
        },
        r_schemas.ArtifactInstanceFilterDescriptorSchema()
    ),
    (
        r_objs.QueueConfiguration(
            parallelism=42,
            priority_function=r_objs.QueuePriorityFunction.USER_TIME_ELDEST_FIRST,
            max_queued_instances=r_objs.QueueMaxQueuedInstances(1000),
            max_running_instances_per_reaction=r_objs.QueueMaxRunningInstancesPerReaction(21),
            max_queued_instances_per_reaction=r_objs.QueueMaxQueuedInstancesPerReaction(500)
        ),
        {
            "maxRunningInstances": "42",
            "priorityFunction": "USER_TIME_ELDEST_FIRST",
            "maxQueuedInstances": {"value": "1000"},
            "maxRunningInstancesPerReaction": {"value": "21"},
            "maxQueuedInstancesPerReaction": {"value": "500"}
        },
        r_schemas.QueueConfigurationSchema()
    ),
    (
        r_objs.QueueIdentifier(queue_id=42),
        {
            "queueId": "42"
        },
        r_schemas.QueueIdentifierSchema()
    ),
    (
        r_objs.QueueIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_id=42)),
        {
            "namespaceIdentifier": {
                "namespaceId": "42"
            }
        },
        r_schemas.QueueIdentifierSchema()
    ),
    (
        r_objs.QueueOperationConfiguration(),
        {},
        r_schemas.QueueOperationConfigurationSchema()
    ),
    (
        r_objs.QueueOperation(reaction_id=42),
        {
            "reactionId": "42",
            "configuration": {
            }
        },
        r_schemas.QueueOperationSchema()
    ),
    (
        r_objs.Queue(
            id_=42,
            namespace_id=42,
            configuration=r_objs.QueueConfiguration(
                parallelism=42,
                priority_function=r_objs.QueuePriorityFunction.USER_TIME_ELDEST_FIRST,
                max_queued_instances=r_objs.QueueMaxQueuedInstances(1000),
                max_running_instances_per_reaction=r_objs.QueueMaxRunningInstancesPerReaction(21),
                max_queued_instances_per_reaction=r_objs.QueueMaxQueuedInstancesPerReaction(500)
            ),
            project_id=42,
        ),
        {
            "id": "42",
            "namespaceId": "42",
            "configuration": {
                "maxRunningInstances": "42",
                "priorityFunction": "USER_TIME_ELDEST_FIRST",
                "maxQueuedInstances": {"value": "1000"},
                "maxRunningInstancesPerReaction": {"value": "21"},
                "maxQueuedInstancesPerReaction": {"value": "500"}
            },
            "projectId": "42"

        },
        r_schemas.QueueSchema()
    ),
    (
        r_objs.LongRunningOperationInstanceNotificationOptions(
            warn_percentile=70,
            warn_runs_count=20,
            warn_scale=1.5,
            crit_percentile=85,
            crit_runs_count=30,
            crit_scale=2.5
        ),
        {
            "warnPercentile": 70,
            "warnRunsCount": 20,
            "warnScale": 1.5,
            "critPercentile": 85,
            "critRunsCount": 30,
            "critScale": 2.5
        },
        r_schemas.LongRunningOperationInstanceNotificationOptionsSchema()
    ),
    (
        r_objs.Triggers(dynamic_trigger=r_objs.DynamicTriggerPlaceholder()),
        {
            "dynamicTrigger": {}
        },
        r_schemas.TriggersSchema()
    ),
    (
        r_objs.OperationGlobals(global_variables={}),
        {
            "globalVariables": {}
        },
        r_schemas.OperationGlobalsSchema()
    ),
    (
        r_objs.OperationGlobals(expression=r_objs.Expression("global a = true;")),
        {
            "expression": {
                "expression": "global a = true;"
            }
        },
        r_schemas.OperationGlobalsSchema()
    ),
    (
        r_objs.ExpressionGlobalVariable(
            origin=r_objs.ExpressionGlobalVariableOrigin.TRIGGER,
            identifier="varName",
            type=r_objs.ReactorEntityType(entity_type_id=0, entity_id=37),
            usage=[r_objs.ExpressionGlobalVariableUsage.AFTER_SUCCESS_EXPRESSION]
        ),
        {
            "origin": "TRIGGER",
            "identifier": "varName",
            "type": {
                "entityTypeId": "0",
                "entityId": "37"
            },
            "usage": ["AFTER_SUCCESS_EXPRESSION"]
        },
        r_schemas.ExpressionGlobalVariableSchema()
    ),
    (
        r_objs.DynamicTriggerList(triggers=[
            r_objs.DynamicTrigger(
                trigger_name="triggerName",
                expression=r_objs.Expression("global a = true;"),
                cron_trigger=r_objs.CronTrigger(cron_expression="0 0 0 * * ?"),
                replacement=r_objs.DynamicTriggerReplacementRequest(42),
            ),
            r_objs.DynamicTrigger(
                trigger_name="dynamic_artifact_trigger",
                expression=r_objs.Expression("global b = 42;"),
                artifact_trigger=r_objs.DynamicArtifactTrigger(
                    triggers=[r_objs.ArtifactReference(
                        namespace_identifier=r_objs.NamespaceIdentifier(
                            namespace_path="b"
                        )
                    )]
                )
            )
        ]),
        {
            "triggers": [
                {
                    "triggerName": "triggerName",
                    "expression": {
                        "expression": "global a = true;"
                    },
                    "replacementRequest": {
                        "sourceTriggerId": "42"
                    },
                    "cronTrigger": {
                        "cronExpression": "0 0 0 * * ?"
                    }
                },
                {
                    "triggerName": "dynamic_artifact_trigger",
                    "expression": {
                        "expression": "global b = 42;"
                    },
                    "artifactTrigger": {
                        "triggerArtifactReferences": [{
                            "namespace": {
                                "namespacePath": "b"
                            }
                        }]
                    }
                }
            ]
        },
        r_schemas.DynamicTriggerListSchema()
    ),
    (
        r_objs.MetricCreateRequest(
            r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
            artifact_id=42,
            custom_tags=r_objs.Attributes({"tag1": "value1"})
        ),
        {
            "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
            "artifactId": "42",
            "customTags": {
                "keyValue": {
                    "tag1": "value1"
                }
            }
        },
        r_schemas.MetricCreateRequestSchema()
    ),
    (
        r_objs.MetricListRequest(queue_id=42),
        {"queueId": "42"},
        r_schemas.MetricListRequestSchema()
    ),
    (
        r_objs.MetricUpdateRequest(
            r_objs.MetricType.REACTION_USERTIME_PROCESSING_DURATION,
            reaction_id=42,
            new_tags=r_objs.Attributes({"tag2": "value2"})
        ),
        {
            "metricType": "REACTION_USERTIME_PROCESSING_DURATION",
            "reactionId": "42",
            "newTags": {
                "keyValue": {
                    "tag2": "value2"
                }
            }
        },
        r_schemas.MetricUpdateRequestSchema()
    ),
    (
        r_objs.MetricDeleteRequest(r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY, artifact_id=42),
        {"metricType": "ARTIFACT_USERTIME_FIRST_DELAY", "artifactId": "42"},
        r_schemas.MetricDeleteRequestSchema()
    ),
    (
        r_objs.MetricReference(
            r_objs.MetricType.REACTION_USERTIME_PROCESSING_DURATION,
            [r_objs.Attributes({"tag1": "value1"}), r_objs.Attributes({"tag1": "value2"}), r_objs.Attributes({"tag2": "value3"})],
            reaction_id=42,
        ),
        {
            "metricType": "REACTION_USERTIME_PROCESSING_DURATION",
            "reactionId": "42",
            "tags": [
                {"keyValue": {"tag1": "value1"}},
                {"keyValue": {"tag1": "value2"}},
                {"keyValue": {"tag2": "value3"}}
            ]
        },
        r_schemas.MetricReferenceSchema()
    ),
    (
        r_objs.DynamicTriggerDescriptor(
            id_=42, reaction_id=42, type_=r_objs.TriggerType.ARTIFACT,
            status=r_objs.TriggerStatus.DELETED,
            name="Daily_trigger",
            creation_time=datetime.datetime(2020, 10, 26, 1, 21, 0, 123000, tzinfo=pytz.UTC),
            data=r_objs.DynamicTrigger(
                trigger_name="triggerName",
                expression=r_objs.Expression("global a = 3.14"),
                replacement=r_objs.DynamicTriggerReplacementRequest(42),
                artifact_trigger=r_objs.DynamicArtifactTrigger(
                    triggers=[r_objs.ArtifactReference(
                        artifact_id=42, namespace_id=42
                    )]
                )
            )
        ),
        {
            "id": "42",
            "reactionId": "42",
            "type": "ARTIFACT",
            "status": "DELETED",
            "name": "Daily_trigger",
            "creationTimestamp": "2020-10-26T01:21:00.123000+00:00",
            "data": {
                "triggerName": "triggerName",
                "expression": {
                    "expression": "global a = 3.14"
                },
                "replacementRequest": {
                    "sourceTriggerId": "42"
                },
                "artifactTrigger": {
                    "triggerArtifactReferences": [{
                        "artifactId": "42",
                        "namespaceId": "42",
                    }]
                }
            }
        },
        r_schemas.DynamicTriggerDescriptorSchema()
    ),
    (
        r_objs.CleanupStrategyDescriptor(
            cleanup_strategies=[r_objs.CleanupStrategy(
                ttl_cleanup_strategy=r_objs.TtlCleanupStrategy(
                    ttl_days=42
                )
            )]
        ),
        {
            "cleanupStrategies": [{
                "ttlCleanupStrategy": {
                    "ttlDays": "42"
                }
            }]
        },
        r_schemas.CleanupStrategyDescriptorSchema()
    )
]


def test_one_of_schema_dumps_as_expected():
    if six.PY2:
        dump = MockSchema().dump(MockObj(field_a=1)).data
    else:
        dump = MockSchema().dump(MockObj(field_a=1))
    assert dump == {"field_a": 1}
    assert not MockSchema().validate({"field_a": 1})


@pytest.mark.parametrize("obj, expected_dump, schema", OBJS_DUMPS_AND_SCHEMAS)
def test_to_json_produce_expected_out(obj, expected_dump, schema):
    observed_dump = r_schemas.to_json(obj, schema)
    if observed_dump != expected_dump:
        logger.warn("Observed object:")
        logger.warn(json.dumps(observed_dump, indent=2))
        logger.warn("Expected object:")
        logger.warn(json.dumps(expected_dump, indent=2))
        pytest.fail()


@pytest.mark.parametrize("expected_obj, dump, schema", OBJS_DUMPS_AND_SCHEMAS)
def test_from_json_produce_expected_obj(expected_obj, dump, schema):
    observed_obj = r_schemas.from_json(dump, schema)
    if observed_obj != expected_obj:
        logger.warn("Observed object:")
        logger.warn(observed_obj)
        logger.warn("Expected object:")
        logger.warn(expected_obj)
        pytest.fail()


BAD_OBJS_AND_SCHEMAS = [
    (MockObj(field_a=1, field_b=2), MockSchema()),
    (r_objs.ArtifactTypeIdentifier(artifact_type_id="test"), r_schemas.ArtifactTypeIdentifierSchema()),
    (r_objs.ArtifactTypeIdentifier(artifact_type_id=1, artifact_type_key="YT_PATH"), r_schemas.ArtifactTypeIdentifierSchema()),
    (r_objs.MetricCreateRequest(r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY, artifact_id=1, reaction_id=1), r_schemas.MetricCreateRequestSchema()),
    (r_objs.MetricListRequest(artifact_id=1, reaction_id=1), r_schemas.MetricListRequestSchema()),
    (r_objs.MetricUpdateRequest(r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY, artifact_id=1, reaction_id=1), r_schemas.MetricUpdateRequestSchema()),
    (r_objs.MetricDeleteRequest(r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY, artifact_id=1, reaction_id=1), r_schemas.MetricDeleteRequestSchema()),
    (r_objs.MetricReference(r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY, [], artifact_id=1, reaction_id=1), r_schemas.MetricReferenceSchema()),
]


@pytest.mark.parametrize("bad_obj, schema", BAD_OBJS_AND_SCHEMAS)
def test_error_on_dump_with_one_of_violation(bad_obj, schema):
    with pytest.raises(r_schemas.ReactorObjectValidationError):
        r_schemas.to_json(bad_obj, schema)


SCHEMAS_AND_BAD_DUMPS = [
    (MockSchema(), {"field_a": 1, "field_b": 2}),
    (r_schemas.ArtifactIdentifierSchema(), {"namespaceIdentifier": {"namespacePath": "/a/b"}, "artifactId": "1231"}),
    (r_schemas.ArtifactIdentifierSchema(), {"artifactId": "1231what"})
]


@pytest.mark.parametrize("schema, bad_dump", SCHEMAS_AND_BAD_DUMPS)
def test_error_on_load_with_one_of_violation(schema, bad_dump):
    with pytest.raises(r_schemas.ReactorObjectValidationError):
        r_schemas.from_json(bad_dump, schema)


def test_unknown_enum_item_parsed_to_special_class():
    bad_status = "MAYBE"
    obj = r_schemas.from_json(
        {
            "id": "42",
            "key": "YT_PATH",
            "name": "Quick Silver",
            "description": "Some description",
            "status": bad_status
        },
        r_schemas.ArtifactTypeSchema()
    )
    assert obj.status == r_objs.UnknownEnumValue(name=bad_status)


def test_parsing_with_unknown_fields_not_fails():
    r_schemas.from_json(MOCK_REQUESTS["test_python3_op_parsing"], r_schemas.OperationSchema())
