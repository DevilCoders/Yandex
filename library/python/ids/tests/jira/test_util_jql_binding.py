# -*- coding: utf-8 -*-

import unittest

from ids.exceptions import WrongValueError

from ids.services.jira.utils.jql_binding import lookup2jql


class TestJQLBinding(unittest.TestCase):
    def test_default_suffix(self):
        jql = 'key = "val"'
        self.assertEqual(lookup2jql({'key': 'val'}), jql)
        self.assertEqual(lookup2jql({'key__exact': 'val'}), jql)

    def test_different_suffixies(self):
        self.assertEqual(lookup2jql({'key__exact': 'val'}), 'key = "val"')
        self.assertEqual(lookup2jql({'key__contains': 'val'}), 'key ~ "val"')
        self.assertEqual(lookup2jql({'key__in': 'val'}), 'key in "val"')
        self.assertEqual(lookup2jql({'key__is': 'val'}), 'key is "val"')
        self.assertEqual(lookup2jql({'key__gt': 'val'}), 'key > "val"')
        self.assertEqual(lookup2jql({'key__gte': 'val'}), 'key >= "val"')
        self.assertEqual(lookup2jql({'key__lt': 'val'}), 'key < "val"')
        self.assertEqual(lookup2jql({'key__lte': 'val'}), 'key <= "val"')
        self.assertEqual(lookup2jql({'key__nexact': 'val'}), 'key != "val"')
        self.assertEqual(lookup2jql({'key__ncontains': 'val'}), 'key !~ "val"')
        self.assertEqual(lookup2jql({'key__nin': 'val'}), 'key not in "val"')
        self.assertEqual(lookup2jql({'key__nis': 'val'}), 'key is not "val"')

    def test_values_transformation(self):
        self.assertEqual(lookup2jql({'key': 'val'}), 'key = "val"')
        self.assertEqual(lookup2jql({'key': ''}), 'key = empty')
        self.assertEqual(lookup2jql({'key': 13}), 'key = "13"')
        self.assertEqual(lookup2jql({'key': 13.123}), 'key = "13.123"')
        self.assertEqual(lookup2jql({'key': True}), 'key = "True"')
        self.assertEqual(lookup2jql({'key': None}), 'key = null')
        self.assertRaises(WrongValueError, lookup2jql, {'key': []})
        self.assertEqual(lookup2jql({'key': [1, 2, 'val']}),
                                     'key = ("1", "2", "val")')
        self.assertEqual(lookup2jql({'key': ['', None, 'empty', 'null']}),
                                     'key = (empty, null, "empty", "null")')

    def test_keys_transformation(self):
        self.assertEqual(lookup2jql({'key': 'val'}), 'key = "val"')
        self.assertRaises(WrongValueError, lookup2jql, {'': 'val'})
        self.assertEqual(lookup2jql({'"Planner Project ID"': 'val'}),
                                     '"Planner Project ID" = "val"')

        # not a part of jql syntax
        self.assertEqual(lookup2jql({'key__complex': 'val'}),
                                     'key.complex = "val"')
        self.assertEqual(lookup2jql({'key__complex__attr': 'val'}),
                                     'key.complex.attr = "val"')
