# -*- coding: utf-8 -*-

from ids.exceptions import IDSException
from ids.utils.fields_mapping import beautify_fields
from ids.services.startrek.repositories.startrek_bound_repository import StartrekBaseBoundRepository


class StartrekCommentsBoundRepository(StartrekBaseBoundRepository):
    """
    Is a comment for issue provider
    """

    RESOURCES = 'comments'

    def _wrap_to_resource(self, obj, resource=None):
        r = (super(StartrekCommentsBoundRepository, self)
             ._wrap_to_resource(obj, resource))
        mapping = self.options.get('fields_mapping', {})
        obj = beautify_fields(obj, mapping)
        r['__all__'] = obj
        r.update(obj)
        return r

    def getiter_from_service(self, lookup):
        issue_key = self.parent_resource['key']
        lookup['key'] = issue_key
        for comment in self.startrek.search_comments(lookup):
            yield self._wrap_to_resource(comment)

    def update_(self, resource, fields):
        issue = self.parent_resource
        issue_key = issue["key"]
        comment_id = resource["id"]
        comment = self.startrek.update_comment(issue_key, comment_id, fields)
        self._wrap_to_resource(comment, resource)

    def create_(self, fields):
        issue_key = self.parent_resource["key"]
        comment = self.startrek.create_comment(issue_key, fields)
        return self._wrap_to_resource(comment)

    def delete_(self, resource):
        raise IDSException("issue-events do not support deleting")
