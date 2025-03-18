# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import PlanBaseRepositoryDeprecated


@registry.add_simple
class ProjectDetailsRepositoryDeprecated(PlanBaseRepositoryDeprecated):
    """
    Репозиторий "Детали проекта"
    """
    connector_method = 'get_one'

    SERVICE = 'plan'
    RESOURCES = 'projectdetails'
