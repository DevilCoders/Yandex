# -*- coding: utf-8 -*-
"""
DBaaS Internal API Base QueryCache class
"""

import logging
import os

import library.python.resource as _ya_res


class BaseQueryCache:
    """
    Really stupid query conf
    """

    def __init__(self, logger_name):
        self.queries = {}
        self.logger = logging.getLogger(logger_name)

    def _read_queries_from_res(self, base_prefix):
        new_queries = {}
        prefix = base_prefix + '/'
        for res_key in _ya_res.iterkeys(prefix):
            file_name = res_key[res_key.index(prefix) + len(prefix) :]
            res_bytes = _ya_res.find(res_key)
            new_queries[os.path.splitext(file_name)[0]] = str(res_bytes, encoding='utf-8')
        self.logger.debug("read %d queries from resources", len(new_queries))
        if not new_queries:
            raise RuntimeError("no queries found for %r prefix" % prefix)
        return new_queries

    def reload(self, base_path, base_prefix):
        """
        Reload query cache
        """
        self.queries = self._read_queries_from_res(base_prefix)

    def stat(self):
        """
        Get number of loaded queries
        """
        return {'cache_size': len(self.queries)}
