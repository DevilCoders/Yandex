# -*- encoding: utf-8 -*-
from __future__ import unicode_literals


from .http import Connector
from .refs.cbrf import Cbrf
from .refs.currency import Currency
from .refs.fias import Fias
from .refs.swift import Swift


class Refs(object):
    """Предоставляет интерфейс для взаимодествия со Справочной."""

    cls_connector = Connector

    def __init__(self, host=None, timeout=None, lang=None):
        """
        :param str|unicode host: Имя хоста, на котором находится Справочная. Будет использован протокол HTTPS.
            Если не указан, будет использован хост по умолчанию (см. .settings.HOST_DEFAULT).

        :param int timeout: Таймаут на подключение. По умолчанию: 5 сек.

        :param str|unicode lang: Код языка, на котором должны быть возвращены результаты.
            Внимание: ресурсы, неподдерживающие локализацию возвращают информацию на английском.

        """
        self._connector = self.cls_connector(host=host, timeout=timeout, lang=lang)

    def get_ref_swift(self):
        """Возвращает объект, дающий доступ к справочнику от SWIFT.

        :rtype: Swift
        """
        return Swift(self)

    def get_ref_cbrf(self):
        """Возвращает объект, дающий доступ к справочнику от Центробанка РФ.

        :rtype: Cbrf
        """
        return Cbrf(self)

    def get_ref_currency(self):
        """Возвращает объект, дающий доступ к справочнику валют.

        :rtype: Currency
        """
        return Currency(self)

    def get_ref_fias(self):
        """Возвращает объект, дающий доступ к справочнику ФИАС.

        :rtype: Fias
        """
        return Fias(self)

    def generic_get_types(self, ref_name):
        """Возвращает описание типов в указанном справочнике в виде словаря,
        где ключи - это имена типов, а значения - их описания.

        :param str|unicode ref_name: Псевдоним справочника.
                Например: swift

        :rtype: dict

        """
        response = self._connector.request(
            ref_name, data='{__schema {types {name description}}}')

        filter_names = {'String', 'Boolean', 'Date', 'DateTime', 'Decimal', 'ID'}

        types = {}
        for type_info in response['data']['__schema']['types']:
            name = type_info['name']

            if not name.startswith('_') and name not in filter_names:
                types[name] = type_info['description']

        return types

    def generic_get_resources(self, ref_name):
        """Возвращает описание ресурсов в указанном справочнике в виде словаря,
        где ключи - это имена ресурсов, а значения - словарь с описанием.

        :param str|unicode ref_name: Псевдоним справочника.
                Например: swift

        :rtype: dict

        """
        response = self._connector.request(
            ref_name, data='{__type(name: "_ApiResources") {fields {name description args {name defaultValue}}}}')

        resources = {}
        for resource_info in response['data']['__type']['fields']:
            name = resource_info['name']
            resources[name] = resource_info

        return resources

    def generic_get_type_fields(self, ref_name, type_name):
        """Возвращает описание полей для указанного типа в указанном справочнике
        в виде словаря, где ключи - это имена полей, а значения - их описания.

        :param str|unicode ref_name: Псевдоним справочника.
                Например: swift

        :param str|unicode type_name: Название типа.
                Например: Event

        :rtype: dict

        """
        response = self._connector.request(
            ref_name,
            data='{__type(name: "%(type)s") {description fields %(fields)s enumValues %(fields)s}}' %
                 {'type': type_name, 'fields': '{name description}'})

        fields = {}

        type_info = response['data']['__type'] or {'fields': [], 'enumValues': []}

        fields_list = type_info['fields'] or []
        fields_list += type_info['enumValues'] or []

        for field_info in fields_list:
            name = field_info['name']
            fields[name] = field_info['description']

        return fields
