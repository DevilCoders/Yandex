"""
Common tasks base. BaseExecutor is defined here.
"""
from contextlib import closing
from types import SimpleNamespace
from typing import Any
from traceback import format_exc

from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from ...exceptions import Interrupted, TimeOut
from ...logs import get_task_prefixed_logger
from ...messages import TaskFinish
from ...metadb import get_master_conn
from ...providers.mlock import Mlock
from ...query import execute
from ...types import Task


def get_safe_revision(dsn, hosts, log, task):
    """
    Get safe reject revision for task
    """
    try:
        with closing(get_master_conn(dsn, hosts, log)) as conn:
            with conn:
                cursor = conn.cursor()
                rev = execute(cursor, 'get_reject_rev', task_id=task['task_id'])
                return rev[0]['rev']
    except Exception as exc:
        log.error('Unable to get safe revision for %s rollback: %s', task['task_id'], repr(exc))
        return None


def chain_exception(original, current):
    """
    Save full traceback of all exceptions and original error
    """
    try:
        raise current from original
    except BaseException as exc:
        traceback = format_exc()
        return exc, traceback


class RunUnsupportedError(Exception):
    """
    Not implemented run method error
    """


SKIP_LOCK_CREATION_ARG = 'lock_is_already_taken'


class BaseExecutor:
    """
    Base class for all executors
    """

    def __init__(self, config: SimpleNamespace, task: Task, queue: Any, args: dict[str, Any]) -> None:
        self.config = config
        self.task = task
        self.args = args
        self.queue = queue
        self.logger = get_task_prefixed_logger(task, __name__)
        self.mlock = Mlock(self.config, self.task, self.queue)
        self.metadb = BaseMetaDBProvider(config, task, queue)
        self.restartable = False

    # pylint: disable=no-self-use
    def run(self):
        """
        Execute steps
        """
        raise RunUnsupportedError('This executor does not support run')

    def acquire_lock(self):
        skip_lock = self.args.get(SKIP_LOCK_CREATION_ARG, False)
        if not skip_lock:
            self.mlock.lock_cluster(sorted(self.args['hosts']))
        else:
            self.logger.info('Lock creation explicitly skipped using "%s" arg', SKIP_LOCK_CREATION_ARG)

    def release_lock(self):
        if not self.args.get(SKIP_LOCK_CREATION_ARG, False):
            self.mlock.unlock_cluster()

    def rollback(self, exception, traceback, initially_failed=False):
        """
        Rollback if run failed
        """
        failed = isinstance(exception, TimeOut) or initially_failed
        safe_revision = get_safe_revision(
            self.config.main.metadb_dsn, self.config.main.metadb_hosts, self.logger, self.task
        )
        if not safe_revision:
            failed = True
        non_revertable = [change for change in self.task['changes'] if not change.can_rollback()]
        if non_revertable:
            self.logger.error(
                'Some changes are not revertable: %s. Will rollback only critical changes.', non_revertable
            )
            failed = True
        for change in reversed(self.task['changes']):
            if not failed or change.critical:
                try:
                    change.rollback(self.task, safe_revision)
                except BaseException as exc:
                    failed = True
                    exception, traceback = chain_exception(exception, exc)

        message = TaskFinish(
            self.task['context'],
            exception=exception,
            traceback=traceback,
            restartable=self.restartable,
        )
        message.rejected = not failed
        return message

    def interrupt(self):
        """
        Handle external interruptions
        """
        if self.task['context'].get('interruptable'):
            raise Interrupted('Interrupted by external signal')
        self.task['context']['interrupted'] = True

    def update_major_version(self, component):
        """
        Update task args
        """
        try:
            if 'cid' not in self.task:
                return
            with self.metadb.get_master_conn() as conn:
                with conn:
                    cursor = conn.cursor()
                    version = execute(cursor, 'get_major_version', cid=self.task['cid'], component=component)
                    self.args['major_version'] = version[0]['major_version']
        except Exception as exc:
            self.logger.error('Unable to update major version %s exception: %s', self.task['task_id'], repr(exc))
            raise exc
