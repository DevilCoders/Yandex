# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

from ._base import RefBase, RefsRequest


class Fias(RefBase):

    alias = 'fias'

    def get_addr_info(self, base_ids=None, parent_ids=None, with_archived=False, levels=None, name='', fields=None):
        """Возвращает информацию по адресообразующим объектам в виде
        списка словарей с информацией.

        :param list base_ids: Список гуидов объектов, для которых необходимо получить информацию.

        :param list parent_ids: Список гуидов родителей объектов, для которых необходимо получить информацию.

        :param bool with_archived: Необходимо ли включать в выборку архивные записи (по умолчанию выключен)

        :param list levels: Добавляет фильтрацию по уровням

        :param str name: Добавляет фильтрацию по наименованию

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id
                * краткое наименование
                * официальное наименование
                * id родительского объекта(при наличии)
                * уровень объекта

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='addresses',
            fields=fields or [
                'baseId',
                'nameShort',
                'nameOfficial',
                'parentId',
                'level'
            ]
        )

        request.add_filter('baseIds', base_ids, add_field='baseId')
        request.add_filter('parentIds', parent_ids, add_field='parentId')
        request.add_filter('archived', with_archived, add_field='statusLiveId')
        request.add_filter('levels', levels, add_field='statusLiveId')
        request.add_filter('name', name, add_field='nameOfficial')

        response = self._request(data=request.build())

        addresses = response['data']['addresses'] or []

        return addresses


    def get_levels(self, fields=None):
        """Возвращает информацию по уровням адресных объектов в виде
        списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id
                * наименование уровня
                * endpoint API

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='levels',
            fields=fields or [
                'id',
                'name',
                'endpoint',
            ]
        )
        response = self._request(data=request.build())

        levels = response['data']['levels'] or []

        return levels

    def get_house_info(
        self, base_ids=None, parent_ids=None, with_archived=False, num=None,
        num_struct='',num_building='', fields=None
    ):
        """Возвращает информацию по домам в виде списка словарей с информацией.

        :param list base_ids: Список гуидов домов, для которых необходимо получить информацию.

        :param list parent_ids: Список гуидов родителей домов (улиц, проспектов и т.д.),
        для которых необходимо получить информацию.

        :param bool with_archived: Необходимо ли включать в выборку архивные записи (по умолчанию выключен)

        :param str num: Добавляет фильтрацию по номеру дома

        :param str num_struct: Добавляет фильтрацию по номеру строения

        :param str num_building: Добавляет фильтрацию по номеру корпуса

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='houses',
            fields=fields or [
                'baseId',
            ]
        )

        request.add_filter('baseIds', base_ids, add_field='baseId')
        request.add_filter('parentIds', parent_ids, add_field='parentId')
        request.add_filter('archived', with_archived, add_field=('dateSince', 'dateTill'))
        request.add_filter('num', num, add_field='num')
        request.add_filter('num_struct', num_struct, add_field='numStruct')
        request.add_filter('num_building', num_building, add_field='numBuilding')


        response = self._request(data=request.build())

        houses = response['data']['houses'] or []

        return houses

    def get_room_info(
        self, base_ids=None, parent_ids=None, with_archived=False, num='', num_flat='', fields=None
    ):
        """Возвращает информацию по квартирам\помещениям в виде списка словарей с информацией.

        :param list base_ids: Список гуидов квартир\помещений, для которых необходимо получить информацию.

        :param list parent_ids: Список гуидов домов для квартир\помещений, для которых необходимо получить информацию.

        :param bool with_archived: Необходимо ли включать в выборку архивные записи (по умолчанию выключен)

        :param str num: Добавляет фильтрацию по номеру помещения

        :param str num_flat: Добавляет фильтрацию по номеру квартиры

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='rooms',
            fields=fields or [
                'baseId',
            ]
        )
        request.add_filter('baseIds', base_ids, add_field='baseId')
        request.add_filter('parentIds', parent_ids, add_field='parentId')
        request.add_filter('archived', with_archived, add_field=('dateSince', 'dateTill'))
        request.add_filter('num', num, add_field='num')
        request.add_filter('numFlat', num_flat, add_field='numFlat')

        response = self._request(data=request.build())

        houses = response['data']['rooms'] or []

        return houses


    def get_stead_info(self, base_ids=None, parent_ids=None, with_archived=False, fields=None):
        """Возвращает информацию по земельным участкам в виде списка словарей с информацией.

        :param list base_ids: Список гуидов участков, для которых необходимо получить информацию.

        :param list parent_ids: Список гуидов родителей участков (улиц, проспектов и т.д.),
        для которых необходимо получить информацию.

        :param bool with_archived: Необходимо ли включать в выборку архивные записи (по умолчанию выключен)

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='steads',
            fields=fields or [
                'baseId',
            ]
        )
        request.add_filter('baseIds', base_ids, add_field='baseId')
        request.add_filter('parentIds', parent_ids, add_field='parentId')
        request.add_filter('archived', with_archived, add_field='statusLiveId')

        response = self._request(data=request.build())

        houses = response['data']['steads'] or []

        return houses


    def get_doc_info(self, base_ids=None, fields=None):
        """Возвращает информацию по нормативным документам в виде списка словарей с информацией.

        :param list base_ids: Список гуидов документов, для которых необходимо получить информацию.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='docs',
            fields=fields or [
                'name'
            ]
        )
        request.add_filter('ids', base_ids, add_field='id')

        response = self._request(data=request.build())

        houses = response['data']['docs'] or []

        return houses

    def get_status_state(self, fields=None):
        """Возвращает информацию о возможных состояний объектов недвижимости в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusStateId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusStateId'] or []

    def get_status_struct(self, fields=None):
        """Возвращает информацию о видах строений в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusStructId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusStructId'] or []

    def get_status_estate(self, fields=None):
        """Возвращает информацию о видах владений в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusEstateId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusEstateId'] or []

    def get_type_room(self, fields=None):
        """Возвращает информацию о типах комнат в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='typeRoom',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['typeRoom'] or []

    def get_type_flat(self, fields=None):
        """Возвращает информацию о типах помещений или офисов в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='typeFlat',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['typeFlat'] or []

    def get_status_actual(self, fields=None):
        """Возвращает информацию о статусах актуальности в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusActualId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusActualId'] or []

    def get_status_center(self, fields=None):
        """Возвращает информацию о статусов центров адресных объектов в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusCenterId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusCenterId'] or []

    def get_status_current(self, fields=None):
        """Возвращает информацию о статусах актуальности записей адресных объектов в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusCurrentId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusCurrentId'] or []

    def get_status_operational(self, fields=None):
        """Возвращает информацию об операциях над адресными объектами в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusOpId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusOpId'] or []

    def get_type_doc(self, fields=None):
        """Возвращает информацию о типах нормативных документов в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='doctypeId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['doctypeId'] or []

    def get_division(self, fields=None):
        """Возвращает информацию о типах деления адресных объектов в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='division',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['division'] or []

    def get_status_live(self, fields=None):
        """Возвращает информацию о статусах актуальности записей адресных объектов по ФИАС в виде списка словарей с информацией.

        :param list fields: Поля, значения которых необходимо вернуть.

            Если не указано, вернутся:
                * базовый id

        :rtype: list[dict]

        """
        request = RefsRequest(
            resource_name='statusLiveId',
            fields=fields or [
                'id',
            ]
        )
        response = self._request(data=request.build())
        return response['data']['statusLiveId'] or []

