"""
Tests for operations helpers

"""

import re
import pytest
from hamcrest import assert_that, has_entries, has_property, starts_with

from dbaas_internal_api import modules
from dbaas_internal_api.core.types import Operation, OperationStatus, ResponseType, make_operation
from dbaas_internal_api.utils.register import get_cluster_traits, get_operations_describer

from ..mocks import mock_feature_flags, mock_logs

# pylint: disable=invalid-name, missing-docstring


def make_op(status):
    op_args = dict.fromkeys(Operation._fields, None)
    op_args['status'] = status
    return Operation(**op_args)


ALL_CLUSTER_TYPES = [
    modules.clickhouse.constants.MY_CLUSTER_TYPE,
    modules.postgres.constants.MY_CLUSTER_TYPE,
    modules.mongodb.constants.MY_CLUSTER_TYPE,
    modules.redis.constants.MY_CLUSTER_TYPE,
    modules.mysql.constants.MY_CLUSTER_TYPE,
]


def make_operation_with_args(cluster_type, operation_type):
    """
    Make operation with arguments witch fit all metadata getters
    """
    op_dict = dict.fromkeys(Operation._fields)
    op_dict.update(
        {
            'cluster_type': cluster_type,
            'operation_type': operation_type.value,
            'metadata': {
                'backup_id': '11.22.33',
                'shard_name': 'test_shard',
                'shard_group_name': 'test_group',
                'host_names': ['test.local'],
                'database_name': 'foo_db',
                'user_name': 'Tom',
                'ml_model_name': 'test_model',
                'format_schema_name': 'test_schema',
                'source_cid': '1.2.3.4',
                'source_folder_id': 'folder1',
                'destination_folder_id': 'folder2',
                'delayed_until': '2000-01-00T00:00:00Z',
                'alert_group_id': "test_ag_id",
            },
            'status': OperationStatus.done.value,
        }
    )
    return make_operation(op_dict)


@pytest.mark.parametrize('cluster_type', ALL_CLUSTER_TYPES)
def test_all_cluster_operations_has_descriptions(mocker, cluster_type):
    mock_feature_flags(mocker)
    for op_type in get_cluster_traits(cluster_type).operations:
        describer = get_operations_describer(cluster_type)
        op = make_operation_with_args(cluster_type, op_type)
        assert describer.get_description(op) is not None, 'There are no description ' 'for {0} {1} operation'.format(
            cluster_type, op_type
        )


def _get_descriptions(cluster_type):
    describer = get_operations_describer(cluster_type)

    return [
        describer.get_description(make_operation_with_args(cluster_type, operation_type))
        for operation_type in get_cluster_traits(cluster_type).operations
    ]


@pytest.mark.parametrize('cluster_type', ALL_CLUSTER_TYPES)
def test_all_metadata_annotations_is_absolute(mocker, cluster_type):
    mock_feature_flags(mocker)
    for op in _get_descriptions(cluster_type):
        assert_that(op.metadata, has_property('annotation', starts_with('yandex.cloud.mdb.')))


@pytest.mark.parametrize('cluster_type', ALL_CLUSTER_TYPES)
def test_all_response_annotations_is_absolute(mocker, cluster_type):
    mock_feature_flags(mocker)
    for op in _get_descriptions(cluster_type):
        if op.response.type is ResponseType.empty:
            continue
        assert_that(op.response, has_property('annotation', starts_with('yandex.cloud.mdb.')))


def _assert_all_metadata_annotations_is_unique(descriptions):
    existed_annotations = set()
    re_metadata = re.compile('Update\\s\\w+\\scluster metadata')
    for i, op in enumerate(descriptions):
        if re_metadata.match(op.description):
            continue
        assert (
            op.metadata.annotation not in existed_annotations
        ), '%s metadata annotation from %r already used somewhere in %r' % (
            op.metadata.annotation,
            op,
            descriptions[:i],
        )
        existed_annotations.add(op.metadata.annotation)


@pytest.mark.parametrize(
    'cluster_type',
    [
        modules.clickhouse.constants.MY_CLUSTER_TYPE,
        modules.mongodb.constants.MY_CLUSTER_TYPE,
        modules.redis.constants.MY_CLUSTER_TYPE,
    ],
)
def test_all_metadata_annotations_is_unique(mocker, cluster_type):
    mock_feature_flags(mocker)
    _assert_all_metadata_annotations_is_unique(_get_descriptions(cluster_type))


def test_all_postgre_annotations_is_unique_except_update_to_10(mocker):
    mock_feature_flags(mocker)
    descriptions = _get_descriptions(modules.postgres.constants.MY_CLUSTER_TYPE)
    descriptions = [
        d
        for d in descriptions
        if d.description
        not in [
            'Upgrade PostgreSQL cluster to 11-1c version',
            'Upgrade PostgreSQL cluster to 11 version',
            'Upgrade PostgreSQL cluster to 12 version',
            'Upgrade PostgreSQL cluster to 13 version',
            'Upgrade PostgreSQL cluster to 14 version',
            'Update PostgreSQL cluster metadata',
        ]
    ]
    print(descriptions)
    _assert_all_metadata_annotations_is_unique(descriptions)


def test_all_mysql_annotations_is_unique_except_update_to_80(mocker):
    mock_feature_flags(mocker)
    descriptions = _get_descriptions(modules.mysql.constants.MY_CLUSTER_TYPE)
    descriptions = [
        d
        for d in descriptions
        if d.description
        not in [
            'Upgrade MySQL cluster to 8.0 version',
            'Update MySQL cluster metadata',
        ]
    ]
    print(descriptions)
    _assert_all_metadata_annotations_is_unique(descriptions)


def test_describe_for_legacy_operation(mocker):
    mock_feature_flags(mocker)
    mock_logs(mocker)

    cluster_type = ALL_CLUSTER_TYPES[0]

    op_dict = dict.fromkeys(Operation._fields)
    op_dict.update(
        {
            'cluster_type': cluster_type,
            'operation_type': 'some_strange_and_legacy_operation',
            'metadata': {},
            'status': OperationStatus.done.value,
        }
    )
    op = make_operation(op_dict)
    describer = get_operations_describer(cluster_type)
    assert describer.get_description(op) is None


MOP = modules.mongodb.traits.MongoDBOperations


@pytest.mark.parametrize(
    ['operation_type', 'db_metadata', 'api_metadata_keys'],
    [
        (
            MOP.user_create,
            {
                'user_name': 'Ivan',
            },
            {
                'userName': 'Ivan',
            },
        ),
        (
            MOP.database_add,
            {
                'database_name': 'demo',
            },
            {
                'databaseName': 'demo',
            },
        ),
        (
            MOP.host_create,
            {
                'host_names': ['node.test'],
            },
            {
                'hostNames': ['node.test'],
            },
        ),
    ],
)
def test_api_metadata(mocker, operation_type, db_metadata, api_metadata_keys):
    mock_feature_flags(mocker)
    mock_logs(mocker)

    cluster_type = modules.mongodb.constants.MY_CLUSTER_TYPE

    op_dict = dict.fromkeys(Operation._fields)
    op_dict.update(
        {
            'operation_type': operation_type,
            'metadata': db_metadata,
            'target_id': 'cluster id',
            'cluster_type': cluster_type,
            'status': OperationStatus.done.value,
        }
    )

    op = make_operation(op_dict)
    describer = get_operations_describer(cluster_type)
    ret = describer.get_description(op)

    assert_that(ret.metadata.metadata, has_entries(**api_metadata_keys))
