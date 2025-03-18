# coding: utf-8
from __future__ import unicode_literals

from datetime import date, datetime
from xml.etree import ElementTree

import six
from six.moves import map

from ids.registry import registry
from ids.exceptions import IDSException
from ids.repositories.base import RepositoryBase
from ids.resource import Resource
from ids.services.calendar.connector import CalendarConnector


@registry.add_simple
class CalendarHolidaysRepository(RepositoryBase):
    """
    Репозиторий праздников из календаря.

    http://wiki.yandex-team.ru/Calendar/holidays
    """

    SERVICE = 'calendar'
    RESOURCES = 'holidays'

    def __init__(self, storage=None, **options):
        super(CalendarHolidaysRepository, self).__init__(storage, **options)
        self.connector = CalendarConnector(**options)

    def getiter_from_service(self, lookup):
        """
        Получить итератор от сервиса.

        lookup params:
            * start_date | date
            * [end_date] | date
            * [out_mode] | (all|holidays|weekdays|overrides|with_names|
                            holidays_and_with_names)
            * country_id | int - id в геобазе.
                || Страна       | GeoBase ID    | Страна    | GeoBase ID    ||
                || Россия       | 225           | Литва     | 117           ||
                || Украина      | 187           | Латвия    | 206           ||
                || Татарстан    | 11119         | Турция    | 983           ||
                || Беларусь     | 149           | США       | 84            ||
                || Казахстан    | 159           | Польша    | 120           ||
                || Молдова      | 208           | Румыния   | 10077         ||

            * [for_yandex] | (0|1) для данной страны следует учесть
                дополнительные правила «для Яндекса»
            * who_am_i
        """
        lookup = self.handle_lookup(lookup)
        raw_xml = self.connector.get(self.RESOURCES, params=lookup)
        days = self.transform_xml_into_dicts(raw_xml=raw_xml)
        return map(self.wrap, days)

    # подготовка запроса
    def handle_lookup(self, lookup):
        self.handle_lookup_required_params(lookup)
        self.handle_lookup_defaults(lookup)
        self.handle_lookup_dates(lookup)
        return lookup

    def handle_lookup_required_params(self, lookup):
        required_params = set(['start_date', 'who_am_i'])
        present_params = set(six.iterkeys(lookup))
        if not required_params.issubset(present_params):
            missing_keys = required_params - present_params
            msg = 'Required lookup keys missing: %s' % ', '.join(missing_keys)
            raise IDSException(msg)

    def handle_lookup_defaults(self, lookup):
        default_params = {
            'out_mode': 'holidays',
            'country_id': 225,
            'for_yandex': 1,
        }
        for key, val in default_params.items():
            if key not in lookup:
                lookup[key] = val

    def handle_lookup_dates(self, lookup):
        for date_param in ('start_date', 'end_date'):
            if date_param not in lookup:
                continue

            if not isinstance(lookup[date_param], date):
                msg = '%s should be date instance'
                raise IDSException(msg % date_param)
            lookup[date_param] = lookup[date_param].isoformat()

    # обработка ответа
    def transform_xml_into_dicts(self, raw_xml):
        root = ElementTree.fromstring(raw_xml.encode('utf-8'))

        for day_elem in root.find('get-holidays/days'):
            raw_dict = dict(day_elem.items())
            yield self.to_python(raw_dict=raw_dict)

    def to_python(self, raw_dict):
        raw_dict['date'] = datetime.strptime(raw_dict['date'], '%Y-%m-%d').date()

        str_to_bool = lambda s: True if s == '1' else False
        raw_dict['is-holiday'] = str_to_bool(raw_dict['is-holiday'])
        raw_dict['is-transfer'] = str_to_bool(raw_dict['is-transfer'])
        return raw_dict

    def wrap(self, obj):
        return Resource(obj)
