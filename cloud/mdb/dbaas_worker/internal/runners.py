# -*- coding: utf-8 -*-
"""
Task arguments resolvers and runners
"""

import json
import logging
import multiprocessing
import signal
import sys
import time
from queue import Empty as QueueEmpty
from traceback import format_exc

from setproctitle import setproctitle  # type: ignore

import opentracing
from dbaas_common.tracing import init_requests_interceptor, init_tracing

from .exceptions import Interrupted, TimeOut
from .except_hook import set_threading_except_hook
from .messages import TaskFinish, TaskUpdate
from .query import execute
from .logs import new_sentry_client
from .state import ExecutorState
from .tasks import get_executor


def get_host_opts(host_row):
    """
    Extracts host geo and flavor args (cpu_limit, memory_limit, etc)
    """
    return {x: host_row[x] for x in host_row if x not in ['name', 'fqdn']}


def resolve_args(conn, task):
    """
    Get all hosts, flavors, dcs from cluster

    Args:
        conn: psycopg2 connection instance
        task: dict {
                  'task_id': str,
                  'cid': str,
                  'task_type': str,
                  'task_args': dict,
              }

    Returns:
        {
            'cid': str,
            'hosts': {
                'fqdn': {
                    'geo': str,
                    'space_limit': int,
                    'cpu_guarantee': int,
                    'cpu_limit': int,
                    'gpu_limit': int,
                    'memory_guarantee': int,
                    'memory_limit': int,
                    'network_guarantee': int,
                    'network_limit': int,
                    'io_limit': int,
                    'io_cores_limit': int,
                }
            }
        }
    """
    log = logging.getLogger('resolve')
    log.info('Resolving %s', task['task_id'])
    cursor = conn.cursor()
    task_args = task['task_args'].copy()
    res = execute(cursor, 'generic_resolve', cid=task['cid'])
    if 'hosts' not in task_args:
        task_args['hosts'] = {}
    for row in res:
        task_args['hosts'][row['fqdn']] = get_host_opts(row)

    return task_args


class Task:
    """
    Simple structure to represent task
    """

    def __init__(self, config, task, task_args):
        self.queue = multiprocessing.Queue(maxsize=10000)
        self.process = TaskRunner(config, task, task_args, self.queue)
        self.process.start()
        self.restart_count = task['restart_count']
        self.changes = []
        self.context = {}
        self.finish_state = None

    @property
    def task_args(self):
        """
        Return task_args
        """
        return self.process.task_args

    def get_current_state(self):
        """
        Poll queue and get return current state
        """
        state = ExecutorState(self.changes, '', self.context)
        try:
            message = self.queue.get(block=False)
            state.updated = True
            if isinstance(message, TaskUpdate):
                state.context.update(message.context)
                if message.changes:
                    state.changes.append(message.changes)
            elif isinstance(message, TaskFinish):
                self.process.join()
                state.interrupted = message.interrupted
                state.rejected = message.rejected
                state.result = True
                if message.error is not None:
                    state.result = False
                    state.comment = message.traceback
                    state.restartable = message.restartable
                    if not state.errors:
                        state.errors = []
                    state.errors.append(message.error)
            else:
                raise RuntimeError(f'Protocol violation. Unknown message: {repr(message)}')
        except QueueEmpty:
            pass

        if self.process.exitcode is not None:
            if self.process.exitcode < 0:
                state.result = False
                state.comment = 'Terminated by signal {signal}'.format(signal=-1 * self.process.exitcode)

        if state.result is not None:
            self.finish_state = state

        return state


class TaskRunner(multiprocessing.Process):
    """
    Task execution class
    """

    def __init__(self, config, task, task_args, queue):
        super().__init__()
        self.config = config
        self.task = task
        self.task_args = task_args
        self.queue = queue
        self.sentry_client = None
        self.tracer = None
        self.executor = None
        self.catch_task_failure = True  # ONLY for unit testing purposes, not a part of production api.

    def interrupt(self, *_):
        """
        Handle external interruption
        """
        if self.executor is not None:
            self.executor.interrupt()
        else:
            raise Interrupted('Interrupted by external signal')

    def timeout(self, *_):
        """
        Raise timeout exception
        """
        # pylint: disable=no-self-use
        raise TimeOut('Timeout expired')

    def _init_sentry(self):
        """
        Initialize sentry client
        """
        self.sentry_client = new_sentry_client(self.config)
        self.sentry_client.tags_context({'task_type': self.task['task_type']})
        self.sentry_client.user_context(
            {
                'id': self.task['created_by'],
                'task': self.task,
                'args': self.task_args,
            }
        )
        set_threading_except_hook()
        # Sadly, but I don't find public way to capture feed errors.
        # Create multiprocessing.Queue subclass looks better,
        # but it's too complicated for that problem
        self.queue._on_queue_feeder_error = self._capture_feed_error

    def _capture_feed_error(self, exc, obj):
        """
        Log feed error and sent it to Sentry
        """
        logging.getLogger('queue').exception('queue feed error on object: %r', obj)
        self._capture_exception(sys.exc_info())

    def _capture_exception(self, exc_info):
        """
        Sentry exception report helper
        """
        try:
            self.sentry_client.captureException(exc_info=exc_info)
        except Exception as exc:
            log = logging.getLogger('sentry_client')
            log.error('Unable to report error to sentry: %s', repr(exc))

    def run(self):
        if self.config.main.sentry_dsn:
            self._init_sentry()

        init_tracing(self, self.config.main.tracing)

        init_requests_interceptor(self.sentry_client)

        setproctitle(
            'dbaas-worker: {task} {task_id}'.format(
                task=self.task['task_type'],
                task_id=self.task['task_id'],
            )
        )
        signal.signal(signal.SIGINT, self.interrupt)
        signal.signal(signal.SIGTERM, self.interrupt)
        signal.signal(signal.SIGALRM, self.timeout)

        # Retrieve task tracing info if any
        if self.task.get('tracing'):
            carrier = json.loads(self.task['tracing'])
            span_context = opentracing.global_tracer().extract(
                format=opentracing.propagation.Format.TEXT_MAP,
                carrier=carrier,
            )
            span_ref = [opentracing.follows_from(span_context)]
        else:
            span_ref = None

        # Start span, tries to follow from task's span
        with opentracing.global_tracer().start_active_span(
            'Worker Task',
            references=span_ref,
            finish_on_close=True,
        ) as scope:
            scope.span.set_tag('worker.task.id', self.task['task_id'])
            scope.span.set_tag('worker.task.type', self.task['task_type'])

            try:
                self.executor = get_executor(self.task, self.config, self.queue, self.task_args)
                signal.alarm(int(self.task['timeout']))
                self.executor.run()
                self.queue.put(TaskFinish(dict()))
            except Interrupted:
                self.queue.put(TaskFinish(dict(), interrupted=True))
            except BaseException as exc:
                scope.span.set_tag(opentracing.tags.ERROR, True)
                traceback = format_exc()
                if not self.catch_task_failure:
                    raise
                if self.executor:
                    self.executor.logger.error('Task execution failed: %s', traceback)
                    self.queue.put(self.executor.rollback(exc, traceback))
                else:
                    self.queue.put(TaskFinish(dict(), exception=exc, traceback=traceback))
                    log = logging.getLogger('runner')
                    log.error('Unable to initialize executor: %s', traceback)
                self._capture_exception(sys.exc_info())

        if self.executor:
            self.executor.logger.info('Task execution finished')

        if self.tracer:
            # Flush spans - https://github.com/jaegertracing/jaeger-client-python/issues/50
            time.sleep(2)
            self.tracer.close()
            time.sleep(2)
