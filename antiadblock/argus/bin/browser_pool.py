from abc import ABC
import logging
import typing as t
from functools import partial

from antiadblock.argus.bin.schemas import Browser
from antiadblock.libs.adb_selenium_lib.browsers.extensions import ExtensionAdb
from antiadblock.argus.bin.browser_group import BrowserGroup, WorkerGroup
from antiadblock.argus.bin.task import Task
from antiadblock.argus.bin.task_queue import TaskQueue
from antiadblock.libs.adb_selenium_lib.schemas import BrowserInfo


class WorkerGroupPool(ABC):
    def __init__(self, name: str) -> None:
        self.name: str = name
        self._logger: logging.Logger = logging.getLogger(self.name)

        self._queue: TaskQueue = TaskQueue(f'{name} Task Queue')
        self._groups: list[WorkerGroup] = []

        self._stopped: bool = False

    def submit(self, task: Task, dependent_on: t.Optional[Task] = None) -> None:
        """Adds task to general queue"""
        assert not self._stopped, 'Cant submit new tasks after stop'
        self._logger.info(f'Submit task ({task})')
        self._queue.put(task, dependent_on)

    def wait_idle(self) -> None:
        """Wait until no pending tasks left"""
        self._logger.info('Waiting idle')
        self._queue.wait_idle()

    def stop(self) -> None:
        """Waits all running groups to stop"""
        self._logger.info('Stopping')
        self._queue.stop()
        for group in self._groups:
            group.stop()
        self._stopped = True

    def __del__(self) -> None:
        if not self._stopped:
            raise RuntimeError('WorkerGroupPool must be stopped before deletion')


class BrowserPool(WorkerGroupPool):
    """Pool of browser groups with single queue."""

    def __init__(self, browsers: t.Iterable[Browser], extensions: dict[BrowserInfo, t.Iterable[ExtensionAdb]]) -> None:
        super().__init__('BrowserPool')
        for browser in browsers:
            for extension in extensions[browser.browser_info]:
                group_browser = browser.copy()
                group_browser.adb_extension = extension
                task_getter = partial(self._queue.get, group_browser)  # Browser's hash is its context identifier
                browser_group = BrowserGroup(group_browser, task_getter)
                self._groups.append(browser_group)
                browser_group.serve()
