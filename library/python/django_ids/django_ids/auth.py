# coding: utf-8

from __future__ import unicode_literals

from django.conf import settings

from ids.connector.plugins_lib import PluginBase
from ids import exceptions

from django_ids.helpers import oauth
from django_ids import settings as django_ids_settings


class MultiAuthPlugin(PluginBase):

    required_params = [
        'auth',
    ]

    def prepare_params(self, params):
        auth = self.connector.auth
        available_auth_types = self.connector.available_auth_types

        if auth.oauth_token and 'oauth' in available_auth_types:
            self.prepare_oauth_params(params, oauth_token=auth.oauth_token)

        elif auth.session_id:
            if 'session_id' in available_auth_types:
                self.prepare_session_id_params(params, session_id=auth.session_id)
            elif 'oauth' in available_auth_types:
                oauth_token = oauth.get_token_by_sessionid(
                    sessionid=auth.session_id,
                    host='yandex-team.ru',
                )
                self.prepare_oauth_params(params, oauth_token=oauth_token)

        elif auth.login and auth.password:
            if 'oauth' in available_auth_types:
                oauth_token = oauth.get_token_by_password(
                    username=auth.login,
                    password=auth.password
                )
                self.prepare_oauth_params(params, oauth_token=oauth_token)

        elif auth.ticket and 'ticket' in available_auth_types:
            self.prepare_ticket_params(params, ticket=auth.ticket)

        # нерекомендуемый способ получения токена
        # работает только, если для всех скоупов, выбранных для приложения
        # включена спецнастройка, позволяющая получать токен таким образом
        elif (
            'oauth' in available_auth_types and
            (auth.uid or django_ids_settings.IDS_DEFAULT_ROBOT_UID)
        ):
            uid = auth.uid or django_ids_settings.IDS_DEFAULT_ROBOT_UID
            try:
                oauth_token = oauth.get_token_by_uid(uid=uid)
            except exceptions.BackendError:
                pass
            else:
                self.prepare_oauth_params(params, oauth_token=oauth_token)
        # нужна возможность выбирать предпочтительные способы
        # аутентификации/получения токена на клиенте
        elif (
            'Authorization' not in params.get('headers', {}) and
            'oauth' in available_auth_types and
            django_ids_settings.IDS_DEFAULT_OAUTH_TOKEN
        ):
            self.prepare_oauth_params(
                params,
                oauth_token=django_ids_settings.IDS_DEFAULT_OAUTH_TOKEN,
            )

    def prepare_oauth_params(self, params, oauth_token):
        headers = params.get('headers', {})
        headers['Authorization'] = 'OAuth ' + oauth_token
        params['headers'] = headers

    def prepare_session_id_params(self, params, session_id):
        cookies = params.get('cookies', {})
        cookies['Session_id'] = session_id
        params['cookies'] = cookies

    def prepare_ticket_params(self, params, ticket):
        headers = params.get('headers', {})
        headers['Ticket'] = ticket
        params['headers'] = headers


class Auth(object):

    uid = None
    login = None
    password = None
    session_id = None
    staff_id = None
    oauth_token = None
    ticket = None
    yauser = None
    user = None
    tvm_app_id = None

    def __init__(self, **kwargs):
        self.yauser = kwargs.get('yauser')
        self.user = kwargs.get('user')
        self.uid = kwargs.get('uid')
        self.login = kwargs.get('login')
        self.staff_id = kwargs.get('staff_id')
        self.password = kwargs.get('password')
        self.session_id = kwargs.get('session_id')
        self.oauth_token = kwargs.get('oauth_token')
        self.ticket = kwargs.get('ticket')
        self.tvm_app_id = kwargs.get('tvm_app_id')

        if self.uid is None and self.yauser is not None:
            self.uid = self.yauser.uid

        if self.login is None and self.yauser is not None:
            self.login = self.yauser.login

        self.extra = {}

    def __repr__(self):
        return "{class_name}(login='{login}', uid='{uid}')".format(
            class_name=self.__class__.__name__,
            login=self.login,
            uid=self.uid,
        )


class SetAuthMiddleware(object):
    """
    Sets auth attribute for django request.
    """
    AUTH_CLASS = getattr(settings, 'IDS_AUTH_CLASS', Auth)

    def process_request(self, request):
        self.set_auth_attribute(request=request)

    def set_auth_attribute(self, request):
        request.auth = self.AUTH_CLASS(
            session_id=self.get_session_id_from_request(request),
            oauth_token=self.get_token_from_request(request),
            yauser=self.get_yauser_from_request(request),
            user=self.get_user_from_request(request),
            tvm_app_id=self.get_tvm_app_id(request),
        )

    def get_token_from_request(self, request):
        auth_header = request.META.get('Authorization')
        return auth_header and auth_header[len('OAuth '):]

    def get_session_id_from_request(self, request):
        return request.COOKIES.get('Session_id')

    def get_yauser_from_request(self, request):
        return getattr(request, 'yauser', None)

    def get_user_from_request(self, request):
        return getattr(request, 'user', None)

    def get_tvm_app_id(self, request):
        return getattr(request, 'tvm_app_id', None)
