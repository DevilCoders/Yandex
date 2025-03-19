"""
Task tests utils
"""

import logging

from cloud.mdb.internal.python.compute.disks import DiskType
from copy import deepcopy
from hamcrest import assert_that, contains_inanyorder, equal_to, is_not, starts_with, has_item

from cloud.mdb.dbaas_worker.internal.messages import TaskUpdate
from cloud.mdb.dbaas_worker.internal.tasks.clickhouse.resetup import TaskType as clickhouse_tasks
from cloud.mdb.dbaas_worker.internal.tasks.common.base import SKIP_LOCK_CREATION_ARG
from cloud.mdb.dbaas_worker.internal.tasks.elasticsearch.resetup import TaskType as elastic_tasks
from cloud.mdb.dbaas_worker.internal.tasks.kafka.resetup import TaskType as kafka_tasks
from cloud.mdb.dbaas_worker.internal.tasks.mongodb.resetup import TaskType as mongo_tasks
from cloud.mdb.dbaas_worker.internal.tasks.greenplum.resetup import TaskType as greenplum_tasks
from cloud.mdb.dbaas_worker.internal.tasks.mysql.resetup import TaskType as mysql_tasks
from cloud.mdb.dbaas_worker.internal.tasks.postgresql.resetup import TaskType as postgrest_tasks
from cloud.mdb.dbaas_worker.internal.tasks.redis.resetup import TaskType as redis_tasks
from test.mocks import checked_run_task_with_mocks, run_task_with_mocks


def get_task_host(**fields):
    """
    Get base host for task args
    """
    ret = dict(
        geo='geo1',
        space_limit=10737418240,
        subnet_id=None,
        platform_id=None,
        cpu_guarantee=1,
        cpu_limit=1,
        gpu_limit=0,
        memory_guarantee=1073741824,
        memory_limit=1073741824,
        network_guarantee=1048576,
        network_limit=1048576,
        io_limit=1048576,
        io_cores_limit=0,
        flavor='test-flavor',
        disk_type_id=None,
        vtype=None,
        vtype_id=None,
        assign_public_ip=None,
        subcid='subcid-test',
        shard_id=None,
        shard_name=None,
        roles=None,
        environment='qa',
        host_group_ids=[],
    )

    ret.update(fields)

    return ret


def get_porto_task_host(**fields):
    """
    Get porto host for task args
    """
    ret = get_task_host()

    ret.update(
        dict(
            platform_id='1',
            disk_type_id='local-ssd',
            vtype='porto',
            subnet_id='0:1589',
        )
    )

    ret.update(fields)

    return ret


def get_compute_task_host(**fields):
    """
    Get compute host for task args
    """
    ret = get_task_host()

    ret.update(
        dict(
            platform_id='mdb-v1',
            disk_type_id=DiskType.network_ssd_nonreplicated.value,
            vtype='compute',
            subnet_id='test-subnet',
            assign_public_ip=False,
        )
    )

    ret.update(fields)

    return ret


def prepare_state(old_state: dict) -> dict:
    new_state = deepcopy(old_state)
    new_state['actions'] = []
    new_state['set_actions'] = set()
    new_state['cancel_actions'] = set()
    new_state['fail_actions'] = set()
    return new_state


def check_task_interrupt_consistency(
    mocker, task_type, task_args, initial_state, feature_flags=None, ignore=None, task_params=None
):
    """
    Check that task produces the same change set on interruption
    """
    start_state = prepare_state(initial_state)
    if ignore is None:
        ignore = []

    reference_messages, _, reference_state = checked_run_task_with_mocks(
        mocker,
        task_type,
        task_args,
        feature_flags=feature_flags,
        state=start_state,
        task_params=task_params,
    )

    reference_changes = set()
    ref_ordered = []
    for message in reference_messages:
        if isinstance(message, TaskUpdate):
            if message.changes:
                for change in [f'{k}:{v}' for k, v in message.changes.items() if k != 'timestamp']:
                    ref_ordered.append(change)
                    reference_changes.add(change)

    for change in ignore:
        if change not in reference_changes:
            ref_ordered.append(change)
            reference_changes.add(change)

    logging.info('Reference state: %s', reference_state)
    assert reference_state['actions'], f'No actions recorded on {task_type} execution'

    logging.info('Recorded actions: %s', ', '.join(reference_state['actions']))
    action = reference_state['actions'][-1]
    logging.info('Checking with interrupt on %s', action)
    state = prepare_state(initial_state)
    state['cancel_actions'].add(action)

    _, context, intermediate_state = checked_run_task_with_mocks(
        mocker,
        task_type,
        task_args,
        feature_flags=feature_flags,
        state=state,
        task_params=task_params,
    )

    intermediate_state['cancel_actions'] = set()

    check_messages, _, interrupted_state = checked_run_task_with_mocks(
        mocker,
        task_type,
        task_args,
        context=context,
        feature_flags=feature_flags,
        state=intermediate_state,
        task_params=task_params,
    )

    check_changes = set()
    check_ord = []
    for message in check_messages:
        if isinstance(message, TaskUpdate):
            if message.changes:
                for change in [f'{k}:{v}' for k, v in message.changes.items() if k != 'timestamp']:
                    check_ord.append(change)
                    check_changes.add(change)

    for change in ignore:
        if change not in check_changes:
            check_ord.append(change)
            check_changes.add(change)

    assert_that(
        set(check_changes),
        contains_inanyorder(*set(reference_changes)),
        f'Got unexpected difference on {action} interrupt',
    )

    return interrupted_state


TASKS_TAKE_LOCK_AT = {
    postgrest_tasks.postgres_online: 1,
    postgrest_tasks.postgres_offline: 1,
    mysql_tasks.mysql_offline: 1,
    mysql_tasks.mysql_online: 1,
    redis_tasks.redis_online: 1,
    redis_tasks.redis_offline: 1,
    kafka_tasks.kafka_offline: 1,
    kafka_tasks.kafka_online: 1,
    clickhouse_tasks.clickhouse_online: 1,
    clickhouse_tasks.clickhouse_offline: 1,
    elastic_tasks.elastic_online: 1,
    elastic_tasks.elastic_offline: 1,
    mongo_tasks.mongo_online: 1,
    mongo_tasks.mongo_offline: 1,
    greenplum_tasks.greenplum_online: 1,
    greenplum_tasks.greenplum_offline: 1,
}


def check_alerts_synchronised(mocker, task_type, task_args, initial_state):
    start_state = prepare_state(initial_state)

    *_, state = checked_run_task_with_mocks(mocker, task_type, task_args, state=start_state)

    assert state['actions'], f'No actions recorded on {task_type} execution'

    assert_that(
        state['actions'],
        has_item(starts_with('solomon-alert.')),
        f'Should have produced action in {task_type=}, touching solomon API',
    )


def check_mlock_usage(mocker, task_type, task_args, initial_state, feature_flags=None):
    """
    Check that task uses mlock correctly
    """
    start_state = prepare_state(initial_state)

    *_, state = checked_run_task_with_mocks(
        mocker, task_type, task_args, feature_flags=feature_flags, state=start_state
    )

    assert state['actions'], f'No actions recorded on {task_type} execution'

    mlock_taken_expected_position = TASKS_TAKE_LOCK_AT.get(task_type, 0)

    assert_that(
        state['actions'][mlock_taken_expected_position],
        equal_to('mlock-create-dbaas-worker-cid-test'),
        f'Action #{mlock_taken_expected_position} in {task_type} should be cluster lock',
    )

    assert_that(
        state['actions'][-1],
        equal_to('mlock-release-dbaas-worker-cid-test'),
        f'Last action in {task_type} should be cluster unlock',
    )


def check_mlock_skipped(mocker, task_type, task_args, initial_state, feature_flags=None):
    start_state = prepare_state(initial_state)

    task_args_for_skipping = deepcopy(task_args)
    task_args_for_skipping[SKIP_LOCK_CREATION_ARG] = True

    *_, state = checked_run_task_with_mocks(
        mocker, task_type, task_args_for_skipping, feature_flags=feature_flags, state=start_state
    )

    assert_that(
        state['actions'],
        is_not(contains_inanyorder(['mlock-create-dbaas-worker-cid-test'])),
        f'No lock action in {task_type} should be present when SKIP_LOCK_CREATION_ARG used',
    )
    assert_that(
        state['actions'][-1],
        is_not(contains_inanyorder(['mlock-release-dbaas-worker-cid-test'])),
        f'No lock action in {task_type} should be present when SKIP_LOCK_CREATION_ARG used',
    )


def check_rejected(mocker, task_type, task_args, initial_state, feature_flags=None):
    """
    Check that task is rejected
    """
    start_state = deepcopy(initial_state)
    start_state['actions'] = []
    start_state['set_actions'] = set()
    start_state['cancel_actions'] = set()

    if not feature_flags:
        feature_flags = []

    messages, _, state = run_task_with_mocks(mocker, task_type, task_args, {}, feature_flags, start_state)

    assert state['actions'], f'No actions recorded on {task_type} execution'

    logging.info('Recorded actions: %s', ', '.join(state['actions']))

    assert_that(messages[-1].rejected, equal_to(True), 'Task should be rejected {}'.format(messages[-1]))


def check_dbm_transfer_called(mocker, task_type, task_args, initial_state, feature_flags=None, context=None):
    start_state = prepare_state(initial_state)

    *_, state = checked_run_task_with_mocks(
        mocker,
        task_type,
        task_args,
        feature_flags=feature_flags,
        state=start_state,
        context=context,
    )

    assert (
        'dbm-container-modify-foo.mdb.yacloud.net' in state['actions']
    ), f'Porto container modify action should be present in {task_type}'

    assert (
        'dbm-finish-transfer-1' in state['actions']
    ), f'Porto container finish transfer action should be present in {task_type}'


def check_dbm_without_transfer(mocker, task_type, task_args, initial_state, feature_flags=None, context=None):
    start_state = prepare_state(initial_state)

    *_, state = checked_run_task_with_mocks(
        mocker,
        task_type,
        task_args,
        feature_flags=feature_flags,
        state=start_state,
        context=context,
    )

    assert (
        'dbm-container-modify-foo.mdb.yacloud.net' not in state['actions']
    ), f'Porto container modify action should not be present in {task_type}'

    assert (
        'dbm-finish-transfer-1' not in state['actions']
    ), f'Porto container finish transfer action should not be present in {task_type}'


def check_compute_delete_instance_called(
    mocker, task_type, task_args, initial_state, feature_flags=None, instance_name='instance-1'
):
    start_state = prepare_state(initial_state)

    *_, state = checked_run_task_with_mocks(
        mocker,
        task_type,
        task_args,
        feature_flags=feature_flags,
        state=start_state,
    )

    assert (
        'compute-delete-' + instance_name in state['actions']
    ), f'Compute instance delete action should be present in {task_type}'


def check_compute_delete_instance_not_called(mocker, task_type, task_args, initial_state, feature_flags=None):
    start_state = prepare_state(initial_state)

    *_, state = checked_run_task_with_mocks(
        mocker,
        task_type,
        task_args,
        feature_flags=feature_flags,
        state=start_state,
    )

    assert (
        'compute-delete-instance-1' not in state['actions']
    ), f'Compute instance delete action should not be present in {task_type}'
