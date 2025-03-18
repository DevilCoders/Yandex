# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

import logging
import json
from functools import partial

from ..exceptions import ApiCallError

if False:  # pragma: nocover
    from ..http import RefsResponse
    from ..client import Refs
    from collections import Iterable


LOG = logging.getLogger(__name__)


class RefsRequest(object):
    """Абстракция для упрощения составления запросов к GraphQL."""

    def __init__(self, resource_name, fields):
        """
        :param str|unicode resource_name:
        :param list fields:
        """
        self._name = resource_name
        self._fields = fields
        self._filters = []

    def add_filter(self, name, value, add_field=None):
        """Добавляет фильтр к запросу.

        :param str|unicode name: Имя фильтра.

        :param bool|Iterable value: Значение

        :param str|unicode|Iterable add_field: Имя поля, которое нужно добавить в выдчу,
            если задейстован фильтр.

        """
        if not value:
            return

        self._filters.append('%s:%s' % (name, json.dumps(value)))

        if add_field:
            if isinstance(add_field, (list, tuple)):
                for field in add_field:
                    self._fields.append(field)
            else:
                self._fields.append(add_field)

    def build(self):
        """Собирает текст запроса.

        :rtype: str|unicode

        """
        filters = self._filters

        if filters:
            filters = '(%s)' % ', '.join(filters)

        else:
            filters = ''

        return '{%s%s{%s}}' % (self._name, filters, ' '.join(self._fields))


class RefBase(object):

    alias = '<unset>'

    def __init__(self, refs):
        """
        :param Refs refs:
        """
        self._refs = refs
        self._request = partial(refs._connector.request, url=self.alias)

    def _do_request(self, **kwargs):
        response = self._request(**kwargs)  # type: RefsResponse
        errors = response.get('errors')
        if errors:
            status_code = response._response.status_code
            msg = json.dumps(errors)
            LOG.debug('API call error, code [%s]:\n%s', status_code, msg)
            raise ApiCallError(msg, status_code)
        return response

    def get_types(self):
        """Возвращает описание типов в данном справочнике в виде словаря,
        где ключи - это имена типов, а значения - их описания.

        :rtype: dict

        """
        return self._refs.generic_get_types(self.alias)

    def get_resources(self):
        """Возвращает описание ресурсов в данном справочнике в виде словаря,
        где ключи - это имена ресурсов, а значения - словарь с описанием.

        :rtype: dict

        """
        return self._refs.generic_get_resources(self.alias)

    def get_type_fields(self, type_name):
        """Возвращает описание полей для указанного типа в данном справочнике
        в виде словаря, где ключи - это имена полей, а значения - их описания.

        :param str|unicode type_name: Название типа.
                Например: Event

        :rtype: dict

        """
        return self._refs.generic_get_type_fields(self.alias, type_name)
