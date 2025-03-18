from __future__ import print_function

from datetime import datetime
from dateutil.tz import tzutc
import json
import pytz
import pickle
import six

import yatest

import pytest
import requests_mock
import requests

from reactor_client.reactor_api import ReactorAPIClientV1, RetryPolicy
from reactor_client.client_base import ReactorAPIException, ReactorAPITimeout, ReactorInternalError

import reactor_client.reactor_objects as r_objects

from library.python.reactor.client.tests.helpers import (
    make_test_cron_reaction,
    make_test_operation
)

TEST_URL = "http://test-url"
TEST_TOKEN = "test-token"


def get_mock_requests(path):
    with open(path, 'rb') as f:
        return json.load(f)


MOCK_REQUESTS = get_mock_requests(yatest.common.source_path('library/python/reactor/client/tests/resourses/test_requests.json'))

TEST_ARTIFACT_GET_RESPONSE = {
    "artifact": {
        "id": "42",
        "artifactTypeId": "42",
        "namespaceId": "42",
        "projectId": "42"
    }
}

TEST_ARTIFACT_TYPE_GET_RESPONSE = {
    "artifactType": {
        "id": "42",
        "key": "YT_PATH",
        "name": "Quick Silver",
        "description": "Some description",
        "status": "DEPRECATED"
    }
}

TEST_ARTIFACT_CREATE_RESPONSE = {
    "artifactId": "42",
    "namespaceId": "42"
}

TEST_ARTIFACT_INSTANCE_INSTANTIATE_RESPONSE = {
    "artifactInstanceId": "42",
    "creationTimestamp": "1985-10-26T01:21:00"
}

TEST_ARTIFACT_INSTANCE_RESPONSE = {
    "result": {
        "id": "42",
        "artifactId": "43",
        "creatorLogin": "logan",
        "metadata": {
            "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
            "value": "42"
        },
        "attributes": {
            "keyValue": {
                "key_1": "key_1",
                "key_2": "key_2"
            }
        },
        "userTimestamp": "1985-10-26T01:21:00.123",
        "creationTimestamp": "1985-10-26T01:22:00.123",
        "status": "ACTIVE",
        "source": "FROM_CODE"
    }
}

TEST_REACTION_CREATE_RESPONSE = {
    "reactionId": "42",
    "namespaceId": "42"
}

TEST_GREET_RESPONSE = {
    "id": 310,
    "login": "shoutpva",
    "message": "Greetings, my friend."
}

TEST_NAMESPACE_GET_RESPONSE = {
    "namespace": {
        "id": "42",
        "parentId": "42",
        "type": "LEAF_SLICE",
        "name": "answer",
        "description": "Answer to the Ultimate Question of Life, the Universe, and Everything"
    }
}

TEST_QUEUE_CREATE_RESPONSE = {
    "queueId": "42",
    "namespaceId": "42"
}

TEST_QUEUE_GET_RESPONSE = {
    "queue": {
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
    "reactions": [{
        "reactionId": "42",
        "configuration": {
        }
    }]
}

TEST_DYNAMIC_TRIGGER_LIST = {
    "triggers": [{
        "id": "42",
        "reactionId": "42",
        "type": "ARTIFACT",
        "status": "DELETED",
        "name": "Daily_trigger",
        "creationTimestamp": "2020-10-26T01:21:00.123",
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
    }]
}


@pytest.fixture
def client():
    return ReactorAPIClientV1(TEST_URL, TEST_TOKEN)


def test_token_in_headers(client):
    with requests_mock.mock() as m:
        def check_token_in_headers(request, context):
            assert request.headers["Authorization"] == "OAuth {}".format(TEST_TOKEN)

        m.post(TEST_URL + "/api/v1/a/get", text=check_token_in_headers)
        client.requester.send_request("a/get")


class APIInTestCase(object):
    def __init__(self, method, args, endpoint_path, response_json, expected_json):
        self.method = method
        self.args = args
        self.endpoint_path = endpoint_path
        self.response_json = response_json
        self.expected_json = expected_json


@pytest.fixture
def api_in_test_cases(client):
    """
    :type client: ReactorAPIClientV1
    """
    return [
        APIInTestCase(
            client.artifact.get,
            dict(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")),
            endpoint_path="/api/v1/a/get",
            response_json=TEST_ARTIFACT_GET_RESPONSE,
            expected_json={
                "artifactIdentifier": {
                    "namespaceIdentifier": {
                        "namespacePath": "test"
                    }
                }
            }
        ),
        APIInTestCase(
            client.artifact_type.get,
            dict(artifact_type_key="YT_PATH"),
            endpoint_path="/api/v1/a/t/get",
            response_json=TEST_ARTIFACT_TYPE_GET_RESPONSE,
            expected_json={
                "artifactTypeIdentifier": {
                    "artifactTypeKey": "YT_PATH"
                }
            }
        ),
        APIInTestCase(
            client.artifact_type.get,
            dict(artifact_type_id=11),
            endpoint_path="/api/v1/a/t/get",
            response_json=TEST_ARTIFACT_TYPE_GET_RESPONSE,
            expected_json={
                "artifactTypeIdentifier": {
                    "artifactTypeId": "11"
                }
            }
        ),
        APIInTestCase(
            client.artifact_trigger.insert,
            dict(reaction_identifier=r_objects.OperationIdentifier(operation_id=42), artifact_instance_ids=[43, 42]),
            endpoint_path="/api/v1/a/t/insert",
            response_json={},
            expected_json={
                "reactionIdentifier": {
                    "operationId": "42"
                },
                "artifactInstanceIds": ["43", "42"]
            }
        ),
        APIInTestCase(
            client.artifact.create,
            dict(
                artifact_type_identifier=r_objects.ArtifactTypeIdentifier(artifact_type_key="YT_PATH"),
                artifact_identifier=r_objects.ArtifactIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/a/b")
                ),
                description="User sessions hourly log",
                permissions=r_objects.NamespacePermissions(roles={
                    "login_1": r_objects.NamespaceRole.READER,
                    "login_2": r_objects.NamespaceRole.WRITER
                }),
                cleanup_strategy=r_objects.CleanupStrategyDescriptor(
                    cleanup_strategies=[r_objects.CleanupStrategy(
                        ttl_cleanup_strategy=r_objects.TtlCleanupStrategy(42)
                    )]
                ),
                project_identifier=r_objects.ProjectIdentifier(project_id=42),
                create_parent_namespaces=True,
                create_if_not_exist=True
            ),
            endpoint_path="/api/v1/a/create",
            response_json=TEST_ARTIFACT_CREATE_RESPONSE,
            expected_json={
                "artifactTypeIdentifier": {
                    "artifactTypeKey": "YT_PATH"
                },
                "artifactIdentifier": {
                    "namespaceIdentifier": {
                        "namespacePath": "/a/b"
                    }
                },
                "description": "User sessions hourly log",
                "permissions": {
                    "roles": {
                        "login_1": "READER",
                        "login_2": "WRITER"
                    },
                    "version": 0
                },
                "cleanupStrategy": {
                    "cleanupStrategies": [{
                        "ttlCleanupStrategy": {
                            "ttlDays": "42"
                        }
                    }]
                },
                "projectIdentifier": {
                    "projectId": "42"
                },
                "createParentNamespaces": True,
                "createIfNotExist": True
            }
        ),
        APIInTestCase(
            client.artifact_instance.instantiate,
            dict(
                artifact_identifier=r_objects.ArtifactIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/a/b")),
                metadata=r_objects.Metadata(type_="/yandex.reactor.artifact.IntArtifactValueProto",
                                            dict_obj={"value": "42"}),
                attributes=r_objects.Attributes(key_value={"key_1": "key_1", "key_2": "key_2"}),
                user_time=datetime(1985, 10, 26, 1, 21),
                create_if_not_exist=True,
            ),

            endpoint_path="/api/v1/a/i/instantiate",
            response_json=TEST_ARTIFACT_INSTANCE_INSTANTIATE_RESPONSE,
            expected_json={
                "artifactIdentifier": {
                    "namespaceIdentifier": {
                        "namespacePath": "/a/b"
                    }
                },
                "metadata": {
                    "@type": "/yandex.reactor.artifact.IntArtifactValueProto",
                    "value": "42"
                },
                "attributes": {
                    "keyValue": {
                        "key_1": "key_1",
                        "key_2": "key_2"
                    }
                },
                "userTimestamp": "1985-10-26T01:21:00+00:00" if six.PY2 else "1985-10-26T01:21:00",
                "createIfNotExist": True,
            }
        ),
        APIInTestCase(
            client.reaction.create,
            dict(
                operation_descriptor=make_test_cron_reaction(),
                create_if_not_exist=True
            ),

            endpoint_path="/api/v1/r/create",
            response_json=TEST_REACTION_CREATE_RESPONSE,
            expected_json=MOCK_REQUESTS["cron_reaction"]
        ),
        APIInTestCase(
            client.namespace_notification.list,
            dict(
                namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/antifraud/arnold/host_cm/reactions/1d")
            ),

            endpoint_path="/api/v1/n/notification/list",
            response_json={"notifications": []},
            expected_json={
                "namespace": {
                    "namespacePath": "/antifraud/arnold/host_cm/reactions/1d"
                }
            }
        ),
        APIInTestCase(
            client.namespace.get,
            dict(
                namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/test")
            ),

            endpoint_path="/api/v1/n/get",
            response_json=TEST_NAMESPACE_GET_RESPONSE,
            expected_json={
                "namespaceIdentifier": {
                    "namespacePath": "/test"
                }
            }
        ),
        APIInTestCase(
            client.namespace.create,
            dict(
                namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/test"),
                description="hourly user sessions processing",
                permissions=r_objects.NamespacePermissions(
                    {
                        "login_1": r_objects.NamespaceRole.READER,
                        "login_2": r_objects.NamespaceRole.WRITER
                    }
                ),
                create_parents=True,
                create_if_not_exist=True
            ),

            endpoint_path="/api/v1/n/create",
            response_json={"namespaceId": "42"},
            expected_json={
                "namespaceIdentifier": {
                    "namespacePath": "/test"
                },
                "description": "hourly user sessions processing",
                "permissions": {
                    "roles": {
                        "login_1": "READER",
                        "login_2": "WRITER"
                    },
                    "version": 0
                },
                "createParents": True,
                "createIfNotExist": True
            }
        ),
        APIInTestCase(
            client.reaction.get,
            dict(operation_identifier=r_objects.OperationIdentifier(operation_id=42)),
            endpoint_path="/api/v1/r/get",
            response_json=MOCK_REQUESTS["reaction_obj_no_descriptors"],
            expected_json={
                "reaction": {
                    "operationId": "42"
                }
            }
        ),
        APIInTestCase(
            client.namespace.delete,
            dict(namespace_identifier_list=[
                r_objects.NamespaceIdentifier(namespace_path="/full/path/to/my/namespace_1"),
                r_objects.NamespaceIdentifier(namespace_path="/full/path/to/my/namespace_2")
            ],
                delete_if_exist=True),
            endpoint_path="/api/v1/n/delete",
            response_json={},
            expected_json={
                "namespaceIdentifier": [{
                    "namespacePath": "/full/path/to/my/namespace_1"
                }, {
                    "namespacePath": "/full/path/to/my/namespace_2"
                }],
                "deleteIfExist": True,
                "forceDelete": False
            }
        ),
        APIInTestCase(
            client.reaction.update,
            dict(status_update_list=[
                r_objects.ReactionStatusUpdate(
                    reaction=r_objects.OperationIdentifier(
                        namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                    ),
                    status_update=r_objects.StatusUpdate.DEPRECATE
                )
            ]),
            endpoint_path="/api/v1/r/update",
            response_json={},
            expected_json={
                "statusUpdates": [{
                    "reaction": {
                        "namespaceIdentifier": {
                            "namespacePath": "test"
                        }
                    },
                    "statusUpdate": "DEPRECATE"
                }]
            }
        ),
        APIInTestCase(
            client.reaction.update,
            dict(reaction_start_configuration_update=[
                r_objects.ReactionStartConfigurationUpdate(
                    reaction_identifier=r_objects.OperationIdentifier(
                        namespace_identifier=r_objects.NamespaceIdentifier(
                            namespace_path="path"
                        )
                    ),
                    waterline_strategies=r_objects.WaterlineStrategies(
                        per_artifact_count_strategy=r_objects.PerArtifactCountStrategy(
                            limit=42
                        )
                    )
                )
            ]),
            endpoint_path="/api/v1/r/update",
            response_json={},
            expected_json={
                "startConfigurationUpdate": [{
                    "reactionIdentifier": {
                        "namespaceIdentifier": {
                            "namespacePath": "path"
                        }
                    },
                    "waterlineStrategies": {
                        "perArtifactCountStrategy": {
                            "limit": "42"
                        },
                    }
                }]
            }
        ),
        APIInTestCase(
            client.namespace_notification.change,
            dict(delete_id_list=[43, 42], notification_descriptor_list=[r_objects.NotificationDescriptor(
                namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test"),
                event_type=r_objects.NotificationEventType.ARTIFACT_INSTANCE_APPEARANCE_DELAY,
                transport=r_objects.NotificationTransportType.EMAIL,
                recipient="etaranov"
            )]),
            endpoint_path="/api/v1/n/notification/change",
            response_json={},
            expected_json={
                "notificationIdsToDelete": ["43", "42"],
                "notificationsToCreate": [{
                    "namespace": {
                        "namespacePath": "test"
                    },
                    "eventType": "ARTIFACT_INSTANCE_APPEARANCE_DELAY",
                    "transport": "EMAIL",
                    "recipient": "etaranov"
                }]
            }
        ),
        APIInTestCase(
            client.reaction_instance.cancel,
            dict(reaction_instance_id=100500),
            endpoint_path="/api/v1/r/i/cancel",
            response_json={},
            expected_json={"reactionInstanceId": "100500"}
        ),
        APIInTestCase(
            client.reaction_instance.get,
            dict(reaction_instance_id=100500),
            endpoint_path="/api/v1/r/i/get",
            response_json={
                "reactionInstance": {
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
                }
            },
            expected_json={"reactionInstanceId": "100500"}
        ),
        APIInTestCase(
            client.artifact_instance.range,
            dict(filter_=r_objects.ArtifactInstanceFilterDescriptor(
                artifact_identifier=r_objects.ArtifactIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="test"
                    )
                ),
                user_timestamp_filter=r_objects.TimestampFilter(time_range=r_objects.TimestampRange(
                    dt_from=datetime(2017, 1, 1, 1, 1, tzinfo=pytz.utc),
                    dt_to=datetime(2017, 1, 1, 1, 2, tzinfo=pytz.utc)
                )),
                limit=30,
                offset=60,
                order_by=r_objects.ArtifactInstanceOrderBy.USER_TIMESTAMP
            )),
            endpoint_path="/api/v1/a/i/get/range",
            response_json={"range": [TEST_ARTIFACT_INSTANCE_RESPONSE["result"]]},
            expected_json={
                "filter": {
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
                }
            }
        ),
        APIInTestCase(
            client.queue.create,
            dict(
                queue_descriptor=r_objects.QueueDescriptor(
                    namespace_descriptor=r_objects.NamespaceDescriptor(
                        namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                    ),
                    configuration=r_objects.QueueConfiguration(
                        parallelism=42,
                        priority_function=r_objects.QueuePriorityFunction.USER_TIME_ELDEST_FIRST,
                        max_queued_instances=r_objects.QueueMaxQueuedInstances(value=1000),
                        max_running_instances_per_reaction=r_objects.QueueMaxRunningInstancesPerReaction(value=21),
                        max_queued_instances_per_reaction=r_objects.QueueMaxQueuedInstancesPerReaction(value=500)
                    ),
                    project_identifier=r_objects.ProjectIdentifier(project_id=42),
                ),
                create_if_not_exist=True
            ),
            endpoint_path="/api/v1/q/create",
            response_json=TEST_QUEUE_CREATE_RESPONSE,
            expected_json={
                "queueDescriptor": {
                    "namespaceDescriptor": {
                        "namespaceIdentifier": {
                            "namespacePath": "test"
                        }
                    },
                    "queueConfig": {
                        "maxRunningInstances": "42",
                        "priorityFunction": "USER_TIME_ELDEST_FIRST",
                        "maxQueuedInstances": {"value": "1000"},
                        "maxRunningInstancesPerReaction": {"value": "21"},
                        "maxQueuedInstancesPerReaction": {"value": "500"}
                    },
                    "projectIdentifier": {"projectId": "42"}
                },
                "createIfNotExist": True
            }
        ),
        APIInTestCase(
            client.queue.create,
            dict(
                queue_descriptor=r_objects.QueueDescriptor(
                    namespace_descriptor=r_objects.NamespaceDescriptor(
                        namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                    ),
                    configuration=r_objects.QueueConfiguration(
                        parallelism=42,
                        priority_function=r_objects.QueuePriorityFunction.USER_TIME_ELDEST_FIRST,
                        max_queued_instances=r_objects.QueueMaxQueuedInstances(value=1000),
                        max_running_instances_per_reaction=r_objects.QueueMaxRunningInstancesPerReaction(value=21),
                        max_queued_instances_per_reaction=r_objects.QueueMaxQueuedInstancesPerReaction(value=500),
                        constraints=r_objects.Constraints(
                            unique_priorities=True,
                            timeout_constraint_violation_policy=r_objects.TimeoutConstraintViolationPolicy(),
                        )
                    ),
                    project_identifier=r_objects.ProjectIdentifier(project_id=42),
                ),
                create_if_not_exist=True
            ),
            endpoint_path="/api/v1/q/create",
            response_json=TEST_QUEUE_CREATE_RESPONSE,
            expected_json={
                "queueDescriptor": {
                    "namespaceDescriptor": {
                        "namespaceIdentifier": {
                            "namespacePath": "test"
                        }
                    },
                    "queueConfig": {
                        "constraints": {
                            "uniquePriorities": True,
                            "uniqueRunningPriorities": False,
                            "uniqueQueuedPriorities": False,
                            "uniquePerReactionRunningPriorities": False,
                            "uniquePerReactionQueuedPriorities": False,
                            "timeoutConstraintViolationPolicy": {}
                        },
                        "maxRunningInstances": "42",
                        "priorityFunction": "USER_TIME_ELDEST_FIRST",
                        "maxQueuedInstances": {"value": "1000"},
                        "maxRunningInstancesPerReaction": {"value": "21"},
                        "maxQueuedInstancesPerReaction": {"value": "500"},
                    },
                    "projectIdentifier": {"projectId": "42"}
                },
                "createIfNotExist": True
            }
        ),
        APIInTestCase(
            client.queue.create,
            dict(
                queue_descriptor=r_objects.QueueDescriptor(
                    namespace_descriptor=r_objects.NamespaceDescriptor(
                        namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                    ),
                    configuration=r_objects.QueueConfiguration(
                        parallelism=42,
                        priority_function=r_objects.QueuePriorityFunction.USER_TIME_ELDEST_FIRST,
                        max_queued_instances=r_objects.QueueMaxQueuedInstances(value=1000),
                        max_running_instances_per_reaction=r_objects.QueueMaxRunningInstancesPerReaction(value=21),
                        max_queued_instances_per_reaction=r_objects.QueueMaxQueuedInstancesPerReaction(value=500),
                        constraints=r_objects.Constraints(
                            unique_running_priorities=True,
                            cancel_constraint_violation_policy=r_objects.CancelConstraintViolationPolicy(),
                        )
                    ),
                    project_identifier=r_objects.ProjectIdentifier(project_id=42),
                ),
                create_if_not_exist=True
            ),
            endpoint_path="/api/v1/q/create",
            response_json=TEST_QUEUE_CREATE_RESPONSE,
            expected_json={
                "queueDescriptor": {
                    "namespaceDescriptor": {
                        "namespaceIdentifier": {
                            "namespacePath": "test"
                        }
                    },
                    "queueConfig": {
                        "constraints": {
                            "uniquePriorities": False,
                            "uniqueRunningPriorities": True,
                            "uniqueQueuedPriorities": False,
                            "uniquePerReactionRunningPriorities": False,
                            "uniquePerReactionQueuedPriorities": False,
                            "cancelConstraintViolationPolicySchema": {}
                        },
                        "maxRunningInstances": "42",
                        "priorityFunction": "USER_TIME_ELDEST_FIRST",
                        "maxQueuedInstances": {"value": "1000"},
                        "maxRunningInstancesPerReaction": {"value": "21"},
                        "maxQueuedInstancesPerReaction": {"value": "500"},
                    },
                    "projectIdentifier": {"projectId": "42"}
                },
                "createIfNotExist": True
            }
        ),
        APIInTestCase(
            client.queue.get,
            dict(
                queue_identifier=r_objects.QueueIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                )
            ),
            endpoint_path="/api/v1/q/get",
            response_json=TEST_QUEUE_GET_RESPONSE,
            expected_json={
                "queueIdentifier": {
                    "namespaceIdentifier": {
                        "namespacePath": "test"
                    }
                }
            }
        ),
        APIInTestCase(
            client.queue.update,
            dict(
                queue_identifier=r_objects.QueueIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                ),
                new_queue_capacity=42,
                new_max_instances_in_queue=42,
                new_max_instances_per_reaction_in_queue=42,
                new_max_running_instances_per_reaction=42,
                remove_reactions=[
                    r_objects.OperationIdentifier(operation_id=42),
                    r_objects.OperationIdentifier(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test"))
                ],
                add_reactions=[
                    r_objects.OperationIdentifier(operation_id=42),
                    r_objects.OperationIdentifier(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test"))
                ],
            ),
            endpoint_path="/api/v1/q/update",
            response_json={},
            expected_json={
                "queueIdentifier": {
                    "namespaceIdentifier": {
                        "namespacePath": "test"
                    }
                },
                "newQueueCapacity": "42",
                "newMaxInstancesInQueue": "42",
                "newMaxInstancesPerReactionInQueue": "42",
                "newMaxRunningInstancesPerReaction": "42",
                "removeReactions": [{
                    "operationId": "42"
                }, {
                    "namespaceIdentifier": {
                        "namespacePath": "test"
                    }
                }],
                "addReactions": [{
                    "operationId": "42"
                }, {
                    "namespaceIdentifier": {
                        "namespacePath": "test"
                    }
                }]
            }
        ),
        APIInTestCase(
            client.namespace.move,
            dict(
                source_identifier=r_objects.NamespaceIdentifier(namespace_path="a"),
                destination_identifier=r_objects.NamespaceIdentifier(namespace_path="b")
            ),
            endpoint_path="/api/v1/n/move",
            response_json={},
            expected_json={
                "sourceIdentifier": {
                    "namespacePath": "a"
                },
                "destinationIdentifier": {
                    "namespacePath": "b"
                }
            }
        ),
        APIInTestCase(
            client.namespace_notification.change_long_running,
            dict(
                operation_identifier=r_objects.OperationIdentifier(operation_id=42),
                options=r_objects.LongRunningOperationInstanceNotificationOptions(
                    warn_percentile=70,
                    warn_runs_count=20,
                    warn_scale=1.5,
                    crit_percentile=85,
                    crit_runs_count=30,
                    crit_scale=2.5
                )
            ),
            endpoint_path="/api/v1/r/looong-running/change",
            response_json={},
            expected_json={
                "reaction": {
                    "operationId": "42"
                },
                "newOptions": {
                    "warnPercentile": 70,
                    "warnRunsCount": 20,
                    "warnScale": 1.5,
                    "critPercentile": 85,
                    "critRunsCount": 30,
                    "critScale": 2.5
                }
            }
        ),
        APIInTestCase(
            client.namespace_notification.get_long_running,
            dict(
                operation_identifier=r_objects.OperationIdentifier(operation_id=42)
            ),
            endpoint_path="/api/v1/r/looong-running/get",
            response_json={
                "options": {
                    "warnPercentile": 70,
                    "warnRunsCount": 20,
                    "warnScale": 1.5,
                    "critPercentile": 85,
                    "critRunsCount": 30,
                    "critScale": 2.5
                }
            },
            expected_json={
                "reaction": {
                    "operationId": "42"
                },
            }
        ),
        APIInTestCase(
            client.artifact_instance.get_status_history,
            dict(
                artifact_instance_id=42
            ),
            endpoint_path="/api/v1/a/i/getStatusHistory",
            response_json={
                "artifactInstanceStatuses": [{
                    "status": "REPLACING",
                    "updateTimestamp": "1985-10-26T01:21:00"
                }]
            },
            expected_json={
                "artifactInstanceId": "42"
            }
        ),
        APIInTestCase(
            client.metric.create,
            dict(
                metric_type=r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                custom_tags={"tag1": "value1"},
                queue_id=42
            ),
            endpoint_path="/api/v1/metric/create",
            response_json={
                "metric": {
                    "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
                    "artifactId": "42",
                    "tags": [{"keyValue": {"tag1": "value1"}}]
                }
            },
            expected_json={
                "metric": {
                    "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
                    "customTags": {
                        "keyValue": {
                            "tag1": "value1"
                        }
                    },
                    "queueId": "42"
                }
            }
        ),
        APIInTestCase(
            client.metric.list,
            dict(artifact_id=42),
            endpoint_path="/api/v1/metric/list",
            response_json={
                "metrics": []
            },
            expected_json={"artifactId": "42"}
        ),
        APIInTestCase(
            client.metric.update,
            dict(
                metric_type=r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                artifact_id=42,
                new_tags={"tag2": "value2"}
            ),
            endpoint_path="/api/v1/metric/update",
            response_json={
                "metric": {
                    "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
                    "artifactId": "42",
                    "tags": [{"keyValue": {"tag2": "value2"}}],
                }
            },
            expected_json={
                "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
                "artifactId": "42",
                "newTags": {"keyValue": {"tag2": "value2"}},
            }
        ),
        APIInTestCase(
            client.metric.delete,
            dict(
                metric_type=r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                artifact_id=42
            ),
            endpoint_path="/api/v1/metric/delete",
            response_json={},
            expected_json={
                "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
                "artifactId": "42"
            }
        ),
        APIInTestCase(
            client.dynamic_trigger.add,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                ),
                triggers=r_objects.DynamicTriggerList(triggers=[
                    r_objects.DynamicTrigger(
                        trigger_name="dynamic_cron_trigger",
                        expression=r_objects.Expression("global a = 42;"),
                        cron_trigger=r_objects.CronTrigger(
                            cron_expression="0 0/5 * * * ?",
                            misfire_policy=r_objects.MisfirePolicy.IGNORE
                        )
                    ),
                    r_objects.DynamicTrigger(
                        trigger_name="dynamic_artifact_trigger",
                        expression=r_objects.Expression("global b = 37;"),
                        artifact_trigger=r_objects.DynamicArtifactTrigger(
                            triggers=[r_objects.ArtifactReference(
                                namespace_identifier=r_objects.NamespaceIdentifier(
                                    namespace_path="b"
                                )
                            )]
                        )
                    )
                ])
            ),
            endpoint_path="/api/v1/t/add",
            response_json=TEST_DYNAMIC_TRIGGER_LIST,
            expected_json={
                "reactionIdentifier": {
                    "namespaceIdentifier": {"namespacePath": "a"}
                },
                "triggers": {
                    "triggers": [
                        {
                            "triggerName": "dynamic_cron_trigger",
                            "expression": {
                                "expression": "global a = 42;"
                            },
                            "cronTrigger": {
                                "cronExpression": "0 0/5 * * * ?",
                                "misfirePolicy": "IGNORE"
                            }
                        },
                        {
                            "triggerName": "dynamic_artifact_trigger",
                            "expression": {
                                "expression": "global b = 37;"
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
                }
            }
        ),
        APIInTestCase(
            client.dynamic_trigger.remove,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                ),
                triggers=[
                    r_objects.DynamicTriggerIdentifier(
                        name="dynamic_artifact_trigger"
                    )
                ]
            ),
            endpoint_path="/api/v1/t/remove",
            response_json={},
            expected_json={
                "reactionIdentifier": {
                    "namespaceIdentifier": {"namespacePath": "a"}
                },
                "triggerIdentifier": [{"name": "dynamic_artifact_trigger"}],
            }
        ),
        APIInTestCase(
            client.dynamic_trigger.update,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                ),
                triggers=[
                    r_objects.DynamicTriggerIdentifier(
                        name="dynamic_artifact_trigger"
                    )
                ],
                action=r_objects.DynamicTriggerAction.ACTIVATE
            ),
            endpoint_path="/api/v1/t/update",
            response_json={},
            expected_json={
                "reactionIdentifier": {
                    "namespaceIdentifier": {"namespacePath": "a"}
                },
                "triggerIdentifier": [{"name": "dynamic_artifact_trigger"}],
                "action": "ACTIVATE"
            }
        ),
        APIInTestCase(
            client.dynamic_trigger.list,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                )
            ),
            endpoint_path="/api/v1/t/list",
            response_json=TEST_DYNAMIC_TRIGGER_LIST,
            expected_json={
                "reactionIdentifier": {
                    "namespaceIdentifier": {"namespacePath": "a"}
                }
            }
        ),
        APIInTestCase(
            client.permission.change,
            dict(
                namespace=r_objects.NamespaceIdentifier(namespace_path="a"),
                revoke=["login"],
                grant=r_objects.NamespacePermissions(
                    roles={"login_1": r_objects.NamespaceRole.READER,
                           "login_2": r_objects.NamespaceRole.WRITER}
                )
            ),
            endpoint_path="/api/v1/n/permission/change",
            response_json={},
            expected_json={
                "namespace": {"namespacePath": "a"},
                "loginsToRevokePermissions": ["login"],
                "permissionsToGrant": {
                    "roles": {
                        "login_1": "READER",
                        "login_2": "WRITER"
                    },
                    "version": 0
                }
            }
        ),
        APIInTestCase(
            client.permission.change,
            dict(
                namespace=r_objects.NamespaceIdentifier(namespace_path="a"),
                revoke=["login"],
                grant=r_objects.NamespacePermissions(
                    roles={"login_1": r_objects.NamespaceRole.READER,
                           "login_2": r_objects.NamespaceRole.WRITER},
                    version=1
                ),
                version=1
            ),
            endpoint_path="/api/v1/n/permission/change",
            response_json={},
            expected_json={
                "namespace": {"namespacePath": "a"},
                "rolesToRevokePermissions": ["login"],
                "permissionsToGrant": {
                    "roles": {
                        "login_1": "READER",
                        "login_2": "WRITER"
                    },
                    "version": 1
                }
            }
        ),
        APIInTestCase(
            client.permission.list,
            dict(namespace=r_objects.NamespaceIdentifier(namespace_path="a")),
            endpoint_path="/api/v1/n/permission/list",
            response_json={
                "permissions": {
                    "roles": {
                        "login_1": "READER",
                        "login_2": "WRITER"
                    }
                }
            },
            expected_json={"namespace": {"namespacePath": "a"}, "version": 0}
        ),
        APIInTestCase(
            client.quota.get,
            dict(namespace=r_objects.NamespaceIdentifier(namespace_path="a")),
            endpoint_path="/api/v1/quota/cleanupStrategy/get",
            response_json={
                "cleanupStrategy": {
                    "cleanupStrategies": [
                        {"ttlCleanupStrategy": {"ttlDays": "42"}}
                    ]
                }
            },
            expected_json={"namespaceIdentifier": {"namespacePath": "a"}}
        ),
        APIInTestCase(
            client.quota.update,
            dict(
                namespace=r_objects.NamespaceIdentifier(namespace_path="a"),
                cleanup_strategy=r_objects.CleanupStrategyDescriptor(
                    cleanup_strategies=[r_objects.CleanupStrategy(
                        ttl_cleanup_strategy=r_objects.TtlCleanupStrategy(
                            ttl_days=14
                        )
                    )]
                )
            ),
            endpoint_path="/api/v1/quota/cleanupStrategy/update",
            response_json={},
            expected_json={
                "namespaceIdentifier": {"namespacePath": "a"},
                "cleanupStrategy": {
                    "cleanupStrategies": [{
                        "ttlCleanupStrategy": {
                            "ttlDays": "14"
                        }
                    }]
                }
            }
        ),
        APIInTestCase(
            client.quota.delete,
            dict(namespace=r_objects.NamespaceIdentifier(namespace_path="a")),
            endpoint_path="/api/v1/quota/cleanupStrategy/delete",
            response_json={},
            expected_json={"namespaceIdentifier": {"namespacePath": "a"}}
        )
    ]


def test_api_receives_correct_jsons(api_in_test_cases):
    """
    :type api_in_test_cases: list[APIInTestCase]
    """
    for test_case in api_in_test_cases:
        with requests_mock.mock() as m:
            def check_request_json(request, context):
                try:
                    assert request.json() == test_case.expected_json
                except Exception as e:
                    import sys
                    print(e, file=sys.stderr)
                    raise
                return json.dumps(test_case.response_json)

            m.post(TEST_URL + test_case.endpoint_path, text=check_request_json)
            test_case.method(**test_case.args)


class APIOutTestCase(object):
    def __init__(self, method, args, endpoint_path, response_json, expected_object):
        self.method = method
        self.args = args
        self.endpoint_path = endpoint_path
        self.response_json = response_json
        self.expected_object = expected_object


@pytest.fixture
def api_out_test_cases(client):
    """
    :type client: ReactorAPIClientV1
    """
    return [
        APIOutTestCase(
            client.artifact.get,
            dict(artifact_id=42),
            endpoint_path="/api/v1/a/get",
            response_json=TEST_ARTIFACT_GET_RESPONSE,
            expected_object=r_objects.Artifact(
                artifact_id=42, artifact_type_id=42,
                namespace_id=42, project_id=42
            )
        ),
        APIOutTestCase(
            client.namespace_notification.list,
            dict(
                namespace_identifier=r_objects.NamespaceIdentifier(
                    namespace_path="/antifraud/arnold/host_cm/reactions/1d")
            ),

            endpoint_path="/api/v1/n/notification/list",
            response_json={
                "notifications": [{
                    "notificationId": "42",
                    "namespaceId": "42",
                    "eventType": "ARTIFACT_INSTANCE_APPEARANCE_DELAY",
                    "transport": "EMAIL",
                    "recipient": "etaranov"
                }]
            },
            expected_object=[r_objects.Notification(
                id_=42,
                namespace_id=42,
                event_type=r_objects.NotificationEventType.ARTIFACT_INSTANCE_APPEARANCE_DELAY,
                transport=r_objects.NotificationTransportType.EMAIL,
                recipient="etaranov"
            )]
        ),
        APIOutTestCase(
            client.artifact_type.get,
            dict(artifact_type_id=11),
            endpoint_path="/api/v1/a/t/get",
            response_json=TEST_ARTIFACT_TYPE_GET_RESPONSE,
            expected_object=r_objects.ArtifactType(
                artifact_type_id=42,
                key="YT_PATH",
                name="Quick Silver",
                description="Some description",
                status=r_objects.ArtifactTypeStatus.DEPRECATED
            )
        ),
        APIOutTestCase(
            client.artifact.create,
            dict(
                artifact_type_identifier=r_objects.ArtifactTypeIdentifier(artifact_type_key="YT_PATH"),
                artifact_identifier=r_objects.ArtifactIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/a/b")),
                description="User sessions hourly log",
                permissions=r_objects.NamespacePermissions(roles={
                    "login_1": r_objects.NamespaceRole.READER,
                    "login_2": r_objects.NamespaceRole.WRITER
                }),
                cleanup_strategy=r_objects.CleanupStrategyDescriptor(
                    cleanup_strategies=[r_objects.CleanupStrategy(
                        ttl_cleanup_strategy=r_objects.TtlCleanupStrategy(42)
                    )]
                ),
                project_identifier=r_objects.ProjectIdentifier(project_id=42),
                create_parent_namespaces=True,
                create_if_not_exist=True
            ),
            endpoint_path="/api/v1/a/create",
            response_json=TEST_ARTIFACT_CREATE_RESPONSE,
            expected_object=r_objects.ArtifactCreateResponse(artifact_id=42, namespace_id=42)
        ),
        APIOutTestCase(
            client.artifact_instance.instantiate,
            dict(
                artifact_identifier=r_objects.ArtifactIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/a/b")),
                metadata=r_objects.Metadata(type_="/yandex.reactor.artifact.IntArtifactValueProto",
                                            dict_obj={"value": "42"}),
                attributes=r_objects.Attributes(key_value={"key_1": "key_1", "key_2": "key_2"}),
                user_time=datetime(1985, 10, 26, 1, 21)
            ),
            endpoint_path="/api/v1/a/i/instantiate",
            response_json=TEST_ARTIFACT_INSTANCE_INSTANTIATE_RESPONSE,
            expected_object=r_objects.ArtifactInstanceInstantiateResponse(
                artifact_instance_id=42,
                creation_time=datetime(1985, 10, 26, 1, 21)
            )
        ),
        APIOutTestCase(
            client.artifact_instance.last,
            dict(
                artifact_identifier=r_objects.ArtifactIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/a/b")),
            ),
            endpoint_path="/api/v1/a/i/get/last",
            response_json=TEST_ARTIFACT_INSTANCE_RESPONSE,
            expected_object=r_objects.ArtifactInstance(
                instance_id=42,
                artifact_id=43,
                creator="logan",
                metadata=r_objects.Metadata(
                    type_="/yandex.reactor.artifact.IntArtifactValueProto",
                    dict_obj={"value": "42"}
                ),
                attributes=r_objects.Attributes(
                    key_value={
                        "key_1": "key_1",
                        "key_2": "key_2"
                    }
                ),
                user_time=datetime(1985, 10, 26, 1, 21, 00, 123000),
                creation_time=datetime(1985, 10, 26, 1, 22, 00, 123000),
                status=r_objects.ArtifactInstanceStatus.ACTIVE,
                source=r_objects.ArtifactInstanceSource.FROM_CODE
            )
        ),
        APIOutTestCase(
            client.reaction.create,
            dict(
                operation_descriptor=make_test_cron_reaction(),
                create_if_not_exist=True
            ),

            endpoint_path="/api/v1/r/create",
            response_json=TEST_REACTION_CREATE_RESPONSE,
            expected_object=r_objects.ReactionReference(namespace_id=42, reaction_id=42)
        ),
        APIOutTestCase(
            client.greet,
            dict(),
            endpoint_path="/api/v1/u/greet",
            response_json=TEST_GREET_RESPONSE,
            expected_object=r_objects.GreetResponse(id_=310, login="shoutpva", message="Greetings, my friend.")
        ),
        APIOutTestCase(
            client.namespace.get,
            dict(
                namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/test")
            ),

            endpoint_path="/api/v1/n/get",
            response_json=TEST_NAMESPACE_GET_RESPONSE,
            expected_object=r_objects.Namespace(
                id_=42,
                parent_id=42,
                type_=r_objects.NamespaceType.LEAF_SLICE,
                name="answer",
                description="Answer to the Ultimate Question of Life, the Universe, and Everything"
            )
        ),
        APIOutTestCase(
            client.namespace.create,
            dict(
                namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/test"),
                description="hourly user sessions processing",
                permissions=r_objects.NamespacePermissions(
                    {
                        "login_1": r_objects.NamespaceRole.READER,
                        "login_2": r_objects.NamespaceRole.WRITER
                    }
                ),
                create_parents=True,
                create_if_not_exist=True
            ),

            endpoint_path="/api/v1/n/create",
            response_json={"namespaceId": "42"},
            expected_object=r_objects.NamespaceCreateResponse(42)
        ),
        APIOutTestCase(
            client.reaction.get,
            dict(operation_identifier=r_objects.OperationIdentifier(operation_id=42)),
            endpoint_path="/api/v1/r/get",
            response_json=MOCK_REQUESTS["reaction_obj_no_descriptors"],
            expected_object=make_test_operation()
        ),
        APIOutTestCase(
            client.reaction.get_queue,
            dict(operation_identifier=r_objects.OperationIdentifier(operation_id=42)),
            endpoint_path="/api/v1/r/get",
            response_json=MOCK_REQUESTS["reaction_with_queue"],
            expected_object=r_objects.Queue(
                id_=4242,
                namespace_id=424242,
                configuration=r_objects.QueueConfiguration(
                    parallelism=42,
                    priority_function=r_objects.QueuePriorityFunction.TIME_NEWEST_FIRST,
                    max_queued_instances=r_objects.QueueMaxQueuedInstances(1000),
                    max_running_instances_per_reaction=r_objects.QueueMaxRunningInstancesPerReaction(21),
                    max_queued_instances_per_reaction=r_objects.QueueMaxQueuedInstancesPerReaction(500)
                )
            )
        ),
        APIOutTestCase(
            client.reaction.get_queue,
            dict(operation_identifier=r_objects.OperationIdentifier(operation_id=42)),
            endpoint_path="/api/v1/r/get",
            response_json=MOCK_REQUESTS["reaction_obj_no_descriptors"],
            expected_object=None
        ),
        APIOutTestCase(
            client.reaction_type.list,
            dict(),
            endpoint_path="/api/v1/r/t/list",
            response_json=MOCK_REQUESTS["reaction_type_response"],
            expected_object=[
                r_objects.ReactionType(
                    id_=42,
                    operation_set_key="nirvana_operations",
                    operation_key="launch_graph",
                    version="2",
                    operation_set_name="Nirvana",
                    operation_name="launch workflow instance",
                    description="launch Nirvana workflow instance",
                    status=r_objects.OperationTypeStatus.READONLY
                )
            ]
        ),
        APIOutTestCase(
            client.reaction_type.list,
            dict(),
            endpoint_path="/api/v1/r/t/list",
            response_json=MOCK_REQUESTS["reaction_type_response_new_key"],
            expected_object=[
                r_objects.ReactionType(
                    id_=42,
                    operation_set_key="nirvana_operations",
                    operation_key="launch_graph",
                    version="2",
                    operation_set_name="Nirvana",
                    operation_name="launch workflow instance",
                    description="launch Nirvana workflow instance",
                    status=r_objects.OperationTypeStatus.READONLY
                )
            ]
        ),
        APIOutTestCase(
            client.namespace.list,
            dict(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")),
            endpoint_path="/api/v1/n/list",
            response_json={
                "namespaces": [{
                    "id": "42",
                    "parentId": "42",
                    "type": "LEAF_SLICE",
                    "name": "answer",
                    "description": "Answer to the Ultimate Question of Life, the Universe, and Everything"
                }]
            },
            expected_object=[
                r_objects.Namespace(
                    id_=42,
                    parent_id=42,
                    type_=r_objects.NamespaceType.LEAF_SLICE,
                    name="answer",
                    description="Answer to the Ultimate Question of Life, the Universe, and Everything"
                )
            ]
        ),
        APIOutTestCase(
            client.namespace.list,
            dict(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")),
            endpoint_path="/api/v1/n/list",
            response_json={
                "namespaces": [{
                    "id": "42",
                    "parentId": "42",
                    "type": "bad_type_name",
                    "name": "answer",
                    "description": "Answer to the Ultimate Question of Life, the Universe, and Everything"
                }]
            },
            expected_object=[
                r_objects.Namespace(
                    id_=42,
                    parent_id=42,
                    type_=r_objects.UnknownEnumValue(name="bad_type_name"),
                    name="answer",
                    description="Answer to the Ultimate Question of Life, the Universe, and Everything"
                )
            ]
        ),
        APIOutTestCase(
            client.namespace.list_names,
            dict(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")),
            endpoint_path="/api/v1/n/list-names",
            response_json={
                "names": ["a", "b", "c"]
            },
            expected_object=["a", "b", "c"]
        ),
        APIOutTestCase(
            client.namespace.resolve_path,
            dict(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")),
            endpoint_path="/api/v1/n/resolve/path",
            response_json={
                "path": "/user_sessions/hahn/build/logs/recommender-reqans-log/fast/create/reactions/v1"
            },
            expected_object="/user_sessions/hahn/build/logs/recommender-reqans-log/fast/create/reactions/v1"
        ),
        APIOutTestCase(
            client.namespace_notification.list,
            dict(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")),
            endpoint_path="/api/v1/n/notification/list",
            response_json={
                "notifications": [{
                    "notificationId": "42",
                    "namespaceId": "42",
                    "eventType": "ARTIFACT_INSTANCE_APPEARANCE_DELAY",
                    "transport": "EMAIL",
                    "recipient": "etaranov"
                }]
            },
            expected_object=[
                r_objects.Notification(
                    id_=42,
                    namespace_id=42,
                    event_type=r_objects.NotificationEventType.ARTIFACT_INSTANCE_APPEARANCE_DELAY,
                    transport=r_objects.NotificationTransportType.EMAIL,
                    recipient="etaranov"
                )
            ]
        ),
        APIOutTestCase(
            client.reaction_instance.list_statuses,
            dict(reaction=r_objects.OperationIdentifier(
                namespace_identifier=r_objects.NamespaceIdentifier(
                    namespace_path="/ads/infra_test_mvp/tasks/test_release/hahn/MyJoinedEFTask"
                )
            )),
            endpoint_path="/api/v1/r/i/list",
            response_json=MOCK_REQUESTS["test_reaction_instance_list"],
            expected_object=[
                r_objects.OperationInstanceStatusView(
                    status=r_objects.ReactionInstanceStatus.COMPLETED,
                    id_=1346365,
                    operation_id=11179
                )
            ]
        ),
        APIOutTestCase(
            client.queue.create,
            dict(
                queue_descriptor=r_objects.QueueDescriptor(
                    namespace_descriptor=r_objects.NamespaceDescriptor(
                        namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                    ),
                    configuration=r_objects.QueueConfiguration(
                        parallelism=42,
                        priority_function=r_objects.QueuePriorityFunction.USER_TIME_ELDEST_FIRST,
                        max_queued_instances=r_objects.QueueMaxQueuedInstances(value=1000),
                        max_running_instances_per_reaction=r_objects.QueueMaxRunningInstancesPerReaction(value=21),
                        max_queued_instances_per_reaction=r_objects.QueueMaxQueuedInstancesPerReaction(value=500)
                    ),
                    project_identifier=r_objects.ProjectIdentifier(project_id=42),
                ),
                create_if_not_exist=True
            ),
            endpoint_path="/api/v1/q/create",
            response_json=TEST_QUEUE_CREATE_RESPONSE,
            expected_object=r_objects.QueueReference(queue_id=42, namespace_id=42)
        ),
        APIOutTestCase(
            client.queue.get,
            dict(
                queue_identifier=r_objects.QueueIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="test")
                )
            ),
            endpoint_path="/api/v1/q/get",
            response_json=TEST_QUEUE_GET_RESPONSE,
            expected_object=r_objects.QueueReactions(
                queue=r_objects.Queue(
                    id_=42,
                    namespace_id=42,
                    configuration=r_objects.QueueConfiguration(
                        parallelism=42,
                        priority_function=r_objects.QueuePriorityFunction.USER_TIME_ELDEST_FIRST,
                        max_queued_instances=r_objects.QueueMaxQueuedInstances(value=1000),
                        max_running_instances_per_reaction=r_objects.QueueMaxRunningInstancesPerReaction(value=21),
                        max_queued_instances_per_reaction=r_objects.QueueMaxQueuedInstancesPerReaction(value=500)
                    ),
                    project_id=42,
                ),
                reactions=[
                    r_objects.QueueOperation(reaction_id=42)
                ]
            )
        ),
        APIOutTestCase(
            client.namespace_notification.get_long_running,
            dict(
                operation_identifier=r_objects.OperationIdentifier(operation_id=42)
            ),
            endpoint_path="/api/v1/r/looong-running/get",
            response_json={
                "options": {
                    "warnPercentile": 70,
                    "warnRunsCount": 20,
                    "warnScale": 1.5,
                    "critPercentile": 85,
                    "critRunsCount": 30,
                    "critScale": 2.5
                }
            },
            expected_object=r_objects.LongRunningOperationInstanceNotificationOptions(
                warn_percentile=70,
                warn_runs_count=20,
                warn_scale=1.5,
                crit_percentile=85,
                crit_runs_count=30,
                crit_scale=2.5
            )
        ),
        APIOutTestCase(
            client.namespace_notification.get_long_running,
            dict(
                operation_identifier=r_objects.OperationIdentifier(operation_id=42)
            ),
            endpoint_path="/api/v1/r/looong-running/get",
            response_json={},
            expected_object=None
        ),
        APIOutTestCase(
            client.artifact_instance.get_status_history,
            dict(
                artifact_instance_id=42
            ),
            endpoint_path="/api/v1/a/i/getStatusHistory",
            response_json={
                "artifactInstanceStatuses": [{
                    "status": "REPLACING",
                    "updateTimestamp": "1985-10-26T01:21:00"
                }]
            },
            expected_object=[
                r_objects.ArtifactInstanceStatusRecord(
                    status=r_objects.ArtifactInstanceStatus.REPLACING,
                    update_timestamp=datetime(1985, 10, 26, 1, 21)
                )
            ]
        ),
        APIOutTestCase(
            client.artifact_instance.get_status_history,
            dict(
                artifact_instance_id=42
            ),
            endpoint_path="/api/v1/a/i/getStatusHistory",
            response_json={
                "artifactInstanceStatuses": [{
                    "status": "REPLACING",
                    "updateTimestamp": "1985-10-26T01:21:00Z[UTC]"
                }]
            },
            expected_object=[
                r_objects.ArtifactInstanceStatusRecord(
                    status=r_objects.ArtifactInstanceStatus.REPLACING,
                    update_timestamp=datetime(1985, 10, 26, 1, 21, tzinfo=tzutc())
                )
            ]
        ),
        APIOutTestCase(
            client.metric.create,
            dict(
                metric_type=r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                artifact_id=42,
                custom_tags={"tag1": "value1"},
            ),
            endpoint_path="/api/v1/metric/create",
            response_json={
                "metric": {
                    "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
                    "artifactId": "42",
                    "tags": [{"keyValue": {"tag1": "value1"}}]
                }
            },
            expected_object=r_objects.MetricReference(
                r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                [r_objects.Attributes(dict(tag1="value1"))],
                artifact_id=42
            )
        ),
        APIOutTestCase(
            client.metric.list,
            dict(reaction_id=42),
            endpoint_path="/api/v1/metric/list",
            response_json={
                "metrics": [
                    {
                        "metricType": "REACTION_USERTIME_PROCESSING_DURATION",
                        "reactionId": "42",
                        "tags": [{"keyValue": {"tag1": "value1"}}, {"keyValue": {"tag1": "value2"}}]
                    },
                    {
                        "metricType": "REACTION_USERTIME_PROCESSING_COUNT",
                        "reactionId": "42",
                        "tags": [{"keyValue": {"tag2": "value3"}}, {"keyValue": {"tag2": "value4"}}]
                    }
                ]
            },
            expected_object=[
                r_objects.MetricReference(
                    r_objects.MetricType.REACTION_USERTIME_PROCESSING_DURATION,
                    [r_objects.Attributes(dict(tag1="value1")), r_objects.Attributes(dict(tag1="value2"))],
                    reaction_id=42
                ),
                r_objects.MetricReference(
                    r_objects.MetricType.REACTION_USERTIME_PROCESSING_COUNT,
                    [r_objects.Attributes(dict(tag2="value3")), r_objects.Attributes(dict(tag2="value4"))],
                    reaction_id=42
                ),
            ]
        ),
        APIOutTestCase(
            client.metric.update,
            dict(
                metric_type=r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                artifact_id=42,
                new_tags={"tag2": "value2"}
            ),
            endpoint_path="/api/v1/metric/update",
            response_json={
                "metric": {
                    "metricType": "ARTIFACT_USERTIME_FIRST_DELAY",
                    "artifactId": "42",
                    "tags": [{"keyValue": {"tag2": "value2"}}],
                }
            },
            expected_object=r_objects.MetricReference(
                r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                [r_objects.Attributes(dict(tag2="value2"))],
                artifact_id=42
            )
        ),
        APIOutTestCase(
            client.metric.delete,
            dict(
                metric_type=r_objects.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
                artifact_id=42
            ),
            endpoint_path="/api/v1/metric/delete",
            response_json={},
            expected_object={}
        ),
        APIOutTestCase(
            client.dynamic_trigger.add,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                ),
                triggers=r_objects.DynamicTriggerList(triggers=[
                    r_objects.DynamicTrigger(
                        trigger_name="dynamic_cron_trigger",
                        expression=r_objects.Expression("global a = 42;"),
                        cron_trigger=r_objects.CronTrigger(
                            cron_expression="0 0/5 * * * ?",
                            misfire_policy=r_objects.MisfirePolicy.IGNORE
                        )
                    ),
                    r_objects.DynamicTrigger(
                        trigger_name="dynamic_artifact_trigger",
                        expression=r_objects.Expression("global b = 37;"),
                        artifact_trigger=r_objects.DynamicArtifactTrigger(
                            triggers=[r_objects.ArtifactReference(
                                namespace_identifier=r_objects.NamespaceIdentifier(
                                    namespace_path="b"
                                )
                            )]
                        )
                    )
                ])
            ),
            endpoint_path="/api/v1/t/add",
            response_json=TEST_DYNAMIC_TRIGGER_LIST,
            expected_object=[r_objects.DynamicTriggerDescriptor(
                id_=42, reaction_id=42, type_=r_objects.TriggerType.ARTIFACT,
                status=r_objects.TriggerStatus.DELETED,
                name="Daily_trigger",
                creation_time=datetime(2020, 10, 26, 1, 21, 0, 123000),
                data=r_objects.DynamicTrigger(
                    trigger_name="triggerName",
                    expression=r_objects.Expression("global a = 3.14"),
                    replacement=r_objects.DynamicTriggerReplacementRequest(42),
                    artifact_trigger=r_objects.DynamicArtifactTrigger(
                        triggers=[r_objects.ArtifactReference(
                            artifact_id=42, namespace_id=42
                        )]
                    )
                )
            )]
        ),
        APIOutTestCase(
            client.dynamic_trigger.remove,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                ),
                triggers=[
                    r_objects.DynamicTriggerIdentifier(
                        name="dynamic_artifact_trigger"
                    )
                ]
            ),
            endpoint_path="/api/v1/t/remove",
            response_json={},
            expected_object={}
        ),
        APIOutTestCase(
            client.dynamic_trigger.update,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                ),
                triggers=[
                    r_objects.DynamicTriggerIdentifier(
                        name="dynamic_artifact_trigger"
                    )
                ],
                action=r_objects.DynamicTriggerAction.ACTIVATE
            ),
            endpoint_path="/api/v1/t/update",
            response_json={},
            expected_object={}
        ),
        APIOutTestCase(
            client.dynamic_trigger.list,
            dict(
                reaction_identifier=r_objects.OperationIdentifier(
                    namespace_identifier=r_objects.NamespaceIdentifier(
                        namespace_path="a"
                    ),
                )
            ),
            endpoint_path="/api/v1/t/list",
            response_json=TEST_DYNAMIC_TRIGGER_LIST,
            expected_object=[r_objects.DynamicTriggerDescriptor(
                id_=42, reaction_id=42, type_=r_objects.TriggerType.ARTIFACT,
                status=r_objects.TriggerStatus.DELETED,
                name="Daily_trigger",
                creation_time=datetime(2020, 10, 26, 1, 21, 0, 123000),
                data=r_objects.DynamicTrigger(
                    trigger_name="triggerName",
                    expression=r_objects.Expression("global a = 3.14"),
                    replacement=r_objects.DynamicTriggerReplacementRequest(42),
                    artifact_trigger=r_objects.DynamicArtifactTrigger(
                        triggers=[r_objects.ArtifactReference(
                            artifact_id=42, namespace_id=42
                        )]
                    )
                )
            )]
        ),
        APIOutTestCase(
            client.permission.change,
            dict(
                namespace=r_objects.NamespaceIdentifier(namespace_path="a"),
                revoke=["login"],
                grant=r_objects.NamespacePermissions(
                    roles={"login_1": r_objects.NamespaceRole.READER,
                           "login_2": r_objects.NamespaceRole.WRITER}
                )
            ),
            endpoint_path="/api/v1/n/permission/change",
            response_json={},
            expected_object={}
        ),
        APIOutTestCase(
            client.permission.list,
            dict(namespace=r_objects.NamespaceIdentifier(namespace_path="a")),
            endpoint_path="/api/v1/n/permission/list",
            response_json={
                "permissions": {
                    "roles": {
                        "login_1": "READER",
                        "login_2": "WRITER"
                    }
                }
            },
            expected_object=r_objects.NamespacePermissions(
                roles={
                    "login_1": r_objects.NamespaceRole.READER,
                    "login_2": r_objects.NamespaceRole.WRITER
                }
            )
        ),
        APIOutTestCase(
            client.permission.list,
            dict(
                namespace=r_objects.NamespaceIdentifier(namespace_path="a"),
                version=1
            ),
            endpoint_path="/api/v1/n/permission/list",
            response_json={
                "permissions": {
                    "roles": {
                        "@user": "READER",
                        "group": "WRITER"
                    },
                    "version": 1
                }
            },
            expected_object=r_objects.NamespacePermissions(
                roles={
                    "@user": r_objects.NamespaceRole.READER,
                    "group": r_objects.NamespaceRole.WRITER
                },
                version=1
            )
        ),
        APIOutTestCase(
            client.quota.get,
            dict(namespace=r_objects.NamespaceIdentifier(namespace_path="a")),
            endpoint_path="/api/v1/quota/cleanupStrategy/get",
            response_json={
                "cleanupStrategy": {
                    "cleanupStrategies": [
                        {"ttlCleanupStrategy": {"ttlDays": "42"}}
                    ]
                }
            },
            expected_object=r_objects.CleanupStrategyDescriptor(
                cleanup_strategies=[r_objects.CleanupStrategy(
                    ttl_cleanup_strategy=r_objects.TtlCleanupStrategy(42)
                )]
            )
        ),
        APIOutTestCase(
            client.quota.update,
            dict(
                namespace=r_objects.NamespaceIdentifier(namespace_path="a"),
                cleanup_strategy=r_objects.CleanupStrategyDescriptor(
                    cleanup_strategies=[r_objects.CleanupStrategy(
                        ttl_cleanup_strategy=r_objects.TtlCleanupStrategy(
                            ttl_days=14
                        )
                    )]
                )
            ),
            endpoint_path="/api/v1/quota/cleanupStrategy/update",
            response_json={},
            expected_object={}
        ),
        APIOutTestCase(
            client.quota.delete,
            dict(namespace=r_objects.NamespaceIdentifier(namespace_path="a")),
            endpoint_path="/api/v1/quota/cleanupStrategy/delete",
            response_json={},
            expected_object={}
        )
    ]


def test_artifact_returned_correctly_on_get(api_out_test_cases):
    """
    :type api_out_test_cases: list[APIOutTestCase]
    """
    for test_case in api_out_test_cases:
        with requests_mock.mock() as m:
            m.post(TEST_URL + test_case.endpoint_path, json=test_case.response_json)
            m.get(TEST_URL + test_case.endpoint_path, json=test_case.response_json)  # TODO: Fix this
            assert test_case.method(**test_case.args) == test_case.expected_object


def test_error_if_create_reaction_with_retries_and_not_set_create_if_not_exist():
    client = ReactorAPIClientV1(TEST_URL, TEST_TOKEN, retry_policy=RetryPolicy(tries=2))
    with pytest.raises(ReactorAPIException):
        client.reaction.create(
            make_test_cron_reaction(),
            create_if_not_exist=False
        )


def test_retries_happen():
    expected_retry_count = 4
    client = ReactorAPIClientV1(TEST_URL, TEST_TOKEN, retry_policy=RetryPolicy(tries=expected_retry_count))
    with requests_mock.mock() as m:
        class CountRetries(object):
            def __init__(self):
                self.retries = 0

            def __call__(self, request, context):
                self.retries += 1
                raise requests.ConnectTimeout

        rc = CountRetries()

        m.post(TEST_URL + "/api/v1/a/get", text=rc)
        try:
            client.requester.send_request("a/get")
        except ReactorAPITimeout:
            pass

        assert rc.retries == expected_retry_count


NO_NAMESPACE_ERROR = {
    'codeDescription': "Can't find Namespace by provided identifier",
    'message': 'Namespace doesn\'t exist. Action = \'Get Namespace\', Identifier = \'namespacePath: "/dfd"\n\', '
               'Security = \'SecurityContext{user=\'npytincev\'(256), level=\'NORMAL\'}\'.',
    'code': 'NAMESPACE_DOES_NOT_EXIST',
    'codeClass': 'RxDomainErrorCode'
}


def test_namespace_check_exist_fails(client):
    with requests_mock.mock() as m:
        m.post(TEST_URL + "/api/v1/n/get", json=NO_NAMESPACE_ERROR, status_code=504)
        assert not client.namespace.check_exists(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/test"))


NO_ARTIFACT_ERROR_V2 = {
    "causeMessages":
        ["Not retriable exception null", "Artifact doesn't exist."],
    "codes": [
        {"code": "ARTIFACT_DOES_NOT_EXIST_ERROR", "codeClass": "RxDomainErrorCode",
         "codeDescription": "Can't find Artifact by provided identifier"},
        {"code": "UNKNOWN_ERROR", "codeClass": "RxCommonErrorCode", "codeDescription": "Unclassified error"}],
    "uuid": "ad4c7765-8c0c-4f3a-a1b3-2c3e7f131a43",
    "message": "ReactorException: Not retriable exception null"
}


def test_artifact_check_exist_fails(client):
    with requests_mock.mock() as m:
        m.post(TEST_URL + "/api/v1/a/get", json=NO_ARTIFACT_ERROR_V2, status_code=504)
        assert not client.artifact.check_exists(namespace_identifier=r_objects.NamespaceIdentifier(namespace_path="/test"))


NO_QUEUE_ERROR = {
    "causeMessages":
        ["Not PG error", "Namespace doesn't exist."],
    "codes": [
        {"code": "NAMESPACE_DOES_NOT_EXIST", "codeClass": "RxDomainErrorCode",
         "codeDescription": "Can't find Namespace by provided identifier"},
        {"code": "UNKNOWN_ERROR", "codeClass": "RxCommonErrorCode", "codeDescription": "Unclassified error"}],
    "uuid": "ad4c7765-8c0c-4f3a-a1b3-2c3e7f131a43",
    "message": "Not PG error"
}


def test_queue_check_exist_fails(client):
    with requests_mock.mock() as m:
        m.post(TEST_URL + "/api/v1/q/get", json=NO_QUEUE_ERROR, status_code=504)
        assert not client.queue.check_exists(
            queue_identifier=r_objects.QueueIdentifier(
                namespace_identifier=r_objects.NamespaceIdentifier(
                    namespace_path='/test'
                )
            )
        )


def test_retries_happen_on_500():
    expected_retry_count = 4
    client = ReactorAPIClientV1(TEST_URL, TEST_TOKEN, retry_policy=RetryPolicy(tries=expected_retry_count))
    with requests_mock.mock() as m:
        class CountRetries(object):
            def __init__(self):
                self.retries = 0

            def __call__(self, request, context):
                self.retries += 1
                return {'code': "SOME_CODE"}

        rc = CountRetries()

        m.post(TEST_URL + "/api/v1/a/get", json=rc, status_code=500)
        try:
            client.requester.send_request("a/get")
        except ReactorInternalError:
            pass

        assert rc.retries == expected_retry_count


def test_retries_happen_on_500_and_empty_string_in_response():
    expected_retry_count = 4
    client = ReactorAPIClientV1(TEST_URL, TEST_TOKEN, retry_policy=RetryPolicy(tries=expected_retry_count))
    with requests_mock.mock() as m:
        class CountRetries(object):
            def __init__(self):
                self.retries = 0

            def __call__(self, request, context):
                self.retries += 1
                return ""

        rc = CountRetries()

        m.post(TEST_URL + "/api/v1/a/get", json=rc, status_code=500)
        try:
            client.requester.send_request("a/get")
        except ReactorInternalError:
            pass

        assert rc.retries == expected_retry_count


def test_url_with_no_schema_raises_error():
    with pytest.raises(ReactorAPIException):
        ReactorAPIClientV1("reactor.ru", "")


def test_correct_url_raise_nothing():
    ReactorAPIClientV1("http://reactor.ru", "")


@pytest.mark.parametrize("exception_type", [ReactorAPIException, ReactorInternalError])
def test_pickle_exception(exception_type):
    exception = exception_type(500, "Test pickle execption")
    dumped = pickle.dumps(exception)
    loaded = pickle.loads(dumped)

    assert type(exception) == type(loaded)
    assert exception.args == loaded.args
    assert vars(exception) == vars(loaded)
