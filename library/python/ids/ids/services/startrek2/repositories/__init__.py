# coding: utf-8


from ids.registry import registry
from .base import create_repository

for repository_name in ('users', 'queues', 'issues', 'issue_types', 'priorities',
                        'groups', 'statuses', 'resolutions', 'versions',
                        'components', 'applications', 'linktypes', 'fields',
                        'screens', 'worklog', 'attachments'):
    registry.add_repository(*create_repository(repository_name))
