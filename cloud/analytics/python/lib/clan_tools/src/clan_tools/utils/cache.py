import os
import pickle
import logging
from time import time
from functools import wraps
from typing import Dict, Tuple, Callable, Any, TypeVar, cast

logger = logging.getLogger(__name__)


Function = TypeVar('Function', bound=Callable[..., Any])


def cached(cachefile: str) -> Callable[[Function], Function]:
    """
    A function that creates a decorator which will use "cachefile" for caching the results of the decorated function "fn".
    """
    def decorator(func: Function) -> Function:
        """Define a decorator for a function `func`"""

        @wraps(func)
        def wrapper(*args: Tuple[Any, ...], **kwargs: Dict[str, Any]) -> Any:   # define a wrapper that will finally call `func`` with all arguments
            # if cache exists -> load it and return its content
            if os.path.exists(cachefile):
                with open(cachefile, 'rb') as cachehandle:
                    logger.info("using cached result from '%s'" % cachefile)
                    return pickle.load(cachehandle)

            # execute the function with all arguments passed
            res = func(*args, **kwargs)

            # write to cache file
            with open(cachefile, 'wb') as cachehandle:
                ts = time()
                logger.info("saving result to cache '%s'" % cachefile)
                pickle.dump(res, cachehandle)
                te = time()
                logger.info('func: {} pickle dump took: {:.3f} sec.'.format(func.__name__, te - ts))
            return res

        return cast(Function, wrapper)

    return decorator

__all__ = ['cached']
