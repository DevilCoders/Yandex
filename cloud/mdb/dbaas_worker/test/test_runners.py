"""
Runners tests
"""

from queue import Queue
from types import SimpleNamespace

from hamcrest import assert_that, equal_to

from cloud.mdb.dbaas_worker.internal.exceptions import TimeOut
from cloud.mdb.dbaas_worker.internal.messages import ExecutorMessage, TaskFinish
from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider, Change
from cloud.mdb.dbaas_worker.internal.runners import TaskRunner
from cloud.mdb.dbaas_worker.internal.tasks.common.base import BaseExecutor


def run_executor(mocker, executor_class):
    """
    Simple helper for running executor under mocks
    """
    queue = Queue(maxsize=10000)
    config = SimpleNamespace(
        main=SimpleNamespace(
            metadb_dsn=None, metadb_hosts=None, sentry_dsn=None, tracing=SimpleNamespace(disabled=True)
        ),
        mlock=SimpleNamespace(
            enabled=False,
        ),
    )
    task = {
        'task_id': 'test_id',
        'task_type': 'test_type',
        'feature_flags': [],
        'folder_id': 'test_folder',
        'context': {},
        'timeout': 0,
        'changes': [],
    }
    task_args = {}

    mocker.patch('signal.signal')
    mocker.patch('signal.alarm')
    mocker.patch('setproctitle.setproctitle')
    get_safe_revision = mocker.patch('cloud.mdb.dbaas_worker.internal.tasks.common.base.get_safe_revision')
    get_safe_revision.return_value = 1

    get_executor = mocker.patch('cloud.mdb.dbaas_worker.internal.runners.get_executor')
    executor = executor_class(config, task, queue, task_args)
    get_executor.return_value = executor

    runner = TaskRunner(config, task, task_args, queue)
    runner.run()

    calls = {}
    finish_message = None

    while not queue.empty():
        message = queue.get()
        executor.logger.error('Got %r', message)
        if not isinstance(message, ExecutorMessage):
            calls.update(message)
        elif isinstance(message, TaskFinish):
            finish_message = message

    assert finish_message

    return calls, finish_message


def test_crit_rollback_on_timeout(mocker):
    """
    Check that all critical changes are rolled back on timeout error
    """

    class TestExecutor(BaseExecutor):
        def run(self):
            provider = BaseProvider(self.config, self.task, self.queue)
            provider.add_change(
                Change(
                    'non-critical', True, rollback=lambda task, safe_refision: self.queue.put({'non-critical': True})
                )
            )
            provider.add_change(
                Change(
                    'critical',
                    True,
                    critical=True,
                    rollback=lambda task, safe_refision: self.queue.put({'critical': True}),
                )
            )
            raise TimeOut()

    calls, _ = run_executor(mocker, TestExecutor)

    assert_that(calls, equal_to({'critical': True}))


def test_criticals_on_rollback_fail(mocker):
    """
    Check that all critical changes are rolled back on rollback error
    """

    class TestExecutor(BaseExecutor):
        def run(self):
            provider = BaseProvider(self.config, self.task, self.queue)
            provider.add_change(
                Change(
                    'critical',
                    True,
                    critical=True,
                    rollback=lambda task, safe_refision: self.queue.put({'critical': True}),
                )
            )
            provider.add_change(
                Change(
                    'non-critical-1',
                    True,
                    rollback=lambda task, safe_refision: self.queue.put({'non-critical-1': True}),
                )
            )
            provider.add_change(Change('non-critical-2', True, rollback=lambda task, safe_refision: 1 / 0))
            raise Exception('task fail')

    calls, _ = run_executor(mocker, TestExecutor)

    assert_that(calls, equal_to({'critical': True}))


def test_reject_on_rollback_success(mocker):
    """
    Check that task is rejected if all rollback calls were successful
    """

    class TestExecutor(BaseExecutor):
        def run(self):
            provider = BaseProvider(self.config, self.task, self.queue)
            provider.add_change(
                Change(
                    'change', True, critical=True, rollback=lambda task, safe_refision: self.queue.put({'change': True})
                )
            )
            raise Exception('fail 1')

    calls, finish_message = run_executor(mocker, TestExecutor)

    assert_that(calls, equal_to({'change': True}))

    assert finish_message.rejected and finish_message.error


def test_fail_on_rollback_fail(mocker):
    """
    Check that task is failed if all rollback failed
    """

    class TestExecutor(BaseExecutor):
        def run(self):
            provider = BaseProvider(self.config, self.task, self.queue)
            provider.add_change(Change('change', True, rollback=lambda task, safe_refision: 1 / 0))
            raise Exception('fail 1')

    _, finish_message = run_executor(mocker, TestExecutor)

    assert finish_message.error and not finish_message.rejected and not finish_message.interrupted


def test_restartable(mocker):
    """
    Check that restartable task has correct restartable attribute in message
    """

    class TestExecutor(BaseExecutor):
        def run(self):
            provider = BaseProvider(self.config, self.task, self.queue)
            self.restartable = True
            provider.add_change(Change('change', True, rollback=lambda task, safe_refision: 1 / 0))
            raise Exception('fail 1')

    _, finish_message = run_executor(mocker, TestExecutor)

    assert finish_message.error and finish_message.restartable and not finish_message.rejected
