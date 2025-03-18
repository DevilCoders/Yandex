# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

from ._base import RefBase, RefsRequest
from ..http import RefsResponse


class Cbrf(RefBase):

    alias = 'cbrf'

    def get_banks_info(self, bics, fields=None, states=None):
        """Возвращает информацию по указанным при помощи БИК банков в виде словаря,
        где ключи - номера БИК, а значения - информация, связанная с номером.

        ВНИМАНИЕ: по умолчанию возвращаются только активные записи.

        :param list bics: БИКи, для которых требуется получить информацию.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * БИК
                * краткое наименование

        :param list states: Состояния записей (в терминах ЦБ: коды контроля)

        :rtype: dict

        """
        fields = fields or [
            'nameFull',
        ]

        fields.extend([
            'bic',
        ])

        request = RefsRequest(resource_name='banks', fields=fields)

        request.add_filter('bic', bics)
        request.add_filter('state', states)

        response = self._do_request(data=request.build())

        results = {}
        for item in response['data']['banks']:
            results[item['bic']] = item

        return results

    def banks_listing(self, fields=None):
        """ Возвращает список активных и ограниченных банков

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * БИК
                * краткое наименование

        :rtype: Iterable[dict]
        """
        fields = fields or [
            'nameFull',
        ]

        fields.extend([
            'bic',
        ])

        request = RefsRequest(resource_name='listing', fields=fields).build()

        page_number = None

        while True:
            params = {'page': page_number} if page_number else {}

            response = self._do_request(data=request, params=params)  # type: RefsResponse
            for item in response['data']['listing']:
                yield item

            page_number = response.next_page

            if not page_number:
                break
