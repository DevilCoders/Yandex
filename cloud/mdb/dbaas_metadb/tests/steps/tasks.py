"""
Tasks steps
"""

from behave import given, when

from cloud.mdb.dbaas_metadb.tests.helpers.cluster_changes import cluster_change
from cloud.mdb.dbaas_metadb.tests.helpers.queries import execute_query_in_transaction, verbose_query

ADD_OPERATION_Q = verbose_query(
    """
    SELECT * FROM code.add_operation(
        i_operation_id   => :task_id,
        i_folder_id      => :folder_id,
        i_cid            => :cid,
        i_task_type      => :task_type,
        i_operation_type => :task_type,
        i_task_args      => jsonb_build_object(),
        i_metadata       => jsonb_build_object(),
        i_user_id        => 'tester',
        i_version        => 42,
        i_delay_by       => :delay_by,
        i_rev            => :rev
    )
"""
)


@given('"{task_type:w}" task')
@when('I add "{task_type:w}" task')
@when('I add "{task_type:w}" task with "{task_id_var:w}" task_id')
@when('I add "{task_type:w}" task with "{delay_by}" delay_by and "{task_id_var:w}" task_id')
def step_add_task(context, task_type, task_id_var="task_id", delay_by=None):
    task_id = getattr(context, task_id_var)
    with cluster_change(context) as (trans, rev):
        execute_query_in_transaction(
            trans,
            ADD_OPERATION_Q,
            {
                "cid": context.cid,
                "folder_id": context.folder_id,
                "task_id": task_id,
                "task_type": task_type,
                "delay_by": delay_by,
                "rev": rev,
            },
        )


@given('that task acquired by worker')
@when('that task acquired by worker')
@when('"{task_id:w}" acquired by worker')
def step_acquire_task(context, task_id='task_id'):
    context.execute_steps(
        f'''
        Given successfully executed query
        """
        SELECT * FROM code.acquire_task(
            i_worker_id => 'Alice',
            i_task_id   => :{task_id}
        )
        """
        '''
    )


@when('that task released by worker')
def step_acquire_task(context):
    context.execute_steps(
        '''
        Given successfully executed query
        """
        SELECT * FROM code.release_task(
            i_worker_id => 'Alice',
            i_task_id   => :task_id,
            i_context   => jsonb_build_object()
        )
        """
        '''
    )


@given('that task finished by worker with result = {result:w}')
@when('that task finished by worker with result = {result:w}')
@when('"{task_id:w}" finished by worker with result = {result:w}')
def step_finish_tasks(context, result, task_id='task_id'):
    context.execute_steps(
        f'''
        Given successfully executed query
        """
        SELECT * FROM code.finish_task(
            i_worker_id => 'Alice',
            i_task_id   => :{task_id},
            i_result    => {result},
            i_changes   => jsonb_build_object(),
            i_comment   => ''
        )
        """
        '''
    )
