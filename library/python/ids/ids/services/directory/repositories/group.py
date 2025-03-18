# coding: utf-8

from __future__ import unicode_literals

from ids.registry import registry
from .base import DirectoryPagingRepository


@registry.add_simple
class GroupRepository(DirectoryPagingRepository):
    RESOURCES = 'group'
