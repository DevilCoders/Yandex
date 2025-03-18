# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import PlanRepository


@registry.add_simple
class ServicesRepo(PlanRepository):
    SERVICE = 'plan'
    RESOURCES = 'services'
