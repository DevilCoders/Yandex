# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import StaffRepository


@registry.add_simple
class StaffPersonRepository(StaffRepository):
    """
    Репозиторий "Список сотрудников"
    """
    SERVICE = 'staff'
    RESOURCES = 'person'
