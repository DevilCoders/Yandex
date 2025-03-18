"""
    List of functions for caching.
"""

import re
from functools import wraps
import time

import ujson


def mongo_cache(backend_type, f, self, request, *args, **kwargs):
    """
        This decorator looks PATH in request, and checks if mongo cache database has it

        :type backend_type: str
        :type f: T
        :type self: api_backend.BasePage
        :type request: flask.ext.restful.Resource

        :param backend_type: backend type (api or wbe)
        :param f: GET handler func for <self> object. Construct json response
        :param self: Flask restfull class processor
        :param request: object with request data (such as url)

        return (dict): json result as python dict
    """
    import utils.mongo.update_wbeapi_cache as update_wbe_api_cache
    from core.db import CURDB

    if self.api.options.enable_cache:
        # create path
        path = None
        for prefix in ["^/unstable(.*)$", "^/trunk(.*)$", "^/stable-\d+-r\d+(.*)$"]:
            m = re.match(prefix, request.environ["PATH_INFO"])
            if m:
                path = m.group(1)
                break
        if path is not None:
            path = '{}?{}'.format(path, request.environ.get('QUERY_STRING', ''))

        # check if we have cached data
        if path is not None:
            options = {
                "action": update_wbe_api_cache.EActions.SHOWCACHED,
                "backend_type": backend_type,
                "commit": CURDB.get_repo().get_last_commit_id(),
                "request": path,
            }

            startt = time.time()
            util_result = update_wbe_api_cache.jsmain(options)
            endt = time.time()
            self.api.app.logger.info('[{}] [DEBUG] Rq <{}, {}, {}>: time to read from cache <{}>, found status status {}'.format(
                    self.api.make_time_str(),
                    backend_type,
                    CURDB.get_repo().get_last_commit_id(),
                    path,
                    endt - startt,
                    util_result is not None,
                )
            )
            if util_result is not None:
                return ujson.loads(util_result)

    # not cached run func and save cache
    func_result = f(self, *args, **kwargs)

    if self.api.options.enable_cache:
        options = {
            "action": update_wbe_api_cache.EActions.UPDATE,
            "backend_type": backend_type,
            "commit": CURDB.get_repo().get_last_commit_id(),
            "request": path,
            "response": ujson.dumps(func_result),
        }
        update_wbe_api_cache.jsmain(options)

    return func_result


def api_mongo_cache(f):
    def tmp(*args, **kwargs):
        return mongo_cache("api", f, *args, **kwargs)

    return tmp


def wbe_mongo_cache(f):
    def tmp(*args, **kwargs):
        return mongo_cache("wbe", f, *args, **kwargs)

    return tmp
