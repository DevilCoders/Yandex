# coding: utf-8
import logging
import json
from requests import session, HTTPError, Response
from requests.adapters import HTTPAdapter

from django.conf import settings

from ylog.context import log_context
from tvm2 import TVM2

logger = logging.getLogger(__name__)


class DirClientError(Exception):
    def __init__(self, message, response):
        super(DirClientError, self).__init__(message)
        self.response = response


class OrganizationNotFoundError(DirClientError):
    pass


class OrganizationAlreadyDeletedError(DirClientError):
    pass


class UserNotFoundError(DirClientError):
    pass


class ResponseDataObsoleteError(DirClientError):
    pass


class ServiceIsNotEnabledError(DirClientError):
    pass


DEFAULT_CONNECTION_TIMEOUT = 15  # in sec.

ORGANIZATION_DELETED_RESPONSE_CODE = 'organization_deleted'

SERVICE_IS_NOT_ENABLED_RESPONSE_CODE = 'service_is_not_enabled'


def make_id_for_req_to_dir(sync_random, dir_req_number, dir_org_id=None):
    """
    Вернуть X-Request-ID для запроса в Директорию.

    Формат: {sync_random}-{dir_req_number}-{dir_org_id}

    @param sync_random: ID текущей сессии синхронизации
    @param dir_req_number: Номер запроса в API Директории
    @param dir_org_id: ID организации Директории
    """
    req_id = '{sync_random}-{dir_req_number:06d}'.format(
        sync_random=sync_random,
        dir_req_number=dir_req_number
    )
    if dir_org_id:
        req_id = '{req_id}-{dir_org_id:010d}'.format(
            req_id=req_id,
            dir_org_id=int(dir_org_id),
        )
    return req_id


class DirClient(object):

    def __init__(self, max_retries=3, timeout=DEFAULT_CONNECTION_TIMEOUT,
                 sync_random=None, req_id_from_dir=None):
        self.max_retries = max_retries
        self.timeout = timeout
        self.sync_random = sync_random
        self.req_id_from_dir = req_id_from_dir
        self.req_number = 0

    @staticmethod
    def _get_dir_url(path):
        url = '{host}/v7/{path}/'.format(
            host=settings.DIRSYNC_DIR_API_HOST,
            path=path,
        )
        logger.info('Going to url "%s"', url)
        return url

    def get_req_id(self, dir_org_id=None):
        if self.req_id_from_dir:
            return self.req_id_from_dir

        if not self.sync_random:
            return None

        return make_id_for_req_to_dir(
            sync_random=self.sync_random,
            dir_req_number=self.req_number,
            dir_org_id=dir_org_id
        )

    def _get_tvm2_client(self):
        return TVM2(
            client_id=settings.DIRSYNC_TVM2_CLIENT_ID,
            secret=settings.DIRSYNC_TVM2_SECRET,
            blackbox_client=settings.DIRSYNC_TVM2_BLACKBOX_CLIENT,
            destinations=(settings.DIRSYNC_DIR_TVM2_CLIENT_ID, ),
        )

    def _make_tvm2_fail_response(self):
        fail_response = Response()
        fail_response.status_code = 401
        fail_response._content = '{"error": "dont get tvm2 service ticket"}'
        return fail_response

    def _get_tvm2_ticket(self):
        tvm2_client = self._get_tvm2_client()
        tvm2_tickets_response = tvm2_client.get_service_tickets(settings.DIRSYNC_DIR_TVM2_CLIENT_ID)
        tvm2_ticket = tvm2_tickets_response.get(settings.DIRSYNC_DIR_TVM2_CLIENT_ID)
        if not tvm2_ticket:
            raise DirClientError("Dont get tvm2 ticket for directory", response=self._make_tvm2_fail_response())
        return tvm2_ticket

    def _session(self, dir_org_id=None):
        request_session = session()

        request_session.mount('http://', HTTPAdapter(max_retries=self.max_retries))
        request_session.mount('https://', HTTPAdapter(max_retries=self.max_retries))

        # TODO к сожалению, не заработало, надо разобраться, возможно нужен внешний сертификат :(
        # request_session.verify = '/etc/ssl/certs/ca-certificates.crt'
        request_session.verify = False

        self.req_number += 1
        if settings.DIRSYNC_USE_TVM2:
            tvm2_ticket = self._get_tvm2_ticket()
            request_session.headers.update({
                    'X-Ya-Service-Ticket': tvm2_ticket,
                })
        else:
            request_session.headers.update({
                'Authorization': 'OAuth %s' % settings.DIRSYNC_OAUTH_TOKEN,
            })

        req_id = self.get_req_id(dir_org_id)
        if req_id:
            request_session.headers.update({
                'X-Request-ID': req_id,
            })

        return request_session

    def get_events(self, dir_org_id, **kwargs):
        """
        Вернуть список событий в организации с id=dir_org_id.
        """
        result, revision = self._get_objects_with_latest_revision(dir_org_id, 'events', **kwargs)

        # TODO к сожалению, при постраничной загрузке сортировка в данных из Директории приезжает невалидная.
        # TODO удалить этот код когда починят сортировку и сделать reverse(result)
        # https://st.yandex-team.ru/DIR-2330
        result.sort(key=lambda e: e['revision'])

        return result

    def get_organizations(self, **kwargs):
        params = {
            'fields': settings.DIRSYNC_FIELDS_ORGANIZATION,
            'per_page': 1000,
        }
        params.update(kwargs)

        resp = self._session().get(
            self._get_dir_url('organizations'),
            timeout=self.timeout,
            params=params,
        )

        if resp.status_code == 401:
            raise OrganizationNotFoundError('You are not authorized', response=resp)

        self._check_error(resp)

        content = resp.json()
        result = content['result']

        while content.get('links') and content['links'].get('next'):
            resp = self._session().get(
                content['links']['next'],
                timeout=self.timeout,
            )
            self._check_error(resp)
            content = resp.json()
            result.extend(content['result'])

        return result

    def get_organization(self, dir_org_id, **kwargs):
        params = {
            'fields': settings.DIRSYNC_FIELDS_ORGANIZATION,
        }
        params.update(kwargs)

        resp = self._session().get(
            self._get_dir_url('organizations/%s' % dir_org_id),
            timeout=self.timeout,
            params=params,
        )

        if resp.status_code == 404 and resp.json().get('code') == ORGANIZATION_DELETED_RESPONSE_CODE:
            raise OrganizationAlreadyDeletedError(
                'Organization with dir_org_id={} already deleted'.format(dir_org_id), response=resp)

        if resp.status_code == 403 and resp.json().get('code') == SERVICE_IS_NOT_ENABLED_RESPONSE_CODE:
            raise ServiceIsNotEnabledError(
                'Wiki service in not enabled in Organization with dir_org_id={}'.format(dir_org_id), response=resp)

        if resp.status_code == 401:
            raise OrganizationNotFoundError('You are not authorized', response=resp)

        self._check_error(resp)

        return resp.json()

    def get_user(self, dir_org_id, dir_user_id, **kwargs):
        """
        Вернуть детали пользователя с заданным id.
        """
        params = {
            'fields': settings.DIRSYNC_FIELDS_USER,
        }
        params.update(kwargs)

        resp = self._session().get(
            self._get_dir_url('users/%s' % dir_user_id),
            headers={'X-Org-ID': str(dir_org_id)},
            timeout=self.timeout,
            params=params,
        )

        if resp.status_code == 404:
            raise UserNotFoundError(
                'User with id=%s not found in organization id=%s' % (dir_user_id, dir_org_id), response=resp
            )

        self._check_error(resp)

        return resp.json()

    def get_user_by_cloud_uid(self, dir_org_id, dir_user_cloud_uid, **kwargs):
        """
        Вернуть детали пользователя с заданным cloud_uid.
        """
        params = {
            'fields': settings.DIRSYNC_FIELDS_USER,
        }
        params.update(kwargs)

        resp = self._session().get(
            self._get_dir_url('users/cloud/%s' % dir_user_cloud_uid),
            headers={'X-Org-ID': str(dir_org_id)},
            timeout=self.timeout,
            params=params,
        )

        if resp.status_code == 404:
            raise UserNotFoundError(
                'User with cloud_uid=%s not found in organization id=%s' % (dir_user_cloud_uid, dir_org_id),
                response=resp
            )

        self._check_error(resp)

        return resp.json()

    def post_service_ready(self, service_slug, dir_org_id):
        """
        Уведомить Директорию о том, что сервис готов
        принимать пользователей указанной организации.
        """
        def _post_service_ready():
            return self._session().post(
                self._get_dir_url('services/%s/ready' % service_slug),
                headers={'X-Org-ID': str(dir_org_id)},
                timeout=self.timeout * 2,
            )

        resp = _post_service_ready()

        if resp.status_code == 504:
            # не дождались ответа, повторим попытку
            resp = _post_service_ready()

        if resp.status_code == 401:
            raise OrganizationNotFoundError('You are not authorized', response=resp)

        self._check_error(resp)

        return resp.json()

    def get_users(self, dir_org_id, **kwargs):
        """
        Вернуть список пользователей в организации с id=dir_org_id.

        @return: кортеж из списка объектов и номера ревизии
        """
        if 'fields' not in kwargs:
            kwargs['fields'] = settings.DIRSYNC_FIELDS_USER
        return self._get_objects_with_latest_revision(dir_org_id, 'users', **kwargs)

    def get_departments(self, dir_org_id, **kwargs):
        """
        Вернуть список департаментов в организации с id=dir_org_id.

        @return: кортеж из списка объектов и номера ревизии
        """
        if 'fields' not in kwargs:
            kwargs['fields'] = settings.DIRSYNC_FIELDS_DEPARTMENT
        return self._get_objects_with_latest_revision(dir_org_id, 'departments', **kwargs)

    def get_groups(self, dir_org_id, **kwargs):
        """
        Вернуть список групп в организации с id=dir_org_id.

        @return: кортеж из списка объектов и номера ревизии
        """
        if 'fields' not in kwargs:
            kwargs['fields'] = settings.DIRSYNC_FIELDS_GROUP
        return self._get_objects_with_latest_revision(dir_org_id, 'groups', **kwargs)

    def set_subscriptions(self, url, event_names):
        """
        Отправить запрос подписки на уведомления о событиях в Директории.

        @param url: метод АПИ сервиса, принимающий уведомления о событиях в Директории
        @param event_names: список названий событий, на которые будет подписан сервис
        """
        data = {'url': url, 'event_names': event_names}
        resp = self._session().post(
            self._get_dir_url('webhooks'),
            data=json.dumps(data),
            headers={'Content-Type':  'application/json'},
            timeout=self.timeout
        )

        self._check_error(resp)
        return resp.json()

    def get_subscriptions(self):
        """
        Получить все подписки сервиса.
        """
        resp = self._session().get(
            self._get_dir_url('webhooks'),
            timeout=self.timeout,
        )

        self._check_error(resp)
        return resp.json()

    def delete_subscription(self, dir_id):
        """
        Удалить подписку сервиса с переданым ID.
        """
        resp = self._session().delete(
            self._get_dir_url('webhooks/%s' % dir_id),
            timeout=self.timeout,
        )

        self._check_error(resp)

    def add_user_to_group(self, dir_org_id, dir_group_id, dir_user_id, dir_admin_user_id):
        """
        Добавить пользователя в команду в организации в Коннекте
        """
        data = {'type': 'user', 'id': int(dir_user_id)}
        resp = self._session().post(
            self._get_dir_url('groups/{}/members'.format(dir_group_id)),
            data=json.dumps(data),
            headers={
                'X-Org-ID': str(dir_org_id),
                'X-USER-IP': '127.0.0.1',
                'X-UID': dir_admin_user_id,
                'Content-Type': 'application/json',
            },
            timeout=self.timeout
        )

        self._check_error(resp)
        return resp.json()

    def _get_objects_with_latest_revision(self, dir_org_id, path, **kwargs):
        while True:
            try:
                result, revision = self._get_objects(dir_org_id, path, **kwargs)
                result.sort(key=lambda e: e['id'])
                return result, revision
            except ResponseDataObsoleteError:
                # ревизии на разных страницах ответа не совпадают, попробуем вытянуть данные еще раз
                with log_context(dir_org_id=dir_org_id, api_path=path):
                    logger.warn('Try to load data of organization with latest revision again')

    def _get_objects(self, dir_org_id, path, **kwargs):
        params = {'per_page': 1000}
        params.update(kwargs)
        resp = self._session(dir_org_id).get(
            self._get_dir_url(path),
            headers={'X-Org-ID': str(dir_org_id)},
            timeout=self.timeout,
            params=params,
        )

        if resp.status_code == 404:
            # при отсутствии данных может прилететь 404 - https://st.yandex-team.ru/DIR-2258
            return [], None

        if resp.status_code == 401:
            raise OrganizationNotFoundError('Organization with id=%s not found' % dir_org_id, response=resp)

        self._check_error(resp)

        content = resp.json()
        revision = resp.headers['x-revision']
        result = content['result']

        while content.get('links') and content['links'].get('next'):
            resp = self._session(dir_org_id).get(
                content['links']['next'],
                headers={'X-Org-ID': str(dir_org_id)},
                timeout=self.timeout,
            )
            self._check_error(resp)
            content = resp.json()
            if revision != resp.headers['x-revision']:
                raise ResponseDataObsoleteError('Revision %s was changed: %s' % (revision, resp.headers['x-revision']),
                                                response=resp)
            result.extend(content['result'])

        return result, revision

    @staticmethod
    def _check_error(resp):
        try:
            return resp.raise_for_status()
        except HTTPError as e:
            raise DirClientError("%s\nResponse content: %s" % (e.message, resp.content), response=resp)
