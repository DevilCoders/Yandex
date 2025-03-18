# -*- coding: utf-8 -*-

from ids.exceptions import IDSException
from ids.utils.fields_mapping import beautify_fields
from .startrek_bound_repository import StartrekBaseBoundRepository


class StartrekLinksBoundRepository(StartrekBaseBoundRepository):
    """
    Is a issue links
    """

    RESOURCES = 'links'

    def _wrap_to_resource(self, obj, resource=None):
        r = (super(StartrekLinksBoundRepository, self)
             ._wrap_to_resource(obj, resource))
        mapping = self.options.get('fields_mapping', {})
        obj = beautify_fields(obj, mapping)
        r['__all__'] = obj
        r.update(obj)
        return r

    def getiter_from_service(self, lookup):
        issue_key = self.parent_resource['key']
        lookup['key'] = issue_key
        for link in self.startrek.search_links(lookup):
            yield self._wrap_to_resource(link)

    def update_(self, resource, fields):
        raise IDSException("issue-links do not support update")

    def create_(self, fields):
        issue_key = self.parent_resource["key"]
        links = self.startrek.create_links(issue_key=issue_key, **fields)
        links = [self._wrap_to_resource(l) for l in links]
        return links

    def delete_(self, resource):
        raise IDSException("issue-events do not support deleting")
