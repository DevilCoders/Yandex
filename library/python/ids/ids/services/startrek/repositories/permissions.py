# -*- coding: utf-8 -*-

from ids.exceptions import IDSException
from ids.utils.fields_mapping import beautify_fields
from .startrek_bound_repository import StartrekBaseBoundRepository


class StartrekIssuePermissionsBoundRepository(StartrekBaseBoundRepository):
    """
    Is an issue-permissions provider
    """

    RESOURCES = 'permissions'

    def _wrap_to_resource(self, obj, resource=None):
        r = (super(StartrekIssuePermissionsBoundRepository, self)
             ._wrap_to_resource(obj, resource))
        mapping = self.options.get('fields_mapping', {})
        obj = beautify_fields(obj, mapping)
        r['__all__'] = obj
        r.update(obj)
        return r

    def getiter_from_service(self, lookup):
        issue_id = self.parent_resource['id']
        lookup['issue_id'] = issue_id
        for issue_permissions in self.startrek.search_issue_permissions(lookup):
            yield self._wrap_to_resource(issue_permissions)

    def update_(self, resource, fields):
        raise IDSException("issue-permissions do not support updating")

    def create_(self, fields):
        raise IDSException("issue-permissions do not support creating")

    def delete_(self, resource):
        raise IDSException("issue-permissions do not support deleting")
