# coding: utf-8

from __future__ import unicode_literals, absolute_import

import six
from six.moves import map
from six.moves.collections_abc import Mapping, Iterable

from ids.resource import Resource


class WikiDictNode(Resource):
    """
    Основной класс, представляющий узел wiki-дерева типа dict.
    Корень вики-разметки также является узлом такого типа.

    * К элементам можно обращаться как к атрибуту (getattr), либо как к полю
      (getitem).
    * Вложенные дикты заворачиваются в этот же класс, вложенные списки в
      WikiListNode, который также правильно завернет вложенные структуры.
    * Основные параметры узла доступны через property (бывают ли еще какие-то?).
    * В данный момент классы не предполагают редактирования/изменения
      данных.
    * Метод filter позволяет искать в дереве по типу узла с учетом атрибутов
      узла.
    """
    @property
    def type(self):
        prefix, node_type = self.block.split('wiki-', 1)
        return node_type

    @property
    def attrs(self):
        return self.get('wiki-attrs', {})

    @property
    def content(self):
        return self.get('content', [])

    # служебные методы
    def __getitem__(self, item):
        value = super(WikiDictNode, self).__getitem__(item)
        return wrap_wiki_node(value)

    def __getattr__(self, item):
        return self.__getitem__(item)

    def get(self, *args, **kwargs):
        value = super(WikiDictNode, self).get(*args, **kwargs)
        return wrap_wiki_node(value)

    # публичные методы
    def filter(self, type, attrs=None):
        from .utils import filter_nodes
        return filter_nodes(self, type, attrs)

    # публичные методы для получения специальных сущностей
    filter_person_nodes = lambda self, **kw: self.filter(type='staff')
    filter_ticket_nodes = lambda self, **kw: self.filter(type='jira')


class WikiListNode(list):
    """
    Класс представляет узел wiki-дерева типа list.
    При обращении к элементам заворачивает вложенные стрктуры в Wiki*Node
    """
    def __getitem__(self, item):
        item = super(WikiListNode, self).__getitem__(item)
        return wrap_wiki_node(item)

    def __iter__(self):
        return map(wrap_wiki_node, super(WikiListNode, self).__iter__())


def wrap_wiki_node(node):
    """
    Пока не очень получилось генерализовать эту враппилку, ее нужно
    инициализировать классом, в который оборачиваем.
    """
    if isinstance(node, Mapping):
        return WikiDictNode(node)
    elif isinstance(node, Iterable) and not isinstance(node, six.string_types):
        return WikiListNode(node)
    else:
        return node
