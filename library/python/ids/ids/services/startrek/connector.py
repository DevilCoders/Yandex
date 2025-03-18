# encoding: utf-8
from __future__ import print_function

import socket
import types
import logging
from datetime import date

import requests

from json import dumps
from copy import copy

import six
from requests.exceptions import ConnectionError, Timeout


from ids.exceptions import (
    AuthError,
    IDSException,
    KeyIsAbsentError,
    ConflictError,
    DuplicateIssueNotFoundError,
    ConfigurationError,
    BackendError,
)
from ids.configuration import get_config


logger = logging.getLogger(__name__)


class BadResponseError(BackendError):
    """
    Бекэнд ответил, что у него произошла необработанная им ошибка.
    """
    pass


DEFAULT_CONNECTION_TRIES = 3


def yield_ignore_absent(func, *args, **kwargs):
    try:
        res = func(*args, **kwargs)
        yield res
    except KeyIsAbsentError:
        pass


class StartrekConnectionError(BackendError):
    def __init__(self, url, *args, **kwargs):
        super(StartrekConnectionError, self).__init__(
            'Cannot connect to backend "{0}"'.format(url),
            *args, **kwargs
        )


class StartrekTimeoutError(BackendError):
    def __init__(self, url, *args, **kwargs):
        super(StartrekTimeoutError, self).__init__(
            'Timeout when connecting to backend "{0}"'.format(url),
            *args, **kwargs
        )


class StartrekConnector(object):
    def __init__(self, cfg, request=None, debug=False):
        if "server" in cfg:
            assert cfg["server"] is not None
            assert cfg["server"].startswith("http")

        timeout = cfg.get('timeout', 2)
        if timeout <= 0:
            raise ConfigurationError('"timeout" must be greater than 0')

        self.request = request
        self.cfg = cfg
        self.token = None
        self.uid = None
        self.login = None
        self.debug = debug
        self.timeout = timeout
        self._init_auth(cfg)
        session = requests.session()
        session.headers = self._get_headers()
        session.params = self._get_login_params()
        self.http = session

    def _init_auth(self, cfg):
        if "oauth2_access_token" in cfg:
            self.token = cfg["oauth2_access_token"]
        if "__uid" in cfg and "__login" in cfg:
            self.uid = cfg["__uid"]
            self.login = cfg["__login"]
        if self.token is None and (self.login is None or self.uid is None):
            raise AuthError("either oauth2_access_token or "
                            "__uid/__login should be specified")

    def _raise_on_error(self, response, message):
        logger.error('startrek error: %s\t%s', response, message)
        if "statusCode" not in response:
            raise BadResponseError(message)

        status_code = response["statusCode"]
        if status_code == 401:
            raise AuthError(message)
        elif status_code == 404:
            errorMessages = ", ".join(response["errorMessages"])
            raise KeyIsAbsentError("{0}: {1}".format(message, errorMessages))
        elif status_code == 409:
            errorMessages = ", ".join(response["errorMessages"])
            raise ConflictError("{0}: {1}".format(message, errorMessages))
        else:
            errors = ", ".join("[{0}] {1}".format(key, value)
                               for (key, value) in response["errors"].items())
            errorMessages = ", ".join(response["errorMessages"])
            raise BadResponseError(
                "{0}: {1}; {2}".format(message, errorMessages, errors))

    def _try_get_json(self, response, message):
        """
        Вернуть json, содержащийся в ответе

        @param response: объект ответа сервера
        @param message: сообщение для эксепшна

        @return: json из объекта ответа
        @raise: BadResponseError
        """
        try:
            response_json = (response.json()
                        if callable(response.json) else response.json)
        except:
            # вернулся кривой json
            raise BadResponseError(message)

        else:
            # обработка случая, когда вернулся ответ без json'а внутри
            if response_json is None:
                raise BadResponseError(message)

        return response_json

    def _get_headers(self):
        if self.token is None:
            return {}
        return {'Authorization': 'OAuth {0}'.format(self.token)}

    def _get_login_params(self):
        params = {}
        if self.login is not None:
            params["__uid"] = self.uid
            params["__login"] = self.login
        return params

    def _prepare_url(self, resource, url_params, data=None):

        if url_params is not None:
            resource = resource % url_params

        if 'server' in self.cfg:
            server = self.cfg['server']
        else:
            ST_CONFIG = get_config('STARTREK_API')
            server = '{protocol}://{host}'.format(**ST_CONFIG)

        url_pattern = '{server}/v1/{resource}/'
        url = url_pattern.format(server=server, resource=resource)

        if self.debug:
            print(url)
            if data is not None:
                print(dumps(data))
        return url

    def _connect_with_retries(self, method, url, *args, **kwargs):
        count = -1
        while True:
            count += 1
            try:
                response = self._do_connect(method, url, *args, **kwargs)

                msg = ('failed to communicate with '
                       'startrek method="{method}", url="{url}"')

                msg = msg.format(method=method, url=url)

                response_json = self._try_get_json(response, msg)

                if response.status_code < 200 or response.status_code >= 300:
                    # вернулась ошибка от стартрека с описанием в json
                    self._raise_on_error(response_json, msg)

                return response_json

            except ConnectionError:
                if count < DEFAULT_CONNECTION_TRIES:
                    continue
                raise StartrekConnectionError(url)
            except BadResponseError:
                if count < DEFAULT_CONNECTION_TRIES:
                    continue
                raise

    def _do_connect(self, method, url, *args, **kwargs):
        """
        Подключиться по HTTP с заданным методом.
        """
        method = getattr(self.http, method)
        try:
            return method(url, *args, **kwargs)
        except Timeout:
            raise StartrekTimeoutError(url)

        except socket.error as err:
            # TODO: после обновления requests до >=1.0 выпилить
            # наш старый requests 0.13 сейчас не ловит
            # socket.error на connection refused
            # https://github.com/kennethreitz/requests/issues/748
            raise ConnectionError(repr(err))

    def _put(self, resource, data, params=None, url_params=None):
        url = self._prepare_url(resource, url_params, data)
        return self._connect_with_retries(
            'put',
            url,
            data=dumps(data),
            params=params,
            timeout=self.timeout,
            verify=False
        )

    def _post(self, resource, data, params=None, url_params=None):
        url = self._prepare_url(resource, url_params, data)
        return self._connect_with_retries(
            'post',
            url,
            data=dumps(data),
            params=params,
            timeout=self.timeout,
            verify=False
        )

    def _get(self, resource, params=None, url_params=None):
        params = params or {}
        url = self._prepare_url(resource, url_params)
        return self._connect_with_retries(
            'get',
            url,
            params=params,
            timeout=self.timeout,
            verify=False
        )

    @staticmethod
    def _encode_filter_value(value):
        if isinstance(value, types.ListType):
            return ",".join(value)
        else:
            return unicode(value)

    def _encode_filter(self, lookup):
        """
        lookup ожидается в виде:
        {
            // Обычные поля
            "queue": "WIKI",

            // Можно передавать массивом
            "assignee": ["theigel", "kolomeetz"],

            // Поля с датой
            "created": {
                "from": "2013-11-22",
                "to": date(2013, 11, 23)
            },

            // Специальные поля
            "query": "<<jql>>",
            "orderBy": "assignee",
            "orderAsc": True,
        }
        """
        params = {'filter': {}}
        for key in ('query', 'orderBy', 'orderAsc'):
            if key in lookup:
                params[key] = lookup[key]
                del lookup[key]

        # Специальная обработка полей с датами
        for key in ('created', 'updated', 'start', 'resolved', 'dueDate'):
            if key in lookup:
                params['filter'][key] = {}
                for subkey, value in six.iteritems(lookup[key]):

                    if subkey not in ('from', 'to') or not isinstance(value, (six.string_types, date)):
                        logger.info('Invalid Startrek issue parameter: "%s": "%s", skipping', key, value)
                        continue

                    if not isinstance(value, six.string_types):
                        value = value.strftime('%Y-%m-%d')
                    params['filter'][key][subkey] = value
                del lookup[key]

        if lookup:
            for field, value in six.iteritems(lookup):
                params['filter'][field] = value

        return params

    def _is_single_value_lookup(self, lookup, key):
        if len(lookup) == 1 and key in lookup:
            if isinstance(lookup[key], six.string_types):
                return True
        return False

    def _is_key_list_lookup(self, lookup, key):
        return len(lookup) == 1 and key in lookup and isinstance(lookup[key], list)

    def update_issue(self, issue, fields):
        assert "key" in issue
        assert "version" in fields

        res = self._put("issues/%(key)s", fields, {}, {"key": issue["key"]})
        return res

    def update_comment(self, issue_key, comment_id, fields):
        assert "text" in fields
        url_params = {"key": issue_key, "comment_id": comment_id}
        res = self._put("issues/%(key)s/comments/%(comment_id)s",
                        fields, {}, url_params)
        return res

    def update_issue_type(self, issue_type, fields):
        assert "id" in issue_type

        fields["id"] = issue_type["id"]
        res = self._put("issuetypes", fields, {}, {"id": issue_type["id"]})
        return res

    def update_queue(self, queue, fields):
        assert "key" in queue

        fields["key"] = queue["key"]
        res = self._put("queues", fields, {}, {"key": queue["key"]})
        return res

    def create_issue(self, issue):
        try:
            return self._create_issue(issue)
        except ConflictError as conflict_error:
            if 'unique' not in issue:
                raise
            else:
                lookup = {'unique': issue['unique']}
                try:
                    return list(self.search_issues(lookup=lookup))[0]
                except IndexError:
                    msg = "{0}; lookup = {1}".format(conflict_error.message,
                                                     lookup)
                    raise DuplicateIssueNotFoundError(msg)

    def _create_issue(self, issue):
        assert 'queue' in issue
        assert "id" in issue["queue"]
        if 'type' in issue:
            assert "id" in issue["type"]
        if "parentIssue" in issue:
            assert "id" in issue["parentIssue"]

        return self._post("issues", issue, {})

    def create_issue_type(self, issue_type):
        assert 'name' in issue_type
        assert 'description' in issue_type

        return self._post("issuetypes", issue_type, {})

    def create_priority(self, priority):
        raise IDSException("does not work yet")
#        return self._post("priorities", priority, {})

    def create_queue(self, queue):
        assert "key" in queue
        assert "name" in queue
        assert "lead" in queue
        assert "login" in queue["lead"]
        assert "defaultType" in queue
        assert "id" in queue["defaultType"]
        assert "defaultPriority" in queue
        assert "id" in queue["defaultPriority"]
        assert "issueTypes" in queue
        for issue_type in queue["issueTypes"]:
            assert "id" in issue_type

        if not "projects" in queue:
            queue["projects"] = []
        if not "versions" in queue:
            queue["versions"] = []
        if not "workflows" in queue:
            queue["workflows"] = {}
        return self._post("queues", queue, {})

    def create_comment(self, issue_key, comment):
        assert "text" in comment

        return self._post("issues/%(key)s/comments",
                          comment, {}, {"key": issue_key})

    def create_links(self, issue_key, relationship, linkedKeys, **kwargs):
        """Про линки читать тут
            http://wiki.yandex-team.ru/tracker/ruchki#linki"""

        assert relationship in (
            'unknown',
            'relates',
            'is dependent by',
            'depends on',
            'is subtask for',
            'is parent task for',
            'duplicates',
            'is duplicated by',
        )

        linkedKeys = '|'.join(linkedKeys)

        params = {'relationship': relationship, 'linkedKeys': linkedKeys,}
        params.update(kwargs)

        return self._post(
            resource="issues/%(key)s/links",
            data={},
            params=params,
            url_params={"key": issue_key}
        )

    def get_one_link(self, issue_key, linkedKey):
        res = self._get(
            resource='issues/%(issue_key)s/links/%(linkedKey)s',
            params={},
            url_params={"issue_key": issue_key, "linkedKey": linkedKey})
        return res

    def search_links(self, lookup):
        if not "key" in lookup:
            raise IDSException("issue key 'key' should be defined")

        issue_key = lookup.pop("key")
        params = {}

        if "linkedKey" in lookup:
            params["linkedKey"] = lookup.pop("linkedKey")
        if "origin" in lookup:
            params["origin"] = lookup.pop("origin")

        if len(lookup) > 0:
            raise IDSException(
                "only {'key': '..', 'linkedKey': '...'},"
                " {'key': '..', 'linkedKey': '...', 'origin': '...'},"
                " {'key': '..', 'origin': '...'}"
                " or {'key': '...'} lookups are supported"
            )

        res = self._get(
            resource="issues/%(issue_key)s/links",
            params=params,
            url_params={"issue_key": issue_key},
        )

        for item in res:
            yield item

    def get_one_issue(self, key, expand=None):
        params = {}
        if expand:
            params = self._encode_filter({"expand": expand})
        res = self._get('issues/%(key)s', params, {"key": key})
        return res

    def get_one_issue_type(self, issue_type_id):
        res = self._get('issuetypes/%(id)s', {}, {"id": issue_type_id})
        return res

    def get_one_priority(self, priority_id):
        res = self._get('priorities/%(id)s', {}, {"id": priority_id})
        return res

    def get_one_queue(self, key):
        res = self._get('queues/%(key)s', {}, {"key": key})
        return res

    def get_one_issue_event(self, issue_id, event_id):
        res = self._get('issues/%(issue_id)s/event/%(event_id)s',
                        {}, {"issue_id": issue_id, "event_id": event_id})
        return res

    def get_issue_permissions(self, issue_id, ptype):
        res = self._get('issues/%(issue_id)s/permissions/%(ptype)s',
                        {}, {"issue_id": issue_id, "ptype": ptype})
        return res

    def get_one_comment(self, issue_key, comment_id):
        res = self._get('issues/%(issue_key)s/comments/%(comment_id)s', {},
                        {"issue_key": issue_key, "comment_id": comment_id})
        return res

    def search_issue_types(self, lookup):
        is_single_issue_type = self._is_single_value_lookup(lookup, "id")
        if is_single_issue_type:
            yield six.next(yield_ignore_absent(self.get_one_issue_type,
                           lookup["id"]))
            return
        elif len(lookup) > 0:
            raise IDSException("only {'id':'...'} lookup is supported")
        res = self._get('issuetypes')
        for item in res:
            yield item

    def search_priorities(self, lookup):
        is_single_priority = self._is_single_value_lookup(lookup, "id")
        if is_single_priority:
            yield six.next(yield_ignore_absent(self.get_one_priority,
                           lookup["id"]))
            return
        elif len(lookup) > 0:
            raise IDSException("only {'id':'...'} lookup is supported")
        res = self._get('priorities', {})
        for item in res:
            yield item

    def search_issue_events(self, lookup):
        if not "issue_id" in lookup:
            raise IDSException("'issue_id' must be defined")
        lookup = copy(lookup)
        issue_id = lookup.pop("issue_id")
        is_one_issue_event = self._is_single_value_lookup(lookup, "event_id")
        if not is_one_issue_event:
            raise IDSException("only {'issue_id':'...', 'event_id':'...' lookup is allowed")
        yield six.next(yield_ignore_absent(self.get_one_issue_event, issue_id, lookup["event_id"]))

    def search_issue_permissions(self, lookup):
        if not "issue_id" in lookup:
            raise IDSException("'issue_id' must be defined")
        lookup = copy(lookup)
        issue_id = lookup.pop("issue_id")
        is_one_issue_permissions_type = self._is_single_value_lookup(lookup, "type")
        if not is_one_issue_permissions_type:
            raise IDSException("only {'issue_id':'...', 'type':'...' lookup is allowed")
        yield six.next(yield_ignore_absent(self.get_issue_permissions, issue_id, lookup["type"]))

    def search_issues(self, lookup, page=1):
        lookup = copy(lookup)

        expand = lookup.pop("expand", None)

        is_one_ticket = self._is_single_value_lookup(lookup, "key")
        if is_one_ticket:
            yield six.next(yield_ignore_absent(self.get_one_issue,
                                               lookup["key"], expand))
            return

        params = {}
        if expand:
            params["expand"] = self._encode_filter_value(expand)

        if self._is_key_list_lookup(lookup, "key"):
            # в случае поиска строго по списку ключей тикетов используем балк-версия ручки v1/issues,
            # так как она позволяет искать с учетом алиасов (см. WIKI-5583)
            issues = self._get('issues/' + len(lookup['key'])*'%s,', params, tuple(lookup['key']))
            if isinstance(issues, list):
                for issue in issues:
                    yield issue
            else:
                # может быть и одно значение, если искать один тикет, но в lookup положить его в list,
                # например: {'key': ['IDS-1']}
                yield issues
            return

        params.update(self._encode_filter(lookup) if lookup else {})

        def read_page(params, page):
            params.update({'page': str(page)})
            res = self._post('search', params)
            return res['issues'], res['pages']

        issues, pages_count = read_page(params=params, page=page)
        for issue in issues:
            yield issue

        while page < pages_count:
            page += 1

            issues, pages_count = read_page(params=params, page=page)
            for issue in issues:
                yield issue

    def search_queues(self, lookup):
        is_one_queue = self._is_single_value_lookup(lookup, "key")
        if is_one_queue:
            yield six.next(yield_ignore_absent(self.get_one_queue, lookup["key"]))
            return
        elif len(lookup) > 0:
            raise IDSException("only {'key':...} lookup is supported")
        params = {}
        res = self._get('queues', params)
        for item in res:
            yield item

    def search_comments(self, lookup):
        if not "key" in lookup:
            raise IDSException("issue key 'key' should be defined")
        issue_key = lookup.pop("key")
        is_one_comment = self._is_single_value_lookup(lookup, "comment_id")
        if is_one_comment:
            yield six.next(yield_ignore_absent(self.get_one_comment, issue_key,
                                               lookup["comment_id"]))
            return
        elif len(lookup) > 0:
            raise IDSException("only {'key':.., 'comment_id':...} or "
                               "{'key':'...'} lookups are supported")
        res = self._get("issues/%(issue_key)s/comments",
                        {}, {"issue_key": issue_key})
        for item in res:
            yield item
