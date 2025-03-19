"""
Operation API utils test
"""
import uuid
from datetime import datetime
from typing import Any

import pytest
from hamcrest import all_of, assert_that, has_entries, has_entry, has_key, not_, equal_to

from dbaas_internal_api.apis.operations_responser import _Cached
from dbaas_internal_api.apis.operations import __name__ as PACKAGE
from dbaas_internal_api.apis.operations import _assert_operation_match_cluster, render_operation_v1
from dbaas_internal_api.core.exceptions import OperationTypeMismatchError
from dbaas_internal_api.core.types import Operation, OperationStatus
from dbaas_internal_api.modules.clickhouse.constants import MY_CLUSTER_TYPE as CLICKHOUSE_CLUSTER
from dbaas_internal_api.modules.clickhouse.traits import ClickhouseTasks
from dbaas_internal_api.modules.mongodb.constants import MY_CLUSTER_TYPE as CLUSTER_TYPE
from dbaas_internal_api.modules.mongodb.traits import MongoDBTasks as Tasks
from dbaas_internal_api.modules.redis.constants import MY_CLUSTER_TYPE as REDIS_CLUSTER
from dbaas_internal_api.modules.redis.traits import RedisTasks

from ..mocks import mock_feature_flags, mock_logs

CID = uuid.uuid1()

OP_REQUIRED_DATA = {
    'id': uuid.uuid1(),
    'target_id': CID,
    'environment': 'unit-tests',
    'created_by': 'unit-test',
    'created_at': datetime.now(),
    'started_at': datetime.now(),
    'modified_at': datetime.now(),
    'metadata': {
        'cid': str(CID),
    },
    'hidden': False,
    'errors': None,
}

# pylint: disable=missing-docstring, invalid-name

CREATE_TASK_VALUE = Tasks.create.value  # pylint: disable=no-member


def make_op(
    status: OperationStatus,
    cluster_type: str = None,
    operation_type: str = CREATE_TASK_VALUE,
) -> Operation:
    return Operation(  # type: ignore
        cluster_type=cluster_type, status=status, operation_type=operation_type, **OP_REQUIRED_DATA
    )


class Test_render_operation_v1:
    def mock_responser(self, mocker):
        mocker.patch(PACKAGE + '.OperationsResponser')

    def test_successfully_finished_operation(self, mocker):
        self.mock_responser(mocker)
        op = make_op(status=OperationStatus.done, cluster_type=CLUSTER_TYPE)

        assert_that(render_operation_v1(op), all_of(has_entry('done', True), not_(has_key('error'))))

    def test_failed__operation(self, mocker):
        mock_feature_flags(mocker)
        op = make_op(status=OperationStatus.failed, cluster_type=CLUSTER_TYPE)

        assert_that(
            render_operation_v1(op),
            has_entries(
                done=True,
                error=has_entries(
                    code=2,
                    message='Unknown error',
                    details=[],
                ),
            ),
        )

    def test_running_operation(self, mocker):
        mock_feature_flags(mocker)
        op = make_op(status=OperationStatus.running, cluster_type=CLUSTER_TYPE)

        assert_that(render_operation_v1(op), has_entry('done', False))

    def test_operation_without_cluster_type(self, mocker):
        self.mock_responser(mocker)
        op = make_op(
            cluster_type=None,
            status=OperationStatus.done,
        )

        assert_that(render_operation_v1(operation=op, cluster_type=CLUSTER_TYPE), has_entry('done', True))

    def test_undescribed_operation(self, mocker):
        self.mock_responser(mocker)
        mock_logs(mocker)
        op = make_op(
            cluster_type=CLUSTER_TYPE,
            status=OperationStatus.done,
            operation_type='some_legacy_operation',
        )

        assert_that(
            render_operation_v1(op),
            all_of(
                has_entry('done', True),
                not_(has_key('description')),
                not_(has_key('metadata')),
            ),
        )


class Test_assert_operation_match_cluster:
    # pylint: disable=no-member
    def make_op(self, cluster_type, operation_type):
        return make_op(status=OperationStatus.done, cluster_type=cluster_type, operation_type=operation_type)

    def test_when_op_matches(self):
        op = self.make_op(REDIS_CLUSTER, RedisTasks.create.value)
        _assert_operation_match_cluster(op, REDIS_CLUSTER)

    def test_when_op_has_not_cluster_but_traits_has_this_type(self, mocker):
        mock_feature_flags(mocker)
        op = self.make_op(None, RedisTasks.delete.value)
        _assert_operation_match_cluster(op, REDIS_CLUSTER)

    def test_raise_when_not_matched(self, mocker):
        mock_logs(mocker)
        op = self.make_op(CLICKHOUSE_CLUSTER, ClickhouseTasks.delete.value)
        with pytest.raises(OperationTypeMismatchError):
            _assert_operation_match_cluster(op, REDIS_CLUSTER)

    def test_raise_when_no_type_in_task_and_not_matcher(self, mocker):
        mock_logs(mocker)
        mock_feature_flags(mocker)
        op = self.make_op(None, ClickhouseTasks.delete.value)
        with pytest.raises(OperationTypeMismatchError):
            _assert_operation_match_cluster(op, REDIS_CLUSTER)

    def test_raise_strange_case(self, mocker):
        mock_logs(mocker)
        mock_feature_flags(mocker)
        op = self.make_op(None, 'mssql_cluster_delete')

        with pytest.raises(OperationTypeMismatchError):
            _assert_operation_match_cluster(op, REDIS_CLUSTER)


class Test_assert_operation_responser_cache:
    # pylint: disable=no-member
    def test_when_cache_is_working(self):
        class EmptyClusterResponcer(_Cached):
            counter = 0

            def _get(self, key: tuple) -> Any:
                self.counter += 1
                return 'test'

        responser = EmptyClusterResponcer('clickhouse_cluster')
        assert_that(responser.counter, equal_to(0))
        assert_that(responser.get(('cid',)), equal_to('test'))
        assert_that(responser.counter, equal_to(1))
        assert_that(responser.get(('cid',)), equal_to('test'))
        assert_that(responser.counter, equal_to(1))
