# -*- coding: utf-8 -*-

import unittest

from ids.repositories.base import RepositoryBase
from ids.repositories.bound_base import BoundRepositoryBase
from ids.resource import Resource
from ids.storages.null import NullStorage


class TestBoundBaseRepository(unittest.TestCase):
    def test_wrapping_to_resource(self):
        res = Resource()
        res['__repository__'] = RepositoryBase(NullStorage())
        msg = 'incorrect __parent__ link'

        rep = BoundRepositoryBase(res, NullStorage())

        # проверим создаваемость полей в новом ресурсе
        r = rep._wrap_to_resource(object())
        self.assertTrue(r['__parent__'] is res, msg)

        del r['__parent__']

        # проверим создаваемость полей в существующем ресурсе
        r = rep._wrap_to_resource(object(), resource=r)
        self.assertTrue(r['__parent__'] is res, msg)
