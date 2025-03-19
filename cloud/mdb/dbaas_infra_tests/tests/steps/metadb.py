"""
Steps related to Metadb
"""
import json
import time

from behave import given, register_type, then, when
from hamcrest import assert_that, has_entry, has_length
from parse_type import TypeBuilder

from tests.helpers.metadb import (create_maintenance_task, get_cluster_backups, get_cluster_tasks, get_first_backup,
                                  get_latest_version, get_task, get_version, set_backup_service_usage,
                                  set_cluster_pillar_value, set_delayed_until_to_now)
from tests.helpers.step_helpers import get_step_data
from tests.helpers.workarounds import retry

register_type(BackupServiceState=TypeBuilder.make_enum({
    'enabled': True,
    'disabled': False,
}))


@given('actual MetaDB version')
@retry(wait_fixed=1000, stop_max_attempt_number=300)
def wait_for_metadb(context):
    """
    Wait until DBaaS MetaDB has latest migrations version
    """
    current = get_version(context)
    latest = get_latest_version(context)
    msg = 'MetaDB has version {current}, latest {latest}'.format(
        current=current,
        latest=latest,
    )
    assert current == latest, msg


@then('in worker_queue exists "{task_type:w}" task')
def step_worker_queue_has_task(context, task_type):
    """
    Get task with `task_type` (include hidden) from worker_queue
    """
    tasks = get_cluster_tasks(
        context=context, task_type=task_type, folder_id=context.folder['folder_id'], cid=context.cluster['id'])
    assert_that(tasks, has_length(1))
    context.task_id = tasks[0]['operation_id']


@when('delayed_until time has come')
def step_set_delayed_until_to_now(context):
    set_delayed_until_to_now(
        context,
        context.task_id,
    )


METADB_TASK_DONE = 'DONE'
METADB_TASK_FAILED = 'FAILED'

METADB_TERMINAL_STATUSES = (
    METADB_TASK_DONE,
    METADB_TASK_FAILED,
)

METADB_BACKUP_DONE = 'DONE'
METADB_BACKUP_FAILED = 'CREATE-ERROR'

METADB_BACKUP_TERMINAL_STATUSES = (
    METADB_BACKUP_DONE,
    METADB_BACKUP_FAILED,
)
METADB_TASK_TIMEOUT = 60 * 3
METADB_BACKUP_TIMEOUT = 60 * 10


@then('this task is done')
def step_task_is_completed(context):
    deadline_to = time.time() + METADB_TASK_TIMEOUT
    task = None
    while time.time() < deadline_to:
        task = get_task(context=context, folder_id=context.folder['folder_id'], task_id=context.task_id)
        if task['status'] in METADB_TERMINAL_STATUSES:
            break
        time.sleep(2)
    assert_that(task, has_entry('status', METADB_TASK_DONE))


@then('initial backup task is done')
def step_initial_backup_is_completed(context):
    deadline_to = time.time() + METADB_BACKUP_TIMEOUT
    task = None
    while time.time() < deadline_to:
        task = get_first_backup(context=context, cid=context.cluster['id'])
        if task['status'] in METADB_BACKUP_TERMINAL_STATUSES:
            break
        time.sleep(2)
    assert_that(task, has_entry('status', METADB_BACKUP_DONE))


@given('backup service is {enabled:BackupServiceState}')
def step_set_backup_service(context, enabled):
    set_backup_service_usage(context, cid=context.cluster['id'], enabled=enabled)


@when('we create maintenance task for {cluster_type:ClusterType} cluster "{cluster_name}"')
def step_create_maintenance(context, cluster_type, cluster_name):
    task_args = json.dumps(get_step_data(context))
    context.operation_id = create_maintenance_task(context, cluster_type, cluster_name, task_args)


@given('cluster pillar path "{pillar_path}" set to "{pillar_value}"')
def cluster_pillar_value_set(context, pillar_path, pillar_value):
    """Updates cluster pillar value by path"""
    set_cluster_pillar_value(context, context.cluster['id'], pillar_path, pillar_value)


@then('mysql backups in metadb equals following')
def step_check_mysql_backups_in_metadb(context):
    backups = get_cluster_backups(context=context, cid=context.cluster['id'])
    i = 0
    for row in context.table:
        reg = row['host_region']
        backup = backups[i]
        hostname = backup['metadata']['meta']['Hostname']
        assert hostname.startswith(reg), "backup {num} is not from region {region} (backup host {act_region})".format(
            num=i, region=reg, act_region=hostname)
        i += 1
