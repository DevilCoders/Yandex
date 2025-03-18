# -*- coding: utf-8 -*-

from six.moves import map

from ids.exceptions import (
    OperationNotPermittedError
)
from ids.registry import registry
from ids.storages.null import NullStorage
from ids.utils.fields_mapping import beautify_fields

from .startrek_repository import StartrekBaseRepository


class StartrekPrioritiesRepository(StartrekBaseRepository):

    RESOURCES = 'priorities'

    def _wrap_to_resource(self, obj, resource=None):
        r = (super(StartrekPrioritiesRepository, self)
             ._wrap_to_resource(obj, resource))
        mapping = self.options.get("fields_mapping", {})
        obj = beautify_fields(obj, mapping)
        r["__all__"] = obj
        r.update(obj)
        return r

    def getiter_from_service(self, lookup):
        return map(self._wrap_to_resource,
                   self.startrek.search_priorities(lookup))

    def update_(self, resource, fields=None):
        raise OperationNotPermittedError('Not Implemented Yet due to '
                                         'STARTREK-766')
#        ticket = self.startrek.update_priority(resource, fields)
#        return self._wrap_to_resource(ticket, resource)

    def create_(self, fields):
        raise OperationNotPermittedError('Not Implemented Yet')
#        ticket = self.startrek.create_priority(fields)
#        return self._wrap_to_resource(ticket)

    def delete_(self, resource):
        raise OperationNotPermittedError('There is no way to delete priorities '
                                         'from startrek!')


def _startrek_priorities_factory(**options):
    storage = NullStorage()
    repository = StartrekPrioritiesRepository(storage, **options)
    return repository

registry.add_repository(
    StartrekPrioritiesRepository.SERVICE,
    StartrekPrioritiesRepository.RESOURCES,
    _startrek_priorities_factory)
