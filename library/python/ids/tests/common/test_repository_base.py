# -*- coding: utf-8 -*-

import unittest
from copy import copy

from ids.repositories.base import RepositoryBase
from ids.storages.null import NullStorage


class TestBaseRepository(unittest.TestCase):
    def setUp(self):
        self.rep = RepositoryBase(NullStorage())
        self.rep.SERVICE = 'test_service'
        self.rep.RESOURCES = 'test_resources'
        self.rep.get_user_session_id = lambda: 'test_id'

    def test_default_options_overwriting(self):
        ops = self.rep._get_default_options()
        # создадим опции, отличающихся от дефолтных
        # новым ключом и значением существующего
        new_ops = RepositoryBase(NullStorage(),
                                    use_service=(not ops['use_service']),
                                    some_option='test_val',
                                ).options

        diff = set(new_ops) - set(ops)
        self.assertEqual(len(diff), 1, 'incorrect items count')
        self.assertEqual(diff.pop(), 'some_option', 'incorrect new items')
        self.assertTrue(new_ops['use_service'] == (not ops['use_service']),
                            'incorrect items value')

    def test_temp_options_overwriting(self):
        self.rep.options['test_option'] = 'test_value'
        ops = copy(self.rep.options)

        with self.rep._temp_options(
                    {
                        'test_option': 'test_another_value',
                        'some_option': 'test_val'
                    }
                ):
            new_ops = self.rep.options

            diff = set(new_ops) - set(ops)
            self.assertEqual(len(diff), 1, 'incorrect items count')
            self.assertEqual(diff.pop(), 'some_option', 'incorrect new items')
            self.assertTrue(new_ops['test_option'] == 'test_another_value',
                                'incorrect items value')

    def test_temp_options_depth(self):
        msg = 'incorrect item value'
        msg_mode_on = 'temp options mode should be on'
        msg_mode_off = 'temp options mode should be off'

        key = 'test_level'
        self.rep.options[key] = 0
        self.assertEqual(len(self.rep._options_state_stack), 0, msg_mode_off)
        with self.rep._temp_options({key: 1}):
            self.assertEqual(self.rep.options[key], 1, msg)
            self.assertEqual(len(self.rep._options_state_stack), 1, msg_mode_on)
            with self.rep._temp_options({key: 2}):
                self.assertEqual(self.rep.options[key], 2, msg)
                self.assertEqual(len(self.rep._options_state_stack), 2, msg_mode_on)
                with self.rep._temp_options({key: 3}):
                    self.assertEqual(self.rep.options[key], 3, msg)
                    self.assertEqual(len(self.rep._options_state_stack), 3, msg_mode_on)
                self.assertEqual(self.rep.options[key], 2, msg)
            self.assertEqual(self.rep.options[key], 1, msg)
        self.assertEqual(self.rep.options[key], 0, msg)
        self.assertEqual(len(self.rep._options_state_stack), 0, msg_mode_off)

    def test_wrapping_to_resource(self):
        obj = object()
        msg_rep = 'incorrect __repository__ link'
        msg_raw = 'incorrect __raw__ link'

        # проверим создаваемость полей в новом ресурсе
        r = self.rep._wrap_to_resource(obj)
        self.assertTrue(r['__repository__'] is self.rep, msg_rep)
        self.assertTrue(r['__raw__'] is obj, msg_raw)

        del r['__repository__']
        del r['__raw__']

        # проверим создаваемость полей в существующем ресурсе
        r = self.rep._wrap_to_resource(obj, resource=r)
        self.assertTrue(r['__repository__'] is self.rep, msg_rep)
        self.assertTrue(r['__raw__'] is obj, msg_raw)

    def test_making_storage_key_similarity(self):
        # один и тот же репозиторий с одинаковыми лукапами
        # должен давать одинаковый хеш
        key1 = self.rep._make_storage_key({'name': 'test', 'which': 'all'})

        lookup = {'which': 'all'}
        lookup['name'] = 'test'
        key2 = self.rep._make_storage_key(lookup)

        self.assertEqual(key1, key2, 'similar lookups has different hashes')

    def test_making_storage_key_difference(self):
        # один и тот же репозиторий с разными лукапами
        # должен давать разные хеши
        msg = 'different lookups has similar hashes'

        key1 = self.rep._make_storage_key({'name': 'test', 'which': 'all'})
        key2 = self.rep._make_storage_key({'name': 'test', 'which': 'all_'})
        self.assertNotEqual(key1, key2, msg)

        key2 = self.rep._make_storage_key({'name': 'test', 'which_': 'all'})
        self.assertNotEqual(key1, key2, msg)

        key2 = self.rep._make_storage_key({'name': 'test'})
        self.assertNotEqual(key1, key2, msg)

    def test_making_storage_key_options_difference(self):
        # один и тот же репозиторий с одинаковыми лукапами и
        # разными опциями должен давать разные хеши
        msg = 'similar lookups with different options has similar hashes'
        lookup = {'name': 'test', 'which': 'all'}

        key1 = self.rep._make_storage_key(lookup)
        self.rep.SERVICE += 'suffix'
        key2 = self.rep._make_storage_key(lookup)
        self.assertNotEqual(key1, key2, msg)

        key1 = self.rep._make_storage_key(lookup)
        self.rep.RESOURCES += 'suffix'
        key2 = self.rep._make_storage_key(lookup)
        self.assertNotEqual(key1, key2, msg)

        key1 = self.rep._make_storage_key(lookup)
        self.rep.get_user_session_id = lambda: 'test_id-suffix'
        key2 = self.rep._make_storage_key(lookup)
        self.assertNotEqual(key1, key2, msg)

    def test_making_storage_key_with_user_fields_lookup(self):
        self.rep.options['fields_mapping'] = {'test': 'long_test_name'}

        nice_lookup = {'test': 'val'}
        clear_lookup = {'long_test_name': 'val'}

        nice_hash = self.rep._make_storage_key(nice_lookup)
        clear_hash = self.rep._make_storage_key(clear_lookup)
        self.assertEqual(nice_hash, clear_hash, 'incorrect user fields handling')
