# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.registry import registry
from .base import DirectoryStaticRepository


@registry.add_simple
class OrganizationRepository(DirectoryStaticRepository):
    RESOURCES = 'organization'
