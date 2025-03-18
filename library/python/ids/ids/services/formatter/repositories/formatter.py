# coding: utf-8
from __future__ import unicode_literals

from ids import exceptions
from ids.repositories.base import RepositoryBase
from ids.registry import registry

from ids.services.formatter.connector import WikiFormatterConnector as Connector
from ids.services.formatter.structure import WikiDictNode


@registry.add_simple
class FormatterRepository(RepositoryBase):
    SERVICE = 'formatter'
    RESOURCES = 'formatter'

    def __init__(self, storage, **options):
        """
        Репозиторий викиформатера.
        """
        super(FormatterRepository, self).__init__(storage, **options)
        self.connector = Connector(**options)

    def convert_to_html(self, wiki_text, config='external', version=3, **params):
        """
        Получить html по вики-разметке.
        @param wiki_text: Вики-разметка
        @param config: Имя конфига для форматтера. Возможные значения:
          https://wiki.yandex-team.ru/wiki/dev/ExternalFormatter/#formatirovanieteksta
        @param params: Будет передано без изменений в коннектор.
        @rtype: unicode

        """
        data = {'text': wiki_text}
        response = self.connector.post(
            resource='html',
            data=data,
            url_vars={'cfg': config, 'version': version},
            **params
        )
        return response.text

    def get_structure(self, wiki_text, config='external', version=3, **params):
        """
        Получить структуру вики-документа.
        @rtype: WikiDictNode
        @see: convert_to_html
        """
        data = {'text': wiki_text}
        response = self.connector.post(
            resource='bemjson',
            data=data,
            url_vars={'cfg': config, 'version': version},
            **params
        )
        try:
            bemjson_as_dict = response.json()
        except Exception as e:
            raise exceptions.BackendError(repr(e))

        if bemjson_as_dict is None:
            # происходит ValueError в json.loads внутри requests.
            raise exceptions.BackendError('Response can\'t be parsed as json')

        return WikiDictNode(bemjson_as_dict)

    def get_diff(self, text_a, text_b, json=False, version=3, **params):
        """
        Получить diff двух текстов.
        @param text_a: Первый текст
        @param text_b: Второй текст
        @param json: Если 1, то вернуть diff в json-структуре, иначе в html
          Подробнее: https://wiki.yandex-team.ru/wiki/externalformatter/#diff
        @param version: Версия wiki-форматтера
        @param params: Будет передано без изменений в коннектор
        @rtype: unicode
        """
        json = 1 if json else 0
        data = {
            'a': text_a,
            'b': text_b,
        }
        response = self.connector.post(
            resource='diff',
            data=data,
            url_vars={'json': json, 'version': version},
            **params
        )
        return response.text
