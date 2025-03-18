# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import StaffRepository


@registry.add_simple
class StaffGeographyRepository(StaffRepository):
    """
    Репозиторий "Географии"
    """
    SERVICE = 'staff'
    RESOURCES = 'geography'
