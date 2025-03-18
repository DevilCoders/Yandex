# coding: utf-8

from __future__ import unicode_literals

from unittest import TestCase

from ids.services.formatter.structure import (
    WikiDictNode as DictNode,
    WikiListNode as ListNode,
)


class WikiDictNodeTest(TestCase):
    def test_type_property_normal(self):
        node = DictNode({'block': 'wiki-header'})

        self.assertTrue(hasattr(node, 'type'))
        self.assertEqual(node.type, 'header')

    def test_type_property_tricky(self):
        node = DictNode({'block': 'wiki-wiki-header'})

        self.assertTrue(hasattr(node, 'type'))
        self.assertEqual(node.type, 'wiki-header')

    def test_attrs_property(self):
        attrs = {'login': 'volozh'}
        node = DictNode({'wiki-attrs': attrs})

        self.assertTrue(hasattr(node, 'attrs'))
        self.assertEqual(node.attrs, attrs)

    def test_attrs_property_no_attrs(self):
        node = DictNode({'not-wiki-attrs': 'DUMMY'})

        self.assertTrue(hasattr(node, 'attrs'))
        self.assertEqual(node.attrs, {})

    def test_content_property(self):
        content = [1, 2, 3]
        node = DictNode({'content': content})

        self.assertTrue(hasattr(node, 'content'))
        self.assertEqual(node.content, content)
        assert isinstance(node.content, ListNode)

    def test_content_property_no_content(self):
        node = DictNode({'not-content': 'DUMMY'})

        self.assertTrue(hasattr(node, 'content'))
        self.assertEqual(node.content, [])


class WikiNodesWrappingTest(TestCase):
    def test_dot_access_simple(self):
        node = DictNode({'key': 'val'})

        self.assertTrue(hasattr(node, 'key'))
        self.assertEqual(node.key, 'val')

    def test_dot_access_nested(self):
        node = DictNode({'user': {'login': 'batman'}})

        self.assertTrue(hasattr(node, 'user'))
        self.assertEqual(node.user, {'login': 'batman'})
        self.assertTrue(hasattr(node.user, 'login'))
        self.assertEqual(node.user.login, 'batman')

    def test_get_method_not_forgotten(self):
        node = DictNode({'user': {'login': 'batman'}})

        user = node.get('user')

        assert isinstance(user, DictNode)

    def test_getitem_still_works(self):
        node = DictNode({'user': {'login': 'batman'}})

        assert 'user' in node
        user = node['user']

        assert isinstance(user, DictNode)

    def test_dot_access_in_lists(self):
        node = DictNode({'users': [
            {'login': 'batman'},
            {'login': 'joker'},
        ]})

        self.assertTrue(hasattr(node, 'users'))
        assert isinstance(node.users, ListNode)

        users = node.users
        self.assertTrue(all(
            isinstance(element, DictNode) for element in users
        ))

        user0 = users[0]
        self.assertTrue(hasattr(user0, 'login'))
        self.assertTrue(user0.login, 'batman')
