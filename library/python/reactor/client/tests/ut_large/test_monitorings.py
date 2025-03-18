import logging
import os

import pytest

from reactor_client import reactor_objects as r_objs
from reactor_client.client_base import ReactorAPIException
from reactor_client.reactor_objects import Relationship
from library.python.reactor.client.tests import helpers
from . import get_path_for_tests, reactor_client, TEST_USER, REACTOR_TEST_PROJECT_PATH


@pytest.yield_fixture(scope='module')
def entities(client):
    test_entities = {}
    try:
        test_entities = set_up(client)
        yield test_entities
    finally:
        tear_down(test_entities, client)


@pytest.fixture(scope='module')
def client():
    return reactor_client()


def set_up(client):
    """
    :rtype: dict[str, r_objs.NamespaceCreateResponse]
    """
    test_dir = get_path_for_tests('monitorings')
    test_folder = None
    artifact = None
    reaction = None
    ut_reaction = None
    queue = None
    try:
        test_folder = client.namespace.create(
            r_objs.NamespaceIdentifier(namespace_path=test_dir),
            '',
            r_objs.NamespacePermissions({TEST_USER: r_objs.NamespaceRole.RESPONSIBLE}),
            create_parents=True,
            create_if_not_exist=True
        )
        artifact = client.artifact.create(
            r_objs.ArtifactTypeIdentifier(artifact_type_key="PRIMITIVE_STRING"),
            r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=os.path.join(test_dir, 'artifact'))),
            '',
            r_objs.NamespacePermissions({TEST_USER: r_objs.NamespaceRole.RESPONSIBLE}),
            project_identifier=r_objs.ProjectIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=REACTOR_TEST_PROJECT_PATH)),
            cleanup_strategy=r_objs.CleanupStrategyDescriptor([r_objs.CleanupStrategy(r_objs.TtlCleanupStrategy(1))]),
            create_parent_namespaces=True,
            create_if_not_exist=True
        )
        reaction = client.reaction.create(
            helpers.make_test_blank_reaction_triggered_by_artifacts(TEST_USER, REACTOR_TEST_PROJECT_PATH, os.path.join(test_dir, 'reaction'), [artifact], Relationship.OR),
            create_if_not_exist=True
        )
        ut_reaction = client.reaction.create(
            helpers.make_test_blank_reaction_triggered_by_artifacts(TEST_USER, REACTOR_TEST_PROJECT_PATH, os.path.join(test_dir, 'ut_reaction'), [artifact], Relationship.USER_TIMESTAMP_EQUALITY),
            create_if_not_exist=True
        )
        queue = client.queue.create(
            helpers.make_test_queue(TEST_USER, REACTOR_TEST_PROJECT_PATH, os.path.join(test_dir, 'queue')),
            create_if_not_exist=True
        )
    except:
        logging.error('Failed to init test case', exc_info=True)
    finally:
        return dict(
            test_folder=test_folder,
            artifact=artifact,
            reaction=reaction,
            ut_reaction=ut_reaction,
            queue=queue
        )


def tear_down(test_entities, client):
    """
    :type test_entities: dict[str, r_objs.NamespaceCreateResponse]
    """
    if test_entities.get('queue') is not None:
        client.namespace.delete([r_objs.NamespaceIdentifier(namespace_id=test_entities['queue'].namespace_id)], delete_if_exist=True)
    if test_entities.get('ut_reaction') is not None:
        client.namespace.delete([r_objs.NamespaceIdentifier(namespace_id=test_entities['ut_reaction'].namespace_id)], delete_if_exist=True)
    if test_entities.get('reaction') is not None:
        client.namespace.delete([r_objs.NamespaceIdentifier(namespace_id=test_entities['reaction'].namespace_id)], delete_if_exist=True)
    if test_entities.get('artifact') is not None:
        client.namespace.delete([r_objs.NamespaceIdentifier(namespace_id=test_entities['artifact'].namespace_id)], delete_if_exist=True)
    if test_entities.get('test_folder') is not None:
        client.namespace.delete([r_objs.NamespaceIdentifier(namespace_id=test_entities['test_folder'].namespace_id)], delete_if_exist=True)


# -------------------------------------------------- TESTS --------------------------------------------------

def create_metric(client, metric_type, entity, custom_tags):
    """
    :type metric_type: r_objs.MetricType
    :type entity: ReactorEntity
    :type custom_tags: dict[str, str]
    :rtype: r_objs.MetricReference
    """
    return client.metric.create(
        metric_type,
        artifact_id=entity.artifact_id,
        reaction_id=entity.reaction_id,
        queue_id=entity.queue_id,
        custom_tags=custom_tags
    )


def get_metric(client, metric_type, entity):
    """
    :type metric_type: r_objs.MetricType
    :type entity: ReactorEntity
    :rtype: r_objs.MetricReference
    """
    metrics = client.metric.list(
        artifact_id=entity.artifact_id,
        reaction_id=entity.reaction_id,
        queue_id=entity.queue_id,
    )
    for metric in metrics:
        if (metric.reaction_id == entity.reaction_id or metric.artifact_id == entity.artifact_id or metric.queue_id == entity.queue_id) and metric.metric_type == metric_type:
            return metric
    return None


def list_metric(client, entity):
    """
    :type entity: ReactorEntity
    :rtype: list[r_objs.MetricReference]
    """
    return client.metric.list(
        artifact_id=entity.artifact_id,
        reaction_id=entity.reaction_id,
        queue_id=entity.queue_id,
    )


def update_metric(client, metric_type, entity, new_custom_tags):
    """
    :type metric_type: r_objs.MetricType
    :type entity: ReactorEntity
    :type new_custom_tags: dict[str, str]
    :rtype: r_objs.MetricReference
    """
    return client.metric.update(
        metric_type,
        artifact_id=entity.artifact_id,
        reaction_id=entity.reaction_id,
        queue_id=entity.queue_id,
        new_tags=new_custom_tags
    )


def delete_metric(client, metric_type, entity):
    """
    :type metric_type: r_objs.MetricType
    :type entity: ReactorEntity
    """
    client.metric.delete(
        metric_type,
        artifact_id=entity.artifact_id,
        reaction_id=entity.reaction_id,
        queue_id=entity.queue_id,
    )


class ReactorEntity(object):
    def __init__(self, artifact_id=None, reaction_id=None, queue_id=None):
        """
        one of
        :type artifact_id: int
        :type reaction_id: int
        :type queue_id: int
        """
        self.artifact_id = artifact_id
        self.reaction_id = reaction_id
        self.queue_id = queue_id


class MetricTestCase(object):
    def __init__(self, to_create, expected_not_found, to_update):
        """
        :type to_create: dict[r_objs.MetricType, dict[str, str]]
        :type expected_not_found: list[r_objs.MetricType]
        :type to_update: dict[r_objs.MetricType, dict[str, str]]
        """
        self.to_create = to_create
        self.not_found = expected_not_found
        self.to_update = to_update


@pytest.fixture
def metric_test_cases(entities):
    """
    :rtype: dict[ReactorEntity, MetricTestCase]
    """
    return {
        ReactorEntity(artifact_id=entities['artifact'].artifact_id): MetricTestCase(
            {r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY: {}, },
            [r_objs.MetricType.ARTIFACT_USERTIME_COUNT, ],
            {r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY: {"eKind": "artifact", "metric": "first_delay", "ver": "v0.1.0"}, },
        ),
        ReactorEntity(reaction_id=entities['ut_reaction'].reaction_id): MetricTestCase(
            {
                r_objs.MetricType.REACTION_USERTIME_PROCESSING_DURATION: {"eKind": "ut_reaction", "metric": "processing_duration"},
                r_objs.MetricType.REACTION_USERTIME_FAILURES: {"eKind": "ut_reaction", "metric": "failures", "ver": "0.1"},
            },
            [r_objs.MetricType.REACTION_USERTIME_ATTEMPTS, r_objs.MetricType.REACTION_USERTIME_PROCESSING_COUNT, ],
            {r_objs.MetricType.REACTION_USERTIME_FAILURES: {"metric": "failures", "ver": "0.2", "tag1": "val1", "tag2": "val2"}, },
        ),
        ReactorEntity(queue_id=entities['queue'].queue_id): MetricTestCase(
            {
                r_objs.MetricType.QUEUE_USERTIME_STATUS_COUNT: {"eKind": "queue", "metric": "ut statuses"},
                r_objs.MetricType.QUEUE_ELDEST: {"eKind": "queue", "metric": "eldest"},
            },
            [r_objs.MetricType.QUEUE_AGGREGATED_STATUS_COUNT, ],
            {r_objs.MetricType.QUEUE_USERTIME_STATUS_COUNT: {"metric": "ut statuses"}, },
        ),
        ReactorEntity(reaction_id=entities['reaction'].reaction_id): MetricTestCase(
            {},
            [],
            {},
        ),
    }


@pytest.fixture
def metric_error_test_cases(entities):
    return {
        'try create metric for non-existent entity': lambda client: create_metric(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_COUNT,
            ReactorEntity(artifact_id=0),
            {"tag1": "value1", "tag2": "value2"}
        ),
        'try create metric with too many custom tags': lambda client: create_metric(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
            ReactorEntity(artifact_id=entities['artifact'].artifact_id),
            {
                "tag1": "value1",
                "tag2": "value2",
                "tag3": "value3",
                "tag4": "value4",
                "tag5": "value5",
                "tag6": "value6",
                "tag7": "value7",
                "tag8": "value8",
                "tag9": "value9",
                "tag10": "value10",
                "tag11": "value11",
                "tag12": "value12"
            },
        ),
        'try create metric for namespace that does not support metric type': lambda client: create_metric(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
            ReactorEntity(queue_id=entities['queue'].queue_id),
            {"tag1": "value1", "tag2": "value2"}
        ),
        'try get metric for non-existent entity': lambda client: get_metric(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_COUNT,
            ReactorEntity(artifact_id=0)
        ),
        'try list metrics for non-existent entity': lambda client: list_metric(
            client,
            ReactorEntity(artifact_id=0)
        ),
        'try update metric for non-existent entity': lambda client: update_metric(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_COUNT,
            ReactorEntity(artifact_id=0),
            {"tag1": "value1", "tag2": "value2"}
        ),
        'try update non-existent metric': lambda client: update_metric(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_COUNT,
            ReactorEntity(artifact_id=entities['artifact'].artifact_id),
            {"newTag1": "newValue1", "newTag2": "newValue2"}
        ),
        'try update metric with too many tags': lambda client: update_metric_after_create(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_FIRST_DELAY,
            ReactorEntity(artifact_id=entities['artifact'].artifact_id),
            {
                "tag1": "value1",
                "tag2": "value2",
                "tag3": "value3",
                "tag4": "value4",
                "tag5": "value5",
                "tag6": "value6",
                "tag7": "value7",
                "tag8": "value8",
                "tag9": "value9",
                "tag10": "value10",
                "tag11": "value11",
                "tag12": "value12"
            },
        ),
        'try delete metric for non-existent entity': lambda client: delete_metric(
            client,
            r_objs.MetricType.ARTIFACT_USERTIME_COUNT,
            ReactorEntity(artifact_id=0)
        ),
        'try delete non-supported metric': lambda client: delete_metric(
            client,
            r_objs.MetricType.QUEUE_ELDEST,
            ReactorEntity(artifact_id=entities['artifact'].artifact_id)
        ),
    }


def update_metric_after_create(client, metric_type, entity, new_custom_tags):
    """
    :type metric_type: r_objs.MetricType
    :type entity: ReactorEntity
    :type new_custom_tags: dict[str, str]
    :rtype: r_objs.MetricReference
    """
    create_metric(client, metric_type, entity, {})
    return update_metric(client, metric_type, entity, new_custom_tags)


def test_metrics(metric_test_cases, client):
    for entity in metric_test_cases:
        assert_create_and_get_metrics(client, entity, metric_test_cases[entity].to_create)

    for entity in metric_test_cases:
        assert_get_non_existing_metrics(client, entity, metric_test_cases[entity].not_found)
        assert_list_existing_metrics(client, entity, metric_test_cases[entity].to_create)

    for entity in metric_test_cases:
        assert_update_and_get_metrics(client, entity, metric_test_cases[entity].to_update)

    for entity in metric_test_cases:
        assert_delete_metrics(client, entity, set(metric_test_cases[entity].to_create.keys()))


def assert_create_and_get_metrics(client, entity, to_create):
    """
    :type entity: ReactorEntity
    :type to_create: dict[r_objs.MetricType, dict[str, str]]
    """
    for metric_type in to_create:
        create_response = create_metric(client, metric_type, entity, to_create[metric_type])
        assert_metric(create_response, metric_type, entity, to_create[metric_type])
        created_metric = get_metric(client, metric_type, entity)
        assert_metric(created_metric, metric_type, entity, to_create[metric_type])
        recreate_response = create_metric(client, metric_type, entity, {"newTag": "someValue"})
        assert_metric(recreate_response, metric_type, entity, to_create[metric_type])
        existing_metric = get_metric(client, metric_type, entity)
        assert_metric(existing_metric, metric_type, entity, to_create[metric_type])


def assert_get_non_existing_metrics(client, entity, expected_not_found):
    """
    :type entity: ReactorEntity
    :type expected_not_found: list[r_objs.MetricType]
    """
    for metric_type in expected_not_found:
        metric = get_metric(client, metric_type, entity)
        assert metric is None


def assert_list_existing_metrics(client, entity, expected):
    """
    :type entity: ReactorEntity
    :type expected: dict[r_objs.MetricType, dict[str, str]]
    """
    existing_metrics = list_metric(client, entity)
    assert len(set([metric.metric_type for metric in existing_metrics])) == len(expected)
    for metric in existing_metrics:
        assert metric.metric_type in expected
        assert_metric(metric, metric.metric_type, entity, expected[metric.metric_type])


def assert_update_and_get_metrics(client, entity, to_update):
    """
    :type entity: ReactorEntity
    :type to_update: dict[r_objs.MetricType, dict[str, str]]
    """
    for metric_type in to_update:
        update_response = update_metric(client, metric_type, entity, to_update[metric_type])
        assert_metric(update_response, metric_type, entity, to_update[metric_type])
        updated_metric = get_metric(client, metric_type, entity)
        assert_metric(updated_metric, metric_type, entity, to_update[metric_type])


def assert_delete_metrics(client, entity, to_delete):
    """
    :type entity: ReactorEntity
    :type to_delete: set[r_objs.MetricType]
    """
    for metric_type in to_delete:
        delete_metric(client, metric_type, entity)
        deleted_metric = get_metric(client, metric_type, entity)
        assert deleted_metric is None


def assert_metric(actual_metric, metric_type, entity, custom_tags):
    """
    :type actual_metric: r_objs.MetricReference
    :type metric_type: r_objs.MetricType
    :type entity: ReactorEntity
    :type custom_tags: dict[str, str]
    """
    assert actual_metric.artifact_id == entity.artifact_id
    assert actual_metric.reaction_id == entity.reaction_id
    assert actual_metric.queue_id == entity.queue_id
    assert actual_metric.metric_type == metric_type
    for tag_list in actual_metric.tags_list:
        assert set(custom_tags.items()).issubset(set(tag_list.key_value.items())) is True


def test_error_metrics(metric_error_test_cases, client):
    for test_name in metric_error_test_cases:
        try:
            with pytest.raises(ReactorAPIException):
                metric_error_test_cases[test_name](client)
            logging.info("Test '%s' passed." % test_name)
        except:
            logging.error("Test '%s' failed." % test_name)
            raise
