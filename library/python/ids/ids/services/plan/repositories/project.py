# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry

from .base import PlanBaseRepositoryDeprecated


@registry.add_simple
class ProjectRepositoryDeprecated(PlanBaseRepositoryDeprecated):
    """
    Репозиторий "Список сервисов"
    """
    connector_method = 'search'

    SERVICE = 'plan'
    RESOURCES = 'project'
