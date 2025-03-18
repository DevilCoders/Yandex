from dataclasses import dataclass
from functools import partial
from antiadblock.argus.bin.browser_group import Worker, WorkerGroup
from antiadblock.argus.bin.browser_pool import WorkerGroupPool
from antiadblock.argus.bin.task import Task


@dataclass
class TaskResult:
    group_id: int
    worker_id: int
    worker_incarnation: int


class TestTask(Task):
    def __init__(self, id: int, identifier: int, max_attempts: int, successes_on: int) -> None:
        self._id = id
        self.identifier = identifier

        self._current_attempt = 0
        self._max_attempts = max_attempts
        self._successes_on = successes_on

        super().__init__()

    def run(self, result: TaskResult):
        self._logger.info(f'{self} – Run attempt {self._current_attempt}/{self._max_attempts} with success on {self._successes_on}')
        if self._future.done():
            raise RuntimeError('Call after completion')
        self._current_attempt += 1
        if self._current_attempt == self._successes_on:
            self._set_result(result)
        elif self._current_attempt >= self._max_attempts:
            self._set_exception(Exception('Task failed'))
        else:
            raise Exception('Bad try')

    def __hash__(self) -> int:
        return self._id

    def __eq__(self, other: 'TestTask'):
        return self._id == other._id

    def __str__(self) -> str:
        return f'Task {self._id}'

    def get_context_identifier(self) -> int:
        return self.identifier


class TestWorker(Worker):
    def __init__(self, id, group_name, failure_channel) -> None:
        super().__init__(id, group_name, failure_channel)
        self._group_id = None
        self._logger.info(f'{self} created')

    def get_context(self) -> TaskResult:
        return TaskResult(self._group_id, self.id, self._incarnation)


class TestGroup(WorkerGroup):
    MAX_WORKERS = 1
    WORKER_TYPE = TestWorker

    def __init__(self, *args, **kwargs):
        group_id = kwargs.pop('group_id')
        super().__init__(*args, **kwargs)
        for worker in self._workers:
            worker._group_id = group_id

    def _get_best_worker(self, task):
        return self._workers[0]


class TestGroupPool(WorkerGroupPool):
    def __init__(self, groups: int) -> None:
        super().__init__('TestGroupPool')
        for i in range(groups):
            task_getter = partial(self._queue.get, i)
            group = TestGroup(f'Group {i}', task_getter, group_id=i)
            self._groups.append(group)
            group.serve()
