from abc import ABC, abstractmethod
import json
import logging
import traceback
import typing as t
from collections import defaultdict
from concurrent.futures import Future
from itertools import count
from queue import Queue
from threading import Lock, Thread

from antiadblock.argus.bin.argus_browser import ArgusBrowser
from antiadblock.argus.bin.schemas import Result, UrlSettings
from antiadblock.argus.bin.utils.s3_interactions import S3Uploader
from antiadblock.argus.bin.utils.utils import current_time
from antiadblock.libs.adb_selenium_lib.browsers.extensions import ExtensionAdb
from antiadblock.libs.adb_selenium_lib.config import AdblockTypes
from antiadblock.libs.adb_selenium_lib.schemas import BrowserInfo

logging.basicConfig(format='%(levelname)-8s [%(asctime)s] %(name)-30s: %(message)s', level=logging.INFO)

POISON = type('POISON', (object,), {})

Identifier = t.Hashable  # Hashable and comparable for equality object


class DependencyFailure(RuntimeError):
    pass


class Task(ABC):
    def __init__(self) -> None:
        self._future: Future = Future()
        self._logger: logging.Logger = logging.getLogger(f'{self}')

    @abstractmethod
    def run(self, context: t.Any) -> None:
        """Raises exception if wants to retry"""
        pass

    @abstractmethod
    def __hash__(self) -> int:
        """Must be hashable"""
        pass

    @abstractmethod
    def __eq__(self, other: 'Task') -> bool:
        """Must be comparable"""
        pass

    @abstractmethod
    def __str__(self) -> str:
        pass

    @abstractmethod
    def get_context_identifier(self) -> Identifier:
        pass

    def result(self, timeout: t.Optional[t.Union[int, float]] = None) -> t.Any:
        """Wait for result and if present return or throw"""
        return self._future.result(timeout)

    def exception(self) -> t.Optional[Exception]:
        return self._future.exception()

    def done(self) -> bool:
        return self._future.done()

    def fail(self) -> None:
        self._set_exception(DependencyFailure())

    def _set_result(self, result: t.Any) -> None:
        self._logger.info('Set result')
        self._future.set_result(result)

    def _set_exception(self, exception: Exception) -> None:
        self._logger.info('Set exception')
        self._future.set_exception(exception)


class ArgusResult:
    def __init__(self, sandbox_task_id: str, s3_client: t.Optional[S3Uploader] = None) -> None:
        self._logger: logging.Logger = logging.getLogger('ArgusResult')

        self._sandbox_task_id: str = sandbox_task_id
        self._s3_client: t.Optional[S3Uploader] = s3_client

        self._result: list[Result] = []
        self._result_lock: Lock = Lock()

        self._result_manager: t.Optional[Thread] = None
        self._result_queue: Queue[Result] = Queue()

        self._start_time: str = current_time()

        if s3_client is not None:
            self._result_manager = Thread(name='Argus Result Image Manager', target=self._manage_results)
            self._result_manager.start()

    def put(self, result: Result) -> None:
        if self._s3_client is None:
            with self._result_lock:
                self._result.append(result)
        else:
            self._result_queue.put(result)

    def get_and_save(self) -> None:
        if self._s3_client is not None:
            self._result_queue.put(POISON)
            self._result_manager.join()

        sbs_test_set_result = {
            'sandbox_id': int(self._sandbox_task_id),
            'start_time': self._start_time,
            'cases': list(self._aggregate_cases(self._result)),
            'filters_lists': [],
            'end_time': current_time(),
        }

        self._save_result(sbs_test_set_result)

    def _manage_results(self) -> None:
        while True:
            try:
                result = self._result_queue.get()
                if result is POISON:
                    return
                self._logger.info(f'Got result {result.browser} {result.adblocker} {result.url}. Uploading to S3')
                result.img_url = self._s3_client.upload_file(result.img_url)
                self._logger.info(f'Image url – {result.img_url}')
                if result.adblocker_url is not None:
                    result.adblocker_url = self._s3_client.upload_file(result.adblocker_url)
                with self._result_lock:
                    self._result.append(result)
            except Exception:
                self._logger.error(traceback.format_exc())

    def _save_result(self, result: dict):
        result_filename = f'result_{self._sandbox_task_id}.json'
        self._logger.info(f'Saving {result_filename}')
        with open(result_filename, 'w') as f:
            json.dump(result, f)
        if self._s3_client is not None:
            self._logger.info(f'Uploading {result_filename} to S3')
            result_json_url = self._s3_client.upload_file(result_filename)
            self._logger.info(f'Result url – {result_json_url}')

    @staticmethod
    def _aggregate_cases(cases: list[Result]) -> t.Iterable[dict]:
        reference_cases = defaultdict(dict)

        reference = [case for case in cases if case.adblocker == AdblockTypes.WITHOUT_ADBLOCK_CRYPTED.adb_name]
        for item in reference:
            reference_cases[item.browser][item.url] = item.headers['x-aab-requestid']
            yield item.dict()

        replay = [case for case in cases if case.adblocker != AdblockTypes.WITHOUT_ADBLOCK_CRYPTED.adb_name]
        for item in replay:
            if item.adblocker != AdblockTypes.WITHOUT_ADBLOCK.adb_name:
                reference_case_id = reference_cases.get(item.browser, {}).get(item.url)
                if reference_case_id is not None:
                    item.has_problem = 'new'
                    item.reference_case_id = reference_case_id
                else:
                    item.has_problem = 'no_reference_case'
            yield item.dict()


class BrowserTask(Task):
    """Single page of a target website."""

    MAX_ATTEMPTS: int = 5  # Hardcoded for first iteration

    __id_generator: t.Iterable[int] = count()

    def __init__(
        self,
        run_id: int,
        url_settings: UrlSettings,
        browser_info: BrowserInfo,
        extension: ExtensionAdb,
        argus_result: ArgusResult,
    ) -> None:
        self.run_id: int = run_id
        self.url_settings: UrlSettings = url_settings
        self.browser_info: BrowserInfo = browser_info
        self.extension: ExtensionAdb = extension

        self.case_id: int = next(BrowserTask.__id_generator)

        self._argus_result: ArgusResult = argus_result
        self._last_exception: t.Optional[Exception] = None
        self._attempts: int = 0

        super().__init__()

    def run(self, argus_browser: ArgusBrowser) -> None:
        try:
            argus_browser.browser.logger = logging.getLogger(f'{self}')
            result = argus_browser.get_result_for_task(self.run_id, self.case_id, self.url_settings)
        except Exception as e:
            self._logger.error(f'Exception occured: {traceback.format_exc()}')
            self._last_exception = e
            self._attempts += 1
            if self._attempts == self.MAX_ATTEMPTS:
                self._set_exception(self._last_exception)
            else:
                raise
        else:
            self._argus_result.put(result)
            self._set_result(result)

    def __hash__(self) -> int:
        return hash(self._get_main_members())

    def __eq__(self, other: t.Any) -> bool:
        if isinstance(other, BrowserTask):
            return self._get_main_members() == other._get_main_members()
        elif isinstance(other, tuple):
            return self._get_main_members() == other
        raise NotImplementedError(f'Equality operator not implemented between BrowserTask and {type(other).__name__}')

    def __str__(self) -> str:
        return f'{self.browser_info.name} {self.extension.type.adb_name} {self.url_settings.url}'.capitalize()

    def _get_main_members(self) -> tuple[str, str, AdblockTypes, str]:
        return self.browser_info.name, self.browser_info.version, self.extension.type, self.url_settings.url

    def get_context_identifier(self) -> tuple[str, str, AdblockTypes]:
        return self.browser_info.name, self.browser_info.version, self.extension.type
