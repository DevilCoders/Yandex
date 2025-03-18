# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

from decimal import Decimal

from ._base import RefBase, RefsRequest
from .utils import cast_to_date_str


class Currency(RefBase):

    alias = 'currency'

    def get_rates_info(self, dates=None, sources=None, codes=None, fields=None):
        """Возвращает о курсах валют в виде списка словарей.

        :param list dates: Даты, на которые требуется получить курсы.
            Объекты даты/датывремени или строка в ISO-формате: 2018-10-15

        :param list sources: Идентификаторы источников курсов, для которых
            требуется получить данные.

        :param list codes: ISO-коды валют, для которых требуется получить курсы.
            Внимание: вернуться курсы, в которых указанные валюты фигурируют
            как в качестве источника, так и цели.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * дата курса
                * валюта источник
                * валюта назначения
                * прямой курс на удиницу
                * обратный курс на единицу

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='rates',
            fields=fields or [
                'date',
                'fromCode',
                'toCode',
                'rateDir',
                'rateInv',
            ]
        )

        if dates:
            for idx, date in enumerate(dates):
                dates[idx] = cast_to_date_str(date)

            request.add_filter('date', dates)

        request.add_filter('source', sources, add_field='source')
        request.add_filter('code', codes)

        response = self._do_request(data=request.build())

        rates = response['data']['rates'] or []

        decimal_fields = {'fromAmount', 'toAmount', 'buy', 'sell', 'rateDir', 'rateInv'}

        for rate in rates:
            # Переводим в Decimal.
            for key, value in rate.items():
                if key in decimal_fields and value is not None:
                    rate[key] = Decimal(value)

        return rates

    def get_listing(self, fields=None):
        """Возвращает данные об известных валютах в виде словаря, индексированного
        буквенным ISO-кодом валюты.


        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * название валюты

        :rtype: dict

        """
        fields = fields or [
            'name',
        ]
        fields.append('alpha')

        request = RefsRequest(resource_name='listing', fields=fields)

        response = self._do_request(data=request.build())

        result = {}

        for currency in response['data']['listing']:
            result[currency['alpha']] = currency

        return result

    def get_sources(self, fields=None):
        """Возвращает данные об известных источниках курсов валют в виде словаря,
        индексированного буквенным кодом источника (обычно совпадает с техбуквенным ISO-кодом
        страны, в которой расположен центальный банк, предоставляющий курс).

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * название источника

        :rtype: dict

        """
        fields = fields or [
            'name',
        ]
        fields.append('alias')

        request = RefsRequest(resource_name='sources', fields=fields)

        response = self._do_request(data=request.build())

        result = {}

        for currency in response['data']['sources']:
            result[currency['alias']] = currency

        return result
