# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import StaffRepository


@registry.add_simple
class StaffEquipmentRepository(StaffRepository):
    """
    Репозиторий "Список оборудования"
    """
    SERVICE = 'staff'
    RESOURCES = 'equipment'
