import logging
import typing as t
from collections import defaultdict
from queue import Queue
from threading import Condition, Lock, Thread

from antiadblock.argus.bin.task import Task, Identifier, POISON


class TaskQueue:
    """Task queue for tasks which must be executed in different context"""

    def __init__(self, name: str) -> None:
        self.name = name
        self._logger: logging.Logger = logging.getLogger(self.name)

        # Dependency -> dependent tasks
        self._dependent_tasks: dict[Task, list[Task]] = defaultdict(list)
        self._completed_dependencies: set[Task] = set()
        self._dependencies_lock: Lock = Lock()
        self._dependency_satisfied: Condition = Condition(self._dependencies_lock)

        self._pending_tasks: list[Task] = []
        self._pendings_tasks_lock: Lock = Lock()

        self._queues: dict[Identifier, Queue[Task]] = defaultdict(Queue)

    def put(self, task: Task, dependent_on: t.Optional[Task] = None) -> None:
        """Put tasks and states its dependencies if any"""
        if dependent_on is not None:
            with self._dependencies_lock:
                if dependent_on not in self._completed_dependencies:
                    if dependent_on not in self._dependent_tasks:
                        t = Thread(name=f'{task}', target=self._dependency_routine, args=(dependent_on,))
                        t.start()
                    self._dependent_tasks[dependent_on].append(task)
                    self._logger.info(f'Added task ({task}) dependent on task ({dependent_on})')
                    return
        self._put(task)

    def get(self, identifier: Identifier) -> Task:
        """Doesnt return task which dependencies arent satisfied"""
        task = self._queues[identifier].get()
        self._logger.info(f'Got task ({task}) for execution')
        return task

    def stop(self) -> None:
        """Poison all browsers queues"""
        self._logger.info('Stopping')
        for queue in self._queues.values():
            queue.put(POISON)

    def wait_idle(self) -> None:
        """Wait until all pending tasks completed"""
        idx = 0
        while idx < len(self._pending_tasks):
            with self._pendings_tasks_lock:
                task = self._pending_tasks[idx]

            self._logger.info(f'Getting result for task ({task})')
            try:
                task.result()
            except Exception:
                pass

            with self._dependency_satisfied:
                while task in self._dependent_tasks:
                    self._dependency_satisfied.wait()

            self._logger.info(f'Got result for task ({task})')
            idx += 1
        with self._pendings_tasks_lock:
            self._pending_tasks = []

    def _put(self, task: Task) -> None:
        with self._pendings_tasks_lock:
            self._pending_tasks.append(task)
        self._queues[task.get_context_identifier()].put(task)
        self._logger.info(f'Put task ({task}) to execution')

    def _dependency_routine(self, task: Task) -> None:
        try:
            task.result()
            failed = False
        except Exception:
            failed = True
        finally:
            with self._dependencies_lock:
                self._completed_dependencies.add(task)
                for dependant_task in self._dependent_tasks[task]:
                    if failed:
                        dependant_task.fail()
                    else:
                        self._put(dependant_task)
                self._dependent_tasks.pop(task)
                self._dependency_satisfied.notify_all()
