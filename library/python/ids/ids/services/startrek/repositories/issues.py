# -*- coding: utf-8 -*-

from six.moves import map

from ids.exceptions import (
    OperationNotPermittedError
)
from ids.registry import registry
from ids.storages.null import NullStorage
from ids.utils.fields_mapping import beautify_fields, unbeautify_fields

from .startrek_repository import StartrekBaseRepository
from .issue_events import StartrekIssueEventsBoundRepository
from .comments import StartrekCommentsBoundRepository
from .links import StartrekLinksBoundRepository
from .permissions import StartrekIssuePermissionsBoundRepository


class StartrekIssuesRepository(StartrekBaseRepository):

    RESOURCES = 'issues'

    def _wrap_to_resource(self, obj, resource=None):
        r = (super(StartrekIssuesRepository, self)
             ._wrap_to_resource(obj, resource))
        mapping = self.options.get("fields_mapping", {})
        obj = beautify_fields(obj, mapping)
        r["__all__"] = obj
        r.update(obj)
        r["rep_issue_events"] = StartrekIssueEventsBoundRepository(r)
        r["rep_comments"] = StartrekCommentsBoundRepository(r)
        r["rep_links"] = StartrekLinksBoundRepository(r)
        r["rep_permissions"] = StartrekIssuePermissionsBoundRepository(r)
        return r

    def getiter_from_service(self, lookup):
        return map(self._wrap_to_resource,
                   self.startrek.search_issues(lookup))

    def update_(self, resource, fields=None):
        mapping = self.options.get("fields_mapping", {})
        fields = unbeautify_fields(fields, mapping)
        ticket = self.startrek.update_issue(resource, fields)
        return self._wrap_to_resource(ticket, resource)

    def create_(self, fields):
        issue = self.startrek.create_issue(fields)
        return self._wrap_to_resource(issue)

    def delete_(self, resource):
        raise OperationNotPermittedError('There is no way to delete tickets '
                                         'from startrek!')


def _startrek_issues_factory(**options):
    storage = NullStorage()
    repository = StartrekIssuesRepository(storage, **options)
    return repository

registry.add_repository(
    StartrekIssuesRepository.SERVICE,
    StartrekIssuesRepository.RESOURCES,
    _startrek_issues_factory)
