# -*- coding: utf-8 -*-

import unittest

import pytest

from ids.storages.local_memory import LocalMemoryStorage
from ids.storages.memcached import MemcachedStorage


class TestStorage(unittest.TestCase):

    STORAGE_TO_TEST_CLASS = None

    def setUp(self):
        if self.STORAGE_TO_TEST_CLASS is None:
            return False

        self.storage = self.STORAGE_TO_TEST_CLASS()

    def test_add_to_storage(self):
        if self.STORAGE_TO_TEST_CLASS is None:
            return

        key = 'test_key'
        self.storage.delete(key)  # чтобы убедиться в чистоте хранилища

        msg = 'error: adding object != getting object'
        obj = [10, {'test': 'val'}, 'string']
        self.assertTrue(self.storage.add(key, obj))
        # добавленный объект должен быть равен полученному
        self.assertEqual(self.storage.get(key), obj, msg)

        new_key = key * 2
        self.storage.delete(new_key)  # чтобы убедиться в чистоте хранилища

        new_obj = {10: [{'test': 'val'}, 'string']}
        self.assertTrue(self.storage.add(new_key, new_obj))
        self.assertEqual(self.storage.get(new_key), new_obj, msg)
        # разные ключи - разные объекты
        self.assertNotEqual(self.storage.get(new_key), self.storage.get(key),
                                'consistency of storage failed')

        # нельзя добавить новый объект с уже существующим ключом
        self.assertFalse(self.storage.add(key, new_obj))

        self.storage.set(key, new_obj)
        self.assertEqual(self.storage.get(key), new_obj, 'overwriting has failed')

    def test_delete_from_storage(self):
        if self.STORAGE_TO_TEST_CLASS is None:
            return

        key = 'test_key'

        # добавляем и проверяем, что по ключу не пусто
        self.storage.delete(key)  # чтобы убедиться в чистоте хранилища
        self.storage.add(key, object())
        self.assertTrue(self.storage.get(key) is not None)

        # удаляем и проверяем, что стало пусто
        self.assertTrue(self.storage.delete(key))
        self.assertTrue(self.storage.get(key) is None)

        # попытка удалить несуществующий ключ не приводит к удалению чего-либо
        key = 'another_key'
        self.assertTrue(self.storage.get(key) is None)
        self.storage.delete(key)
        self.assertTrue(self.storage.get(key) is None)


@pytest.mark.skip
class TestLocalMemoryStorage(TestStorage):

    STORAGE_TO_TEST_CLASS = LocalMemoryStorage


@pytest.mark.skip
class TestMemcachedStorage(TestStorage):

    STORAGE_TO_TEST_CLASS = MemcachedStorage
