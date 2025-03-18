# coding: utf-8

from __future__ import unicode_literals

from pretend import stub
from unittest import TestCase

from ids.services.formatter.utils import (
    is_type_matched,
    is_attrs_matched,
    filter_nodes,
)


class FilterTest(TestCase):
    def test_is_type_matched(self):
        node = stub(type='text')

        self.assertTrue(is_type_matched(node, 'text'))
        self.assertFalse(is_type_matched(node, 'bullet_list'))

    def test_is_attrs_matched(self):
        node = stub(attrs={'login': 'devil', 'number': 666})

        self.assertTrue(is_attrs_matched(node, {'login': 'devil'}))
        self.assertTrue(is_attrs_matched(node, {'number': 666}))
        self.assertTrue(is_attrs_matched(
            node,
            {'login': 'devil', 'number': 666}
        ))
        self.assertFalse(is_attrs_matched(node, {'age': 300}))
        self.assertFalse(is_attrs_matched(
            node,
            {'login': 'devil', 'number': 666, 'age': 300}
        ))

    def test_filter_by_type_simple(self):
        node = stub(type='text', attrs={'login': 'DUMMY'}, content=[])

        text_nodes = list(filter_nodes(node, type='text'))

        self.assertEqual(len(text_nodes), 1)
        self.assertEqual(text_nodes[0], node)

        person_nodes = list(filter_nodes(node, type='person'))
        self.assertEqual(len(person_nodes), 0)

    def test_filter_by_type_and_attrs_simple(self):
        node = stub(type='text', attrs={'login': 'batman'}, content=[])

        batman_text_nodes = list(
            filter_nodes(node, type='text', attrs={'login': 'batman'})
        )

        self.assertEqual(len(batman_text_nodes), 1)
        self.assertEqual(batman_text_nodes[0], node)

        joker_text_nodes = list(
            filter_nodes(node, type='text', attrs={'login': 'joker'})
        )

        self.assertEqual(len(joker_text_nodes), 0)

    def test_search_in_content(self):
        person_node = stub(type='person', attrs={}, content=[])
        text_node = stub(type='text', attrs={}, content=[])
        root = stub(type='text', attrs={}, content=[person_node, text_node])

        text_nodes = list(filter_nodes(root, type='text'))

        self.assertEqual(len(text_nodes), 2)
        self.assertEqual(text_nodes[0], root)
        self.assertEqual(text_nodes[1], text_node)

        person_nodes = list(filter_nodes(root, type='person'))

        self.assertEqual(len(person_nodes), 1)
        self.assertEqual(person_nodes[0], person_node)
