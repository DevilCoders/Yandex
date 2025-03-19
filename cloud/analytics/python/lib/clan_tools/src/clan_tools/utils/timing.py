import logging
from functools import wraps
from time import time, process_time
from typing import Dict, Tuple, Callable, Any, TypeVar, cast

logger = logging.getLogger(__name__)


Function = TypeVar('Function', bound=Callable[..., Any])


def timing(func: Function) -> Function:
    @wraps(func)
    def wrapper(*args: Tuple[Any, ...], **kwargs: Dict[str, Any]) -> Any:
        ts, ts_cpu = time(), process_time()
        result = func(*args, **kwargs)
        te, te_cpu = time(), process_time()
        logger.info(
            ('func: {} took: {:.3f} sec, {:.3f} sec CPU time.').format(func.__name__, te - ts, te_cpu - ts_cpu))
        return result

    return cast(Function, wrapper)

__all__ = ['timing']
