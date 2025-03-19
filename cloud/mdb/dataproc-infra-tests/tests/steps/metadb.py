"""
Steps related to Metadb
"""
import time

from behave import given, then, when
from hamcrest import assert_that, has_entry, has_length

from tests.helpers.metadb import (
    get_cluster_tasks,
    get_latest_version,
    get_masternode,
    get_task,
    get_version,
    remove_feature_flag,
    set_delayed_until_to_now,
    set_feature_flag,
)
from tests.helpers.workarounds import retry


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


@given('feature flag "{feature_flag:w}" is set')
def feature_flag_set(context, feature_flag):
    set_feature_flag(context, feature_flag)


@then('feature flag "{feature_flag:w}" is removed')
def feature_flag_removed(context, feature_flag):
    remove_feature_flag(context, feature_flag)


@when('masternode host is loaded into context')
def load_masternode_instance_id(context):
    if not hasattr(context, 'masternode_instance_id'):
        context.masternode_fqdn, context.masternode_instance_id = get_masternode(context)


@then('in worker_queue exists "{task_type:w}" task')
def step_worker_queue_has_task(context, task_type):
    """
    Get task with `task_type` (include hidden) from worker_queue
    """
    tasks = get_cluster_tasks(
        context=context, task_type=task_type, folder_id=context.folder['folder_id'], cid=context.cluster['id']
    )
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
METADB_TASK_TIMEOUT = 60 * 3


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
