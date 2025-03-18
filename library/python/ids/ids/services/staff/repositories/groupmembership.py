# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import StaffRepository


@registry.add_simple
class StaffGroupMembershipRepository(StaffRepository):
    """
    Репозиторий "Список членств в группа"
    """
    SERVICE = 'staff'
    RESOURCES = 'groupmembership'
