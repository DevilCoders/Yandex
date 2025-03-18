# coding: utf-8

from yandex_tracker_client.collections import *


class Issues(Issues):
    _priority = 0.5
    relative_scroll_path = '/{api_version}/issues/_searchRelativeScroll'

    @injected_property
    def remotelinks(self, issue):
        return self._associated(IssueRemoteLinks, issue=issue.key)

    @injected_property
    def webhooks(self, issue):
        return self._associated(WebHooks, issue=issue.key)

    def relative_scroll(self, per_page=None, filter=None, from_id=None, **kwargs):
        """ API method that allows to fetch more than 10000 issues. Issues are sorted by id field.
        :param from_id: the last issue.id on the previous page, this issue won't be included
        in response.
        """
        data = {}
        if filter:
            data['filter'] = filter
        if from_id:
            data['from'] = from_id

        return self._execute_request(
            self._connection.post,
            path=self.relative_scroll_path,
            params=dict(kwargs, perPage=per_page),
            data=data,
        )


class Queues(Queues):
    _priority = 0.5
    relative_scroll_path = '/{api_version}/queues/_searchRelativeScroll'

    def relative_scroll(self, per_page=None, from_id=None, **kwargs):
        """ API method that allows to fetch more than 10000 queues. Queues are sorted by id field.
        :param from_id: the last queue.id on the previous page, this queue won't be included
        in response
        """
        params = dict(kwargs, perPage=per_page)
        if from_id:
            params['from'] = from_id

        return self._execute_request(
            self._connection.post,
            path=self.relative_scroll_path,
            params=params,
        )


class Goals(Collection):
    path = '/{api_version}/goals/{id}'
    fields = {
        'id': None,
        'title': None,
        'customers': [],
        'implementers': [],
        'importance': None,
        'responsible': None,
        'status': None,
        'tags': [],
        'description': None,
        'comment': None,
    }


class RemoteLinks(Collection):
    path = '/{api_version}/remotelinks/{id}'
    fields = {
        'id': None,
        'self': None,
        'type': None,
        'direction': None,
        'object': None,
        'createdBy': None,
        'createdAt': None,
        'updatedBy': None,
        'updatedAt': None,
    }
    _noindex = True


class IssueRemoteLinks(Collection):
    path = '/{api_version}/issues/{issue}/remotelinks/{id}'
    fields = RemoteLinks.fields
    _priority = 1

    def create(self, relationship, key, origin, params=None, **kwargs):
        assert 'issue' in self._vars
        super(IssueRemoteLinks, self).create(
            relationship=relationship,
            key=key,
            origin=origin,
            params=params,
            **kwargs
        )


class Applications(Collection):
    path = '/{api_version}/applications/{id}'
    fields = {
        'id': None,
        'type': None,
        'self': None,
        'name': None,
    }


class ApplicationObjects(Collection):
    path = '/{api_version}/applications/{application}/objects/{id}'
    fields = {
        'id': None,
        'key': None,
        'self': None,
        'application': None,
    }
    _priority = 1
    _noindex = True


class Offices(Collection):
    path = '/{api_version}/offices/{id}'
    fields = {
        'id': None,
        'self': None,
        'display': None,
    }


class WebHooks(Collection):
    path = '/{api_version}/webhooks/{id}'
    fields = {
        'id': None,
        'self': None,
        'endpoint': None,
        'filter': {},
        'enabled': None,
    }


class Services(Collection):
    """Extra get params = localized"""
    path = '/{version}/services/{id}'
    fields = {
        'id': None,
        'self': None,
        'name': None,
    }
