# -*- coding: utf-8 -*-

import unittest

from ids.utils.fields_mapping import unbeautify_fields, beautify_fields


class TestFieldsMapping(unittest.TestCase):
    def test_unbeautify(self):
        lookup = {'field': 'value', 'user_field': 'user_value'}
        mapping = {'user_field': 'long_ugly_name'}

        clean_lookup = unbeautify_fields(lookup, mapping)

        # количество полей не должно измениться
        self.assertTrue(len(clean_lookup) == len(lookup))
        # поля, отсутствующие в маппинге не должны измениться
        self.assertEqual(lookup['field'], clean_lookup['field'])
        # поля изменились, согласно маппингу, но значения не поменялись
        self.assertEqual(lookup['user_field'], clean_lookup['long_ugly_name'])

    def test_beautify(self):
        lookup = {'field': 'value', 'long_ugly_name': 'user_value'}
        mapping = {'user_field': 'long_ugly_name'}

        clean_lookup = beautify_fields(lookup, mapping)

        # количество полей не должно измениться
        self.assertTrue(len(clean_lookup) == len(lookup))
        # поля, отсутствующие в маппинге не должны измениться
        self.assertEqual(lookup['field'], clean_lookup['field'])
        # поля изменились, согласно маппингу, но значения не поменялись
        self.assertEqual(clean_lookup['user_field'], lookup['long_ugly_name'])
