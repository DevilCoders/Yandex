from collections import OrderedDict

from click import argument, ClickException, group, option, pass_context
from cloud.mdb.cli.common.formatting import print_response
from cloud.mdb.cli.common.parameters import DateTimeParamType, ListParamType
from cloud.mdb.cli.common.prompts import confirm_dangerous_action
from cloud.mdb.cli.dbaas.internal.db import db_transaction
from cloud.mdb.cli.dbaas.internal.intapi import ClusterType
from cloud.mdb.cli.dbaas.internal.metadb.maintenance_task import (
    cancel_maintenance_task,
    get_maintenance_task,
    get_maintenance_tasks,
    update_maintenance_task_plan_ts,
    update_maintenance_task_status,
)
from cloud.mdb.cli.dbaas.internal.metadb.task import get_task, is_terminated, reject_task, update_task
from cloud.mdb.cli.dbaas.internal.utils import cluster_status_options, DELETED_CLUSTER_STATUSES


@group('maintenance-task')
def maintenance_task_group():
    """Commands to manage maintenance tasks."""
    pass


@maintenance_task_group.command('get')
@argument('task_id', metavar='ID')
@pass_context
def get_command(ctx, task_id):
    """Get maintenance task."""
    print_response(ctx, get_maintenance_task(ctx, task_id))


@maintenance_task_group.command('list')
@option(
    '-t',
    '--task',
    '--tasks',
    'task_ids',
    type=ListParamType(),
    help='Filter objects to output by one or several worker tasks. Multiple values can be specified through a comma.',
)
@option('--cloud', 'cloud_id', help='Filter objects to output by cloud.')
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
    '--type',
    '--config-id',
    '--config',
    'type',
    help='Filter objects to output by maintenance type. The value can be a pattern to match with'
    ' the syntax of LIKE clause patterns.',
)
@option('--completed', is_flag=True, help='Output only completed maintenance tasks.')
@option('--planned', is_flag=True, help='Output only planned maintenance tasks.')
@option('--canceled', '--cancelled', 'canceled', is_flag=True, help='Output only canceled maintenance tasks.')
@option('--failed', 'failed', is_flag=True, help='Output only failed maintenance tasks.')
@option('-l', '--limit', type=int, default=1000, help='Limit the max number of objects in the output.')
@option('-q', '--quiet', is_flag=True, help='Output only maintenance task IDs.')
@option('-s', '--separator', help='Value separator for quiet mode.')
@pass_context
def list_maintenance_tasks_command(
    ctx,
    cluster_ids,
    task_ids,
    cluster_statuses,
    exclude_cluster_statuses,
    completed,
    planned,
    canceled,
    failed,
    limit,
    quiet,
    separator,
    **kwargs,
):
    """List maintenance tasks."""

    def _table_formatter(task):
        return OrderedDict(
            (
                ('task_id', task['task_id']),
                ('type', task['type']),
                ('status', task['status']),
                ('create_ts', task['create_ts']),
                ('plan_ts', task['plan_ts']),
                ('cluster_id', task['cluster_id']),
                ('env', task['cluster_env']),
                ('cluster_status', task['cluster_status']),
            )
        )

    statuses = []
    if completed:
        statuses.append('COMPLETED')
    if planned:
        statuses.append('PLANNED')
    if canceled:
        statuses.append('CANCELED')
    if failed:
        statuses.append('FAILED')
    statuses = statuses if statuses else None

    if exclude_cluster_statuses is None and not any((cluster_statuses, cluster_ids, task_ids)):
        exclude_cluster_statuses = DELETED_CLUSTER_STATUSES

    maintenance_tasks = get_maintenance_tasks(
        ctx,
        cluster_ids=cluster_ids,
        task_ids=task_ids,
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        statuses=statuses,
        limit=limit,
        **kwargs,
    )
    print_response(
        ctx,
        maintenance_tasks,
        default_format='table',
        table_formatter=_table_formatter,
        quiet=quiet,
        separator=separator,
        id_key='task_id',
    )


@maintenance_task_group.command('mark-completed')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def mark_completed_command(ctx, task_ids, force):
    """Mark one or several maintenance tasks as completed."""
    tasks = _get_tasks(ctx, task_ids)

    confirm_msg = 'You are going to perform potentially dangerous action and mark maintenance'
    confirm_msg += ' tasks as completed.' if len(task_ids) > 1 else f' task "{task_ids[0]}" as completed.'
    confirm_dangerous_action(confirm_msg, force)

    for task in tasks:
        if task['status'] == 'CANCELED':
            _complete_canceled_task(ctx, task)
            print(f'Maintenance task "{task["task_id"]}" marked completed')
        elif task['status'] == 'PLANNED':
            _complete_planned_task(ctx, task)
            print(f'Maintenance task "{task["task_id"]}" canceled and marked completed')
        elif task['status'] == 'COMPLETED':
            print(f'Maintenance task "{task["task_id"]}" is already completed')
        else:
            raise ClickException(
                f'Maintenance task "{task["task_id"]}" in "{task["status"]}" status cannot be marked completed'
            )


def _complete_canceled_task(ctx, maintenance_task):
    update_maintenance_task_status(ctx, maintenance_task['task_id'], status='COMPLETED')


def _complete_planned_task(ctx, maintenance_task):
    task_id = maintenance_task['task_id']
    with db_transaction(ctx, 'metadb'):
        task = get_task(ctx, task_id)
        if not is_terminated(task):
            reject_task(
                ctx,
                task_id,
                errors=[
                    {
                        "code": 1,
                        "type": "Cancelled",
                        "message": "The planned maintenance was canceled",
                        "exposable": True,
                    }
                ],
            )

        update_maintenance_task_status(ctx, task_id, status='COMPLETED')


@maintenance_task_group.command('cancel')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@option('--force', is_flag=True, help='Suppress confirmation prompts and sanity checks.')
@pass_context
def cancel_command(ctx, task_ids, force):
    """Cancel one or several maintenance tasks.

    NOTE: canceled tasks are eligible for rescheduling."""
    tasks = _get_tasks(ctx, task_ids)

    confirm_msg = 'You are going to perform potentially dangerous action and cancel maintenance'
    confirm_msg += ' tasks.' if len(task_ids) > 1 else f' task "{task_ids[0]}".'
    confirm_dangerous_action(confirm_msg, force)

    for task in tasks:
        if task['status'] == 'CANCELED':
            print(f'Maintenance task "{task["task_id"]}" is already canceled')
        else:
            cancel_maintenance_task(ctx, task['task_id'])
            print(f'Maintenance task "{task["task_id"]}" canceled')


@maintenance_task_group.command('reschedule')
@argument('task_ids', metavar='TASKS', type=ListParamType())
@argument('plan_ts', metavar='TIME', type=DateTimeParamType())
@pass_context
def reschedule_command(ctx, task_ids, plan_ts):
    """Reschedule one or several maintenance tasks."""
    tasks = _get_tasks(ctx, task_ids)
    for task in tasks:
        with db_transaction(ctx, 'metadb'):
            update_maintenance_task_plan_ts(ctx, task['task_id'], plan_ts=plan_ts)
            update_task(ctx, task['task_id'], delayed_ts=plan_ts)
            print(f'Maintenance task "{task["task_id"]}" was rescheduled')


def _get_tasks(ctx, task_ids):
    tasks = get_maintenance_tasks(ctx, task_ids=task_ids)

    if not tasks:
        raise ClickException('No maintenance tasks found.')

    if len(task_ids) != len(tasks):
        not_found_task_ids = set(task_ids) - set(task['task_id'] for task in tasks)
        if len(not_found_task_ids) == 1:
            raise ClickException(f'Maintenance task "{not_found_task_ids.pop()}" not found.')
        else:
            raise ClickException(f'Multiple maintenance tasks were not found: {",".join(not_found_task_ids)}')

    return tasks
