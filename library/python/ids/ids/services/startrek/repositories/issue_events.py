# -*- coding: utf-8 -*-

from ids.exceptions import IDSException
from ids.utils.fields_mapping import beautify_fields

from .startrek_bound_repository import StartrekBaseBoundRepository


class StartrekIssueEventsBoundRepository(StartrekBaseBoundRepository):
    """
    Is an issue-event provider
    """

    RESOURCES = 'issue_events'

    def _wrap_to_resource(self, obj, resource=None):
        r = (super(StartrekIssueEventsBoundRepository, self)
             ._wrap_to_resource(obj, resource))
        mapping = self.options.get('fields_mapping', {})
        obj = beautify_fields(obj, mapping)
        r['__all__'] = obj
        r.update(obj)
        return r

    def getiter_from_service(self, lookup):
        issue_id = self.parent_resource['id']
        lookup['issue_id'] = issue_id
        for issue_event in self.startrek.search_issue_events(lookup):
            yield self._wrap_to_resource(issue_event)

    def update_(self, resource, fields):
        raise IDSException("issue-events do not support updating")

    def create_(self, fields):
        raise IDSException("issue-events do not support creating")

    def delete_(self, resource):
        raise IDSException("issue-events do not support deleting")
