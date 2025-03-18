# -*- coding: utf-8 -*-

import unittest

from ids.exceptions import KeyIsAbsentError, KeyAlreadyExistsError
from ids.registry import _Registry


class TestRegistry(unittest.TestCase):
    def setUp(self):
        self.registry = _Registry()

    def test_add_repository(self):
        srv = 'test_service'
        res = 'test_resources'
        obj = object()
        factory = lambda **kwargs: obj

        # если в реестр ничего не добавлялось - там должно быть пусто
        self.assertRaises(KeyIsAbsentError,
                            self.registry.get_repository, srv, res, user_agent='test')

        # при вызове get, добавленная в реестр фабрика должна отработать
        self.registry.add_repository(srv, res, factory)
        rep = self.registry.get_repository(srv, res, user_agent='test')
        self.assertEqual(id(rep), id(obj), 'registry should call factory method')

        # нельзя замещать что-то, что уже добавлено в реестр
        self.assertRaises(KeyAlreadyExistsError,
                            self.registry.add_repository, srv, res, factory)

    def test_get_repository(self):
        srv = 'test_service'
        res = 'test_resources'

        # если в реестр ничего не добавлялось - там должно быть пусто
        self.assertRaises(KeyIsAbsentError,
                            self.registry.get_repository, srv, res, user_agent='test')

    def test_delete_repository(self):
        srv = 'test_service'
        res = 'test_resources'

        # если в реестр ничего не добавлялось - удаление вызовет ошибку
        self.assertRaises(KeyIsAbsentError,
                            self.registry.delete_repository, srv, res)

        # при вызове delete, последущий get должен поднимать исключение
        self.registry.add_repository(srv, res, lambda **kwargs: object())
        try:  # get не должен вызывать исключения
            self.registry.get_repository(srv, res, user_agent='test')
        except Exception:
            self.fail('get_repository should properly return object')
        self.registry.delete_repository(srv, res)
        self.assertRaises(KeyIsAbsentError, self.registry.get_repository, srv, res, user_agent='test')
