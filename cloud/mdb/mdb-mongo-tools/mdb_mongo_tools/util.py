"""
Common functions
"""
import contextlib
import copy
import logging
import os
import shutil
import subprocess  # nosec
from shlex import split as shlex_split
from typing import Any, Dict, Generator, Optional, Tuple, Union

import filelock
import tenacity
import yaml

from mdb_mongo_tools.exceptions import LocalLockNotAcquired

MAX_RETRIES = 3
DEFAULT_RETRY_DELAY = 3
DEFAULT_EXEC_TIMEOUT = 10


def recursively_update(base_dict: Dict[Any, Any], update_dict: Dict[Any, Any]) -> None:
    """
    Update dict
    """
    for key, value in update_dict.items():
        if isinstance(value, dict):
            if key not in base_dict:
                base_dict[key] = {}
            recursively_update(base_dict[key], update_dict[key])
        else:
            base_dict[key] = value


def purge_dir_contents(path: str) -> None:
    """
    Delete directory contents
    """
    try:
        for file_name in os.listdir(path):
            file_path = os.path.join(path, file_name)
            if os.path.isfile(file_path):
                os.unlink(file_path)
            else:
                shutil.rmtree(file_path)
    except FileNotFoundError:
        pass


def exec_command(cmd: str, timeout: int = DEFAULT_EXEC_TIMEOUT) -> Tuple[int, str, str]:
    """
    Execute command with timeout
    """
    proc = subprocess.Popen(shlex_split(cmd), stdout=subprocess.PIPE, stderr=subprocess.PIPE)  # nosec
    try:
        std_out, std_err = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        std_out, std_err = proc.communicate()

    return proc.returncode, std_out.decode("utf-8"), std_err.decode("utf-8")


def setup_logging(conf: Dict[str, str]) -> None:
    """
    Configure logging
    """
    root_level = getattr(logging, conf['log_level_root'].upper(), None)
    log_file = conf.get('log_file', '')
    logging.basicConfig(level=root_level, format=conf['log_format'], filename=log_file)


def load_config(filename: str, default_config: Dict[Any, Any]) -> Dict[Any, Any]:
    """
    Apply custom config to default
    """
    config = copy.deepcopy(default_config)
    try:
        with open(filename) as fobj:
            custom_config = yaml.safe_load(fobj)
            if custom_config:
                recursively_update(config, custom_config)
    except FileNotFoundError:
        pass

    return config


@contextlib.contextmanager
def local_lock(path: str) -> Generator[filelock.FileLock, None, None]:
    """
    Local lock wrapper
    """
    lock = filelock.FileLock(path)

    try:
        lock.acquire(timeout=0)
        yield lock
    except filelock.Timeout:
        raise LocalLockNotAcquired

    lock.release()


def retry(
        exception_types: Union[type, Tuple[type, ...]] = Exception,  # pylint: disable=bad-whitespace
        max_attempts: Optional[int] = None,
        max_interval: Optional[float] = 5,
        max_wait: Optional[int] = 10):
    """
    Function decorator that retries wrapped function on failures
    """

    def _log_retry(retry_state: tenacity.RetryCallState) -> None:
        logging.debug("Retrying %s.%s in %.2fs, attempt: %s, reason: %r", retry_state.fn.__module__,
                      retry_state.fn.__qualname__, retry_state.next_action.sleep, retry_state.attempt_number,
                      retry_state.outcome.exception())

    assert not all([max_attempts, max_wait]), "max_attempts and max_wait are mutually exclusive"
    stop = tenacity.stop_after_delay(max_wait)

    if max_attempts is not None:
        stop = tenacity.stop_after_attempt(max_attempts)  # type: ignore

    return tenacity.retry(
        retry=tenacity.retry_if_exception_type(exception_types),
        wait=tenacity.wait_random_exponential(multiplier=0.5, max=max_interval),
        stop=stop,
        reraise=True,
        before_sleep=_log_retry)
