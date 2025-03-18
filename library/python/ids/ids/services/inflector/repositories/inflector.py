# coding: utf-8
from __future__ import unicode_literals

import logging

import six

from ids.repositories.base import RepositoryBase
from ids.registry import registry
from ..connector import InflectorConnector
from ..resource import WordResource, TitleResource
from ..errors import InflectorBadArguments

logger = logging.getLogger(__name__)


@registry.add_simple
class InflectorRepository(RepositoryBase):
    SERVICE = 'inflector'
    RESOURCES = 'inflector'
    RESOURCE_CLASSES = {
        'word': WordResource,
        'title': TitleResource
    }

    def __init__(self, storage, **options):
        super(InflectorRepository, self).__init__(storage, **options)
        self.connector = InflectorConnector(**options)

    def get_user_session_id(self):
        return 'ANONYMOUS'

    def inflect_string(self, word, case, inflect_like_fio=False):
        return self.get_string_inflections(word, inflect_like_fio)[case]

    def inflect_person(self, person, case):
        if isinstance(person, six.string_types):
            # Склонять как строку с флажком fio=1, потому что неизвестен
            # порядок частей имени в ответе склонятора
            return self.inflect_string(person, case, inflect_like_fio=True)
        return self.get_person_inflections(person)[case]

    def get_string_inflections(self, word, inflect_like_fio=False):
        """
        Просклонять строку

        @param word: строка

        @rtype: WordResource
        """
        return WordResource(
            self.connector.get('word', url_vars={
                'wizclient': self.connector.user_agent,
                'word': word,
                'fio': '1' if inflect_like_fio else '0'
            })
        )

    def get_person_inflections(self,
                               person):
        """
        Просклонять ФИО

        @param person: dict-like объект с ключами
        first_name, last_name, middle_name, gender
        или объект с такими атрибутами

        @rtype: TitleResource"""
        try:
            try:
                first_name, last_name, \
                    middle_name, gender = self._extract_from_dict(person)
            except (TypeError, AttributeError, KeyError):
                first_name, last_name, \
                    middle_name, gender = self._extract_from_object(person)
            assert gender in ('m', 'f'), 'Wrong gender!'
            assert first_name, 'Wrong first_name!'
            assert last_name, 'Wrong last_name!'
        except (AttributeError, AssertionError):
            raise InflectorBadArguments(
                "Can't extract args from person neither as from dict "
                "nor as from object ",
                person
            )
        return TitleResource(
            self.connector.get('title', url_vars={
                'wizclient': self.connector.user_agent,
                'first_name': first_name,
                'last_name': last_name,
                'middle_name': middle_name,
                'gender': gender
            })
        )

    def _extract_from_dict(self, person):
        return self._extract_from_any(
            lambda obj, key, default=None: obj.get(key, default),
            person
        )

    def _extract_from_object(self, person):
        return self._extract_from_any(
            getattr, person
        )

    def _extract_from_any(self, extractor, any):
        return (
            extractor(any, 'first_name', None),
            extractor(any, 'last_name', None),
            extractor(any, 'middle_name', ''),
            extractor(any, 'gender', 'm'),
        )
