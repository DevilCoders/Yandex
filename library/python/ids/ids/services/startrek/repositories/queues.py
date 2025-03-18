# -*- coding: utf-8 -*-

from six.moves import map

from ids.exceptions import (
    OperationNotPermittedError
)
from ids.registry import registry
from ids.storages.null import NullStorage
from ids.utils.fields_mapping import beautify_fields

from .startrek_repository import StartrekBaseRepository


class StartrekQueuesRepository(StartrekBaseRepository):

    RESOURCES = 'queues'

    def _wrap_to_resource(self, obj, resource=None):
        mapping = self.options.get("fields_mapping", {})
        r = (super(StartrekQueuesRepository, self)
             ._wrap_to_resource(obj, resource))
        obj = beautify_fields(obj, mapping)
        r["__all__"] = obj
        r.update(obj)
        return r

    def getiter_from_service(self, lookup):
        return map(self._wrap_to_resource,
                   self.startrek.search_queues(lookup))

    def update_(self, resource, fields=None):
        ticket = self.startrek.update_queue(resource, fields)
        return self._wrap_to_resource(ticket, resource)

    def create_(self, fields):
        ticket = self.startrek.create_queue(fields)
        return self._wrap_to_resource(ticket)

    def delete_(self, resource):
        raise OperationNotPermittedError('There is no way to delete queues '
                                         'from Startrek!')


def _startrek_queues_factory(**options):
    storage = NullStorage()
    repository = StartrekQueuesRepository(storage, **options)
    return repository

registry.add_repository(
    StartrekQueuesRepository.SERVICE,
    StartrekQueuesRepository.RESOURCES,
    _startrek_queues_factory)
