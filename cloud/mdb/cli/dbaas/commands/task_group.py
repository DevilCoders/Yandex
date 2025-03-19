from collections import OrderedDict
from itertools import chain

from click import argument, ClickException, group, option, pass_context
from cloud.mdb.cli.common.formatting import (
    print_response,
    print_header,
)
from cloud.mdb.cli.common.parameters import JsonParamType, ListParamType, DateTimeParamType, NullableParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.common.utils import ensure_at_least_one_option, is_not_null
from cloud.mdb.cli.dbaas.internal.config import get_config
from cloud.mdb.cli.dbaas.internal.db import db_transaction
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.metadb.cluster import cluster_lock, get_clusters
from cloud.mdb.cli.dbaas.internal.metadb.task import (
    acquire_task,
    create_task,
    finish_task,
    format_task_args,
    format_task_changes,
    format_task_tracing,
    get_task,
    get_tasks,
    ParallelTaskExecuter,
    reject_task,
    restart_task,
    TaskNotFound,
    update_task,
    wait_task,
    get_task_run_history,
    cancel_task,
)
from cloud.mdb.cli.dbaas.internal.utils import cluster_status_options, format_references


@group('task')
def task_group():
    """Task management commands."""
    pass


@task_group.command('get')
@argument('untyped_id', metavar='ID')
@pass_context
def get_command(ctx, untyped_id):
    """Get task.

    For getting task by related object, ID argument accepts tracing ID in addition to task ID.
    """
    try:
        task = get_task(ctx, task_id=untyped_id)
    except TaskNotFound:
        task = get_task(ctx, tracing_id=untyped_id)

    result = OrderedDict((key, value) for key, value in task.items())
    result['task_args'] = format_task_args(task['task_args'])
    result['changes'] = format_task_changes(ctx, task['changes'])
    result['tracing'] = format_task_tracing(ctx, task['tracing'])
    result['references'] = _format_references(ctx, task)

    print_response(ctx, result)


def _format_references(ctx, task):
    macros = {
        'task_id': task['task_id'],
    }
    return format_references(ctx, 'task_references', macros)


@task_group.command('list')
@option('-f', '--folder', 'folder_id', help='Filter objects to output by folder.')
@option(
    '-c',
    '--cluster',
    '--clusters',
    'cluster_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several clusters. Multiple values can be specified through a comma.',
)
@option(
    '--cluster-type',
    '--cluster-types',
    'cluster_types',
    type=ListParamType(ClusterType()),
    help='Filter objects to output by one or several cluster types. Multiple values can be specified through a comma.',
)
@option('-e', '--env', '--cluster-env', 'cluster_env', help='Filter objects to output by cluster environment.')
@cluster_status_options()
@option(
    '-H',
    '--host',
    '--hosts',
    'hostnames',
    type=ListParamType(),
    help='Filter objects to output by one or several host names. Matching is performed by task args.',
)
@option('-t', '--task', '--tasks', 'task_ids', type=ListParamType(), help='Filter objects to output by tasks IDs.')
@option(
    '--type',
    '--task-type',
    'task_type',
    help='Filter objects to output by task type. The value can be a pattern to match with'
    ' the syntax of LIKE clause patterns.',
)
@option(
    '--created-by',
    type=ListParamType(),
    help='Filter objects to output by created_by field. Multiple values can be specified through a comma.',
)
@option(
    '--exclude-created-by',
    type=ListParamType(),
    help='Filter objects to not output by created_by field. Multiple values can be specified through a comma.',
)
@option('--pending', is_flag=True, help='Output only not finished tasks.')
@option('--failed', is_flag=True, help='Output only failed tasks.')
@option('--hidden', is_flag=True, default=None, help='Output only hidden tasks.')
@option('--visible', is_flag=True, default=None, help='Output only visible (non-hidden) tasks.')
@option(
    '--canceled/--no-canceled',
    '--cancelled/--no-cancelled',
    'canceled',
    default=None,
    help='Output only canceled/not canceled tasks.',
)
@option(
    '--downtimed',
    is_flag=True,
    default=None,
    help='Output only downtimed tasks (failed tasks with overridden worker_id).',
)
@option('--maintenance', is_flag=True, help='Output only maintenance tasks.')
@option('--maintenance-type', 'maintenance_type', help='Output only maintenance tasks with the specified type.')
@option('--rev', '--revision', 'revision', type=int)
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only task IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_command(ctx, hidden, visible, canceled, cluster_ids, failed, downtimed, limit, quiet, separator, **kwargs):
    """List tasks."""

    def _filter(task):
        if canceled is not None:
            task_canceled = False
            for error in task['errors'] or []:
                if error['type'] == 'Cancelled':
                    task_canceled = True

            return canceled == task_canceled

        return True

    if hidden and visible:
        ctx.fail('Options --hidden and --visible cannot be used together.')

    if visible:
        hidden = False

    if cluster_ids:
        clusters = get_clusters(ctx, cluster_ids=cluster_ids)
        cluster_ids = [c['id'] for c in clusters]

    exclude_worker_ids = None
    if downtimed:
        exclude_worker_ids = get_config(ctx)['worker']['hosts']
        failed = True

    tasks = get_tasks(
        ctx,
        cluster_ids=cluster_ids,
        exclude_worker_ids=exclude_worker_ids,
        failed=failed,
        hidden=hidden,
        limit=limit,
        **kwargs,
    )
    tasks = [task for task in tasks if _filter(task)]
    for task in tasks:
        task['changes'] = format_task_changes(ctx, task['changes'])

    print_response(
        ctx,
        tasks,
        default_format='table',
        fields=[
            'task_id',
            'task_type',
            'create_ts',
            'create_rev',
            'finish_ts',
            'finish_rev',
            'cluster_id',
            'result',
        ],
        quiet=quiet,
        separator=separator,
        id_key='task_id',
    )


@task_group.command('restart-history')
@argument('untyped_id', metavar='ID')
@pass_context
def restart_history_command(ctx, untyped_id):
    """Get task restart history."""
    try:
        task = get_task(ctx, task_id=untyped_id)
    except TaskNotFound:
        task = get_task(ctx, tracing_id=untyped_id)

    restart_history = get_task_run_history(ctx, task['task_id'])
    for attempt in chain(restart_history, [task]):
        print_header(f'Attempt {attempt["restart_count"] + 1}')
        print_response(ctx, _format_restart_attempt(ctx, attempt))


def _format_restart_attempt(ctx, attempt):
    result = OrderedDict()
    result['start_ts'] = attempt['start_ts']
    result['restart_count'] = attempt['restart_count']
    result['notes'] = attempt['notes']
    result['worker_id'] = attempt['worker_id']
    result['acquire_rev'] = attempt['acquire_rev']
    result['finish_ts'] = attempt['finish_ts']
    result['finish_rev'] = attempt['finish_rev']
    result['result'] = attempt['result']
    result['errors'] = attempt['errors']
    result['changes'] = format_task_changes(ctx, attempt['changes'])
    result['comment'] = attempt['comment']
    return result


@task_group.command('create')
@argument('cluster_id', metavar='CLUSTER')
@argument('task_type')
@argument('operation_type')
@argument('task_args')
@option('--hidden', is_flag=True, help='Mark creating task as hidden. Hidden tasks are not visible to end users.')
@pass_context
def create_command(ctx, cluster_id, task_type, operation_type, task_args, hidden):
    """Create task."""
    with cluster_lock(ctx, cluster_id):
        task_id = create_task(
            ctx,
            cluster_id=cluster_id,
            task_type=task_type,
            operation_type=operation_type,
            task_args=task_args,
            hidden=hidden,
            wait=False,
        )

    wait_task(ctx, task_id)


@task_group.command('restart')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@option(
    '--restart/--no-restart',
    'restart',
    default=None,
    help='Enable/Disable restart during task execution. If not specified, task configuration is left intact.',
)
@option(
    '--health/--no-health',
    '--health-check/--no-health-check',
    'health_check',
    default=None,
    help='Enable/Disable health checks during task execution. If not specified, task configuration is left intact.',
)
@option(
    '--fast-mode/--no-fast-mode',
    '--fast/--no-fast',
    'fast_mode',
    default=None,
    help='Enable/Disable fast mode. If enabled, task is executed with maximum parallelism level.'
    ' Applicable for ClickHouse clusters only for now. If not specified, task configuration is left intact.',
)
@option('--timeout', help='Update task timeout before restart. If not specified, task configuration is left intact.')
@option(
    '--delayed',
    'delayed_ts',
    type=NullableParamType(DateTimeParamType()),
    help='Set new value for task delay timestamp. The value "null" unsets delay.'
    ' If not specified, task configuration is left intact.',
)
@option(
    '--task-arg',
    'task_arg_kv',
    type=(str, JsonParamType()),
    help='Update task argument with the specified key and value before restart.'
    ' If not specified, task configuration is left intact.',
)
@option('-p', '--parallel', type=int, default=8, help='Maximum number of tasks to run in parallel.')
@option('-k', '--keep-going', is_flag=True, help='Do not stop on the first failed task.')
@pass_context
def restart_command(
    ctx, task_ids, force, restart, health_check, fast_mode, timeout, delayed_ts, task_arg_kv, parallel, keep_going
):
    """Restart one or several tasks."""

    def _restart(task):
        with db_transaction(ctx, 'metadb'):
            task_id = task['task_id']

            changed = False
            task_args = task['task_args']
            if restart is not None:
                task_args['restart'] = restart
                changed = True
            if health_check is not None:
                task_args['disable_health_check'] = not health_check
                changed = True
            if fast_mode is not None:
                task_args['fast_mode'] = fast_mode
                changed = True

            if task_arg_kv is not None:
                key, value = task_arg_kv
                task_args[key] = value
                changed = True

            if changed or timeout or delayed_ts:
                update_task(ctx, task_id, timeout=timeout, delayed_ts=delayed_ts, task_args=task_args)

            restart_task(ctx, task_id, force=force)

            if len(task_ids) > 1:
                print(f'Task "{task_id}" restarted')

            return task_id

    sync_mode = False if is_not_null(delayed_ts) else True
    executer = ParallelTaskExecuter(ctx, parallel, keep_going=keep_going, wait=sync_mode)
    for task in _get_tasks(ctx, task_ids):
        executer.submit(_restart, task)

    completed, failed = executer.run()

    if failed and len(task_ids) > 1:
        raise ClickException(f'Failed tasks: {", ".join(failed)}')


@task_group.command('mark-completed')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def mark_completed_command(ctx, task_ids, force):
    """Mark one or several tasks as completed."""
    tasks = _get_tasks(ctx, task_ids)

    confirm_msg = 'You are going to perform potentially dangerous action and mark'
    confirm_msg += ' tasks as completed.' if len(task_ids) > 1 else f' task "{task_ids[0]}" as completed.'
    confirm_msg += (
        ' It can make actual cluster state out of sync with metadb state. In this case synchronization'
        ' must be performed manually. Depending on task type it may include: deploy configuration changes,'
        ' creation of compute instances, registration of hosts in conductor, etc.'
    )
    confirm_dangerous_action(confirm_msg, force)

    for task in tasks:
        task_id = task['task_id']
        finish_task(ctx, task_id, result=True, force=force)
        print(f'Task "{task["task_id"]}" was updated.')


@task_group.command('mark-failed')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def mark_failed_command(ctx, task_ids, force):
    """Mark one or several tasks as failed."""
    tasks = _get_tasks(ctx, task_ids)

    confirm_msg = 'You are going to perform potentially dangerous action and mark'
    confirm_msg += ' tasks as failed.' if len(task_ids) > 1 else f' task "{task_ids[0]}" as failed.'
    confirm_msg += (
        ' It can make actual cluster state out of sync with metadb state. In this case synchronization'
        ' must be performed manually. Depending on task type it may include: deploy configuration changes,'
        ' creation of compute instances, registration of hosts in conductor, etc.'
    )
    confirm_dangerous_action(confirm_msg, force)

    for task in tasks:
        task_id = task['task_id']
        finish_task(ctx, task_id, result=False, force=force)
        print(f'Task "{task_id}" was updated.')


@task_group.command('acquire')
@argument('task_id', metavar='TASK')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def acquire_command(ctx, task_id, force):
    """Acquire task."""
    confirm_dangerous_action(
        f'You are going to perform potentially dangerous action and acquire task "{task_id}". You will have to manually'
        f' switch the task to terminal state or restart it. Otherwise the task will hang forever.',
        force,
    )

    acquire_task(ctx, task_id, force=force)


@task_group.command('cancel')
@argument('task_id', metavar='TASK')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def cancel_command(ctx, task_id, force):
    """Cancel task."""
    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    cancel_task(ctx, task_id)
    print(f'Cancel flag has been set for task {task_id}, awaiting task termination.')
    wait_task(ctx, task_id)


@task_group.command('reject')
@argument('task_id', metavar='TASK')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def reject_command(ctx, task_id, force):
    """Reject task."""
    confirm_dangerous_action(
        f'You are going to perform potentially dangerous action and reject task "{task_id}". It only reverts cluster'
        f' state in metadb. Synchronization of actual cluster state with metadb must be performed manually.'
        f' Depending on task type it may include: deploy configuration changes, creation of compute instances,'
        f' registration of hosts in conductor, etc.',
        force,
    )

    reject_task(ctx, task_id, force=force)


@task_group.command('downtime')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@option('--comment', default='juggler check downtimed')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def downtime_command(ctx, task_ids, comment, force):
    """Downtime one or several tasks.

    Internally it overrides task worker_id.
    """
    tasks = _get_tasks(ctx, task_ids)

    for task in tasks:
        task_id = task['task_id']
        if task['result'] is not False:
            raise ClickException(f'Task "{task_id}" cannot be downtimed as it\'s not failed.')

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    worker_hosts = get_config(ctx)['worker']['hosts']

    for task in tasks:
        task_id = task['task_id']

        if task['worker_id'] in worker_hosts:
            new_worker_id = task['worker_id'] + ';' + comment
            update_task(ctx, task_id=task_id, worker_id=new_worker_id)
            print(f'Task "{task_id}" was downtimed.')
        else:
            print(f'Task "{task_id}" already downtimed.')


@task_group.command('update')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@option('--hidden', type=bool, help='Mark the task as hidden. Hidden tasks are not visible to end users.')
@option('--timeout', help='Set new task timeout.')
@option(
    '--delayed',
    'delayed_ts',
    type=NullableParamType(DateTimeParamType()),
    help='Set new value for task delay timestamp. The value "null" unsets delay.',
)
@option('--task-args', type=JsonParamType(), help='Set new value for task arguments.')
@option('--task-arg', 'task_arg_kv', type=(str, JsonParamType()), help='Set new value for the specified task argument.')
@option('--worker-id', help='Set new worker ID.')
@option('--errors', type=JsonParamType(), help='Set new value for task errors.')
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def update_command(ctx, task_ids, force, task_arg_kv, task_args, **kwargs):
    """Update one or several tasks."""
    tasks = _get_tasks(ctx, task_ids)

    ensure_at_least_one_option(
        ctx,
        (task_arg_kv, task_args, *kwargs.values()),
        'At least one of --hidden, --no-delayed-ts, --timeout, --task-arg, --task-args, --worker-id or --errors'
        ' option must be specified.',
    )

    confirm_dangerous_action('You are going to perform potentially dangerous action.', force)

    for task in tasks:
        task_id = task['task_id']
        if task_args is None:
            task_args = task['task_args']
        if task_arg_kv is not None:
            key, value = task_arg_kv
            task_args[key] = value

        update_task(ctx, task_id=task_id, task_args=task_args, **kwargs)
        print(f'Task "{task_id}" was updated.')


@task_group.command('wait')
@argument('task_id', metavar='TASK')
@pass_context
def wait_command(ctx, task_id):
    """Wait for task completion."""
    wait_task(ctx, task_id)


def _get_tasks(ctx, task_ids):
    tasks = get_tasks(ctx, task_ids=task_ids)

    if not tasks:
        raise ClickException('No tasks found.')

    if len(task_ids) != len(tasks):
        not_found_task_ids = set(task_ids) - set(task['task_id'] for task in tasks)
        if len(not_found_task_ids) == 1:
            raise ClickException(f'Task "{not_found_task_ids.pop()}" not found.')
        else:
            raise ClickException(f'Multiple tasks were not found: {",".join(not_found_task_ids)}')

    return tasks
