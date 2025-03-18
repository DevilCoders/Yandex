# coding: utf-8

from __future__ import unicode_literals
import warnings

from ids.registry import registry

WARN = "Function {0} is deprecated. Use formatter_repository.{0} instead"


def convert_to_html(wiki_text, user_agent, config='external', **params):
    """
    Получить html по вики-разметке.
    @param wiki_text: Вики-разметка
    @param user_agent: Произвольная строка, по которой ВФ индентифицирует
        потребителя, например lunapark.
    @param config: Имя конфига для форматтера. Возможные значения:
       https://wiki.yandex-team.ru/wiki/dev/ExternalFormatter/#formatirovanieteksta
    @param params: Будет передано без изменений.
    @rtype: unicode
    """
    warnings.warn(WARN.format("convert_to_html"), DeprecationWarning)
    formatter = registry.get_repository('formatter', 'formatter',
                                        user_agent=user_agent)
    return formatter.convert_to_html(wiki_text, config, **params)


def get_structure(wiki_text, user_agent, config='external', **params):
    """
    Получить структуру вики-документа.
    @rtype: WikiDictNode
    @see: convert_to_html
    """
    warnings.warn(WARN.format("get_structure"), DeprecationWarning)
    formatter = registry.get_repository('formatter', 'formatter',
                                        user_agent=user_agent)
    return formatter.get_structure(wiki_text, config, **params)
