# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry
from ids.repositories.base import RepositoryBase
from ids.services.calendar_internal.connector import (
    CalendarInternalConnector,
    CalendarInternalError,
)


@registry.add_simple
class CalendarInternalEventRepository(RepositoryBase):
    """
    Репозиторий одного события из календаря.

    https://wiki.yandex-team.ru/Calendar/api/new-web/#get-event
    """
    SERVICE = 'calendar_internal'
    RESOURCES = 'event'

    connector_cls = CalendarInternalConnector

    def getiter_from_service(self, lookup):
        return self.connector.get(
            resource=self.RESOURCES,
            url_vars={'method': 'get'},
            params=lookup,
        )

    def update_(self, resource, fields):
        response = self.connector.post(
            resource=self.RESOURCES,
            url_vars={'method': 'update'},
            params=resource,  # пробросятся в requests и преобразуются в query-параметры
            json=fields,
        )
        error = response.get('error')
        if error:
            raise CalendarInternalError(error['name'], error['message'])
