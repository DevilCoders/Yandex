# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from . import ABCRepository


@registry.add_simple
class ServiceMembersRepository(ABCRepository):
    """
    Репозиторий команд сервисов в ABC
    """
    SERVICE = 'abc'
    RESOURCES = 'service_members'
