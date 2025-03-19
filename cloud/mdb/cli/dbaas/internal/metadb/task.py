import json
import os
import sys
import time
from collections import deque, OrderedDict
from datetime import timezone
from getpass import getuser

from click import ClickException

from cloud.mdb.cli.common.formatting import format_timestamp, print_response
from cloud.mdb.cli.common.utils import parse_timestamp, flatten_nullable
from cloud.mdb.cli.dbaas.internal.common import to_overlay_fqdn
from cloud.mdb.cli.dbaas.internal.config import config_option
from cloud.mdb.cli.dbaas.internal.db import db_query, db_transaction, MultipleRecordsError
from cloud.mdb.cli.dbaas.internal.metadb.common import to_db_cluster_types
from cloud.mdb.cli.dbaas.internal.metadb.exceptions import TaskIsInProgress, TaskNotFound
from cloud.mdb.cli.dbaas.internal.utils import dry_run_mode, generate_id, get_polling_interval


def get_task(ctx, task_id=None, *, tracing_id=None):
    """
    Get task from metadb.
    """
    tasks = get_tasks(ctx, task_ids=[task_id] if task_id else None, tracing_id=tracing_id)

    if not tasks:
        raise TaskNotFound(task_id)

    if len(tasks) > 1:
        raise MultipleRecordsError()

    return tasks[0]


def get_tasks(
    ctx,
    *,
    folder_id=None,
    cluster_ids=None,
    task_ids=None,
    task_type=None,
    pending=None,
    failed=None,
    maintenance=None,
    maintenance_type=None,
    worker_ids=None,
    exclude_worker_ids=None,
    hostnames=None,
    hidden=None,
    created_by=None,
    exclude_created_by=None,
    cluster_env=None,
    cluster_types=None,
    cluster_statuses=None,
    exclude_cluster_statuses=None,
    tracing_id=None,
    revision=None,
    restart_count=None,
    limit=None,
):
    if hostnames:
        hostnames = [to_overlay_fqdn(ctx, fqdn) for fqdn in hostnames]

    query = """
        SELECT
            t.task_id,
            t.task_type,
            t.operation_type,
            mt.config_id "maintenance_type",
            t.hidden,
            t.create_ts,
            t.created_by,
            t.create_rev,
            t.target_rev,
            t.delayed_until"delayed_ts",
            t.start_ts "start_ts",
            t.restart_count,
            t.notes,
            t.worker_id,
            t.acquire_rev,
            t.cancelled,
            t.end_ts "finish_ts",
            t.finish_rev,
            t.result,
            t.errors,
            t.metadata,
            t.version,
            t.task_args,
            t.timeout,
            t.changes,
            t.context,
            t.comment,
            t.tracing,
            t.cid "cluster_id"
        FROM dbaas.worker_queue t
        JOIN dbaas.clusters c ON (c.cid = t.cid)
        JOIN dbaas.folders f ON (f.folder_id = t.folder_id)
        LEFT JOIN dbaas.maintenance_tasks mt USING (task_id)
        WHERE true
        {% if folder_id %}
          AND f.folder_ext_id = %(folder_id)s
        {% endif %}
        {% if cluster_ids is not none %}
          AND t.cid = ANY(%(cluster_ids)s)
        {% endif %}
        {% if task_ids is not none %}
          AND t.task_id = ANY(%(task_ids)s)
        {% endif %}
        {% if task_type %}
          AND t.task_type::text LIKE %(task_type)s
        {% endif %}
        {% if pending %}
          AND t.result IS NULL
        {% endif %}
        {% if failed %}
          AND NOT t.result
        {% endif %}
        {% if hidden is not none %}
          AND t.hidden = %(hidden)s
        {% endif %}
        {% if created_by is not none %}
          AND t.created_by = ANY(%(created_by)s)
        {% endif %}
        {% if exclude_created_by is not none %}
          AND t.created_by != ALL(%(exclude_created_by)s)
        {% endif %}
        {% if maintenance %}
          AND mt.config_id IS NOT NULL
        {% endif %}
        {% if maintenance_type %}
          AND mt.config_id LIKE %(maintenance_type)s
        {% endif %}
        {% if worker_ids is not none %}
          AND t.worker_id = ANY(%(worker_ids)s)
        {% endif %}
        {% if exclude_worker_ids is not none %}
          AND t.worker_id != ALL(%(exclude_worker_ids)s)
        {% endif %}
        {% if hostnames is not none %}
          AND (t.task_args#>>'{host}' = ANY(%(hostnames)s) OR t.task_args#>>'{host,fqdn}' = ANY(%(hostnames)s))
        {% endif %}
        {% if cluster_env %}
          AND c.env::text = %(cluster_env)s
        {% endif %}
        {% if cluster_types %}
          AND c.type = ANY(%(cluster_types)s::dbaas.cluster_type[])
        {% endif %}
        {% if cluster_statuses %}
          AND c.status = ANY(%(cluster_statuses)s::dbaas.cluster_status[])
        {% endif %}
        {% if exclude_cluster_statuses %}
          AND c.status != ALL(%(exclude_cluster_statuses)s::dbaas.cluster_status[])
        {% endif %}
        {% if tracing_id %}
          AND split_part(t.tracing::json#>>'{"uber-trace-id"}', ':', 1) = %(tracing_id)s
        {% endif %}
        {% if revision %}
          AND %(revision)s >= t.create_rev
          AND (t.finish_rev IS NULL OR %(revision)s <= t.finish_rev)
        {% endif %}
        {% if restart_count is not none %}
          AND restart_count = %(restart_count)s
        {% endif %}
        ORDER BY t.create_ts DESC
        {% if limit %}
        LIMIT {{ limit }}
        {% endif %}
        """
    return db_query(
        ctx,
        'metadb',
        query,
        folder_id=folder_id,
        cluster_ids=cluster_ids,
        task_ids=task_ids,
        task_type=task_type,
        pending=pending,
        failed=failed,
        hidden=hidden,
        maintenance=maintenance,
        maintenance_type=maintenance_type,
        worker_ids=worker_ids,
        exclude_worker_ids=exclude_worker_ids,
        hostnames=hostnames,
        created_by=created_by,
        exclude_created_by=exclude_created_by,
        cluster_env=cluster_env,
        cluster_types=to_db_cluster_types(cluster_types),
        cluster_statuses=cluster_statuses,
        exclude_cluster_statuses=exclude_cluster_statuses,
        tracing_id=tracing_id,
        revision=revision,
        restart_count=restart_count,
        limit=limit,
    )


def get_task_run(ctx, task_id, restart_count):
    """
    Get task run with the specified restart count value.
    """
    tasks = get_tasks(ctx, task_ids=[task_id], restart_count=restart_count)
    if tasks:
        return tasks[0]

    history = get_task_run_history(ctx, task_id=task_id, restart_count=restart_count)
    if history:
        return history[0]

    raise ClickException(f'Task run with restart count {restart_count} not found for task {task_id}.')


def get_task_run_history(ctx, task_id, restart_count=None):
    """
    Get task restart history from metadb.
    """
    query = """
        SELECT
            task_id,
            task_type,
            operation_type,
            result,
            cancelled,
            create_ts,
            create_rev,
            created_by,
            target_rev,
            start_ts "start_ts",
            timeout,
            worker_id,
            acquire_rev,
            end_ts "finish_ts",
            finish_rev,
            metadata,
            version,
            hidden,
            task_args,
            changes,
            context,
            comment,
            notes,
            errors,
            restart_count,
            tracing,
            cid "cluster_id"
        FROM dbaas.worker_queue_restart_history
        WHERE task_id = %(task_id)s
        {% if restart_count is not none %}
          AND restart_count = %(restart_count)s
        {% endif %}
        ORDER BY restart_count
        """
    return db_query(
        ctx,
        'metadb',
        query,
        task_id=task_id,
        restart_count=restart_count,
    )


def create_task(
    ctx,
    *,
    cluster_id,
    task_type,
    operation_type,
    task_args=None,
    metadata=None,
    hidden=False,
    wait=False,
    timeout=None,
    delay=None,
    revision=None,
):
    if not task_args:
        task_args = {}

    metadata = json.dumps(metadata) if metadata else '{}'

    task_id = generate_id(ctx)
    db_query(
        ctx,
        'metadb',
        """
        WITH cluster AS (
            SELECT
                cid,
                folder_id,
                code.rev(c) rev
            FROM dbaas.clusters c
            WHERE c.cid = %(cluster_id)s
        )
        SELECT code.add_operation(
            i_operation_id    => %(task_id)s,
            i_cid             => c.cid,
            i_folder_id       => c.folder_id,
            i_operation_type  => %(operation_type)s,
            i_task_type       => %(task_type)s,
            i_task_args       => %(task_args)s,
            i_metadata        => %(metadata)s,
            i_user_id         => %(user)s,
            i_version         => 2,
            i_hidden          => %(hidden)s,
            i_time_limit      => %(timeout)s,
        {% if delay %}
            i_delay_by        => %(delay)s,
        {% endif %}
        {% if revision %}
            i_rev             => %(revision)s
        {% else %}
            i_rev             => c.rev
        {% endif %}
        )
        FROM cluster c;
        """,
        task_id=task_id,
        cluster_id=cluster_id,
        task_type=task_type,
        operation_type=operation_type,
        task_args=task_args,
        metadata=metadata,
        hidden=hidden,
        timeout=timeout,
        user=getuser(),
        delay=delay,
        revision=revision,
    )

    if wait:
        wait_task(ctx, task_id)

    return task_id


def create_finished_task(ctx, *, cluster_id, operation_type, hidden=False, revision=None, comment=None):
    task_id = generate_id(ctx)
    db_query(
        ctx,
        'metadb',
        """
        INSERT INTO dbaas.worker_queue (
            task_id,
            cid,
            folder_id,
            result,
            start_ts,
            end_ts,
            task_type,
            task_args,
            created_by,
            operation_type,
            metadata,
            comment,
            hidden,
            version,
            timeout,
            create_rev,
            acquire_rev,
            finish_rev,
            unmanaged
        )
        SELECT
            %(task_id)s,
            c.cid,
            c.folder_id,
            true,
            now(),
            now(),
            code.finished_operation_task_type(),
            '{}',
            %(user)s,
            %(operation_type)s,
            '{}',
            %(comment)s,
            %(hidden)s,
            2,
            '1 second',
        {% if revision %}
            %(revision)s,
            %(revision)s,
            %(revision)s,
        {% else %}
            code.rev(c),
            code.rev(c),
            code.rev(c),
        {% endif %}
             false
        FROM dbaas.clusters c
        WHERE c.cid = %(cluster_id)s;
        """,
        task_id=task_id,
        cluster_id=cluster_id,
        operation_type=operation_type,
        hidden=hidden,
        revision=revision,
        comment=comment,
        user=getuser(),
        fetch=False,
    )

    return task_id


def update_task(ctx, task_id, **kwargs):
    _update_task(ctx, task_id, **kwargs)


def restart_task(ctx, task_id, *, force=False):
    _restart_task(ctx, task_id, force=force)


def acquire_task(ctx, task_id, *, worker_id=None, force=False):
    with db_transaction(ctx, 'metadb'):
        task = get_task(ctx, task_id)

        if worker_id is None:
            worker_id = os.environ.get('USER')

        _ensure_task_can_be_acquired(ctx, task, force=force)
        _acquire_task(ctx, task_id, worker_id=worker_id)
        _update_task(
            ctx,
            task_id,
            context=task['context'],
            changes=task['changes'],
            errors=task['errors'],
            comment=task['comment'],
        )


def cancel_task(ctx, task_id):
    _cancel_task(ctx, task_id)


def finish_task(ctx, task_id, *, result, changes=None, errors=None, comment=None, worker_id=None, force=False):
    with db_transaction(ctx, 'metadb'):
        task = get_task(ctx, task_id)

        if worker_id is None:
            worker_id = os.environ.get('USER')

        if changes is None:
            changes = task['changes']

        if comment is None:
            comment = task['comment']

        _ensure_task_can_be_acquired(ctx, task, force)
        _acquire_task(ctx, task_id, worker_id=worker_id)
        _finish_task(
            ctx,
            task_id,
            worker_id=worker_id,
            result=result,
            changes=changes,
            errors=errors,
            comment=comment,
        )


def reject_task(ctx, task_id, *, changes=None, errors=None, comment=None, worker_id=None, force=False):
    with db_transaction(ctx, 'metadb'):
        task = get_task(ctx, task_id)

        if worker_id is None:
            worker_id = os.environ.get('USER')

        if changes is None:
            changes = task['changes']

        if comment is None:
            comment = task['comment']

        _ensure_task_can_be_acquired(ctx, task, force)
        _acquire_task(ctx, task_id, worker_id=worker_id)
        _reject_task(
            ctx,
            task_id,
            worker_id=worker_id,
            changes=changes,
            errors=errors,
            comment=comment,
            force=force,
        )


def wait_task(ctx, task_id, *, terminate=True):
    if dry_run_mode(ctx):
        print(task_id)
        return

    task = get_task(ctx, task_id)
    print(f'{format_timestamp(ctx, task["create_ts"])} task created')
    print_response(
        ctx,
        OrderedDict(
            (
                ('task_id', task['task_id']),
                ('task_type', task['task_type']),
                ('operation_type', task['operation_type']),
                ('maintenance_type', task['maintenance_type']),
                ('hidden', task['hidden']),
                ('created_by', task['created_by']),
                ('create_rev', task['create_rev']),
                ('target_rev', task['target_rev']),
                ('task_args', format_task_args(task['task_args'])),
                ('timeout', task['timeout']),
                ('cluster_id', task['cluster_id']),
            )
        ),
    )

    started = False
    last_step = None
    polling_interval = get_polling_interval(ctx)
    while True:
        if not started and task['start_ts']:
            print(f'{format_timestamp(ctx, task["start_ts"])} task started')
            print_response(
                ctx,
                OrderedDict(
                    (
                        ('restart_count', task['restart_count']),
                        ('notes', task['notes']),
                        ('worker_id', task['worker_id']),
                        ('acquire_rev', task['acquire_rev']),
                    )
                ),
            )
            started = True

        new_steps = deque()
        for step in reversed(task['changes'] or []):
            if step == last_step:
                break

            new_steps.appendleft(step)

        for step in new_steps:
            print(_format_task_change(ctx, step))

        last_step = new_steps[-1] if new_steps else last_step

        if task['result'] is None:
            time.sleep(polling_interval)
            task = get_task_run(ctx, task_id=task_id, restart_count=task['restart_count'])
            continue

        completed = task['result']
        print(f'{format_timestamp(ctx, task["finish_ts"])} task {"completed" if completed else "failed"}')
        details = OrderedDict((('finish_rev', task['finish_rev']),))
        if not completed:
            details.update(
                (
                    ('comment', task['comment']),
                    ('errors', task['errors']),
                )
            )
        print_response(ctx, details)
        break

    if terminate:
        exit(0 if task['result'] else 1)

    return task


class ParallelTaskExecuter:
    def __init__(self, ctx, parallel_task_count, *, wait=True, keep_going=False):
        self.ctx = ctx
        self.parallel_task_count = parallel_task_count
        self.keep_going = keep_going
        self.wait = wait
        self.submitted = deque()

    def submit(self, function, *args, **kwargs):
        self.submitted.append((function, args, kwargs))

    def run(self):
        completed = []
        failed = []
        running = deque()

        while True:
            while self.submitted:
                if failed and not self.keep_going:
                    break

                if self.parallel_task_count and len(running) >= self.parallel_task_count:
                    break

                function, args, kwargs = self.submitted.popleft()
                try:
                    task_id = function(*args, **kwargs)
                    if task_id:
                        running.append(task_id)
                except ClickException as e:
                    if self.keep_going:
                        print(str(e), file=sys.stderr)
                    else:
                        raise

            if not running:
                break

            if dry_run_mode(self.ctx):
                completed.append(running.popleft())
                continue

            if not self.wait:
                running.popleft()
                continue

            if completed or failed:
                print()

            task = wait_task(self.ctx, running.popleft(), terminate=False)
            if task['result']:
                completed.append(task['task_id'])
            else:
                failed.append(task['task_id'])

        return completed, failed


class SequentialTaskExecuter(ParallelTaskExecuter):
    def __init__(self, ctx, keep_going=False):
        super().__init__(ctx, parallel_task_count=1, keep_going=keep_going)


def is_terminated(task):
    """
    Return True if the task in terminal state.
    """
    return task['result'] is not None


def is_in_progress(task):
    """
    Return True if the task in in progress.
    """
    return task['result'] is None and task['worker_id']


def format_task_args(task_args):
    feature_flags = task_args.get('feature_flags')
    if isinstance(feature_flags, list):
        task_args['feature_flags'] = ','.join(sorted(feature_flags))

    return task_args


def format_task_changes(ctx, changes):
    return [_format_task_change(ctx, change) for change in (changes or [])]


def _format_task_change(ctx, change):
    timestamp = parse_timestamp(ctx, change["timestamp"], timezone.utc)
    message = ', '.join(f'{k}: {v}' for k, v in change.items() if k != 'timestamp')
    return f'{format_timestamp(ctx, timestamp)} {message}'


def format_task_tracing(ctx, tracing_info):
    jaeger_url = config_option(ctx, 'jaeger', 'url', None)

    try:
        uber_trace_id = json.loads(tracing_info)['uber-trace-id']
        trace_id = uber_trace_id.split(':')[0]
        return f'{jaeger_url}/trace/{trace_id}' if jaeger_url else trace_id
    except TypeError:
        return tracing_info


def _ensure_task_can_be_acquired(ctx, task, force):
    task_id = task['task_id']
    worker_id = task['worker_id']

    terminated = is_terminated(task)

    if is_in_progress(task):
        if not force:
            raise TaskIsInProgress(task_id, worker_id)

        _finish_task(
            ctx,
            task_id,
            worker_id=worker_id,
            result=False,
            changes=task['changes'],
            comment=task['comment'],
        )
        terminated = True

    if terminated:
        _restart_task(ctx, task_id, force=True)


def _restart_task(ctx, task_id, force):
    db_query(
        ctx,
        'metadb',
        """
        SELECT code.restart_task(
            i_task_id   => %(task_id)s,
            i_force     => %(force)s,
            i_notes     => %(notes)s);
        """,
        task_id=task_id,
        force=force,
        notes=f'restarted by {getuser()} via dbaas cli',
    )


def _acquire_task(ctx, task_id, *, worker_id=None):
    db_query(
        ctx,
        'metadb',
        """
        SELECT code.acquire_task(
            i_task_id   => %(task_id)s,
            i_worker_id => %(worker_id)s);
        """,
        task_id=task_id,
        worker_id=worker_id,
    )


def _cancel_task(ctx, task_id):
    db_query(
        ctx,
        'metadb',
        """
        SELECT code.cancel_task(i_task_id   => %(task_id)s);
        """,
        task_id=task_id,
    )


def _finish_task(ctx, task_id, *, result, changes=None, errors=None, comment=None, worker_id=None):
    changes = json.dumps(changes or [])

    if errors:
        errors = json.dumps(errors)

    db_query(
        ctx,
        'metadb',
        """
        SELECT code.finish_task(
            i_task_id   => %(task_id)s,
            i_worker_id => %(worker_id)s,
            i_result    => %(result)s,
            i_changes   => %(changes)s,
            i_comment   => %(comment)s,
            i_errors    => %(errors)s);
        """,
        task_id=task_id,
        worker_id=worker_id,
        result=result,
        changes=changes,
        errors=errors,
        comment=comment,
    )


def _reject_task(ctx, task_id, *, changes=None, errors=None, comment=None, worker_id=None, force=False):
    changes = json.dumps(changes or [])

    if errors:
        errors = json.dumps(errors)

    db_query(
        ctx,
        'metadb',
        """
        SELECT code.reject_task(
            i_task_id   => %(task_id)s,
            i_worker_id => %(worker_id)s,
            i_changes   => %(changes)s,
            i_comment   => %(comment)s,
            i_errors    => %(errors)s,
            i_force     => %(force)s);
        """,
        task_id=task_id,
        worker_id=worker_id,
        changes=changes,
        errors=errors,
        comment=comment,
        force=force,
    )


def _update_task(
    ctx,
    task_id,
    *,
    hidden=None,
    delayed_ts=None,
    timeout=None,
    task_args=None,
    worker_id=None,
    context=None,
    changes=None,
    errors=None,
    comment=None,
):
    if changes is not None:
        changes = json.dumps(changes)

    if errors is not None:
        errors = json.dumps(errors)

    has_delayed_ts, delayed_ts = flatten_nullable(delayed_ts)

    db_query(
        ctx,
        'metadb',
        """
        UPDATE dbaas.worker_queue
        SET
        {% if has_delayed_ts %}
            delayed_until = %(delayed_ts)s,
        {% endif %}
        {% if timeout %}
            timeout = %(timeout)s,
        {% endif %}
        {% if task_args is not none %}
            task_args = %(task_args)s,
        {% endif %}
        {% if worker_id %}
            worker_id = %(worker_id)s,
        {% endif %}
        {% if context is not none %}
            context = %(context)s,
        {% endif %}
        {% if changes is not none %}
            changes = %(changes)s,
        {% endif %}
        {% if errors is not none %}
            errors = %(errors)s,
        {% endif %}
        {% if comment is not none %}
            comment = %(comment)s,
        {% endif %}
            hidden = {{ 'hidden' if hidden is none else '%(hidden)s' }}
        WHERE task_id = %(task_id)s;
        """,
        task_id=task_id,
        hidden=hidden,
        has_delayed_ts=has_delayed_ts,
        delayed_ts=delayed_ts,
        timeout=timeout,
        task_args=json.dumps(task_args) if task_args is not None else None,
        worker_id=worker_id,
        context=context,
        changes=changes,
        errors=errors,
        comment=comment,
        fetch=False,
    )
