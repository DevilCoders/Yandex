# coding: utf-8

from __future__ import unicode_literals

from ids.registry import registry
from .base import DirectoryPagingRepository


@registry.add_simple
class UserRepository(DirectoryPagingRepository):
    # документация АПИ – https://api.directory.ws.yandex.ru/docs/playground.html#spisok-sotrudnikov

    RESOURCES = 'user'
