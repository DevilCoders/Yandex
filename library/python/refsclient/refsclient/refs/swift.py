# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

from ._base import RefBase, RefsRequest
from .utils import cast_to_date_str, cast_from_date_str


class Swift(RefBase):

    alias = 'swift'

    def get_bics_info(self, bics, fields=None):
        """Возвращает информацию по указанным BIC в виде словаря,
        где ключи - номера bic11, а значения - информация, связанная с номером.

        ВНИМАНИЕ: возвращаются только активные (актуальные) записи.

        :param list bics: BICи, для которых требуется получить информацию.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * номер bic8
                * номер подразделения (bic8 + номер_подразделения = bic11)
                * наименование организации

        :rtype: dict

        """
        fields = fields or [
            'instName',
        ]

        fields.extend([
            'bic8',
            'bicBranch',
        ])

        request = RefsRequest(resource_name='bics', fields=fields)
        request.add_filter('bic', bics)

        response = self._do_request(data=request.build())

        results = {}

        for item in response['data']['bics']:
            bic11 = item['bic8'] + item['bicBranch']
            results[bic11] = item

        return results

    def get_holidays_info(self, date_since=None, date_to=None, countries=None, fields=None):
        """Возвращает информацию по нерабочим (праздничным, выходным) дням в виде
        списка словарей с информацией.

        :param date|datetime|str|unicode date_since: Дата, начиная с которой
            требуется получить информацию. Объект даты/датывремени или строка
            в ISO-формате: 2018-10-15. Если не указана, используется текущая дата.

        :param date|datetime|str|unicode date_to: Дата, по которую (т.е. включительно)
            требуется получить информацию. Объект даты/датывремени или строка
            в ISO-формате: 2018-10-15. Если не указана, используется текущая дата.

        :param list[str] countries: Двухбуквенные идентификтаоры стран (например, RU),
            для которых требуется получить онформацию.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * дата нерабочего дня
                * двухвуквенный код страны (например, RU)

        :rtype: list[dict]

        """
        fields = fields or [
            'date',
            'countryCode',
        ]

        request = RefsRequest(resource_name='holidays', fields=fields)
        request.add_filter('country', countries)

        if date_since:
            request.add_filter('dateSince', cast_to_date_str(date_since))

        if date_to:
            request.add_filter('dateTo', cast_to_date_str(date_to))

        response = self._do_request(data=request.build())

        holidays = response['data']['holidays'] or []

        if 'date' in fields:
            for holiday in holidays:
                holiday['date'] = cast_from_date_str(holiday['date'])

        return holidays
