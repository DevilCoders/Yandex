# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import StaffRepository


@registry.add_simple
class StaffTableRepository(StaffRepository):
    """
    Репозиторий "Список столов"
    """
    SERVICE = 'staff'
    RESOURCES = 'table'
