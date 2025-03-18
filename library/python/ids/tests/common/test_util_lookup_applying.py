# -*- coding: utf-8 -*-

import operator
import unittest

from ids.utils.lookup_applying import object_lookupable


class TestLookupApplying(unittest.TestCase):
    def test_applying_simple(self):
        # запрос повторяет структуру объекта, для простого случая
        self.assertTrue(object_lookupable({'key': 'val'}, {'key': 'val'}))

        # запрос повторяет структуру объекта, для простого случая
        self.assertFalse(object_lookupable({'key': 'val'}, {'key': 'val1'}))

    def test_applying_complex(self):
        obj = {'test': {'long': {'key': 13}}}

        # запрос повторяет структуру объекта, включая значение
        lookup = {'test__long__key': 13}
        self.assertTrue(object_lookupable(lookup, obj))

        # запрос повторяет структуру объекта, но имеет другое значение
        lookup = {'test__long__key': 1}
        self.assertFalse(object_lookupable(lookup, obj))

        # пустой лукап всегда подходит
        self.assertTrue(object_lookupable({}, obj))

    def test_applying_multy(self):
        obj = {'test': {'long': {'key': 13}, 'new': 1}}

        # запрос повторяет структуру объекта, включая значение
        lookup = {'test__long__key': 13, 'test__new': 1}
        self.assertTrue(object_lookupable(lookup, obj))

        # запрос повторяет структуру объекта, но имеет другое значение
        lookup = {'test__long__key': 13, 'test__new': 2}
        self.assertFalse(object_lookupable(lookup, obj))

        # запрос повторяет структуру объекта, но имеет другое значение
        lookup = {'test__long__key': 12, 'test__new': 1}
        self.assertFalse(object_lookupable(lookup, obj))

        # запрос повторяет структуру объекта, но имеет другое значение
        lookup = {'test__long__key': 12, 'test__new': 2}
        self.assertFalse(object_lookupable(lookup, obj))

        # пустой лукап всегда подходит
        self.assertTrue(object_lookupable({}, obj))

    def test_default_prefix(self):
        # одинаковая реакция с суффиксом по-умолчанию
        self.assertTrue(object_lookupable({'key__exact': 'val'}, {'key': 'val'}))
        self.assertFalse(object_lookupable({'key__exact': 'val'}, {'key': 'val1'}))
        self.assertTrue(object_lookupable({'key': 'val'}, {'key': 'val'}))
        self.assertFalse(object_lookupable({'key': 'val'}, {'key': 'val1'}))

    def test_access_methods(self):
        class SomeClass(object):
            pass

        obj = SomeClass()
        obj.key = 'val'
        obj.complex = SomeClass()
        obj.complex.smth = 1

        self.assertTrue(object_lookupable({'key': 'val'}, obj,
                            access_method=operator.attrgetter))
        self.assertFalse(object_lookupable({'key': 'val1'}, obj,
                            access_method=operator.attrgetter))

        self.assertTrue(object_lookupable({'complex__smth': 1}, obj,
                            access_method=operator.attrgetter))
        self.assertFalse(object_lookupable({'complex__smth': 0}, obj,
                            access_method=operator.attrgetter))

    def test_applicable_suffixies(self):
        self.assertFalse(object_lookupable({'key__exact': 'val'}, {'key': 'Val'}))
        self.assertTrue(object_lookupable({'key__iexact': 'val'}, {'key': 'Val'}))

        self.assertTrue(object_lookupable({'key__contains': 'val'}, {'key': 'value'}))
        self.assertFalse(object_lookupable({'key__contains': 'val'}, {'key': 'Value'}))
        self.assertTrue(object_lookupable({'key__icontains': 'val'}, {'key': 'Value'}))

        self.assertTrue(object_lookupable({'key__startswith': 'val'}, {'key': 'value'}))
        self.assertFalse(object_lookupable({'key__startswith': 'val'}, {'key': 'Value'}))
        self.assertTrue(object_lookupable({'key__istartswith': 'val'}, {'key': 'Value'}))

        self.assertTrue(object_lookupable({'key__endswith': 'lue'}, {'key': 'value'}))
        self.assertFalse(object_lookupable({'key__endswith': 'lue'}, {'key': 'ValuE'}))
        self.assertTrue(object_lookupable({'key__iendswith': 'lue'}, {'key': 'ValuE'}))

        self.assertTrue(object_lookupable({'key__in': [1, 'qwe', True]}, {'key': 'qwe'}))
        self.assertFalse(object_lookupable({'key__in': [1, 'qwe', True]}, {'key': 0}))
        self.assertTrue(object_lookupable({'key__in': [1, ['qwe'], True]}, {'key': ['qwe']}))

        self.assertTrue(object_lookupable({'key__isnull': True}, {'key': None}))
        self.assertFalse(object_lookupable({'key__isnull': False}, {'key': None}))
        self.assertTrue(object_lookupable({'key__isnull': False}, {'key': 0}))

        self.assertTrue(object_lookupable({'key__regex': '\w+(\d+).(\d+)\w+'}, {'key': 'num12=3qw'}))
        self.assertFalse(object_lookupable({'key__regex': '\w+(\d+\d+)\w+'}, {'key': 'num12=3qw'}))
        self.assertTrue(object_lookupable({'key__regex': '.*'}, {'key': 'num12=3qw'}))

        self.assertTrue(object_lookupable({'key__gt': 0}, {'key': 1}))
        self.assertFalse(object_lookupable({'key__gt': 1}, {'key': 1}))
        self.assertTrue(object_lookupable({'key__gte': 1}, {'key': 1}))

        self.assertTrue(object_lookupable({'key__lt': 2}, {'key': 1}))
        self.assertFalse(object_lookupable({'key__lt': 2}, {'key': 2}))
        self.assertTrue(object_lookupable({'key__lte': 2}, {'key': 2}))

        self.assertTrue(object_lookupable({'key__range': (0, 2)}, {'key': 0}))
        self.assertTrue(object_lookupable({'key__range': (0, 2)}, {'key': 1}))
        self.assertTrue(object_lookupable({'key__range': (0, 2)}, {'key': 2}))
        self.assertFalse(object_lookupable({'key__range': (0, 2)}, {'key': 3}))
        self.assertFalse(object_lookupable({'key__range': (0, 2)}, {'key': -1}))
        self.assertTrue(object_lookupable({'key__range': (False, True)}, {'key': True}))
