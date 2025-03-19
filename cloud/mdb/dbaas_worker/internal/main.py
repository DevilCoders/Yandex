# -*- coding: utf-8 -*-
"""
Main module. DbaasWorker is defined here.
"""

import json
import os
import signal
import socket
import sys
import time
import traceback
from logging import getLogger
from logging.config import dictConfig
from secrets import randbelow

import psycopg2
import requests  # noqa
from opentracing_instrumentation.client_hooks.requests import patcher  # type: ignore
from setproctitle import setproctitle  # type: ignore

from .config import get_config
from .logs import new_sentry_client
from .metadb import DatabaseConnectionError, get_master_conn
from .query import execute
from .runners import Task, resolve_args
from .tasks.tasks_version import TASKS_VERSION

patcher.install_patches()


def get_worker_id() -> str:
    """
    Return worker_id
    """
    return socket.getfqdn()


def poll(txn, acquire_fail_limit):
    """
    Try to lock task in queue
    """
    cursor = txn.cursor()
    for poll_type in ['poll_new', 'poll_delayed']:
        task = execute(cursor, poll_type, version=TASKS_VERSION, acquire_fail_limit=acquire_fail_limit)
        if task:
            try:
                ret = execute(cursor, 'acquire_task', worker_id=get_worker_id(), task_id=task[0]['task_id'])
                return ret[0]
            except psycopg2.Error:
                # We need to rollback/commit explicitly because txn context manager will rollback on exception
                txn.rollback()
                increment_cursor = txn.cursor()
                execute(increment_cursor, 'increment_failed_acquire_count', task_id=task[0]['task_id'])
                txn.commit()
                raise


def finish_task(txn, task_id, result, changes, comment, task_args, errors):
    """
    Write task result to db
    """
    if result and 'target-pillar-id' in task_args:
        execute(
            txn.cursor(),
            'delete_task_target_pillar',
            fetch=False,
            target_id=task_args['target-pillar-id'],
        )
    execute(
        txn.cursor(),
        'finish_task',
        fetch=False,
        worker_id=get_worker_id(),
        task_id=task_id,
        result=bool(result),
        changes=json.dumps(changes),
        comment=comment,
        errors=json.dumps(errors) if errors is not None else errors,
    )


def reschedule_task(txn, task_id, result, changes, comment, task_args, errors, interval):
    """
    Finish task and restart it with delay
    """
    finish_task(txn, task_id, result, changes, comment, task_args, errors)
    execute(txn.cursor(), 'reschedule_task', fetch=False, task_id=task_id, interval=interval)
    execute(txn.cursor(), 'restart_task', fetch=False, task_id=task_id)


def release_task(txn, task_id, context):
    """
    Mark task as free to acquire and save context
    """
    persistent_context = {k: v for k, v in context.items() if k not in ['interruptable', 'interrupted']}
    execute(
        txn.cursor(),
        'release_task',
        fetch=False,
        worker_id=get_worker_id(),
        task_id=task_id,
        context=json.dumps(persistent_context),
    )


def reject_task(txn, task_id, changes, comment, errors):
    """
    Mark task as rejected
    """
    execute(
        txn.cursor(),
        'reject_task',
        fetch=False,
        worker_id=get_worker_id(),
        task_id=task_id,
        changes=json.dumps(changes),
        comment=comment,
        errors=json.dumps(errors) if errors is not None else errors,
    )


def update_task(txn, task_id, changes, comment, context):
    """
    Update task state in db
    """
    execute(
        txn.cursor(),
        'update_task',
        fetch=False,
        worker_id=get_worker_id(),
        task_id=task_id,
        changes=json.dumps(changes),
        comment=comment,
        context=json.dumps(context),
    )


class DbaasWorker:
    """
    DbaasWorker class
    """

    def __init__(self, config_path):
        self.conn = None
        self.active_tasks = {}
        self.config = get_config(config_path)
        setproctitle('dbaas-worker: main')
        dictConfig(
            {
                'version': 1,
                'disable_existing_loggers': True,
                'formatters': {
                    'json': {
                        '()': 'cloud.mdb.internal.python.logs.format.json.JsonFormatter',
                    },
                },
                'handlers': {
                    'streamhandler': {
                        'level': self.config.main.log_level,
                        'class': 'logging.StreamHandler',
                        'formatter': 'json',
                        'stream': 'ext://sys.stderr',
                    },
                },
                'loggers': {
                    '': {
                        'handlers': ['streamhandler'],
                        'level': 'DEBUG',
                    },
                    'retry': {
                        'handlers': ['streamhandler'],
                        'level': 'DEBUG',
                    },
                    'raven': {
                        'handlers': ['streamhandler'],
                        'level': 'INFO',
                    },
                },
            }
        )
        self.log = getLogger('main')
        self.should_run = True
        self.shutdown = False
        signal.signal(signal.SIGINT, self.stop)
        signal.signal(signal.SIGTERM, self.stop)
        self.sentry_client = new_sentry_client(self.config)

    def _reconnect(self):
        self.conn = get_master_conn(
            self.config.main.metadb_dsn,
            self.config.main.metadb_hosts,
            self.log,
        )

    def can_poll(self):
        """
        Check if we have available executors and not shutting down
        """
        return (not self.shutdown) and (len(self.active_tasks) < self.config.main.max_tasks)

    def start_task(self, txn, task):
        """
        Resolve task args and start task
        """
        args = resolve_args(txn, task)
        task['changes'] = []
        self.active_tasks[task['task_id']] = Task(self.config, task, args)

    def check_running_tasks(self, txn):
        """
        Poll all running tasks for updates
        """
        if self.active_tasks:
            cancelled = [
                x['task_id'] for x in execute(txn.cursor(), 'poll_cancelled', task_ids=tuple(self.active_tasks))
            ]
        else:
            cancelled = []
        for task_id, task in self.active_tasks.copy().items():
            state = task.get_current_state()

            if state.interrupted:
                release_task(txn, task_id, state.context)
                del self.active_tasks[task_id]
            elif state.rejected:
                reject_task(txn, task_id, state.changes, state.comment, state.errors)
                del self.active_tasks[task_id]
            elif task.finish_state:
                if task.finish_state.restartable and self.config.restart.max_attempts > task.restart_count:
                    reschedule_task(
                        txn,
                        task_id,
                        task.finish_state.result,
                        task.finish_state.changes,
                        task.finish_state.comment,
                        task.task_args,
                        task.finish_state.errors,
                        randbelow(self.config.restart.base * 2**task.restart_count),
                    )
                else:
                    finish_task(
                        txn,
                        task_id,
                        task.finish_state.result,
                        task.finish_state.changes,
                        task.finish_state.comment,
                        task.task_args,
                        task.finish_state.errors,
                    )
                del self.active_tasks[task_id]
            elif state.updated:
                update_task(txn, task_id, state.changes, state.comment, state.context)
        for task_id in cancelled:
            if task_id in self.active_tasks:
                task = self.active_tasks[task_id]
                if task.process.is_alive():
                    # We will collect failure result on next iteration
                    os.kill(task.process.pid, signal.SIGKILL)

    def release_stale_tasks(self, txn):
        """
        Find running tasks not in active_tasks and release them
        """
        cursor = txn.cursor()
        worker_id = get_worker_id()
        tasks = {x['task_id'] for x in execute(cursor, 'get_running_tasks', version=TASKS_VERSION, worker_id=worker_id)}
        for task_id in tasks:
            if task_id not in self.active_tasks:
                self.log.error('Unexpected stale task: %s', task_id)
                # We explicitly drop context here because we don't know that saving it is safe at this point
                release_task(txn, task_id, {})
        # We also could have a running task that is finished or acquired by another worker
        # So we terminate such tasks and log error
        for task_id, task in self.active_tasks.copy().items():
            if task_id not in tasks:
                self.log.error(
                    'Database state and internal state diverged. %s is not running by %s in db', task_id, worker_id
                )
                try:
                    if task.process.is_alive():
                        os.kill(task.process.pid, signal.SIGKILL)
                    task.process.join()
                    del self.active_tasks[task_id]
                except Exception as exc:
                    self.log.warning(repr(exc))

    def start(self):
        """
        Starts poller
        """
        while self.should_run:
            try:
                if not self.conn:
                    self._reconnect()
                with self.conn as txn:
                    self.release_stale_tasks(txn)
                with self.conn as txn:
                    self.check_running_tasks(txn)
                if self.shutdown:
                    if not self.active_tasks:
                        self.should_run = False
                    else:
                        self.stop()
                if self.can_poll():
                    with self.conn as txn:
                        task = poll(txn, self.config.main.acquire_fail_limit)
                        if task:
                            self.start_task(txn, task)
            except DatabaseConnectionError as exc:
                self.log.warning(repr(exc))
            except Exception:
                self.log.error('Unexpected error: %s', traceback.format_exc())
                try:
                    self.sentry_client.captureException(exc_info=sys.exc_info())
                except Exception as send_to_sentry_exc:
                    self.log.warning('Unable to report error to sentry: %s', repr(send_to_sentry_exc))
                if self.conn is not None and not self.conn.closed:
                    try:
                        self.conn.close()
                    except Exception as conn_close_exc:
                        self.log.warning('Unable to close orphan conn: %s', repr(conn_close_exc))
                self.conn = None
            time.sleep(self.config.main.poll_interval)

    def stop(self, *_):
        """
        Set internal state to "stopping" and relay term to all executors
        """
        self.shutdown = True
        for task in self.active_tasks.values():
            try:
                if task.process.is_alive():
                    task.process.terminate()
            except Exception as exc:
                self.log.warning(repr(exc))
