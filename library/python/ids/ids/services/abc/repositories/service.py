# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from . import ABCRepository


@registry.add_simple
class ServiceRepository(ABCRepository):
    """
    Репозиторий сервисов ABC
    """
    SERVICE = 'abc'
    RESOURCES = 'service'
