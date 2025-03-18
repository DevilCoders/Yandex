# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry
from ids.repositories.base import RepositoryBase
from ids.resource import Resource
from ids.services.calendar_internal.connector import CalendarInternalConnector


@registry.add_simple
class CalendarInternalHolidaysRepository(RepositoryBase):
    """
    Репозиторий праздников из календаря.

    https://wiki.yandex-team.ru/Calendar/api/new-web/#get-holidays
    """

    SERVICE = 'calendar_internal'
    RESOURCES = 'holidays'

    connector_cls = CalendarInternalConnector

    def getiter_from_service(self, lookup):
        response = self.connector.get(self.RESOURCES, params=lookup)
        return map(self.wrap, response[self.RESOURCES])

    def wrap(self, obj):
        return Resource(obj)
