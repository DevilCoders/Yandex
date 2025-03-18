# -*- coding: utf-8 -*-

import re

from six.moves import map

from ids.exceptions import (
        OperationNotPermittedError,
        KeyIsAbsentError,
        BackendError,
    )
from ids.registry import registry
from ids.repositories.base import RepositoryBase
from ids.storages.null import NullStorage

from ids.services.jira.connector import JiraConnector
from ids.services.jira.repositories.comments import JiraCommentsBoundRepository
from ids.services.jira.utils.jql_binding import lookup2jql


def _fix_fields_for_jql(lookup):
    new_lookup = {}
    for key in lookup:
        new_key = key
        res = re.match(r'customfield_(\d+)', key)
        if res is not None:
            new_key = 'cf[{0}]'.format(res.group(1))
        new_lookup[new_key] = lookup[key]
    return new_lookup


class JiraTicketsRepository(RepositoryBase):
    '''
    Является провайдером тикетов (Resource) из Jira.
    Работает через python-jira5.

    Структура ресурса-тикета (не все поля могут быть в конкретных экземплярах
    а также поля может не быть в самом ресурсе, тогда его можно найти
    в поле __all__):
        assignee:
            active: bool
            displayName: str
            emailAddress: str
            name: str
        created: time (== str like '2012-09-19T15:12:47.000+0400')
        description: str
        environment: str
        issuetype:
            id: str
            name: str
            description: str
            subtask: bool
        id / key: str
        labels: list of str
        parent:
            key: str
            fields:
                issuetype:
                    id: str
                    name: str
                    description: str
                    subtask bool
                priority:
                    id: str
                    name: str
                status:
                    id: str
                    name: str
                    description: str
                summary: str
        priority:
            id: str
            name: str
        project:
            key: str
            name: str
        reporter:
            active: bool
            displayName: str
            emailAddress: str
            name: str
        resolution:
            id: str
            name: str
            description: str
        resolutiondate: time
        status:
            id: str
            name: str
            description: str
        summary: str
        timeestimate: int
        timeoriginalestimate: int
        timespent: int
        updated: time
        workratio: int

    Под-репозитории (* не реализовано):
        rep_comments
        *rep_attachement
            attachment:
                author:
                    active: bool
                    displayName: str
                    emailAddress: str
                    name: str
                content: link (== str like 'https://jira.test.tools.yandex-team.ru/secure/attachment/390773/file')
                created: time
                filename: str
                mimeType: mime (== str like 'application/octet-stream')
                size: int
        *rep_components
            components:
                name: str
        *rep_fix_versions
            fixVersions:
                archived: bool
                description: str
                name: str
                releaseDate: date (== str like '2012-09-19')
                released: bool
        *rep_affected_versions
            versions:
                archived: bool
                description: str
                name: str
                releaseDate: date (== str like '2012-09-19')
                released: bool
        *rep_labels
            labels: list of str
        *rep_subtasks
            subtasks:
                fields:
                    issuetype:
                        id: str
                        name: str
                        description: str
                        subtask: bool
                    priority:
                        id: str
                        name: str
                    status:
                        id: str
                        name: str
                        description: str
                    summary: str
                key: str

    Возможные значения некоторых полей (в скобках id):
        issuetype:
            Bug(1)
            New Feature(2)
            Task(3)
            Improvement(4)
            Sub-task(5)

        status:
            Open(1)
            In Progress(3)
            Reopened(4)
            Resolved(5)
            Closed(6)

        resolution:
            Fixed(1)
            Will Not Fix(2)
            Duplicate(3)
            Incomplete(4)
            Cannot Reproduce(5)
            Later(6)

        priority:
            Blocker(1)
            Critical(2)
            Normal(3)
            Minor(4)
            Trivial(5)
    '''

    SERVICE = 'jira'
    RESOURCES = 'tickets'

    JIRA_CONNECTOR_CLASS = JiraConnector

    def _get_default_options(self):
        '''
        oauth2_access_token(str): авторизационный токен для oauth2
        server(str): сервер jira, к которому осуществлять коннект
        '''

        d = super(JiraTicketsRepository, self)._get_default_options()

        d.update({
            'oauth2_access_token': None,
            'server': None,  # example: https://jira.yandex-team.ru
        })

        return d

    def __init__(self, storage, **options):
        super(JiraTicketsRepository, self).__init__(storage, **options)

    def _wrap_to_resource(self, obj, resource=None):
        def wrap_with_user_fields(obj, resource):
            mapping = self.options['fields_mapping']
            for field in mapping:
                try:
                    raw_field = mapping[field]
                    val = obj.raw['fields'][raw_field]
                    resource[field] = val
                    resource[raw_field] = val
                except KeyError:
                    pass

        r = super(JiraTicketsRepository, self)._wrap_to_resource(obj, resource)

        fields = [
            'assignee',
            'created',
            'description',
            'environment',
            'issuetype',
            'parent',
            'priority',
            'project',
            'reporter',
            'resolution',
            'resolutiondate',
            'status',
            'summary',
            'updated',

            'components',
            'fixVersions',
            'labels',
        ]

        for field in fields:
            try:
                r[field] = obj.raw['fields'][field]
            except KeyError:
                pass

        wrap_with_user_fields(obj, r)

        # rep_ означает что это не просто аттрибут,
        # а целый репозиторий со своей логикой обновления
        '''
        TODO:
        r['rep_attachement'] =
        r['rep_components'] =
        r['rep_fix_versions'] =
        r['rep_labels'] =
        r['rep_subtasks'] =
        '''
        r['rep_comments'] = JiraCommentsBoundRepository(r)

        r['key'] = obj.raw['key']
        r['id'] = r['key']

        r['__all__'] = obj.raw['fields']

        return r

    def get_user_session_id(self):
        return self.options['oauth2_access_token']

    def _getiter_lookup_hook(self, lookup):
        lookup = super(JiraTicketsRepository, self)._getiter_lookup_hook(lookup)
        lookup = _fix_fields_for_jql(lookup)
        return lookup

    def getiter_from_service(self, lookup):
        '''
        @param lookup: dict
            Т.к. в ресурсе лежат сериализованные поля из джиры, то имена
            для JQL и для ресурса иногда отличются.

            Возможные значения [название поля в ресурсе, если отличается]:
                affectedVersion [versions]
                assignee
                comment [rep_comments]
                component [rep_components]
                created / createdDate [created]
                description
                environment
                fixVersion [rep_fix_versions]
                issueKey / id / issue / key [id / key]
                labels [labels / rep_labels]
                originalEstimate / timeOriginalEstimate [timeoriginalestimate]
                parent
                priority
                project
                remainingEstimate / timeEstimate [timeestimate]
                reporter
                resolution
                resolved / resolutionDate [resolutiondate]
                status
                summary
                type / issueType [issuetype]
                timeSpent [timespent]
                updated / updatedDate [updated]
                workRatio [workratio]

                text - master field, подробнее в документации atlassian

                Custom Field (варианты записи):
                    customfield_<CustomFieldID>
                    CustomFieldName

                Filter (варианты записи):
                    filter
                    request
                    savedFilter
                    searchRequest

            Формат значений для date-field'ов (варианты записи):
                "yyyy/MM/dd HH:mm"
                "yyyy-MM-dd HH:mm"
                "yyyy/MM/dd"
                "yyyy-MM-dd"
            Альтернативный формат для date-field'ов для указания
            относительного времени:
                "w" - weeks
                "d" - days
                "h" - hours
                "m" - minutes

            Подробнее: https://confluence.atlassian.com/display/JIRA/Advanced+Searching#AdvancedSearching-FieldsReference

        @example: obj.getiter(lookup={
                                        'summary': 'new summary',
                                        'assignee': 'mixael',
                                        'created__gt': '-1d',
                                    }
                        )

        Возвращает тикеты с указанными параметрами.

        Метод не нужно вызывать напрямую. Вместо этого стоит вызывать
        базовый getiter: он подхватывает реализацию текущего метода.
        Суффиксы в лукапах (contains и т.п.) берутся из utils/jql_binding.
        В возвращаемых ресурсах может не быть полей, указанных в запросе.
        Для этого случая есть __all__.
        '''

        connector = self.get_connector()
        return map(self._wrap_to_resource,
                   connector.get_all_issues(lookup2jql(lookup)))

    def update_(self, resource, fields):
        '''
        @param fields: dict
            Возможные значения:
                assignee:
                    name: str
                description: str
                environment: str
                labels: list of str
                priority:
                    id: str
                    name: str
                reporter:
                    name: str
                summary: str
                issuetype:
                    id: str
                    name: str

        @example: obj.update(resource,
                             fields={
                                        'issuetype': {'name': 'Task'},
                                        'summary': 'new summary',
                                        'description': 'A new summary',
                                        'assignee': {'name': mixael'},
                                    }
                            )

        Обновляет указанные поля у переданного тикета.
        '''

        issue = resource['__raw__']
        issue.update(fields=fields)
        self._wrap_to_resource(issue, resource)  # resource changing

    def create_(self, fields):
        '''
        @param fields: dict
            Структура соответствует структуре тикета,
            т.е. вместо 'assignee__name': X надо писать 'assignee': {'name': X}
            В разных проектах разные поля доступны для записи, поэтому надо
            все проверять на практике. Кроме того для установки могут быть доступны
            и пользовательские поля.

            Возможные значения (* - обязательные):
                assignee:
                    name: str
                description: str
                environment: str
                *issuetype
                labels: list of str
                parent:
                    key: str
                priority:
                    id: str
                    name: str
                *project
                reporter:
                    name: str
                *summary

        @example: obj.create(fields={
                        'issuetype': {'name': 'Bug'},
                        'project': {'key': 'PLAN'},
                        'summary': 'Еще один тикет в планнер.'
                    })

        Создает тикет в джире.

        Если поле issuetype не указано, то issuetype будет выбран, как первый
        вариант из списка типов тикетов, поддерживаемых данным проектом.
        Возможные значения (базовые) для issuetype написаны в структуре тикета.
        Полный список смотрится через python-jira5: jira.issue_types(),
        но не все варианты могут поддерживаться конкретным инстансом джиры или
        конкретным проектом.
        Чтобы узнать, какие типы поддерживаются проектом есть python-jira5:
        jira.createmeta(projectKeys='PLAN').

        Если указано поле parent, то issuetype должен быть Sub-task. И наоборот.
        '''

        if 'issuetype' not in fields:  # находим первый тип через createmeta
            if 'project' not in fields:
                raise KeyIsAbsentError('project should be specified in fields')

            connector = self.get_connector()
            jira = connector.connection

            project = fields['project']
            kws = {}
            if 'key' in project:
                kws['projectKeys'] = project['key']
            elif 'name' in project:  # находим key проекта по имени
                query = 'project = ' + project['name']
                try:
                    results = jira.search_issues(query, maxResults=1)
                except connector.backend_exceptions as e:
                    raise BackendError(e)
                kws['projectKeys'] = results[0].fields.project.key
            else:
                raise KeyIsAbsentError('to define project use "key" or "name" keys')

            kws['expand'] = 'projects.issuetypes'
            first_id = jira.createmeta(**kws)['projects'][0]['issuetypes'][0]['id']

            fields['issuetype'] = {'id': first_id}

        connector = self.get_connector()
        jira = connector.connection

        try:
            return self._wrap_to_resource(jira.create_issue(fields=fields))
        except connector.backend_exceptions as e:
            raise BackendError(e)

    def delete_(self, resource):
        '''
        @param resource: Resource

        Удаляет указанный тикет из репозитория.
        Метод не поддерживается.
        '''

        raise OperationNotPermittedError('There is no way to delete tickets from jira!')
        resource['__raw__'].delete()
        resource.clear()

    def get_connector(self):
        return self.JIRA_CONNECTOR_CLASS(
            self.options['server'],
            self.options['oauth2_access_token'],
            timeout=self.options.get('timeout')
        )

def _jira_tickets_factory(**options):
    storage = NullStorage()
    repository = JiraTicketsRepository(storage, **options)
    return repository

registry.add_repository(
    JiraTicketsRepository.SERVICE,
    JiraTicketsRepository.RESOURCES,
    _jira_tickets_factory)
