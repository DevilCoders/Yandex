# -*- coding: utf-8 -*-

from ids.repositories.bound_base import BoundRepositoryBase
from ids.storages.null import NullStorage
from ids.utils.lookup_applying import object_lookupable
from ids.exceptions import BackendError
from ids.services.jira.connector import JiraConnector


class JiraCommentsBoundRepository(BoundRepositoryBase):
    '''
    Является провайдером комментариев для тикетов Jira.
    Работает через python-jira5.

    Структура ресурса-комментария:
        author:
            active: bool
            displayName: str
            emailAddress: str
            name: str
        updateAuthor:
            active: bool
            displayName: str
            emailAddress: str
            name: str
        body: str
        created: time (== str like '2012-09-19T15:12:47.000+0400')
        updated: time
    '''

    SERVICE = 'jira'
    RESOURCES = 'comments'

    OPTIONS_KEYS_TO_COPY = ('server', 'oauth2_access_token')

    JIRA_CONNECTOR_CLASS = JiraConnector

    def __init__(self, parent_resource, **options):
        super(JiraCommentsBoundRepository, self).__init__(
                            parent_resource, storage=NullStorage(), **options
                        )

    def _wrap_to_resource(self, obj, resource=None):
        def wrap_with_user_fields(obj, resource):
            mapping = self.options['fields_mapping']
            for field in mapping:
                try:
                    raw_field = mapping[field]
                    val = obj.raw[raw_field]
                    resource[field] = val
                    resource[raw_field] = val
                except KeyError:
                    pass

        r = super(JiraCommentsBoundRepository, self)._wrap_to_resource(obj, resource)

        fields = [
            'author',
            'body',
            'created',
            'updateAuthor',
            'updated',
        ]

        for field in fields:
            try:
                r[field] = obj.raw[field]
            except KeyError:
                pass

        wrap_with_user_fields(obj, r)

        r['__all__'] = obj.raw

        return r

    def getiter_from_service(self, lookup):
        '''
        @param lookup: dict
            Возможные значения: все из структуры комментария

        @example: obj.getiter({'author__name__startswith': 'dw',
                               'body__contains': 'test'})

        Возвращает итератор по списку комментариев, привязанных к ресурсу.
        Суффиксы в лукапах (contains и т.п.) берутся из utils/lookup_applying.
        '''

        connector = self.get_connector()
        jira = connector.connection

        try:
            for comment in jira.comments(self.parent_resource['key']):
                if object_lookupable(lookup, comment.raw):
                    yield self._wrap_to_resource(comment)
        except connector.backend_exceptions as e:
            raise BackendError(e)

    def update_(self, resource, fields):
        '''
        @param fields: dict
            Возможные значения:
                body: str

        @example: obj.update(fields={'body': 'hello, test!'})

        Обновляет указанный комментарий.
        Родитель не обновляется!
        '''

        if 'body' not in fields:
            fields['body'] = ''

        comment = resource['__raw__']
        comment.update(body=fields['body'])
        self._wrap_to_resource(comment, resource)  # resource changing

    def create_(self, fields):
        '''
        @param fields: dict
            Возможные значения:
                body: str

        @example: obj.create(fields={'body': 'hello, test!'})

        Создает комментарий в родительском ресурсе-тикете.
        Родитель не обновляется!
        '''

        if 'body' not in fields:
            fields['body'] = ''

        connector = self.get_connector()
        jira = connector.connection

        try:
            c = jira.add_comment(issue=self.parent_resource['key'], body=fields['body'])
            # TODO: договоренность о том, что при обновлении ресурса из sub-rep'а,
            # ресурс-родитель должен обновиться пользователем, а не библиотекой
            # self.parent_resource['__raw__']['fields']['comment']['comments'].append(c.raw)
        except connector.backend_exceptions as e:
            raise BackendError(e)

        return self._wrap_to_resource(c)

    def delete_(self, resource):
        '''
        @param resource: Resource

        @example: obj.delete(res)

        Удаляет указанный комментарий из репозитория.
        Родитель не обновляется!
        '''

        resource['__raw__'].delete()
        resource.clear()

    def get_connector(self):
        return self.JIRA_CONNECTOR_CLASS(
            self.options['server'],
            self.options['oauth2_access_token'],
            timeout=self.options.get('timeout')
        )
