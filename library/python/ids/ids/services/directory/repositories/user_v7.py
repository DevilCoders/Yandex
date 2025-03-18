# coding: utf-8

from __future__ import unicode_literals

from ids.registry import registry
from .base import DirectoryPagingRepository


@registry.add_simple
class UserV7Repository(DirectoryPagingRepository):
    # документация АПИ – https://api.directory.ws.yandex.ru/docs/playground.html#spisok-sotrudnikov

    RESOURCES = 'user_v7'
