import logging
import traceback
import typing as t
from abc import ABC, abstractmethod
from queue import Queue
from random import randint
from threading import Thread

from antiadblock.argus.bin.argus_browser import ArgusBrowser
from antiadblock.argus.bin.schemas import Browser
from antiadblock.argus.bin.task import BrowserTask, Task, POISON

FailureChannel = Queue[int]


class Worker(ABC):
    _thread: Thread

    def __init__(self, id: int, group_name: str, failure_channel: FailureChannel) -> None:
        self.id: int = id
        self.name: str = f'{group_name} Worker {self.id}'
        self._logger: logging.Logger = logging.getLogger(self.name)
        self._incarnation: int = 0
        self._tasks_served: int = 0

        self._failure_channel: FailureChannel = failure_channel
        self.task_queue: Queue[BrowserTask] = Queue()

        self.current_task: t.Optional[BrowserTask] = None

    def serve(self) -> None:
        name = f'{self.name} inc {self._incarnation}'
        self._logger.info(f'Serving {name}')
        self._thread = Thread(name=name, target=self._serve_tasks)
        self._thread.start()
        self._incarnation += 1

    def stop(self) -> None:
        self._logger.info(f'Stopping {self.name}')
        self.task_queue.put(POISON)
        self._thread.join()
        self._logger.info(f'Stopped {self.name}')

    def join(self) -> None:
        self._thread.join()

    def _serve_tasks(self) -> None:
        try:
            context = self.get_context()
            while True:
                if self.current_task is None:
                    self._logger.info('Waiting for new task')
                    task = self.task_queue.get()
                    self._logger.info(f'Got new task ({task}) from queue')
                    if task is POISON:
                        break
                    self.current_task = task
                task = self.current_task
                try:
                    task.run(context)
                except Exception:
                    self._logger.warning(f'Got exception from task ({self.current_task}): {traceback.format_exc()}. Restarting')
                    self._failure_channel.put(self.id)
                    break
                else:
                    self._logger.info(f'Completed task ({self.current_task})')
                    self._tasks_served += 1
                    self.current_task = None
        except Exception:
            self._logger.error(traceback.format_exc())
            self._failure_channel.put(self.id)

    def __str__(self) -> str:
        return self.name

    @abstractmethod
    def get_context(self) -> t.Any:
        pass


class WorkerGroup(ABC):
    MAX_WORKERS: int
    WORKER_TYPE: type = Worker

    def __init__(self, name: str, task_getter: t.Callable[[], Task], *context_args: t.Any) -> None:
        self.name: str = name
        self._logger = logging.getLogger(self.name)

        self._task_getter: t.Callable[[], Task] = task_getter
        self._task_manager: Thread = Thread(name=f'{self.name} Task Manager', target=self._manage_tasks)

        self._failure_channel: FailureChannel = Queue()
        self._worker_manager: Thread = Thread(name=f'{self.name} Worker Manager', target=self._manage_workers)

        self._workers: list[Worker] = []
        for i in range(self.MAX_WORKERS):
            self._logger.info(f'Created worker {i}')
            self._workers.append(self.WORKER_TYPE(i, self.name, self._failure_channel, *context_args))

    def serve(self) -> None:
        self._logger.info('Serving')
        for worker in self._workers:
            worker.serve()

        self._worker_manager.start()
        self._task_manager.start()

    def stop(self) -> None:
        self._logger.info(f'Stopping {self.name}')
        for worker in self._workers:
            worker.stop()
        self._failure_channel.put(POISON)
        self._worker_manager.join()
        self._task_manager.join()
        self._logger.info(f'Stopped {self.name}')

    def _manage_tasks(self) -> None:
        self._logger.info('Serving')
        try:
            while True:
                self._logger.info('Waiting for new task')
                task = self._task_getter()
                self._logger.info(f'Got task ({task})')
                if task is POISON:
                    break
                best_worker = self._get_best_worker(task)
                self._logger.info(f'Got best worker for task ({task}): {best_worker}')
                best_worker.task_queue.put(task)
        except Exception:
            self._logger.error(traceback.format_exc())

    def _manage_workers(self) -> None:
        self._logger.info('Serving')
        try:
            while True:
                broken_idx = self._failure_channel.get()
                if broken_idx is POISON:
                    break
                broken_worker = self._workers[broken_idx]
                broken_worker.join()
                broken_worker.serve()
        except Exception:
            self._logger.error(traceback.format_exc())

    @abstractmethod
    def _get_best_worker(self, task: Task) -> Worker:
        pass


class BrowserWorker(Worker):
    """
    Single instance of selenium browser with state.

    At the moment state is only a detect.
    """

    def __init__(self, id: int, group_name: str, failure_channel: FailureChannel, browser: Browser) -> None:
        super().__init__(id, group_name, failure_channel)
        self._browser: Browser = browser

    def get_context(self) -> ArgusBrowser:
        browser = self._browser.create_browser()
        argus_browser = ArgusBrowser(browser)
        return argus_browser


class BrowserGroup(WorkerGroup):
    """
    Group of browsers similar by browser type and extension.
    Efficiently distributes tasks over workers. Handles worker failures.
    """

    MAX_WORKERS = 1  # Replication is at 1 for simplicity of first iteration
    WORKER_TYPE = BrowserWorker

    def __init__(self, browser: Browser, task_getter: t.Callable[[], BrowserTask]) -> None:
        name = f'{browser.browser_info.name} {browser.adb_extension.type.adb_name}'.capitalize()
        super().__init__(name, task_getter, browser)

    def _get_best_worker(self, task: BrowserTask) -> BrowserWorker:
        idx = randint(0, self.MAX_WORKERS - 1)
        return self._workers[idx]
