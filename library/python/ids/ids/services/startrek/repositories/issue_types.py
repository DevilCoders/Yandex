# -*- coding: utf-8 -*-

from six.moves import map

from ids.exceptions import (
    OperationNotPermittedError
)
from ids.registry import registry
from ids.storages.null import NullStorage
from ids.utils.fields_mapping import beautify_fields

from .startrek_repository import StartrekBaseRepository


class StartrekIssueTypesRepository(StartrekBaseRepository):
    RESOURCES = 'issue_types'

    def _wrap_to_resource(self, obj, resource=None):
        r = (super(StartrekIssueTypesRepository, self)
             ._wrap_to_resource(obj, resource))
        mapping = self.options.get("fields_mapping", {})
        obj = beautify_fields(obj, mapping)
        r["__all__"] = obj
        r.update(obj)
        return r

    def getiter_from_service(self, lookup):
        return map(self._wrap_to_resource,
                   self.startrek.search_issue_types(lookup))

    def update_(self, resource, fields=None):
        issue_type = self.startrek.update_issue_type(resource, fields)
        return self._wrap_to_resource(issue_type, resource)

    def create_(self, fields):
        ticket = self.startrek.create_issue_type(fields)
        return self._wrap_to_resource(ticket)

    def delete_(self, resource):
        raise OperationNotPermittedError('There is no way to delete issue '
                                         'types from startrek!')


def _startrek_issue_types_factory(**options):
    storage = NullStorage()
    repository = StartrekIssueTypesRepository(storage, **options)
    return repository


registry.add_repository(
    StartrekIssueTypesRepository.SERVICE,
    StartrekIssueTypesRepository.RESOURCES,
    _startrek_issue_types_factory)
